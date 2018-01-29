// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

// This work is based on:
// TODO: references
namespace meshopt
{

const size_t kVertexBlockSize = 256;

static unsigned char* encodeVertexBlock(unsigned char* data, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, const unsigned int* prediction)
{
	(void)prediction;

	assert(vertex_count > 0);

	for (size_t k = 0; k < vertex_size; ++k)
	{
		*data++ = vertex_data[k];
	}

	for (size_t k = 0; k < vertex_size; ++k)
	{
		size_t vertex_offset = k;

		for (size_t i = 1; i < vertex_count; ++i)
		{
			*data++ = vertex_data[vertex_offset + vertex_size] - vertex_data[vertex_offset];

			vertex_offset += vertex_size;
		}
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
	// TODO: partial bounds check?..
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

				unsigned int p =
					(na < 256 && nb < 256 && nc < 256)
					? (na << 16) | (nb << 8) | nc
					: 0;

				result[result_offset++] = p;
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
			unsigned char codeaux = codeaux_table[codetri & 15];

			int fea = 0;
			int feb = codeaux >> 4;
			int fec = codeaux & 15;

			// slow path: read a full byte for codeaux instead of using a table lookup
			if ((codetri & 15) >= 14)
			{
				fea = (codetri & 15) == 14 ? 0 : 15;

				codeaux = *data++;

				feb = codeaux >> 4;
				fec = codeaux & 15;
			}

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
	// TODO!
	// if (data != data_safe_end)
		// return 0;

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

	(void)index_buffer;
	(void)index_buffer_size;

	const unsigned char* vertex_data = static_cast<const unsigned char*>(vertices);

	unsigned char* data = buffer;

	unsigned int prediction[kVertexBlockSize];
	DecodePredictionState pstate = {};

	size_t vertex_offset = 0;

	if (index_buffer)
	{
		for (;;)
		{
			size_t psize = decodeVertexPrediction(pstate, prediction, sizeof(prediction) / sizeof(prediction[0]), index_count, index_buffer, index_buffer_size);
			if (psize == 0)
				break;

			size_t block_size = psize;

			if (vertex_offset + block_size > vertex_count)
				break;

			data = encodeVertexBlock(data, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, prediction);
			vertex_offset += block_size;
		}
	}

	while (vertex_offset < vertex_count)
	{
		size_t block_size = (vertex_offset + kVertexBlockSize < vertex_count) ? kVertexBlockSize : vertex_count - vertex_offset;

		data = encodeVertexBlock(data, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, 0);
		vertex_offset += block_size;
	}

	assert(size_t(data - buffer) <= buffer_size);

	return data - buffer;
}

size_t meshopt_encodeVertexBufferBound(size_t vertex_count, size_t vertex_size)
{
	return vertex_count * vertex_size * 2;
}
