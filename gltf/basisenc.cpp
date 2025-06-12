#ifdef WITH_BASISU

#define BASISU_NO_ITERATOR_DEBUG_LEVEL

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wdeprecated-builtins"
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
    {1, 1, 0, 4.f},
    {1, 32, 0, 3.f},
    {1, 64, 1, 2.f},
    {1, 96, 1, 1.5f},
    {1, 128, 1, 1.f}, // quality arguments aligned with basisu defaults
    {1, 150, 1, 0.8f},
    {1, 170, 1, 0.6f},
    {1, 192, 1, 0.4f}, // gltfpack defaults
    {1, 224, 2, 0.2f},
    {1, 255, 2, 0.f},
};

static void fillParams(basisu::basis_compressor_params& params, const char* input, const char* output, bool uastc, int width, int height, const BasisSettings& bs, const ImageInfo& info, const Settings& settings)
{
	if (uastc)
	{
		static const uint32_t s_level_flags[basisu::TOTAL_PACK_UASTC_LEVELS] = {basisu::cPackUASTCLevelFastest, basisu::cPackUASTCLevelFaster, basisu::cPackUASTCLevelDefault, basisu::cPackUASTCLevelSlower, basisu::cPackUASTCLevelVerySlow};

		params.m_uastc = true;

#if BASISU_LIB_VERSION >= 160
		params.m_pack_uastc_ldr_4x4_flags &= ~basisu::cPackUASTCLevelMask;
		params.m_pack_uastc_ldr_4x4_flags |= s_level_flags[bs.uastc_l];

		params.m_rdo_uastc_ldr_4x4 = bs.uastc_q > 0;
		params.m_rdo_uastc_ldr_4x4_quality_scalar = bs.uastc_q;
		params.m_rdo_uastc_ldr_4x4_dict_size = 1024;
#else
		params.m_pack_uastc_flags &= ~basisu::cPackUASTCLevelMask;
		params.m_pack_uastc_flags |= s_level_flags[bs.uastc_l];

		params.m_rdo_uastc = bs.uastc_q > 0;
		params.m_rdo_uastc_quality_scalar = bs.uastc_q;
		params.m_rdo_uastc_dict_size = 1024;
#endif
	}
	else
	{
		params.m_compression_level = bs.etc1s_l;

#if BASISU_LIB_VERSION >= 160
		params.m_etc1s_quality_level = bs.etc1s_q;
		params.m_etc1s_max_endpoint_clusters = 0;
		params.m_etc1s_max_selector_clusters = 0;
#else
		params.m_quality_level = bs.etc1s_q;
		params.m_max_endpoint_clusters = 0;
		params.m_max_selector_clusters = 0;
#endif

		params.m_no_selector_rdo = info.normal_map;
		params.m_no_endpoint_rdo = info.normal_map;
	}

	params.m_perceptual = info.srgb;

	params.m_mip_gen = true;
	params.m_mip_srgb = info.srgb;

	params.m_resample_width = width;
	params.m_resample_height = height;

	params.m_y_flip = settings.texture_flipy;

	params.m_create_ktx2_file = true;
	params.m_ktx2_srgb_transfer_func = info.srgb;

	if (uastc)
	{
		params.m_ktx2_uastc_supercompression = basist::KTX2_SS_ZSTANDARD;
		params.m_ktx2_zstd_supercompression_level = 9;
	}

	params.m_read_source_images = true;

#if BASISU_LIB_VERSION >= 150
	params.m_write_output_basis_or_ktx2_files = true;
#else
	params.m_write_output_basis_files = true;
#endif

	params.m_source_filenames.resize(1);
	params.m_source_filenames[0] = input;

	params.m_out_filename = output;

	params.m_status_output = false;
}

static const char* prepareEncode(basisu::basis_compressor_params& params, const cgltf_image& image, const char* input_path, const ImageInfo& info, const Settings& settings, const std::string& temp_prefix, std::string& temp_input, std::string& temp_output)
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

	temp_input = temp_prefix + mimeExtension(mime_type.c_str());
	temp_output = temp_prefix + ".ktx2";

	if (!writeFile(temp_input.c_str(), img_data))
		return "error writing temporary file";

	int quality = settings.texture_quality[info.kind];
	bool uastc = settings.texture_mode[info.kind] == TextureMode_UASTC;

	const BasisSettings& bs = kBasisSettings[quality - 1];

	fillParams(params, temp_input.c_str(), temp_output.c_str(), uastc, width, height, bs, info, settings);

	return NULL;
}

void encodeImages(std::string* encoded, const cgltf_data* data, const std::vector<ImageInfo>& images, const char* input_path, const Settings& settings)
{
	basisu::basisu_encoder_init();

	basisu::vector<basisu::basis_compressor_params> params(data->images_count);
	basisu::vector<basisu::parallel_results> results(data->images_count);

	std::string temp_prefix = getTempPrefix();

	std::vector<std::string> temp_inputs(data->images_count);
	std::vector<std::string> temp_outputs(data->images_count);

	for (size_t i = 0; i < data->images_count; ++i)
	{
		const cgltf_image& image = data->images[i];
		ImageInfo info = images[i];

		if (settings.texture_mode[info.kind] == TextureMode_Raw)
			continue;

		if (const char* error = prepareEncode(params[i], image, input_path, info, settings, temp_prefix + "-" + std::to_string(i), temp_inputs[i], temp_outputs[i]))
			encoded[i] = error;

		// image is ready to encode in parallel
	}

	uint32_t num_threads = settings.texture_jobs == 0 ? std::thread::hardware_concurrency() : settings.texture_jobs;

	basisu::basis_parallel_compress(num_threads, params, results);

	for (size_t i = 0; i < data->images_count; ++i)
	{
		if (params[i].m_source_filenames.empty())
			; // encoding was skipped or preparation resulted in an error
		else if (results[i].m_error_code == basisu::basis_compressor::cECFailedReadingSourceImages)
			encoded[i] = "error decoding source image";
		else if (results[i].m_error_code != basisu::basis_compressor::cECSuccess)
			encoded[i] = "error encoding image";
		else if (!readFile(temp_outputs[i].c_str(), encoded[i]))
			encoded[i] = "error reading temporary file";
	}

	for (size_t i = 0; i < data->images_count; ++i)
	{
		if (!temp_inputs[i].empty())
			removeFile(temp_inputs[i].c_str());
		if (!temp_outputs[i].empty())
			removeFile(temp_outputs[i].c_str());
	}
}
#endif
