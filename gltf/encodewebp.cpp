// This file is part of gltfpack; see gltfpack.h for version/license details
#ifdef WITH_LIBWEBP
#include "gltfpack.h"

#include "webp/decode.h"
#include "webp/encode.h"

#ifndef WITH_LIBWEBP_BASIS
#include "imageio/image_dec.h"
#endif

#include <atomic>
#include <memory>
#include <thread>

static int writeWebP(const uint8_t* data, size_t data_size, const WebPPicture* picture)
{
	std::string* encoded = static_cast<std::string*>(picture->custom_ptr);
	encoded->append(reinterpret_cast<const char*>(data), data_size);
	return 1;
}

// when gltfpack is built with Basis Universal, we have easy access to its jpeg/png decoders
#ifdef WITH_LIBWEBP_BASIS
namespace pv_png
{
void* load_png(const void* pImage_buf, size_t buf_size, uint32_t desired_chans, uint32_t& width, uint32_t& height, uint32_t& num_chans);
} // namespace pv_png

namespace jpgd
{
static const uint32_t cFlagLinearChromaFiltering = 1;
unsigned char* decompress_jpeg_image_from_memory(const unsigned char* pSrc_data, int src_data_size, int* width, int* height, int* actual_comps, int req_comps, uint32_t flags = 0);
} // namespace jpgd

typedef int (*WebPImageReader)(const uint8_t* const data, size_t data_size, struct WebPPicture* const pic, int keep_alpha, struct Metadata* const metadata);

static int readPngBasis(const uint8_t* const data, size_t data_size, struct WebPPicture* const pic, int keep_alpha, struct Metadata* const metadata)
{
	(void)keep_alpha;
	(void)metadata;

	uint32_t width = 0, height = 0, channels = 0;
	void* img = pv_png::load_png(data, data_size, 4, width, height, channels);
	if (!img)
		return 0;

	pic->width = width;
	pic->height = height;
	int ok = WebPPictureImportRGBA(pic, static_cast<uint8_t*>(img), width * 4);
	free(img);
	return ok;
}

static int readJpegBasis(const uint8_t* const data, size_t data_size, struct WebPPicture* const pic, int keep_alpha, struct Metadata* const metadata)
{
	(void)keep_alpha;
	(void)metadata;

	int width = 0, height = 0, channels = 0;
	unsigned char* img = jpgd::decompress_jpeg_image_from_memory(data, int(data_size), &width, &height, &channels, 4, jpgd::cFlagLinearChromaFiltering);
	if (!img)
		return 0;

	pic->width = width;
	pic->height = height;
	int ok = WebPPictureImportRGBA(pic, img, width * 4);
	free(img);
	return ok;
}
#endif

static const char* encodeWebP(const cgltf_image& image, const char* input_path, const ImageInfo& info, const Settings& settings, std::string& encoded)
{
	WebPConfig config;
	if (!WebPConfigInit(&config))
		return "error initializing WebP";

	std::string img_data;
	std::string mime_type;

	if (!readImage(image, input_path, img_data, mime_type))
		return "error reading source file";

	if (mime_type != "image/png" && mime_type != "image/jpeg")
		return NULL;

	int width = 0, height = 0;
	if (!getDimensions(img_data, mime_type.c_str(), width, height))
		return "error parsing image header";

	adjustDimensions(width, height, settings.texture_scale[info.kind], settings.texture_limit[info.kind], settings.texture_pow2);

#ifdef WITH_LIBWEBP_BASIS
	WebPImageReader reader = (mime_type == "image/jpeg") ? readJpegBasis : readPngBasis;
#else
	WebPInputFileFormat format = (mime_type == "image/jpeg") ? WEBP_JPEG_FORMAT : WEBP_PNG_FORMAT;
	WebPImageReader reader = WebPGetImageReader(format);
	if (!reader)
		return "unsupported image format";
#endif

	WebPPicture pic;
	if (!WebPPictureInit(&pic))
		return "error initializing picture";

	std::unique_ptr<WebPPicture, void (*)(WebPPicture*)> pic_storage(&pic, WebPPictureFree);

	if (!reader(reinterpret_cast<const uint8_t*>(img_data.data()), img_data.size(), &pic, 1, NULL))
		return "error decoding source image";

	if ((width != pic.width || height != pic.height) && !WebPPictureRescale(&pic, width, height))
		return "error resizing image";

	int quality = settings.texture_quality[info.kind];

	if (info.normal_map)
		config.quality = float(50 + quality * 5); // map 1-10 to 55-100
	else
		config.quality = float(20 + quality * 8); // map 1-10 to 28-100

	config.emulate_jpeg_size = 1; // for flatter quality curve

	pic.writer = writeWebP;
	pic.custom_ptr = &encoded;
	encoded.clear();

	if (!WebPEncode(&config, &pic))
		return "error encoding image";

	return NULL;
}

void encodeImagesWebP(std::string* encoded, const cgltf_data* data, const std::vector<ImageInfo>& images, const char* input_path, const Settings& settings)
{
	std::atomic<size_t> next_image{0};

	auto encode = [&]()
	{
		for (;;)
		{
			size_t i = next_image++;
			if (i >= data->images_count)
				break;

			const cgltf_image& image = data->images[i];
			ImageInfo info = images[i];

			if (settings.texture_mode[info.kind] == TextureMode_WebP)
				if (const char* error = encodeWebP(image, input_path, info, settings, encoded[i]))
					encoded[i] = error;
		}
	};

	// we use main thread as a worker as well
	size_t worker_count = settings.texture_jobs == 0 ? std::thread::hardware_concurrency() : settings.texture_jobs;
	size_t thread_count = worker_count > 0 ? worker_count - 1 : 0;

	std::vector<std::thread> threads;
	for (size_t i = 0; i < thread_count; ++i)
		threads.emplace_back(encode);

	encode();

	for (size_t i = 0; i < thread_count; ++i)
		threads[i].join();
}

#endif
