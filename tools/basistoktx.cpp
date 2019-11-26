#include <string>
#include <stdexcept>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "basisu_format.h"
#include "ktx2_format.h"

template <typename T>
void read(const std::string& data, size_t offset, T& result)
{
	if (offset + sizeof(T) > data.size())
		throw std::out_of_range("read");

	memcpy(&result, &data[offset], sizeof(T));
}

template <typename T>
void write(std::string& data, const T& value)
{
	data.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

template <typename T>
void write(std::string& data, size_t offset, const T& value)
{
	if (offset + sizeof(T) > data.size())
		throw std::out_of_range("write");

	memcpy(&data[offset], &value, sizeof(T));
}

void basisToKtx(const std::string& basis, std::string& ktx)
{
	ktx.clear();

	basist::basis_file_header basis_header;
	read(basis, 0, basis_header);

	assert(basis_header.m_sig == basist::basis_file_header::cBASISSigValue);

	assert(basis_header.m_total_slices > 0);
	assert(basis_header.m_total_images == 1);

	assert(basis_header.m_format == 0);
	assert(basis_header.m_flags & basist::cBASISHeaderFlagETC1S);
	assert(!(basis_header.m_flags & basist::cBASISHeaderFlagYFlipped));
	assert(basis_header.m_tex_type == basist::cBASISTexType2D);

	bool has_alpha = (basis_header.m_flags & basist::cBASISHeaderFlagHasAlphaSlices) != 0;

	std::vector<basist::basis_slice_desc> slices(basis_header.m_total_slices);

	for (size_t i = 0; i < basis_header.m_total_slices; ++i)
		read(basis, basis_header.m_slice_desc_file_ofs + i * sizeof(basist::basis_slice_desc), slices[i]);

	uint32_t width = slices[0].m_orig_width;
	uint32_t height = slices[0].m_orig_height;
	uint32_t levels = has_alpha ? slices.size() / 2 : slices.size();

	KTX_header2 ktx_header = { KTX2_IDENTIFIER_REF };
	ktx_header.typeSize = 1;
	ktx_header.pixelWidth = width;
	ktx_header.pixelHeight = height;
	ktx_header.layerCount = 0; // TODO: 1 would make more sence but KTX spec says 0
	ktx_header.faceCount = 1;
	ktx_header.levelCount = levels;
	ktx_header.supercompressionScheme = KTX_SUPERCOMPRESSION_BASIS;

	size_t header_size = sizeof(KTX_header2) + levels * sizeof(ktxLevelIndexEntry);

	size_t dfd_size = 0;

	size_t bgd_size =
		sizeof(ktxBasisGlobalHeader) + sizeof(ktxBasisSliceDesc) * levels +
		basis_header.m_endpoint_cb_file_size + basis_header.m_selector_cb_file_size + basis_header.m_tables_file_size;

	ktx_header.dataFormatDescriptor.byteOffset = header_size;
	ktx_header.dataFormatDescriptor.byteLength = dfd_size;

	ktx_header.supercompressionGlobalData.byteOffset = header_size + dfd_size;
	ktx_header.supercompressionGlobalData.byteLength = bgd_size;

	// KTX2 header
	write(ktx, ktx_header);

	size_t ktx_level_offset = ktx.size();

	for (size_t i = 0; i < levels; ++i)
	{
		ktxLevelIndexEntry le = {}; // This will be patched later
		write(ktx, le);
	}

	// data format descriptor
	// TODO

	// supercompression global data
	ktxBasisGlobalHeader sgd_header = {};
	sgd_header.globalFlags = KTX_BU_SLICE_ETC1S | (has_alpha ? KTX_BU_SLICE_HAS_ALPHA : 0);
	sgd_header.endpointCount = basis_header.m_total_endpoints;
	sgd_header.selectorCount = basis_header.m_total_selectors;
	sgd_header.endpointsByteLength = basis_header.m_endpoint_cb_file_size;
	sgd_header.selectorsByteLength = basis_header.m_selector_cb_file_size;
	sgd_header.tablesByteLength = basis_header.m_tables_file_size;
	sgd_header.extendedByteLength = basis_header.m_extended_file_size;

	write(ktx, sgd_header);

	size_t sgd_level_offset = ktx.size();

	for (size_t i = 0; i < levels; ++i)
	{
		ktxBasisSliceDesc sgd_slice = {}; // This will be patched later
		write(ktx, sgd_slice);
	}

	ktx.append(basis.substr(basis_header.m_endpoint_cb_file_ofs, basis_header.m_endpoint_cb_file_size));
	ktx.append(basis.substr(basis_header.m_selector_cb_file_ofs, basis_header.m_selector_cb_file_size));
	ktx.append(basis.substr(basis_header.m_tables_file_ofs, basis_header.m_tables_file_size));
	ktx.append(basis.substr(basis_header.m_extended_file_ofs, basis_header.m_extended_file_size));

	ktx.resize((ktx.size() + 7) & ~7);

	// mip levels
	for (size_t i = 0; i < levels; ++i)
	{
		size_t slice_index = (levels - i - 1) * (has_alpha + 1);
		const basist::basis_slice_desc& slice = slices[slice_index];
		const basist::basis_slice_desc* slice_alpha = has_alpha ? &slices[slice_index + 1] : 0;

		assert(slice.m_image_index == 0);
		assert(slice.m_level_index == levels - i - 1);

		size_t file_offset = ktx.size();

		ktx.append(basis.substr(slice.m_file_ofs, slice.m_file_size));

		if (slice_alpha)
			ktx.append(basis.substr(slice_alpha->m_file_ofs, slice_alpha->m_file_size));

		ktxLevelIndexEntry le = {};
		le.byteOffset = file_offset;
		le.byteLength = ktx.size() - file_offset;
		le.uncompressedByteLength = 0; // TODO: spec clarification for Basis

		write(ktx, ktx_level_offset + i * sizeof(ktxLevelIndexEntry), le);

		ktxBasisSliceDesc sgd_slice = {};
		sgd_slice.sliceByteOffset = 0;
		sgd_slice.sliceByteLength = slice.m_file_size;

		if (slice_alpha)
		{
			sgd_slice.alphaSliceByteOffset = slice.m_file_size;
			sgd_slice.alphaSliceByteLength = slice_alpha->m_file_size;
		}

		write(ktx, sgd_level_offset + i * sizeof(ktxBasisSliceDesc), sgd_slice);

		if (i + 1 != levels)
			ktx.resize((ktx.size() + 7) & ~7);
	}
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

	std::string basis;
	if (!readFile(argv[1], basis))
		return 1;

	std::string ktx;
	basisToKtx(basis, ktx);

	if (!writeFile(argv[2], ktx))
		return 1;

	return 0;
}
#endif