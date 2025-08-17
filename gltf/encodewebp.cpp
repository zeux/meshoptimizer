// This file is part of gltfpack; see gltfpack.h for version/license details
#ifdef WITH_LIBWEBP
#include "gltfpack.h"

#include "webp/decode.h"
#include "webp/encode.h"

#ifdef WITH_LIBWEBP_IMAGEDEC
#include "imageio/image_dec.h"
#endif

static const char* prepareEncodeWebP(const cgltf_image& image, const char* input_path, const ImageInfo& info, const Settings& settings, std::string& webp_output)
{
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

	WebPPicture pic;
	if (!WebPPictureInit(&pic))
		return "error initializing WebP picture";

	WebPInputFileFormat format;
	if (mime_type == "image/png")
		format = WEBP_PNG_FORMAT;
	else if (mime_type == "image/jpeg")
		format = WEBP_JPEG_FORMAT;
	else
		return NULL;

	WebPImageReader reader = WebPGetImageReader(format);
	if (!reader)
		return "error getting WebP image reader";

	if (!reader((const uint8_t*)img_data.data(), img_data.size(), &pic, 1, NULL))
	{
		WebPPictureFree(&pic);
		return "error decoding source image";
	}

	if (width != pic.width || height != pic.height)
	{
		if (!WebPPictureRescale(&pic, width, height))
		{
			WebPPictureFree(&pic);
			return "error resizing image";
		}
	}

	WebPConfig config;
	if (!WebPConfigInit(&config))
	{
		WebPPictureFree(&pic);
		return "error initializing WebP config";
	}

	config.lossless = 0;
	config.quality = settings.texture_quality[info.kind] * 10; // map 1-10 to 10-100

	WebPMemoryWriter writer;
	WebPMemoryWriterInit(&writer);
	pic.writer = WebPMemoryWrite;
	pic.custom_ptr = &writer;

	if (!WebPEncode(&config, &pic))
	{
		WebPMemoryWriterClear(&writer);
		WebPPictureFree(&pic);
		return "error encoding WebP image";
	}

	webp_output.assign((const char*)writer.mem, writer.size);
	WebPMemoryWriterClear(&writer);
	WebPPictureFree(&pic);

	return NULL;
}

void encodeImagesWebP(std::string* encoded, const cgltf_data* data, const std::vector<ImageInfo>& images, const char* input_path, const Settings& settings)
{
	for (size_t i = 0; i < data->images_count; ++i)
	{
		const cgltf_image& image = data->images[i];
		ImageInfo info = images[i];

		if (settings.texture_mode[info.kind] != TextureMode_WebP)
			continue;

		if (const char* error = prepareEncodeWebP(image, input_path, info, settings, encoded[i]))
		{
			encoded[i] = error;
		}
	}
}

#endif
