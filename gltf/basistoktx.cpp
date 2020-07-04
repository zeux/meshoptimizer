// This file is part of gltfpack; see gltfpack.h for version/license details
#include <stdexcept>
#include <string>
#include <vector>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../extern/basisu_format.h"
#include "../extern/khr_df.h"

// KTX Specification: 2. File Structure
struct Ktx2Header
{
	uint8_t identifier[12];
	uint32_t vkFormat;
	uint32_t typeSize;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t layerCount;
	uint32_t faceCount;
	uint32_t levelCount;
	uint32_t supercompressionScheme;

	uint32_t dfdByteOffset;
	uint32_t dfdByteLength;
	uint32_t kvdByteOffset;
	uint32_t kvdByteLength;
	uint64_t sgdByteOffset;
	uint64_t sgdByteLength;
};

struct Ktx2LevelIndex
{
	uint64_t byteOffset;
	uint64_t byteLength;
	uint64_t uncompressedByteLength;
};

enum
{
	Ktx2SupercompressionSchemeNone = 0,
	Ktx2SupercompressionSchemeBasis = 1,
};

// KTX Specification: 3.1. identifier
static const uint8_t Ktx2FileIdentifier[12] = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A, //
};

// KTX Specification: 3.12.2. Basis Universal Global Data
struct Ktx2BasisGlobalHeader
{
	uint16_t endpointCount;
	uint16_t selectorCount;
	uint32_t endpointsByteLength;
	uint32_t selectorsByteLength;
	uint32_t tablesByteLength;
	uint32_t extendedByteLength;
};

struct Ktx2BasisImageDesc
{
	uint32_t imageFlags;
	uint32_t rgbSliceByteOffset;
	uint32_t rgbSliceByteLength;
	uint32_t alphaSliceByteOffset;
	uint32_t alphaSliceByteLength;
};

template <typename T>
static void read(const std::string& data, size_t offset, T& result)
{
	if (offset + sizeof(T) > data.size())
		throw std::out_of_range("read");

	memcpy(&result, &data[offset], sizeof(T));
}

template <typename T>
static void write(std::string& data, const T& value)
{
	data.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

template <typename T>
static void write(std::string& data, size_t offset, const T& value)
{
	if (offset + sizeof(T) > data.size())
		throw std::out_of_range("write");

	memcpy(&data[offset], &value, sizeof(T));
}

static void createDfd(std::vector<uint32_t>& result, bool alpha, bool srgb, bool uastc)
{
	int sample_count = (!uastc && alpha) ? 2 : 1;
	int descriptor_size = KHR_DF_WORD_SAMPLESTART + sample_count * KHR_DF_WORD_SAMPLEWORDS;

	result.clear();
	result.resize(1 + descriptor_size);

	result[0] = (1 + descriptor_size) * sizeof(uint32_t);

	uint32_t* dfd = &result[1];

	KHR_DFDSETVAL(dfd, VENDORID, KHR_DF_VENDORID_KHRONOS);
	KHR_DFDSETVAL(dfd, DESCRIPTORTYPE, KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT);
	KHR_DFDSETVAL(dfd, VERSIONNUMBER, KHR_DF_VERSIONNUMBER_1_3);
	KHR_DFDSETVAL(dfd, DESCRIPTORBLOCKSIZE, descriptor_size * sizeof(uint32_t));
	KHR_DFDSETVAL(dfd, MODEL, uastc ? KHR_DF_MODEL_UASTC : KHR_DF_MODEL_ETC1S);
	KHR_DFDSETVAL(dfd, PRIMARIES, KHR_DF_PRIMARIES_BT709);
	KHR_DFDSETVAL(dfd, TRANSFER, srgb ? KHR_DF_TRANSFER_SRGB : KHR_DF_TRANSFER_LINEAR);
	KHR_DFDSETVAL(dfd, FLAGS, KHR_DF_FLAG_ALPHA_STRAIGHT);

	if (uastc)
	{
		// UASTC is ASTC 4x4 - but texelBlockDimension encodes size-1
		KHR_DFDSETVAL(dfd, TEXELBLOCKDIMENSION0, 3);
		KHR_DFDSETVAL(dfd, TEXELBLOCKDIMENSION1, 3);

		// Every block is 16 bytes = 128 bits (encoded as 127)
		KHR_DFDSETVAL(dfd, BYTESPLANE0, 16);
		KHR_DFDSETSVAL(dfd, 0, BITLENGTH, 127);

		// The single sample stores either RGB or RGBA data
		assert(sample_count == 1);
		KHR_DFDSETSVAL(dfd, 0, CHANNELID, alpha ? KHR_DF_CHANNEL_UASTC_RGBA : KHR_DF_CHANNEL_UASTC_RGB);
	}
	else
	{
		// Every decoded block is 8 bytes = 64 bits (encoded as 63)
		KHR_DFDSETSVAL(dfd, 0, BITLENGTH, 63);

		// RGB is stored in sample 0, alpha is stored in sample 1
		KHR_DFDSETSVAL(dfd, 0, CHANNELID, KHR_DF_CHANNEL_ETC1S_RGB);

		if (alpha)
			KHR_DFDSETSVAL(dfd, 1, CHANNELID, KHR_DF_CHANNEL_ETC1S_AAA);
	}

	for (int i = 0; i < sample_count; ++i)
	{
		KHR_DFDSETSVAL(dfd, i, SAMPLELOWER, 0);
		KHR_DFDSETSVAL(dfd, i, SAMPLEUPPER, ~0u);
	}
}

std::string basisToKtx(const std::string& data, bool srgb, bool uastc)
{
	std::string ktx;

	basist::basis_file_header basis_header;
	read(data, 0, basis_header);

	assert(basis_header.m_sig == basist::basis_file_header::cBASISSigValue);

	assert(basis_header.m_total_slices > 0);
	assert(basis_header.m_total_images == 1);

	assert(basis_header.m_tex_format == uint32_t(uastc ? basist::cUASTC4x4 : basist::cETC1S));
	assert(!(basis_header.m_flags & basist::cBASISHeaderFlagETC1S) == uastc);
	assert(!(basis_header.m_flags & basist::cBASISHeaderFlagYFlipped));
	assert(basis_header.m_tex_type == basist::cBASISTexType2D);

	if (uastc)
	{
		assert(basis_header.m_endpoint_cb_file_size == 0);
		assert(basis_header.m_selector_cb_file_size == 0);
		assert(basis_header.m_tables_file_size == 0);
		assert(basis_header.m_extended_file_size == 0);
	}

	bool has_alpha = (basis_header.m_flags & basist::cBASISHeaderFlagHasAlphaSlices) != 0;
	bool alpha_slices = has_alpha && !uastc;
	bool basis = !uastc;

	std::vector<basist::basis_slice_desc> slices(basis_header.m_total_slices);

	for (size_t i = 0; i < basis_header.m_total_slices; ++i)
		read(data, basis_header.m_slice_desc_file_ofs + i * sizeof(basist::basis_slice_desc), slices[i]);

	assert(slices[0].m_level_index == 0);
	uint32_t width = slices[0].m_orig_width;
	uint32_t height = slices[0].m_orig_height;
	uint32_t levels = alpha_slices ? uint32_t(slices.size()) / 2 : uint32_t(slices.size());

	Ktx2Header ktx_header = {};
	memcpy(ktx_header.identifier, Ktx2FileIdentifier, sizeof(Ktx2FileIdentifier));
	ktx_header.typeSize = 1;
	ktx_header.pixelWidth = width;
	ktx_header.pixelHeight = height;
	ktx_header.layerCount = 0;
	ktx_header.faceCount = 1;
	ktx_header.levelCount = levels;
	ktx_header.supercompressionScheme = basis ? Ktx2SupercompressionSchemeBasis : Ktx2SupercompressionSchemeNone;

	size_t header_size = sizeof(Ktx2Header) + levels * sizeof(Ktx2LevelIndex);

	std::vector<uint32_t> dfd;
	createDfd(dfd, has_alpha, srgb, uastc);

	const char* kvp_data[][2] = {
	    {"KTXwriter", "gltfpack"},
	};

	std::string kvp;

	for (size_t i = 0; i < sizeof(kvp_data) / sizeof(kvp_data[0]); ++i)
	{
		const char* key = kvp_data[i][0];
		const char* value = kvp_data[i][1];

		write(kvp, uint32_t(strlen(key) + strlen(value) + 2));
		kvp += key;
		kvp += '\0';
		kvp += value;
		kvp += '\0';

		kvp.resize((kvp.size() + 3) & ~3);
	}

	size_t kvp_size = kvp.size();
	size_t dfd_size = dfd.size() * sizeof(uint32_t);

	ktx_header.dfdByteOffset = uint32_t(header_size);
	ktx_header.dfdByteLength = uint32_t(dfd_size);

	ktx_header.kvdByteOffset = uint32_t(header_size + dfd_size);
	ktx_header.kvdByteLength = uint32_t(kvp_size);

	if (basis)
	{
		size_t bgd_size =
		    sizeof(Ktx2BasisGlobalHeader) + sizeof(Ktx2BasisImageDesc) * levels +
		    basis_header.m_endpoint_cb_file_size + basis_header.m_selector_cb_file_size + basis_header.m_tables_file_size;

		ktx_header.sgdByteOffset = (header_size + dfd_size + kvp_size + 7) & ~7;
		ktx_header.sgdByteLength = bgd_size;
	}

	// KTX2 header
	write(ktx, ktx_header);

	size_t ktx_level_offset = ktx.size();

	for (size_t i = 0; i < levels; ++i)
	{
		Ktx2LevelIndex le = {}; // This will be patched later
		write(ktx, le);
	}

	// data format descriptor
	for (size_t i = 0; i < dfd.size(); ++i)
		write(ktx, dfd[i]);

	// key/value pair data
	ktx += kvp;
	ktx.resize((ktx.size() + 7) & ~7);

	// supercompression global data
	if (basis)
	{
		Ktx2BasisGlobalHeader sgd_header = {};
		sgd_header.endpointCount = uint16_t(basis_header.m_total_endpoints);
		sgd_header.selectorCount = uint16_t(basis_header.m_total_selectors);
		sgd_header.endpointsByteLength = basis_header.m_endpoint_cb_file_size;
		sgd_header.selectorsByteLength = basis_header.m_selector_cb_file_size;
		sgd_header.tablesByteLength = basis_header.m_tables_file_size;
		sgd_header.extendedByteLength = basis_header.m_extended_file_size;

		write(ktx, sgd_header);
	}

	size_t sgd_level_offset = ktx.size();

	if (basis)
	{
		for (size_t i = 0; i < levels; ++i)
		{
			Ktx2BasisImageDesc sgd_image = {}; // This will be patched later
			write(ktx, sgd_image);
		}

		ktx.append(data.substr(basis_header.m_endpoint_cb_file_ofs, basis_header.m_endpoint_cb_file_size));
		ktx.append(data.substr(basis_header.m_selector_cb_file_ofs, basis_header.m_selector_cb_file_size));
		ktx.append(data.substr(basis_header.m_tables_file_ofs, basis_header.m_tables_file_size));
		ktx.append(data.substr(basis_header.m_extended_file_ofs, basis_header.m_extended_file_size));
	}

	// align to UASTC block size (16b) before writing mips
	if (uastc)
	{
		ktx.resize((ktx.size() + 15) & ~15);
	}

	// mip levels
	for (size_t i = 0; i < levels; ++i)
	{
		size_t level_index = levels - i - 1;
		size_t slice_index = level_index * (alpha_slices + 1);

		const basist::basis_slice_desc& slice = slices[slice_index];
		const basist::basis_slice_desc* slice_alpha = alpha_slices ? &slices[slice_index + 1] : 0;

		assert(slice.m_image_index == 0);
		assert(slice.m_level_index == level_index);

		size_t file_offset = ktx.size();

		ktx.append(data.substr(slice.m_file_ofs, slice.m_file_size));

		if (slice_alpha)
			ktx.append(data.substr(slice_alpha->m_file_ofs, slice_alpha->m_file_size));

		Ktx2LevelIndex le = {};
		le.byteOffset = file_offset;
		le.byteLength = ktx.size() - file_offset;
		le.uncompressedByteLength = basis ? 0 : slice.m_file_size;

		write(ktx, ktx_level_offset + level_index * sizeof(Ktx2LevelIndex), le);

		if (basis)
		{
			Ktx2BasisImageDesc sgd_image = {};
			sgd_image.rgbSliceByteOffset = 0;
			sgd_image.rgbSliceByteLength = slice.m_file_size;

			if (slice_alpha)
			{
				sgd_image.alphaSliceByteOffset = slice.m_file_size;
				sgd_image.alphaSliceByteLength = slice_alpha->m_file_size;
			}

			write(ktx, sgd_level_offset + level_index * sizeof(Ktx2BasisImageDesc), sgd_image);
		}
	}

	return ktx;
}

#ifdef STANDALONE
bool readFile(const char* path, std::string& data)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return false;

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (length <= 0)
	{
		fclose(file);
		return false;
	}

	data.resize(length);
	size_t result = fread(&data[0], 1, data.size(), file);
	fclose(file);

	return result == data.size();
}

bool writeFile(const char* path, const std::string& data)
{
	FILE* file = fopen(path, "wb");
	if (!file)
		return false;

	size_t result = fwrite(&data[0], 1, data.size(), file);
	fclose(file);

	return result == data.size();
}

int main(int argc, const char** argv)
{
	if (argc < 2)
		return 1;

	std::string data;
	if (!readFile(argv[1], data))
		return 1;

	std::string ktx = basisToKtx(data, true, true);

	if (!writeFile(argv[2], ktx))
		return 1;

	return 0;
}
#endif
