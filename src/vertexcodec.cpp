// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

#include <stdio.h>

#define SIMD 0

#if SIMD
#include <tmmintrin.h>
#endif

// This work is based on:
// TODO: references
namespace meshopt
{

const size_t kVertexBlockSizeBytes = 8192;
const size_t kVertexBlockMaxSize = 256;
const size_t kByteGroupSize = 16;

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

#if SIMD
static unsigned char kDecodeBytesGroupShuffle[256][8];
static unsigned char kDecodeBytesGroupCount[256];
static bool gDecodeBytesGroupInitialized;

static void decodeBytesGroupBuildTables()
{
	if (gDecodeBytesGroupInitialized)
		return;

	for (int mask = 0; mask < 256; ++mask)
	{
		unsigned char shuffle[8];
		memset(shuffle, 0x80, 8);

		unsigned char count = 0;

		for (int i = 0; i < 8; ++i)
			if (mask & (1 << (7 - i)))
			{
				shuffle[i] = count;
				count++;
			}

		memcpy(kDecodeBytesGroupShuffle[mask], shuffle, 8);
		kDecodeBytesGroupCount[mask] = count;
	}

	gDecodeBytesGroupInitialized = true;
}

static __m128i decodeShuffleMask(unsigned char mask0, unsigned char mask1)
{
	__m128i sm0 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&kDecodeBytesGroupShuffle[mask0]));
	__m128i sm1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&kDecodeBytesGroupShuffle[mask1]));

	unsigned char cnt0 = kDecodeBytesGroupCount[mask0];
	__m128i sm1off = _mm_add_epi8(sm1, _mm_set1_epi8(cnt0));

	return _mm_or_si128(sm0, _mm_slli_si128(sm1off, 8));
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

		for (size_t i = 0; i < buffer_size; i += kByteGroupSize)
		{
			size_t header_offset = i / kByteGroupSize;

			int bitslog2 = (header[header_offset / 4] >> ((header_offset % 4) * 2)) & 3;

			// TODO: OOB check; decodeBytesGroup can statically read at most 8+16=24 bytes
			data = decodeBytesGroup(data, buffer + i, bitslog2);
			if (!data)
				return 0;
		}

		return data;
	}
	else
	{
		// TODO: malformed data, we might want to return a different error code upstream?
		return 0;
	}
}

size_t getVertexBlockSize(size_t vertex_size)
{
	// make sure the entire block fits into the scratch buffer
	size_t result = kVertexBlockSizeBytes / vertex_size;

	// align to byte group size; we encode each byte as a byte group
	// if vertex block is misaligned, it results in wasted bytes, so just truncate the block size
	result &= ~(kByteGroupSize - 1);

	return (result < kVertexBlockMaxSize) ? result : kVertexBlockMaxSize;
}

static unsigned char* encodeVertexBlock(unsigned char* data, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, const unsigned int* prediction, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= kVertexBlockMaxSize);

	unsigned char buffer[kVertexBlockMaxSize];
	assert(sizeof(buffer) % kByteGroupSize == 0);

	// we sometimes encode elements we didn't fill when rounding to kByteGroupSize
	memset(buffer, 0, sizeof(buffer));

	for (size_t k = 0; k < vertex_size; ++k)
	{
		size_t vertex_offset = k;

		for (size_t i = 0; i < vertex_count; ++i)
		{
			unsigned char p = (i == 0) ? last_vertex[k] : vertex_data[vertex_offset - vertex_size];

			if (prediction && prediction[i])
			{
				unsigned int pa = (prediction[i] >> 16) & 0xff;
				unsigned int pb = (prediction[i] >> 8) & 0xff;
				unsigned int pc = (prediction[i] >> 0) & 0xff;
				assert(pa > 0 && pb > 0 && pc > 0);

				if (pa <= i && pb <= i && pc <= i)
				{
					unsigned char va = vertex_data[vertex_offset - pa * vertex_size];
					unsigned char vb = vertex_data[vertex_offset - pb * vertex_size];
					unsigned char vc = vertex_data[vertex_offset - pc * vertex_size];

					p = va + vb - vc;
				}
			}

			unsigned char delta = zigzag8(vertex_data[vertex_offset] - p);

			buffer[i] = delta;

			vertex_offset += vertex_size;
		}

		data = encodeBytes(data, buffer, (vertex_count + kByteGroupSize - 1) & ~(kByteGroupSize - 1));
	}

	for (size_t k = 0; k < vertex_size; ++k)
	{
		last_vertex[k] = vertex_data[vertex_size * (vertex_count - 1) + k];
	}

	return data;
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

static void transposeVertexBlock16(unsigned char* transposed, const unsigned char* buffer, size_t vertex_count_aligned)
{
	for (size_t j = 0; j < vertex_count_aligned; j += 16)
	{
		__m128i r0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 0 * vertex_count_aligned));
		__m128i r1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 1 * vertex_count_aligned));
		__m128i r2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 2 * vertex_count_aligned));
		__m128i r3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 3 * vertex_count_aligned));
		__m128i r4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 4 * vertex_count_aligned));
		__m128i r5 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 5 * vertex_count_aligned));
		__m128i r6 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 6 * vertex_count_aligned));
		__m128i r7 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 7 * vertex_count_aligned));
		__m128i r8 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 8 * vertex_count_aligned));
		__m128i r9 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 9 * vertex_count_aligned));
		__m128i r10 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 10 * vertex_count_aligned));
		__m128i r11 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 11 * vertex_count_aligned));
		__m128i r12 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 12 * vertex_count_aligned));
		__m128i r13 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 13 * vertex_count_aligned));
		__m128i r14 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 14 * vertex_count_aligned));
		__m128i r15 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + j + 15 * vertex_count_aligned));

		transpose_16x16(r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15);

		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 0 * 16), r0);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 1 * 16), r1);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 2 * 16), r2);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 3 * 16), r3);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 4 * 16), r4);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 5 * 16), r5);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 6 * 16), r6);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 7 * 16), r7);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 8 * 16), r8);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 9 * 16), r9);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 10 * 16), r10);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 11 * 16), r11);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 12 * 16), r12);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 13 * 16), r13);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 14 * 16), r14);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(transposed + j * 16 + 15 * 16), r15);
	}
}

static const unsigned char* decodeVertexBlock(const unsigned char* data, const unsigned char* data_end, unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, const unsigned int* prediction, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= kVertexBlockMaxSize);
	assert(vertex_size % 16 == 0); // TODO

	unsigned char buffer[kVertexBlockMaxSize * 16];
	unsigned char transposed[kVertexBlockMaxSize * 16];

	size_t vertex_count_aligned = (vertex_count + kByteGroupSize - 1) & ~(kByteGroupSize - 1);

	for (size_t k = 0; k < vertex_size; k += 16)
	{
		for (size_t j = 0; j < 16; ++j)
		{
			data = decodeBytes(data, data_end, buffer + j * vertex_count_aligned, vertex_count_aligned);
			if (!data)
				return 0;
		}

		transposeVertexBlock16(transposed, buffer, vertex_count_aligned);

		size_t vertex_offset = k;

		__m128i pi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&last_vertex[k]));

		for (size_t i = 0; i < vertex_count; ++i)
		{
			__m128i p = pi;

			if (prediction && prediction[i])
			{
				unsigned int pa = (prediction[i] >> 16) & 0xff;
				unsigned int pb = (prediction[i] >> 8) & 0xff;
				unsigned int pc = (prediction[i] >> 0) & 0xff;
				assert(pa > 0 && pb > 0 && pc > 0);

				if (pa <= i && pb <= i && pc <= i)
				{
					__m128i va = _mm_loadu_si128(reinterpret_cast<__m128i*>(&transposed[(i - pa) * 16]));
					__m128i vb = _mm_loadu_si128(reinterpret_cast<__m128i*>(&transposed[(i - pb) * 16]));
					__m128i vc = _mm_loadu_si128(reinterpret_cast<__m128i*>(&transposed[(i - pc) * 16]));

					p = _mm_sub_epi8(_mm_add_epi8(va, vb), vc);
				}
			}

			__m128i v = _mm_add_epi8(unzigzag8(_mm_loadu_si128(reinterpret_cast<__m128i*>(&transposed[i * 16]))), p);

			_mm_storeu_si128(reinterpret_cast<__m128i*>(&vertex_data[vertex_offset]), v);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(&transposed[i * 16]), v);

			pi = v;

			vertex_offset += vertex_size;
		}
	}

	for (size_t k = 0; k < vertex_size; ++k)
	{
		last_vertex[k] = vertex_data[vertex_size * (vertex_count - 1) + k];
	}

	return data;
}
#else
static const unsigned char* decodeVertexBlock(const unsigned char* data, const unsigned char* data_end, unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, const unsigned int* prediction, unsigned char last_vertex[256])
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

		for (size_t i = 0; i < vertex_count; ++i)
		{
			unsigned char p = (i == 0) ? last_vertex[k] : vertex_data[vertex_offset - vertex_size];

			if (prediction && prediction[i])
			{
				unsigned int pa = (prediction[i] >> 16) & 0xff;
				unsigned int pb = (prediction[i] >> 8) & 0xff;
				unsigned int pc = (prediction[i] >> 0) & 0xff;
				assert(pa > 0 && pb > 0 && pc > 0);

				if (pa <= i && pb <= i && pc <= i)
				{
					unsigned char va = vertex_data[vertex_offset - pa * vertex_size];
					unsigned char vb = vertex_data[vertex_offset - pb * vertex_size];
					unsigned char vc = vertex_data[vertex_offset - pc * vertex_size];

					p = va + vb - vc;
				}
			}

			vertex_data[vertex_offset] = unzigzag8(buffer[i]) + p;

			vertex_offset += vertex_size;
		}
	}

	for (size_t k = 0; k < vertex_size; ++k)
	{
		last_vertex[k] = vertex_data[vertex_size * (vertex_count - 1) + k];
	}

	return data;
}
#endif

typedef unsigned int VertexFifo[16];
typedef unsigned int EdgeFifo[16][3];

static void pushEdgeFifo(EdgeFifo fifo, unsigned int a, unsigned int b, unsigned int c, size_t& offset)
{
	fifo[offset][0] = a;
	fifo[offset][1] = b;
	fifo[offset][2] = c;
	offset = (offset + 1) & 15;
}

static void pushVertexFifo(VertexFifo fifo, unsigned int v, size_t& offset)
{
	fifo[offset] = v;
	offset = (offset + 1) & 15;
}

static unsigned int decodeVByte(const unsigned char*& data)
{
	unsigned char lead = *data++;

	// fast path: single byte
	if (lead < 128)
		return lead;

	// slow path: up to 4 extra bytes
	// note that this loop always terminates, which is important for malformed data
	unsigned int result = lead & 127;
	unsigned int shift = 7;

	for (int i = 0; i < 4; ++i)
	{
		unsigned char group = *data++;
		result |= (group & 127) << shift;
		shift += 7;

		if (group < 128)
			break;
	}

	return result;
}

static unsigned int decodeIndex(const unsigned char*& data, unsigned int next, unsigned int last)
{
	(void)next;

	unsigned int v = decodeVByte(data);
	unsigned int d = (v >> 1) ^ -int(v & 1);

	return last + d;
}

struct DecodePredictionState
{
	EdgeFifo edgefifo;
	VertexFifo vertexfifo;
	size_t edgefifooffset;
	size_t vertexfifooffset;

	unsigned int next;
	unsigned int last;

	size_t code_offset;
	size_t data_offset;

	size_t index_offset;
};

static size_t decodeVertexPrediction(DecodePredictionState& state, unsigned int* result, size_t result_size, size_t index_count, const unsigned char* buffer, size_t buffer_size)
{
	assert(index_count % 3 == 0);

	// the minimum valid encoding is 1 byte per triangle and a 16-byte codeaux table
	if (buffer_size < index_count / 3 + 16)
		return 0;

	size_t edgefifooffset = state.edgefifooffset;
	size_t vertexfifooffset = state.vertexfifooffset;

	unsigned int next = state.next;
	unsigned int last = state.last;

	// since we store 16-byte codeaux table at the end, triangle data has to begin before data_safe_end
	const unsigned char* code = buffer + state.code_offset;
	const unsigned char* data = buffer + index_count / 3 + state.data_offset;
	const unsigned char* data_safe_end = buffer + buffer_size - 16;

	const unsigned char* codeaux_table = data_safe_end;

	size_t result_offset = 0;
	size_t i = state.index_offset;

	for (; i < index_count; i += 3)
	{
		if (result_offset + 3 > result_size)
			break;

		// make sure we have enough data to read for a triangle
		// each triangle reads at most 16 bytes of data: 1b for codeaux and 5b for each free index
		// after this we can be sure we can read without extra bounds checks
		if (data > data_safe_end)
			return 0;

		unsigned char codetri = *code++;

		int fe = codetri >> 4;

		if (fe < 15)
		{
			// fifo reads are wrapped around 16 entry buffer
			unsigned int a = state.edgefifo[(edgefifooffset - 1 - fe) & 15][0];
			unsigned int b = state.edgefifo[(edgefifooffset - 1 - fe) & 15][1];
			unsigned int co = state.edgefifo[(edgefifooffset - 1 - fe) & 15][2];

			int fec = codetri & 15;

			unsigned int c = (fec == 0) ? next++ : state.vertexfifo[(vertexfifooffset - 1 - fec) & 15];

			// note that we need to update the last index since free indices are delta-encoded
			if (fec == 15)
				last = c = decodeIndex(data, next, last);

			// output prediction data
			if (fec == 0)
			{
				unsigned int na = c - a;
				unsigned int nb = c - b;
				unsigned int nc = c - co;

				unsigned int p = (na << 16) | (nb << 8) | nc;

				result[result_offset++] = (na | nb | nc) < 256 ? p : 0;
			}

			// push vertex/edge fifo must match the encoding step *exactly* otherwise the data will not be decoded correctly
			if (fec == 0 || fec == 15)
				pushVertexFifo(state.vertexfifo, c, vertexfifooffset);

			pushEdgeFifo(state.edgefifo, c, b, a, edgefifooffset);
			pushEdgeFifo(state.edgefifo, a, c, b, edgefifooffset);
		}
		else
		{
			// fast path: read codeaux from the table; we wrap table index so this access is memory-safe
			// slow path: read a full byte for codeaux instead of using a table lookup
			unsigned char codeaux = (codetri & 15) >= 14 ? *data++ : codeaux_table[codetri & 15];

			int fea = (codetri & 15) == 15 ? 15 : 0;
			int feb = codeaux >> 4;
			int fec = codeaux & 15;

			// fifo reads are wrapped around 16 entry buffer
			// also note that we increment next for all three vertices before decoding indices - this matches encoder behavior
			unsigned int a = (fea == 0) ? next++ : 0;
			unsigned int b = (feb == 0) ? next++ : state.vertexfifo[(vertexfifooffset - feb) & 15];
			unsigned int c = (fec == 0) ? next++ : state.vertexfifo[(vertexfifooffset - fec) & 15];

			// note that we need to update the last index since free indices are delta-encoded
			if (fea == 15)
				last = a = decodeIndex(data, next, last);

			if (feb == 15)
				last = b = decodeIndex(data, next, last);

			if (fec == 15)
				last = c = decodeIndex(data, next, last);

			// output prediction data
			if (fea == 0)
				result[result_offset++] = 0;

			if (feb == 0)
				result[result_offset++] = 0;

			if (fec == 0)
				result[result_offset++] = 0;

			// push vertex/edge fifo must match the encoding step *exactly* otherwise the data will not be decoded correctly
			if (fea == 0 || fea == 15)
				pushVertexFifo(state.vertexfifo, a, vertexfifooffset);

			if (feb == 0 || feb == 15)
				pushVertexFifo(state.vertexfifo, b, vertexfifooffset);

			if (fec == 0 || fec == 15)
				pushVertexFifo(state.vertexfifo, c, vertexfifooffset);

			pushEdgeFifo(state.edgefifo, b, a, c, edgefifooffset);
			pushEdgeFifo(state.edgefifo, c, b, a, edgefifooffset);
			pushEdgeFifo(state.edgefifo, a, c, b, edgefifooffset);
		}
	}

	// we should've read all data bytes and stopped at the boundary between data and codeaux table
	if (i == index_count && data != data_safe_end)
		return 0;

	state.code_offset = code - buffer;
	state.data_offset = data - buffer - index_count / 3;
	state.index_offset = i;

	state.edgefifooffset = edgefifooffset;
	state.vertexfifooffset = vertexfifooffset;

	state.next = next;
	state.last = last;

	return result_offset;
}

} // namespace meshopt

size_t meshopt_encodeVertexBuffer(unsigned char* buffer, size_t buffer_size, const void* vertices, size_t vertex_count, size_t vertex_size, size_t index_count, const unsigned char* index_buffer, size_t index_buffer_size)
{
	using namespace meshopt;

	assert(vertex_size > 0 && vertex_size <= 256);
	assert(index_count % 3 == 0);
	assert(index_buffer == 0 || index_buffer_size > 0);

	const unsigned char* vertex_data = static_cast<const unsigned char*>(vertices);

	unsigned char* data = buffer;

	unsigned char last_vertex[256];

	for (size_t k = 0; k < vertex_size; ++k)
	{
		last_vertex[k] = vertex_data[k];

		*data++ = last_vertex[k];
	}

	size_t vertexBlockSize = getVertexBlockSize(vertex_size);

	const size_t prediction_capacity = kVertexBlockMaxSize + 2;
	unsigned int prediction[prediction_capacity];

	DecodePredictionState pstate = {};

	size_t vertex_offset = 0;
	size_t prediction_offset = 0;

	if (index_buffer)
	{
		for (;;)
		{
			size_t psize = decodeVertexPrediction(pstate, prediction + prediction_offset, prediction_capacity - prediction_offset, index_count, index_buffer, index_buffer_size);
			if (psize == 0)
				break;

			size_t block_size = psize + prediction_offset;

			if (vertex_offset + block_size > vertex_count)
				break;

			size_t block_size_clamped = (block_size > vertexBlockSize) ? vertexBlockSize : block_size;

			data = encodeVertexBlock(data, vertex_data + vertex_offset * vertex_size, block_size_clamped, vertex_size, prediction, last_vertex);
			vertex_offset += block_size_clamped;

			prediction_offset = block_size - block_size_clamped;
			memset(&prediction[0], 0, prediction_offset * sizeof(prediction[0]));
		}
	}

	while (vertex_offset < vertex_count)
	{
		size_t block_size = (vertex_offset + vertexBlockSize < vertex_count) ? vertexBlockSize : vertex_count - vertex_offset;

		data = encodeVertexBlock(data, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, 0, last_vertex);
		vertex_offset += block_size;
	}

	assert(size_t(data - buffer) <= buffer_size);
	(void)buffer_size;

	return data - buffer;
}

size_t meshopt_encodeVertexBufferBound(size_t vertex_count, size_t vertex_size)
{
	// TODO: This significantly overestimates worst case, refine
	return vertex_count * vertex_size * 2;
}

int meshopt_decodeVertexBuffer(void* destination, size_t vertex_count, size_t vertex_size, size_t index_count, const unsigned char* buffer, size_t buffer_size, const unsigned char* index_buffer, size_t index_buffer_size)
{
	using namespace meshopt;

	assert(vertex_size > 0 && vertex_size <= 256);
	assert(index_count % 3 == 0);
	assert(index_buffer == 0 || index_buffer_size > 0);

#if SIMD
	decodeBytesGroupBuildTables();
#endif

	unsigned char* vertex_data = static_cast<unsigned char*>(destination);

	const unsigned char* data = buffer;
	const unsigned char* data_end = buffer + buffer_size;

	if (size_t(data_end - data) < vertex_size)
		return -1;

	unsigned char last_vertex[256];

	// TODO: bounds checks on data
	for (size_t k = 0; k < vertex_size; ++k)
	{
		last_vertex[k] = *data++;

		vertex_data[k] = last_vertex[k];
	}

	size_t vertexBlockSize = getVertexBlockSize(vertex_size);

	const size_t prediction_capacity = kVertexBlockMaxSize + 2;
	unsigned int prediction[prediction_capacity];

	DecodePredictionState pstate = {};

	size_t vertex_offset = 0;
	size_t prediction_offset = 0;

	if (index_buffer)
	{
		for (;;)
		{
			size_t psize = decodeVertexPrediction(pstate, prediction + prediction_offset, prediction_capacity - prediction_offset, index_count, index_buffer, index_buffer_size);
			if (psize == 0)
				break;

			size_t block_size = psize + prediction_offset;

			if (vertex_offset + block_size > vertex_count)
				break;

			size_t block_size_clamped = (block_size > vertexBlockSize) ? vertexBlockSize : block_size;

			data = decodeVertexBlock(data, data_end, vertex_data + vertex_offset * vertex_size, block_size_clamped, vertex_size, prediction, last_vertex);
			if (!data)
				return -2;

			vertex_offset += block_size_clamped;

			prediction_offset = block_size - block_size_clamped;
			memset(&prediction[0], 0, prediction_offset * sizeof(prediction[0]));
		}
	}

	while (vertex_offset < vertex_count)
	{
		size_t block_size = (vertex_offset + vertexBlockSize < vertex_count) ? vertexBlockSize : vertex_count - vertex_offset;

		data = decodeVertexBlock(data, data_end, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, 0, last_vertex);
		if (!data)
			return -2;

		vertex_offset += block_size;
	}

	if (data != data_end)
		return -3;

	return 0;
}
