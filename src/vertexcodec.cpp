// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

#define SIMD 0

#if SIMD
#include <tmmintrin.h>
#endif

namespace meshopt
{

const size_t kVertexBlockSizeBytes = 8192;
const size_t kVertexBlockMaxSize = 256;
const size_t kByteGroupSize = 16;

size_t getVertexBlockSize(size_t vertex_size)
{
	// align vertex to 16b to facilitate SIMD implementation
	size_t vertex_size_16 = (vertex_size + 15) & ~15;

	// make sure the entire block fits into the scratch buffer
	size_t result = kVertexBlockSizeBytes / vertex_size_16;

	// align to byte group size; we encode each byte as a byte group
	// if vertex block is misaligned, it results in wasted bytes, so just truncate the block size
	result &= ~(kByteGroupSize - 1);

	return (result < kVertexBlockMaxSize) ? result : kVertexBlockMaxSize;
}

inline unsigned char zigzag8(unsigned char v)
{
	return (v >> 7) | ((v ^ -(v >> 7)) << 1);
}

inline unsigned char unzigzag8(unsigned char v)
{
	return (-(v & 1)) ^ (v >> 1);
}

static bool encodeBytesFits(const unsigned char* buffer, size_t buffer_size, int bits)
{
	for (size_t k = 0; k < buffer_size; ++k)
		if (buffer[k] >= (1 << bits))
			return false;

	return true;
}

static size_t encodeBytesGroupMeasure(const unsigned char* buffer, int bits)
{
	assert(bits >= 1 && bits <= 8);

	if (bits == 8)
		return kByteGroupSize;

	size_t result = kByteGroupSize * bits / 8;

	unsigned char sentinel = (1 << bits) - 1;

	for (size_t i = 0; i < kByteGroupSize; ++i)
		result += buffer[i] >= sentinel;

	return result;
}

static unsigned char* encodeBytesGroup(unsigned char* data, const unsigned char* buffer, int bits)
{
	assert(bits >= 1 && bits <= 8);

	if (bits == 8)
	{
		memcpy(data, buffer, kByteGroupSize);
		return data + kByteGroupSize;
	}

	size_t byte_size = 8 / bits;
	assert(kByteGroupSize % byte_size == 0);

	// fixed portion: bits bits for each value
	// variable portion: full byte for each out-of-range value (using 1...1 as sentinel)
	unsigned char sentinel = (1 << bits) - 1;

	for (size_t i = 0; i < kByteGroupSize; i += byte_size)
	{
		unsigned char byte = 0;

		for (size_t k = 0; k < byte_size; ++k)
		{
			unsigned char enc = (buffer[i + k] >= sentinel) ? sentinel : buffer[i + k];

			byte <<= bits;
			byte |= enc;
		}

		*data++ = byte;
	}

	for (size_t i = 0; i < kByteGroupSize; ++i)
	{
		if (buffer[i] >= sentinel)
		{
			*data++ = buffer[i];
		}
	}

	return data;
}

static unsigned char* encodeBytes(unsigned char* data, const unsigned char* buffer, size_t buffer_size)
{
	assert(buffer_size % kByteGroupSize == 0);

	if (encodeBytesFits(buffer, buffer_size, 0))
	{
		*data++ = 0;

		return data;
	}
	else
	{
		*data++ = 1;

		unsigned char* header = data;

		// round number of groups to 4 to get number of header bytes
		size_t header_size = (buffer_size / kByteGroupSize + 3) / 4;

		data += header_size;

		memset(header, 0, header_size);

		for (size_t i = 0; i < buffer_size; i += kByteGroupSize)
		{
			int best_bits = 8;
			size_t best_size = kByteGroupSize; // assume encodeBytesVar(8) just stores as is

			for (int bits = 1; bits < 8; bits *= 2)
			{
				size_t size = encodeBytesGroupMeasure(buffer + i, bits);

				if (size < best_size)
				{
					best_bits = bits;
					best_size = size;
				}
			}

			int bitslog2 = (best_bits == 1) ? 0 : (best_bits == 2) ? 1 : (best_bits == 4) ? 2 : 3;
			assert((1 << bitslog2) == best_bits);

			size_t header_offset = i / kByteGroupSize;

			header[header_offset / 4] |= bitslog2 << ((header_offset % 4) * 2);

			unsigned char* next = encodeBytesGroup(data, buffer + i, best_bits);

			assert(data + best_size == next);
			data = next;
		}

		return data;
	}
}

static unsigned char* encodeVertexBlock(unsigned char* data, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= kVertexBlockMaxSize);

	unsigned char buffer[kVertexBlockMaxSize];
	assert(sizeof(buffer) % kByteGroupSize == 0);

	// we sometimes encode elements we didn't fill when rounding to kByteGroupSize
	memset(buffer, 0, sizeof(buffer));

	for (size_t k = 0; k < vertex_size; ++k)
	{
		size_t vertex_offset = k;

		unsigned char p = last_vertex[k];

		for (size_t i = 0; i < vertex_count; ++i)
		{
			buffer[i] = zigzag8(vertex_data[vertex_offset] - p);

			p = vertex_data[vertex_offset];

			vertex_offset += vertex_size;
		}

		data = encodeBytes(data, buffer, (vertex_count + kByteGroupSize - 1) & ~(kByteGroupSize - 1));
	}

	memcpy(last_vertex, &vertex_data[vertex_size * (vertex_count - 1)], vertex_size);

	return data;
}

#if SIMD
#define Z 128
static const unsigned char kDecodeBytesGroupShuffle[256][8] = {
	{Z,Z,Z,Z,Z,Z,Z,Z,},{Z,Z,Z,Z,Z,Z,Z,0,},{Z,Z,Z,Z,Z,Z,0,Z,},{Z,Z,Z,Z,Z,Z,0,1,},{Z,Z,Z,Z,Z,0,Z,Z,},{Z,Z,Z,Z,Z,0,Z,1,},{Z,Z,Z,Z,Z,0,1,Z,},{Z,Z,Z,Z,Z,0,1,2,},
	{Z,Z,Z,Z,0,Z,Z,Z,},{Z,Z,Z,Z,0,Z,Z,1,},{Z,Z,Z,Z,0,Z,1,Z,},{Z,Z,Z,Z,0,Z,1,2,},{Z,Z,Z,Z,0,1,Z,Z,},{Z,Z,Z,Z,0,1,Z,2,},{Z,Z,Z,Z,0,1,2,Z,},{Z,Z,Z,Z,0,1,2,3,},
	{Z,Z,Z,0,Z,Z,Z,Z,},{Z,Z,Z,0,Z,Z,Z,1,},{Z,Z,Z,0,Z,Z,1,Z,},{Z,Z,Z,0,Z,Z,1,2,},{Z,Z,Z,0,Z,1,Z,Z,},{Z,Z,Z,0,Z,1,Z,2,},{Z,Z,Z,0,Z,1,2,Z,},{Z,Z,Z,0,Z,1,2,3,},
	{Z,Z,Z,0,1,Z,Z,Z,},{Z,Z,Z,0,1,Z,Z,2,},{Z,Z,Z,0,1,Z,2,Z,},{Z,Z,Z,0,1,Z,2,3,},{Z,Z,Z,0,1,2,Z,Z,},{Z,Z,Z,0,1,2,Z,3,},{Z,Z,Z,0,1,2,3,Z,},{Z,Z,Z,0,1,2,3,4,},
	{Z,Z,0,Z,Z,Z,Z,Z,},{Z,Z,0,Z,Z,Z,Z,1,},{Z,Z,0,Z,Z,Z,1,Z,},{Z,Z,0,Z,Z,Z,1,2,},{Z,Z,0,Z,Z,1,Z,Z,},{Z,Z,0,Z,Z,1,Z,2,},{Z,Z,0,Z,Z,1,2,Z,},{Z,Z,0,Z,Z,1,2,3,},
	{Z,Z,0,Z,1,Z,Z,Z,},{Z,Z,0,Z,1,Z,Z,2,},{Z,Z,0,Z,1,Z,2,Z,},{Z,Z,0,Z,1,Z,2,3,},{Z,Z,0,Z,1,2,Z,Z,},{Z,Z,0,Z,1,2,Z,3,},{Z,Z,0,Z,1,2,3,Z,},{Z,Z,0,Z,1,2,3,4,},
	{Z,Z,0,1,Z,Z,Z,Z,},{Z,Z,0,1,Z,Z,Z,2,},{Z,Z,0,1,Z,Z,2,Z,},{Z,Z,0,1,Z,Z,2,3,},{Z,Z,0,1,Z,2,Z,Z,},{Z,Z,0,1,Z,2,Z,3,},{Z,Z,0,1,Z,2,3,Z,},{Z,Z,0,1,Z,2,3,4,},
	{Z,Z,0,1,2,Z,Z,Z,},{Z,Z,0,1,2,Z,Z,3,},{Z,Z,0,1,2,Z,3,Z,},{Z,Z,0,1,2,Z,3,4,},{Z,Z,0,1,2,3,Z,Z,},{Z,Z,0,1,2,3,Z,4,},{Z,Z,0,1,2,3,4,Z,},{Z,Z,0,1,2,3,4,5,},
	{Z,0,Z,Z,Z,Z,Z,Z,},{Z,0,Z,Z,Z,Z,Z,1,},{Z,0,Z,Z,Z,Z,1,Z,},{Z,0,Z,Z,Z,Z,1,2,},{Z,0,Z,Z,Z,1,Z,Z,},{Z,0,Z,Z,Z,1,Z,2,},{Z,0,Z,Z,Z,1,2,Z,},{Z,0,Z,Z,Z,1,2,3,},
	{Z,0,Z,Z,1,Z,Z,Z,},{Z,0,Z,Z,1,Z,Z,2,},{Z,0,Z,Z,1,Z,2,Z,},{Z,0,Z,Z,1,Z,2,3,},{Z,0,Z,Z,1,2,Z,Z,},{Z,0,Z,Z,1,2,Z,3,},{Z,0,Z,Z,1,2,3,Z,},{Z,0,Z,Z,1,2,3,4,},
	{Z,0,Z,1,Z,Z,Z,Z,},{Z,0,Z,1,Z,Z,Z,2,},{Z,0,Z,1,Z,Z,2,Z,},{Z,0,Z,1,Z,Z,2,3,},{Z,0,Z,1,Z,2,Z,Z,},{Z,0,Z,1,Z,2,Z,3,},{Z,0,Z,1,Z,2,3,Z,},{Z,0,Z,1,Z,2,3,4,},
	{Z,0,Z,1,2,Z,Z,Z,},{Z,0,Z,1,2,Z,Z,3,},{Z,0,Z,1,2,Z,3,Z,},{Z,0,Z,1,2,Z,3,4,},{Z,0,Z,1,2,3,Z,Z,},{Z,0,Z,1,2,3,Z,4,},{Z,0,Z,1,2,3,4,Z,},{Z,0,Z,1,2,3,4,5,},
	{Z,0,1,Z,Z,Z,Z,Z,},{Z,0,1,Z,Z,Z,Z,2,},{Z,0,1,Z,Z,Z,2,Z,},{Z,0,1,Z,Z,Z,2,3,},{Z,0,1,Z,Z,2,Z,Z,},{Z,0,1,Z,Z,2,Z,3,},{Z,0,1,Z,Z,2,3,Z,},{Z,0,1,Z,Z,2,3,4,},
	{Z,0,1,Z,2,Z,Z,Z,},{Z,0,1,Z,2,Z,Z,3,},{Z,0,1,Z,2,Z,3,Z,},{Z,0,1,Z,2,Z,3,4,},{Z,0,1,Z,2,3,Z,Z,},{Z,0,1,Z,2,3,Z,4,},{Z,0,1,Z,2,3,4,Z,},{Z,0,1,Z,2,3,4,5,},
	{Z,0,1,2,Z,Z,Z,Z,},{Z,0,1,2,Z,Z,Z,3,},{Z,0,1,2,Z,Z,3,Z,},{Z,0,1,2,Z,Z,3,4,},{Z,0,1,2,Z,3,Z,Z,},{Z,0,1,2,Z,3,Z,4,},{Z,0,1,2,Z,3,4,Z,},{Z,0,1,2,Z,3,4,5,},
	{Z,0,1,2,3,Z,Z,Z,},{Z,0,1,2,3,Z,Z,4,},{Z,0,1,2,3,Z,4,Z,},{Z,0,1,2,3,Z,4,5,},{Z,0,1,2,3,4,Z,Z,},{Z,0,1,2,3,4,Z,5,},{Z,0,1,2,3,4,5,Z,},{Z,0,1,2,3,4,5,6,},
	{0,Z,Z,Z,Z,Z,Z,Z,},{0,Z,Z,Z,Z,Z,Z,1,},{0,Z,Z,Z,Z,Z,1,Z,},{0,Z,Z,Z,Z,Z,1,2,},{0,Z,Z,Z,Z,1,Z,Z,},{0,Z,Z,Z,Z,1,Z,2,},{0,Z,Z,Z,Z,1,2,Z,},{0,Z,Z,Z,Z,1,2,3,},
	{0,Z,Z,Z,1,Z,Z,Z,},{0,Z,Z,Z,1,Z,Z,2,},{0,Z,Z,Z,1,Z,2,Z,},{0,Z,Z,Z,1,Z,2,3,},{0,Z,Z,Z,1,2,Z,Z,},{0,Z,Z,Z,1,2,Z,3,},{0,Z,Z,Z,1,2,3,Z,},{0,Z,Z,Z,1,2,3,4,},
	{0,Z,Z,1,Z,Z,Z,Z,},{0,Z,Z,1,Z,Z,Z,2,},{0,Z,Z,1,Z,Z,2,Z,},{0,Z,Z,1,Z,Z,2,3,},{0,Z,Z,1,Z,2,Z,Z,},{0,Z,Z,1,Z,2,Z,3,},{0,Z,Z,1,Z,2,3,Z,},{0,Z,Z,1,Z,2,3,4,},
	{0,Z,Z,1,2,Z,Z,Z,},{0,Z,Z,1,2,Z,Z,3,},{0,Z,Z,1,2,Z,3,Z,},{0,Z,Z,1,2,Z,3,4,},{0,Z,Z,1,2,3,Z,Z,},{0,Z,Z,1,2,3,Z,4,},{0,Z,Z,1,2,3,4,Z,},{0,Z,Z,1,2,3,4,5,},
	{0,Z,1,Z,Z,Z,Z,Z,},{0,Z,1,Z,Z,Z,Z,2,},{0,Z,1,Z,Z,Z,2,Z,},{0,Z,1,Z,Z,Z,2,3,},{0,Z,1,Z,Z,2,Z,Z,},{0,Z,1,Z,Z,2,Z,3,},{0,Z,1,Z,Z,2,3,Z,},{0,Z,1,Z,Z,2,3,4,},
	{0,Z,1,Z,2,Z,Z,Z,},{0,Z,1,Z,2,Z,Z,3,},{0,Z,1,Z,2,Z,3,Z,},{0,Z,1,Z,2,Z,3,4,},{0,Z,1,Z,2,3,Z,Z,},{0,Z,1,Z,2,3,Z,4,},{0,Z,1,Z,2,3,4,Z,},{0,Z,1,Z,2,3,4,5,},
	{0,Z,1,2,Z,Z,Z,Z,},{0,Z,1,2,Z,Z,Z,3,},{0,Z,1,2,Z,Z,3,Z,},{0,Z,1,2,Z,Z,3,4,},{0,Z,1,2,Z,3,Z,Z,},{0,Z,1,2,Z,3,Z,4,},{0,Z,1,2,Z,3,4,Z,},{0,Z,1,2,Z,3,4,5,},
	{0,Z,1,2,3,Z,Z,Z,},{0,Z,1,2,3,Z,Z,4,},{0,Z,1,2,3,Z,4,Z,},{0,Z,1,2,3,Z,4,5,},{0,Z,1,2,3,4,Z,Z,},{0,Z,1,2,3,4,Z,5,},{0,Z,1,2,3,4,5,Z,},{0,Z,1,2,3,4,5,6,},
	{0,1,Z,Z,Z,Z,Z,Z,},{0,1,Z,Z,Z,Z,Z,2,},{0,1,Z,Z,Z,Z,2,Z,},{0,1,Z,Z,Z,Z,2,3,},{0,1,Z,Z,Z,2,Z,Z,},{0,1,Z,Z,Z,2,Z,3,},{0,1,Z,Z,Z,2,3,Z,},{0,1,Z,Z,Z,2,3,4,},
	{0,1,Z,Z,2,Z,Z,Z,},{0,1,Z,Z,2,Z,Z,3,},{0,1,Z,Z,2,Z,3,Z,},{0,1,Z,Z,2,Z,3,4,},{0,1,Z,Z,2,3,Z,Z,},{0,1,Z,Z,2,3,Z,4,},{0,1,Z,Z,2,3,4,Z,},{0,1,Z,Z,2,3,4,5,},
	{0,1,Z,2,Z,Z,Z,Z,},{0,1,Z,2,Z,Z,Z,3,},{0,1,Z,2,Z,Z,3,Z,},{0,1,Z,2,Z,Z,3,4,},{0,1,Z,2,Z,3,Z,Z,},{0,1,Z,2,Z,3,Z,4,},{0,1,Z,2,Z,3,4,Z,},{0,1,Z,2,Z,3,4,5,},
	{0,1,Z,2,3,Z,Z,Z,},{0,1,Z,2,3,Z,Z,4,},{0,1,Z,2,3,Z,4,Z,},{0,1,Z,2,3,Z,4,5,},{0,1,Z,2,3,4,Z,Z,},{0,1,Z,2,3,4,Z,5,},{0,1,Z,2,3,4,5,Z,},{0,1,Z,2,3,4,5,6,},
	{0,1,2,Z,Z,Z,Z,Z,},{0,1,2,Z,Z,Z,Z,3,},{0,1,2,Z,Z,Z,3,Z,},{0,1,2,Z,Z,Z,3,4,},{0,1,2,Z,Z,3,Z,Z,},{0,1,2,Z,Z,3,Z,4,},{0,1,2,Z,Z,3,4,Z,},{0,1,2,Z,Z,3,4,5,},
	{0,1,2,Z,3,Z,Z,Z,},{0,1,2,Z,3,Z,Z,4,},{0,1,2,Z,3,Z,4,Z,},{0,1,2,Z,3,Z,4,5,},{0,1,2,Z,3,4,Z,Z,},{0,1,2,Z,3,4,Z,5,},{0,1,2,Z,3,4,5,Z,},{0,1,2,Z,3,4,5,6,},
	{0,1,2,3,Z,Z,Z,Z,},{0,1,2,3,Z,Z,Z,4,},{0,1,2,3,Z,Z,4,Z,},{0,1,2,3,Z,Z,4,5,},{0,1,2,3,Z,4,Z,Z,},{0,1,2,3,Z,4,Z,5,},{0,1,2,3,Z,4,5,Z,},{0,1,2,3,Z,4,5,6,},
	{0,1,2,3,4,Z,Z,Z,},{0,1,2,3,4,Z,Z,5,},{0,1,2,3,4,Z,5,Z,},{0,1,2,3,4,Z,5,6,},{0,1,2,3,4,5,Z,Z,},{0,1,2,3,4,5,Z,6,},{0,1,2,3,4,5,6,Z,},{0,1,2,3,4,5,6,7,},
};
#undef Z

static const unsigned char kDecodeBytesGroupCount[256] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
};

static __m128i decodeShuffleMask(unsigned char mask0, unsigned char mask1)
{
	__m128i sm0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&kDecodeBytesGroupShuffle[mask0]));
	__m128i sm1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&kDecodeBytesGroupShuffle[mask1]));
	__m128i sm1off = _mm_set1_epi8(kDecodeBytesGroupCount[mask0]);

	__m128i sm1r = _mm_add_epi8(sm1, sm1off);

	return _mm_unpacklo_epi64(sm0, sm1r);
}

static const unsigned char* decodeBytesGroup(const unsigned char* data, unsigned char* buffer, int bitslog2)
{
	switch (bitslog2)
	{
	case 0:
	{
		unsigned char mask0 = data[0];
		unsigned char mask1 = data[1];

		__m128i rest = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 2));

		__m128i shuf = decodeShuffleMask(mask0, mask1);

		// note: this will set unused entries to 0 since shuf mask will have high bits set to 1
		__m128i result = _mm_shuffle_epi8(rest, shuf);

		_mm_storeu_si128(reinterpret_cast<__m128i*>(buffer), result);

		return data + 2 + kDecodeBytesGroupCount[mask0] + kDecodeBytesGroupCount[mask1];
	}

	case 1:
	{
		__m128i sel2 = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(data));
		__m128i rest = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 4));

		__m128i sel22 = _mm_and_si128(_mm_set1_epi8(15), _mm_unpacklo_epi8(_mm_srli_epi16(sel2, 4), sel2));
		__m128i sel2222 = _mm_and_si128(_mm_set1_epi8(3), _mm_unpacklo_epi8(_mm_srli_epi16(sel22, 2), sel22));

		__m128i mask = _mm_cmpeq_epi8(sel2222, _mm_set1_epi8(3));
		int mask16 = _mm_movemask_epi8(_mm_shuffle_epi8(mask, _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)));
		unsigned char mask0 = (unsigned char)(mask16 >> 8);
		unsigned char mask1 = (unsigned char)(mask16 & 255);

		__m128i shuf = decodeShuffleMask(mask0, mask1);

		__m128i result = _mm_or_si128(_mm_shuffle_epi8(rest, shuf), _mm_andnot_si128(mask, sel2222));

		_mm_storeu_si128(reinterpret_cast<__m128i*>(buffer), result);

		return data + 4 + kDecodeBytesGroupCount[mask0] + kDecodeBytesGroupCount[mask1];
	}

	case 2:
	{
		__m128i sel4 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(data));
		__m128i rest = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 8));

		__m128i sel44 = _mm_and_si128(_mm_set1_epi8(15), _mm_unpacklo_epi8(_mm_srli_epi16(sel4, 4), sel4));

		__m128i mask = _mm_cmpeq_epi8(sel44, _mm_set1_epi8(15));
		int mask16 = _mm_movemask_epi8(_mm_shuffle_epi8(mask, _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)));
		unsigned char mask0 = (unsigned char)(mask16 >> 8);
		unsigned char mask1 = (unsigned char)(mask16 & 255);

		__m128i shuf = decodeShuffleMask(mask0, mask1);

		__m128i result = _mm_or_si128(_mm_shuffle_epi8(rest, shuf), _mm_andnot_si128(mask, sel44));

		_mm_storeu_si128(reinterpret_cast<__m128i*>(buffer), result);

		return data + 8 + kDecodeBytesGroupCount[mask0] + kDecodeBytesGroupCount[mask1];
	}

	case 3:
	{
		__m128i rest = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));

		__m128i result = rest;

		_mm_storeu_si128(reinterpret_cast<__m128i*>(buffer), result);

		return data + 16;
	}

	default:
		assert(!"Unexpected bit length"); // This can never happen since bitslog2 is a 2-bit value
		return data;
	}
}
#else
template <int bits>
static const unsigned char* decodeBytesGroupImpl(const unsigned char* data, unsigned char* buffer)
{
	size_t byte_size = 8 / bits;
	assert(kByteGroupSize % byte_size == 0);

	const unsigned char* data_var = data + kByteGroupSize / byte_size;

	// fixed portion: bits bits for each value
	// variable portion: full byte for each out-of-range value (using 1...1 as sentinel)
	unsigned char sentinel = (1 << bits) - 1;

	for (size_t i = 0; i < kByteGroupSize; i += byte_size)
	{
		unsigned char byte = *data++;

		for (size_t k = 0; k < byte_size; ++k)
		{
			unsigned char enc = byte >> (8 - bits);
			byte <<= bits;

			unsigned char encv = *data_var;

			buffer[i + k] = (enc == sentinel) ? encv : enc;
			data_var += enc == sentinel;
		}
	}

	return data_var;
}

static const unsigned char* decodeBytesGroup(const unsigned char* data, unsigned char* buffer, int bitslog2)
{
	switch (bitslog2)
	{
	case 0:
		return decodeBytesGroupImpl<1>(data, buffer);
	case 1:
		return decodeBytesGroupImpl<2>(data, buffer);
	case 2:
		return decodeBytesGroupImpl<4>(data, buffer);
	case 3:
		memcpy(buffer, data, kByteGroupSize);
		return data + kByteGroupSize;
	default:
		assert(!"Unexpected bit length"); // This can never happen since bitslog2 is a 2-bit value
		return data;
	}
}
#endif

static const unsigned char* decodeBytes(const unsigned char* data, const unsigned char* data_end, unsigned char* buffer, size_t buffer_size)
{
	assert(buffer_size % kByteGroupSize == 0);

	if (size_t(data_end - data) < 1)
		return 0;

	unsigned char encoding = *data++;

	if (encoding == 0)
	{
		memset(buffer, 0, buffer_size);

		return data;
	}
	else if (encoding == 1)
	{
		const unsigned char* header = data;

		// round number of groups to 4 to get number of header bytes
		size_t header_size = (buffer_size / kByteGroupSize + 3) / 4;

		if (size_t(data_end - data) < header_size)
			return 0;

		data += header_size;

		size_t i = 0;

		// fast-path: process 4 groups at a time, do a shared bounds check - each group reads <=32b
		for (; i + kByteGroupSize * 4 <= buffer_size && size_t(data_end - data) <= 128; i += kByteGroupSize * 4)
		{
			size_t header_offset = i / kByteGroupSize;
			unsigned char header_byte = header[header_offset / 4];

			data = decodeBytesGroup(data, buffer + i + kByteGroupSize * 0, (header_byte >> 0) & 3);
			data = decodeBytesGroup(data, buffer + i + kByteGroupSize * 1, (header_byte >> 2) & 3);
			data = decodeBytesGroup(data, buffer + i + kByteGroupSize * 2, (header_byte >> 4) & 3);
			data = decodeBytesGroup(data, buffer + i + kByteGroupSize * 3, (header_byte >> 6) & 3);
		}

		// slow-path: process remaining groups
		for (; i < buffer_size; i += kByteGroupSize)
		{
			if (size_t(data_end - data) < 32)
				return 0;

			size_t header_offset = i / kByteGroupSize;

			int bitslog2 = (header[header_offset / 4] >> ((header_offset % 4) * 2)) & 3;

			data = decodeBytesGroup(data, buffer + i, bitslog2);
		}

		return data;
	}
	else
	{
		// TODO: malformed data, we might want to return a different error code upstream?
		return 0;
	}
}

#if SIMD
inline __m128i transpose_4x4(__m128i m)
{
	return _mm_shuffle_epi8(m, _mm_setr_epi8(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15));
}

#define combine_4_2bits(n0, n1, n2, n3) (n0 + (n1 << 2) + (n2 << 4) + (n3 << 6))
#define _128_shuffle(x, y, n0, n1, n2, n3) _mm_shuffle_ps(x, y, combine_4_2bits(n0, n1, n2, n3))
#define _128i_shuffle(x, y, n0, n1, n2, n3) _mm_castps_si128(_128_shuffle(_mm_castsi128_ps(x), _mm_castsi128_ps(y), n0, n1, n2, n3))

inline void transpose_4x4_dwords(__m128i w0, __m128i w1,
                                 __m128i w2, __m128i w3,
                                 __m128i& r0, __m128i& r1,
                                 __m128i& r2, __m128i& r3)
{
	// 0  1  2  3
	// 4  5  6  7
	// 8  9  10 11
	// 12 13 14 15

	__m128i x0 = _128i_shuffle(w0, w1, 0, 1, 0, 1); // 0 1 4 5
	__m128i x1 = _128i_shuffle(w0, w1, 2, 3, 2, 3); // 2 3 6 7
	__m128i x2 = _128i_shuffle(w2, w3, 0, 1, 0, 1); // 8 9 12 13
	__m128i x3 = _128i_shuffle(w2, w3, 2, 3, 2, 3); // 10 11 14 15

	r0 = _128i_shuffle(x0, x2, 0, 2, 0, 2);
	r1 = _128i_shuffle(x0, x2, 1, 3, 1, 3);
	r2 = _128i_shuffle(x1, x3, 0, 2, 0, 2);
	r3 = _128i_shuffle(x1, x3, 1, 3, 1, 3);
}

inline void transpose_16x16(
    __m128i& x0, __m128i& x1, __m128i& x2, __m128i& x3,
    __m128i& x4, __m128i& x5, __m128i& x6, __m128i& x7,
    __m128i& x8, __m128i& x9, __m128i& x10, __m128i& x11,
    __m128i& x12, __m128i& x13, __m128i& x14, __m128i& x15)
{
	__m128i w00, w01, w02, w03;
	__m128i w10, w11, w12, w13;
	__m128i w20, w21, w22, w23;
	__m128i w30, w31, w32, w33;

	transpose_4x4_dwords(x0, x1, x2, x3, w00, w01, w02, w03);
	transpose_4x4_dwords(x4, x5, x6, x7, w10, w11, w12, w13);
	transpose_4x4_dwords(x8, x9, x10, x11, w20, w21, w22, w23);
	transpose_4x4_dwords(x12, x13, x14, x15, w30, w31, w32, w33);
	w00 = transpose_4x4(w00);
	w01 = transpose_4x4(w01);
	w02 = transpose_4x4(w02);
	w03 = transpose_4x4(w03);
	w10 = transpose_4x4(w10);
	w11 = transpose_4x4(w11);
	w12 = transpose_4x4(w12);
	w13 = transpose_4x4(w13);
	w20 = transpose_4x4(w20);
	w21 = transpose_4x4(w21);
	w22 = transpose_4x4(w22);
	w23 = transpose_4x4(w23);
	w30 = transpose_4x4(w30);
	w31 = transpose_4x4(w31);
	w32 = transpose_4x4(w32);
	w33 = transpose_4x4(w33);
	transpose_4x4_dwords(w00, w10, w20, w30, x0, x1, x2, x3);
	transpose_4x4_dwords(w01, w11, w21, w31, x4, x5, x6, x7);
	transpose_4x4_dwords(w02, w12, w22, w32, x8, x9, x10, x11);
	transpose_4x4_dwords(w03, w13, w23, w33, x12, x13, x14, x15);
}

__m128i unzigzag8(__m128i v)
{
	__m128i xl = _mm_sub_epi8(_mm_setzero_si128(), _mm_and_si128(v, _mm_set1_epi8(1)));
	__m128i xr = _mm_and_si128(_mm_srli_epi16(v, 1), _mm_set1_epi8(127));

	return _mm_xor_si128(xl, xr);
}

static void transposeVertexBlock16(unsigned char* transposed, const unsigned char* buffer, size_t vertex_count_aligned, size_t vertex_size_16, const unsigned char* last_vertex)
{
	__m128i pi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(last_vertex));

	for (size_t j = 0; j < vertex_count_aligned; j += 16)
	{
#define LOAD(i) __m128i r##i = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + i * vertex_count_aligned))
#define FIXD(i) r##i = pi = _mm_add_epi8(pi, unzigzag8(r##i))
#define SAVE(i) _mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + (j + i) * vertex_size_16), r##i)

		LOAD(0); LOAD(1); LOAD(2); LOAD(3); LOAD(4); LOAD(5); LOAD(6); LOAD(7); LOAD(8); LOAD(9); LOAD(10); LOAD(11); LOAD(12); LOAD(13); LOAD(14); LOAD(15);

		transpose_16x16(r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15);
		FIXD(0); FIXD(1); FIXD(2); FIXD(3); FIXD(4); FIXD(5); FIXD(6); FIXD(7); FIXD(8); FIXD(9); FIXD(10); FIXD(11); FIXD(12); FIXD(13); FIXD(14); FIXD(15);

		SAVE(0); SAVE(1); SAVE(2); SAVE(3); SAVE(4); SAVE(5); SAVE(6); SAVE(7); SAVE(8); SAVE(9); SAVE(10); SAVE(11); SAVE(12); SAVE(13); SAVE(14); SAVE(15);

#undef LOAD
#undef FIXD
#undef SAVE
	}
}

static const unsigned char* decodeVertexBlock(const unsigned char* data, const unsigned char* data_end, unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= kVertexBlockMaxSize);

	unsigned char buffer[kVertexBlockMaxSize * 16];
	unsigned char transposed[kVertexBlockSizeBytes];

	size_t vertex_size_16 = (vertex_size + 15) & ~15;
	size_t vertex_count_aligned = (vertex_count + kByteGroupSize - 1) & ~(kByteGroupSize - 1);

	assert(vertex_size_16 * vertex_count_aligned <= sizeof(transposed));

	for (size_t k = 0; k < vertex_size; k += 16)
	{
		for (size_t j = 0; j < 16 && k + j < vertex_size; ++j)
		{
			data = decodeBytes(data, data_end, buffer + j * vertex_count_aligned, vertex_count_aligned);
			if (!data)
				return 0;
		}

		transposeVertexBlock16(transposed + k, buffer, vertex_count_aligned, vertex_size_16, &last_vertex[k]);
	}

	if (vertex_size & 15)
	{
		size_t read = vertex_size_16;
		size_t write = vertex_size;

		for (size_t i = 1; i < vertex_count; ++i)
		{
			for (size_t k = 0; k < vertex_size; k += 16)
			{
				__m128i v = _mm_loadu_si128(reinterpret_cast<__m128i*>(&transposed[read + k]));
				_mm_storeu_si128(reinterpret_cast<__m128i*>(&transposed[write + k]), v);
			}

			read += vertex_size_16;
			write += vertex_size;
		}
	}

	memcpy(vertex_data, transposed, vertex_count * vertex_size);

	memcpy(last_vertex, &transposed[vertex_size * (vertex_count - 1)], vertex_size);

	return data;
}
#else
static const unsigned char* decodeVertexBlock(const unsigned char* data, const unsigned char* data_end, unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= kVertexBlockMaxSize);

	unsigned char buffer[kVertexBlockMaxSize];
	assert(sizeof(buffer) % kByteGroupSize == 0);

	for (size_t k = 0; k < vertex_size; ++k)
	{
		data = decodeBytes(data, data_end, buffer, (vertex_count + kByteGroupSize - 1) & ~(kByteGroupSize - 1));
		if (!data)
			return 0;

		size_t vertex_offset = k;

		unsigned char p = last_vertex[k];

		for (size_t i = 0; i < vertex_count; ++i)
		{
			unsigned char v = unzigzag8(buffer[i]) + p;

			vertex_data[vertex_offset] = v;
			p = v;

			vertex_offset += vertex_size;
		}

		last_vertex[k] = p;
	}

	return data;
}
#endif

} // namespace meshopt

size_t meshopt_encodeVertexBuffer(unsigned char* buffer, size_t buffer_size, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	using namespace meshopt;

	assert(vertex_size > 0 && vertex_size <= 256);

	const unsigned char* vertex_data = static_cast<const unsigned char*>(vertices);

	unsigned char* data = buffer;

	unsigned char last_vertex[256];
	memcpy(last_vertex, vertex_data, vertex_size);

	size_t vertex_block_size = getVertexBlockSize(vertex_size);

	size_t vertex_offset = 0;

	while (vertex_offset < vertex_count)
	{
		size_t block_size = (vertex_offset + vertex_block_size < vertex_count) ? vertex_block_size : vertex_count - vertex_offset;

		data = encodeVertexBlock(data, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, last_vertex);
		vertex_offset += block_size;
	}

	// write first vertex to the end of the stream and pad it to 32 bytes; this is important to simplify bounds checks in decoder
	if (vertex_size < 32)
	{
		memset(data, 0, 32 - vertex_size);
		data += 32 - vertex_size;
	}

	memcpy(data, vertex_data, vertex_size);
	data += vertex_size;

	assert(size_t(data - buffer) <= buffer_size);
	(void)buffer_size;

	return data - buffer;
}

size_t meshopt_encodeVertexBufferBound(size_t vertex_count, size_t vertex_size)
{
	using namespace meshopt;

	size_t vertex_block_size = getVertexBlockSize(vertex_size);
	size_t vertex_block_count = (vertex_count + vertex_block_size - 1) / vertex_block_size;

	size_t vertex_block_header_size = 1 + (vertex_block_size / kByteGroupSize + 3) / 4;
	size_t vertex_block_data_size = vertex_block_size;

	size_t tail_size = vertex_size < 32 ? 32 : vertex_size;

	return vertex_block_count * vertex_size * (vertex_block_header_size + vertex_block_data_size) + tail_size;
}

int meshopt_decodeVertexBuffer(void* destination, size_t vertex_count, size_t vertex_size, const unsigned char* buffer, size_t buffer_size)
{
	using namespace meshopt;

	assert(vertex_size > 0 && vertex_size <= 256);

	unsigned char* vertex_data = static_cast<unsigned char*>(destination);

	const unsigned char* data = buffer;
	const unsigned char* data_end = buffer + buffer_size;

	if (size_t(data_end - data) < vertex_size)
		return -1;

	unsigned char last_vertex[256];
	memcpy(last_vertex, data_end - vertex_size, vertex_size);

	size_t vertex_block_size = getVertexBlockSize(vertex_size);

	size_t vertex_offset = 0;

	while (vertex_offset < vertex_count)
	{
		size_t block_size = (vertex_offset + vertex_block_size < vertex_count) ? vertex_block_size : vertex_count - vertex_offset;

		data = decodeVertexBlock(data, data_end, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, last_vertex);
		if (!data)
			return -2;

		vertex_offset += block_size;
	}

	size_t tail_size = vertex_size < 32 ? 32 : vertex_size;

	if (size_t(data_end - data) != tail_size)
		return -3;

	return 0;
}
