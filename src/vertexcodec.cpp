// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

#include <stdio.h>

#define TRACE 0

// This work is based on:
// TODO: references
namespace meshopt
{

const size_t kVertexBlockSize = 256;
const size_t kByteGroupSize = 16;

inline unsigned char zigzag8(unsigned char v)
{
	return (v >> 7) | ((v ^ -(v >> 7)) << 1);
}

inline unsigned char unzigzag8(unsigned char v)
{
	return (-(v & 1)) ^ (v >> 1);
}

#if TRACE > 0
inline int bits(unsigned char v)
{
	int result = 0;
	while (v >= (1 << result))
		result++;

	return result;
}

inline int bitsset(unsigned char v)
{
	int result = 0;

	while (v)
	{
		result += (v & 1);
		v >>= 1;
	}

	return result;
}
#endif

#if TRACE > 1
static void traceEncodeVertexBlock(const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, const unsigned int* prediction)
{
	printf("vertex block; count %d\n", int(vertex_count));

	{
		for (size_t k = 0; k < vertex_size; ++k)
		{
			printf("%02x    ", vertex_data[k]);
		}

		printf("| base\n");
	}

	int uniq[256] = {};
	int max[256] = {};
	int orv[256] = {};
	int sumb[256] = {};
	bool uniqb[256][256] = {};

	for (size_t i = 1; i < vertex_count; ++i)
	{
		for (size_t k = 0; k < vertex_size; ++k)
		{
			size_t vertex_offset = i * vertex_size + k;

			unsigned char p = vertex_data[vertex_offset - vertex_size];

			if (prediction && prediction[i])
			{
				unsigned char pa = prediction[i] >> 16;
				unsigned char pb = prediction[i] >> 8;
				unsigned char pc = prediction[i] >> 0;
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

			if (!uniqb[k][delta])
			{
				uniqb[k][delta] = true;
				uniq[k]++;
			}

			if (delta > max[k])
			{
				max[k] = delta;
			}

			orv[k] |= delta;

			sumb[k] += bits(delta);

		#if TRACE > 2
			printf("%02x/%02x ", vertex_data[vertex_offset], delta);
		#endif
		}

	#if TRACE > 2
		printf("| ");

		if (prediction && prediction[i])
		{
			unsigned char pa = prediction[i] >> 16;
			unsigned char pb = prediction[i] >> 8;
			unsigned char pc = prediction[i] >> 0;
			assert(pa > 0 && pb > 0 && pc > 0);

			if (pa <= i && pb <= i && pc <= i)
			{
				printf("pgram %d %d %d", pa, pb, pc);
			}
			else
			{
				printf("pdelta");
			}
		}
		else
		{
			printf("delta");
		}

		printf("\n");
	#endif
	}

	for (size_t k = 0; k < vertex_size; ++k)
		printf("%-3d   ", uniq[k]);

	printf("| uniq\n");

	for (size_t k = 0; k < vertex_size; ++k)
		printf("%02x    ", max[k]);

	printf("| max\n");

	for (size_t k = 0; k < vertex_size; ++k)
		printf("%d     ", bits(max[k]));

	printf("| maxbits\n");

	for (size_t k = 0; k < vertex_size; ++k)
		printf("%3.1f   ", double(sumb[k]) / double(vertex_count - 1));

	printf("| avgbits\n");

	for (size_t k = 0; k < vertex_size; ++k)
		printf("%d     ", bitsset(orv[k]));

	printf("| bits set\n");
}
#endif

#if TRACE > 0
struct EncodeVertexBlockStats
{
	size_t bytes[256];
	size_t bitsopt[256];
	size_t bitsenc[256];

	size_t headers[256];
	size_t content[256];

	size_t current_headers;
	size_t current_content;
};

static EncodeVertexBlockStats encodeVertexBlockStats;

static void dumpEncodeVertexBlockStats(size_t vertex_count, size_t vertex_size)
{
	const EncodeVertexBlockStats& stats = encodeVertexBlockStats;

	size_t bytes = 0;
	size_t bitsopt = 0;
	size_t bitsenc = 0;
	size_t headers = 0;
	size_t content = 0;

	for (size_t k = 0; k < 256; ++k)
		if (stats.bytes[k])
		{
			printf("%2d: %d bytes (optimal %d bytes, optenc %d bytes; headers %d, content %d)\n", int(k), int(stats.bytes[k]), int(stats.bitsopt[k]) / 8, int(stats.bitsenc[k]) / 8, int(stats.headers[k]), int(stats.content[k]));
			bytes += stats.bytes[k];
			bitsopt += stats.bitsopt[k];
			bitsenc += stats.bitsenc[k];
			headers += stats.headers[k];
			content += stats.content[k];
		}

	printf("total: %d bytes (optimal %dd bytes, optenc %d bytes; headers %d, content %d)\n", int(bytes), int(bitsopt) / 8, int(bitsenc) / 8, int(headers), int(content));

	if (vertex_size == 16)
	{
		// assume the following layout:
		// 6b position
		// 2b padding
		// 3b normal
		// 1b padding
		// 4b uv
		size_t bytes_pos = stats.bytes[0] + stats.bytes[1] + stats.bytes[2] + stats.bytes[3] + stats.bytes[4] + stats.bytes[5] + stats.bytes[6] + stats.bytes[7];
		size_t bytes_nrm = stats.bytes[8] + stats.bytes[9] + stats.bytes[10] + stats.bytes[11];
		size_t bytes_tex = stats.bytes[12] + stats.bytes[13] + stats.bytes[14] + stats.bytes[15];

		printf("pos: %d bytes, %.1f bpv\n", int(bytes_pos), float(bytes_pos) / float(vertex_count) * 8);
		printf("nrm: %d bytes, %.1f bpv\n", int(bytes_nrm), float(bytes_nrm) / float(vertex_count) * 8);
		printf("tex: %d bytes, %.1f bpv\n", int(bytes_tex), float(bytes_tex) / float(vertex_count) * 8);
	}
}
#endif

static bool encodeBytesFits(const unsigned char* buffer, size_t buffer_size, int bits)
{
	for (size_t k = 0; k < buffer_size; ++k)
		if (buffer[k] >= (1 << bits))
			return false;

	return true;
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

static const unsigned char* decodeBytesGroup(const unsigned char* data, const unsigned char* data_end, unsigned char* buffer, int bits)
{
	assert(bits >= 1 && bits <= 8);

	// TODO: missing OOB data checks
	(void)data_end;

	if (bits == 8)
	{
		memcpy(buffer, data, kByteGroupSize);

		return data + kByteGroupSize;
	}

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

			buffer[i + k] = (enc == sentinel) ? *data_var++ : enc;
		}
	}

	return data_var;
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

	#if TRACE > 0
		encodeVertexBlockStats.current_headers += header_size;
	#endif

		for (size_t i = 0; i < buffer_size; i += kByteGroupSize)
		{
			int best_bits = 8;
			size_t best_size = kByteGroupSize; // assume encodeBytesVar(8) just stores as is

			for (int bits = 1; bits < 8; bits *= 2)
			{
				unsigned char* end = encodeBytesGroup(data, buffer + i, bits);

				if (size_t(end - data) < best_size)
				{
					best_bits = bits;
					best_size = end - data;
				}
			}

			int bitslog2 = (best_bits == 1) ? 0 : (best_bits == 2) ? 1 : (best_bits == 4) ? 2 : 3;
			assert((1 << bitslog2) == best_bits);

			size_t header_offset = i / kByteGroupSize;

			header[header_offset / 4] |= bitslog2 << ((header_offset % 4) * 2);

			data = encodeBytesGroup(data, buffer + i, best_bits);
		}

	#if TRACE > 0
		encodeVertexBlockStats.current_content += data - header - header_size;
	#endif

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
			int bits = 1 << bitslog2;

			data = decodeBytesGroup(data, data_end, buffer + i, bits);
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

static unsigned char* encodeVertexBlock(unsigned char* data, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, const unsigned int* prediction, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= 256);

#if TRACE > 1
	traceEncodeVertexBlock(vertex_data, vertex_count, vertex_size, prediction);
#endif

	unsigned char buffer[256];
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

	#if TRACE > 0
		unsigned char* olddata = data;
	#endif

		data = encodeBytes(data, buffer, (vertex_count + kByteGroupSize - 1) & ~(kByteGroupSize - 1));

	#if TRACE > 0
		EncodeVertexBlockStats& stats = encodeVertexBlockStats;

		stats.bytes[k] += data - olddata;

		for (size_t i = 0; i < vertex_count; ++i)
		{
			stats.bitsopt[k] += bits(buffer[i]);
			stats.bitsenc[k] += bits(buffer[i]) + bits(bits(buffer[i]));
		}

		stats.headers[k] += stats.current_headers;
		stats.content[k] += stats.current_content;

		stats.current_headers = 0;
		stats.current_content = 0;
	#endif
	}

	for (size_t k = 0; k < vertex_size; ++k)
	{
		last_vertex[k] = vertex_data[vertex_size * (vertex_count - 1) + k];
	}

	return data;
}

static const unsigned char* decodeVertexBlock(const unsigned char* data, const unsigned char* data_end, unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, const unsigned int* prediction, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= 256);

	unsigned char buffer[256];
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
			unsigned int a = state.edgefifo[(state.edgefifooffset - 1 - fe) & 15][0];
			unsigned int b = state.edgefifo[(state.edgefifooffset - 1 - fe) & 15][1];
			unsigned int co = state.edgefifo[(state.edgefifooffset - 1 - fe) & 15][2];

			int fec = codetri & 15;

			unsigned int c = (fec == 0) ? state.next++ : state.vertexfifo[(state.vertexfifooffset - 1 - fec) & 15];

			// note that we need to update the last index since free indices are delta-encoded
			if (fec == 15)
				state.last = c = decodeIndex(data, state.next, state.last);

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
				pushVertexFifo(state.vertexfifo, c, state.vertexfifooffset);

			pushEdgeFifo(state.edgefifo, c, b, a, state.edgefifooffset);
			pushEdgeFifo(state.edgefifo, a, c, b, state.edgefifooffset);
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
			unsigned int a = (fea == 0) ? state.next++ : 0;
			unsigned int b = (feb == 0) ? state.next++ : state.vertexfifo[(state.vertexfifooffset - feb) & 15];
			unsigned int c = (fec == 0) ? state.next++ : state.vertexfifo[(state.vertexfifooffset - fec) & 15];

			// note that we need to update the last index since free indices are delta-encoded
			if (fea == 15)
				state.last = a = decodeIndex(data, state.next, state.last);

			if (feb == 15)
				state.last = b = decodeIndex(data, state.next, state.last);

			if (fec == 15)
				state.last = c = decodeIndex(data, state.next, state.last);

			// output prediction data
			if (fea == 0)
				result[result_offset++] = 0;

			if (feb == 0)
				result[result_offset++] = 0;

			if (fec == 0)
				result[result_offset++] = 0;

			// push vertex/edge fifo must match the encoding step *exactly* otherwise the data will not be decoded correctly
			if (fea == 0 || fea == 15)
				pushVertexFifo(state.vertexfifo, a, state.vertexfifooffset);

			if (feb == 0 || feb == 15)
				pushVertexFifo(state.vertexfifo, b, state.vertexfifooffset);

			if (fec == 0 || fec == 15)
				pushVertexFifo(state.vertexfifo, c, state.vertexfifooffset);

			pushEdgeFifo(state.edgefifo, b, a, c, state.edgefifooffset);
			pushEdgeFifo(state.edgefifo, c, b, a, state.edgefifooffset);
			pushEdgeFifo(state.edgefifo, a, c, b, state.edgefifooffset);
		}
	}

	// we should've read all data bytes and stopped at the boundary between data and codeaux table
	if (i == index_count && data != data_safe_end)
		return 0;

	state.code_offset = code - buffer;
	state.data_offset = data - buffer - index_count / 3;
	state.index_offset = i;

	return result_offset;
}

}

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

	const size_t prediction_capacity = kVertexBlockSize + 2;
	unsigned int prediction[prediction_capacity];

	DecodePredictionState pstate = {};

#if TRACE > 0
	memset(&encodeVertexBlockStats, 0, sizeof(encodeVertexBlockStats));
#endif

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

			size_t block_size_clamped = (block_size > kVertexBlockSize) ? kVertexBlockSize : block_size;

			data = encodeVertexBlock(data, vertex_data + vertex_offset * vertex_size, block_size_clamped, vertex_size, prediction, last_vertex);
			vertex_offset += block_size_clamped;

			prediction_offset = block_size - block_size_clamped;
			memset(&prediction[0], 0, prediction_offset * sizeof(prediction[0]));
		}
	}

	while (vertex_offset < vertex_count)
	{
		size_t block_size = (vertex_offset + kVertexBlockSize < vertex_count) ? kVertexBlockSize : vertex_count - vertex_offset;

		data = encodeVertexBlock(data, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, 0, last_vertex);
		vertex_offset += block_size;
	}

#if TRACE > 0
	dumpEncodeVertexBlockStats(vertex_count, vertex_size);
#endif

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

	const size_t prediction_capacity = kVertexBlockSize + 2;
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

			size_t block_size_clamped = (block_size > kVertexBlockSize) ? kVertexBlockSize : block_size;

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
		size_t block_size = (vertex_offset + kVertexBlockSize < vertex_count) ? kVertexBlockSize : vertex_count - vertex_offset;

		data = decodeVertexBlock(data, data_end, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, 0, last_vertex);
		if (!data)
			return -2;

		vertex_offset += block_size;
	}

	if (data != data_end)
		return -3;

	return 0;
}
