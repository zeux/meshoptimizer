#ifdef WITH_BASISU

#define BASISU_NO_ITERATOR_DEBUG_LEVEL

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

#include "encoder/basisu_comp.h"

#include "gltfpack.h"

struct BasisSettings
{
	int etc1s_l;
	int etc1s_q;
	int uastc_l;
	float uastc_q;
};

static const BasisSettings kBasisSettings[10] = {
    {1, 1, 0, 1.5f},
    {1, 6, 0, 1.f},
    {1, 20, 1, 1.0f},
    {1, 50, 1, 0.75f},
    {1, 90, 1, 0.5f},
    {1, 128, 1, 0.4f},
    {1, 160, 1, 0.34f},
    {1, 192, 1, 0.29f}, // default
    {1, 224, 2, 0.26f},
    {1, 255, 2, 0.f},
};

static std::unique_ptr<basisu::job_pool> gJobPool;

void encodeInit(int jobs)
{
	using namespace basisu;

	basisu_encoder_init();

	uint32_t num_threads = jobs == 0 ? std::thread::hardware_concurrency() : jobs;

	gJobPool.reset(new job_pool(num_threads));
}

static bool encodeInternal(const char* input, const char* output, bool yflip, bool normal_map, bool linear, bool uastc, int uastc_l, float uastc_q, int etc1s_l, int etc1s_q, int zstd_l, int width, int height)
{
	using namespace basisu;

	basis_compressor_params params;

	params.m_multithreading = gJobPool->get_total_threads() > 1;
	params.m_pJob_pool = gJobPool.get();

	if (uastc)
	{
		static const uint32_t s_level_flags[TOTAL_PACK_UASTC_LEVELS] = {cPackUASTCLevelFastest, cPackUASTCLevelFaster, cPackUASTCLevelDefault, cPackUASTCLevelSlower, cPackUASTCLevelVerySlow};

		params.m_uastc = true;

		params.m_pack_uastc_flags &= ~cPackUASTCLevelMask;
		params.m_pack_uastc_flags |= s_level_flags[uastc_l];

		params.m_rdo_uastc = uastc_q > 0;
		params.m_rdo_uastc_quality_scalar = uastc_q;
		params.m_rdo_uastc_dict_size = 1024;
	}
	else
	{
		params.m_compression_level = etc1s_l;
		params.m_quality_level = etc1s_q;
		params.m_max_endpoint_clusters = 0;
		params.m_max_selector_clusters = 0;

		params.m_no_selector_rdo = normal_map;
		params.m_no_endpoint_rdo = normal_map;
	}

	params.m_perceptual = !linear;

	params.m_mip_gen = true;
	params.m_mip_srgb = !linear;

	params.m_resample_width = width;
	params.m_resample_height = height;

	params.m_y_flip = yflip;

	params.m_create_ktx2_file = true;
	params.m_ktx2_srgb_transfer_func = !linear;

	if (zstd_l)
	{
		params.m_ktx2_uastc_supercompression = basist::KTX2_SS_ZSTANDARD;
		params.m_ktx2_zstd_supercompression_level = zstd_l;
	}

	params.m_read_source_images = true;
	params.m_write_output_basis_files = true;

	params.m_source_filenames.resize(1);
	params.m_source_filenames[0] = input;

	params.m_out_filename = output;

	params.m_status_output = false;

	basis_compressor c;

	if (!c.init(params))
		return false;

	return c.process() == basis_compressor::cECSuccess;
}

static const char* encodeImage(const std::string& data, const char* mime_type, std::string& result, const ImageInfo& info, const Settings& settings)
{
	TempFile temp_input(mimeExtension(mime_type));
	TempFile temp_output(".ktx2");

	if (!writeFile(temp_input.path.c_str(), data))
		return "error writing temporary file";

	int quality = settings.texture_quality[info.kind];
	bool uastc = settings.texture_mode[info.kind] == TextureMode_UASTC;

	const BasisSettings& bs = kBasisSettings[quality - 1];

	int width = 0, height = 0;
	if (!getDimensions(data, mime_type, width, height))
		return "error parsing image header";

	adjustDimensions(width, height, settings);

	int zstd = uastc ? 9 : 0;

	if (!encodeInternal(temp_input.path.c_str(), temp_output.path.c_str(), settings.texture_flipy, info.normal_map, !info.srgb, uastc, bs.uastc_l, bs.uastc_q, bs.etc1s_l, bs.etc1s_q, zstd, width, height))
		return "error encoding image";

	if (!readFile(temp_output.path.c_str(), result))
		return "error reading temporary file";

	return nullptr;
}

void encodeImages(std::string* encoded, const cgltf_data* data, const std::vector<ImageInfo>& images, const char* input_path, const Settings& settings)
{
	assert(gJobPool);

	for (size_t i = 0; i < data->images_count; ++i)
	{
		const cgltf_image& image = data->images[i];
		ImageInfo info = images[i];

		if (settings.texture_mode[info.kind] == TextureMode_Raw)
			continue;

		gJobPool->add_job([=]() {
			std::string img_data;
			std::string mime_type;
			std::string result;

			if (!readImage(image, input_path, img_data, mime_type))
				encoded[i] = "error reading source file";
			else if (const char* error = encodeImage(img_data, mime_type.c_str(), result, info, settings))
				encoded[i] = error;
			else
				encoded[i].swap(result);
		}, nullptr); // explicitly pass token to make sure we're using thread-safe job_pool implementation
	}

	gJobPool->wait_for_all();
}
#endif
