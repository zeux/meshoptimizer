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

static std::unique_ptr<basisu::job_pool> gJobPool;

void encodeBasisInit(int jobs)
{
	using namespace basisu;

	basisu_encoder_init();

	uint32_t num_threads = jobs == 0 ? std::thread::hardware_concurrency() : jobs;

	gJobPool.reset(new job_pool(num_threads));
}

bool encodeBasisInternal(const char* input, const char* output, bool yflip, bool normal_map, bool linear, bool uastc, int uastc_l, float uastc_q, int etc1s_l, int etc1s_q, int zstd_l, int width, int height)
{
	using namespace basisu;

	basis_compressor_params params;

	params.m_multithreading = gJobPool->get_total_threads() > 1;
	params.m_pJob_pool = gJobPool.get();

	if (uastc)
	{
		static const uint32_t s_level_flags[TOTAL_PACK_UASTC_LEVELS] = { cPackUASTCLevelFastest, cPackUASTCLevelFaster, cPackUASTCLevelDefault, cPackUASTCLevelSlower, cPackUASTCLevelVerySlow };

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

void encodePush(const std::function<void()>& job)
{
	assert(gJobPool);

	gJobPool->add_job(job, nullptr); // explicitly pass token to make sure we're using thread-safe job_pool implementation
}

void encodeWait()
{
	assert(gJobPool);

	gJobPool->wait_for_all();
}
#endif
