// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <cassert>
#include <cstring>

// This work is based on:
// Fabian Giesen. Simple lossless index buffer compression & follow-up. 2013
// Conor Stokes. Vertex Cache Optimised Index Buffer Compression. 2014
namespace meshopt
{

typedef unsigned int VertexFifo[16];
typedef unsigned int EdgeFifo[16][2];

static const unsigned int kTriangleIndexOrder[3][3] =
    {
        // clang-format array formatting is broken :/
        {0, 1, 2},
        {1, 2, 0},
        {2, 0, 1},
};

static int rotateTriangle(unsigned int a, unsigned int b, unsigned int c, unsigned int next)
{
	(void)a;

	return (b == next) ? 1 : (c == next) ? 2 : 0;
}

static int getEdgeFifo(EdgeFifo fifo, unsigned int a, unsigned int b, unsigned int c, size_t offset)
{
	for (int i = 0; i < 16; ++i)
	{
		unsigned int index = (offset - 1 - i) & 15;
		unsigned int e0 = fifo[index][0];
		unsigned int e1 = fifo[index][1];

		if (e0 == a && e1 == b)
			return (i << 2) | 0;
		if (e0 == b && e1 == c)
			return (i << 2) | 1;
		if (e0 == c && e1 == a)
			return (i << 2) | 2;
	}

	return -1;
}

static void pushEdgeFifo(EdgeFifo fifo, unsigned int a, unsigned int b, size_t& offset)
{
	fifo[offset][0] = a;
	fifo[offset][1] = b;
	offset = (offset + 1) & 15;
}

static int getVertexFifo(VertexFifo fifo, unsigned int v, size_t offset)
{
	for (int i = 0; i < 16; ++i)
	{
		unsigned int index = (offset - 1 - i) & 15;

		if (fifo[index] == v)
			return i;
	}

	return -1;
}

static void pushVertexFifo(VertexFifo fifo, unsigned int v, size_t& offset)
{
	fifo[offset] = v;
	offset = (offset + 1) & 15;
}

static void encodeVarInt(unsigned char*& data, unsigned int v)
{
	do
	{
		*data++ = (v & 127) | (v > 127 ? 128 : 0);
		v >>= 7;
	} while (v);
}

static unsigned int decodeVarInt(const unsigned char*& data)
{
	unsigned int result = 0;
	unsigned int shift = 0;
	unsigned char group;

	do
	{
		group = *data++;
		result |= (group & 127) << shift;
		shift += 7;
	} while (group & 128);

	return result;
}
}

size_t meshopt_encodeIndexBuffer(unsigned char* buffer, size_t buffer_size, const unsigned int* indices, size_t index_count)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);

	EdgeFifo edgefifo;
	memset(edgefifo, -1, sizeof(edgefifo));

	VertexFifo vertexfifo;
	memset(vertexfifo, -1, sizeof(vertexfifo));

	size_t edgefifooffset = 0;
	size_t vertexfifooffset = 0;

	unsigned int next = 0;

	unsigned char* data = buffer;

	for (size_t i = 0; i < index_count; i += 3)
	{
		int fer = getEdgeFifo(edgefifo, indices[i + 0], indices[i + 1], indices[i + 2], edgefifooffset);

		if (fer >= 0 && (fer >> 2) < 15)
		{
			const unsigned int* order = kTriangleIndexOrder[fer & 3];

			unsigned int a = indices[i + order[0]], b = indices[i + order[1]], c = indices[i + order[2]];

			int fe = fer >> 2;
			int fc = getVertexFifo(vertexfifo, c, vertexfifooffset);

			int fec = (fc >= 1 && fc < 15) ? fc : (c == next) ? (next++, 0) : 15;

			*data++ = static_cast<unsigned char>((fe << 4) | fec);

			if (fec == 15)
				encodeVarInt(data, next - c);

			if (fec == 0 || fec == 15)
				pushVertexFifo(vertexfifo, c, vertexfifooffset);

			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
		else
		{
			const unsigned int* order = kTriangleIndexOrder[rotateTriangle(indices[i + 0], indices[i + 1], indices[i + 2], next)];

			unsigned int a = indices[i + order[0]], b = indices[i + order[1]], c = indices[i + order[2]];

			int fa = getVertexFifo(vertexfifo, a, vertexfifooffset);
			int fb = getVertexFifo(vertexfifo, b, vertexfifooffset);
			int fc = getVertexFifo(vertexfifo, c, vertexfifooffset);

			int fea = (fa >= 0 && fa < 14) ? (fa + 1) : (a == next) ? (next++, 0) : 15;
			int feb = (fb >= 0 && fb < 14) ? (fb + 1) : (b == next) ? (next++, 0) : 15;
			int fec = (fc >= 0 && fc < 14) ? (fc + 1) : (c == next) ? (next++, 0) : 15;

			*data++ = static_cast<unsigned char>((15 << 4) | fea);
			*data++ = static_cast<unsigned char>((feb << 4) | fec);

			if (fea == 15)
				encodeVarInt(data, next - a);

			if (feb == 15)
				encodeVarInt(data, next - b);

			if (fec == 15)
				encodeVarInt(data, next - c);

			if (fea == 0 || fea == 15)
				pushVertexFifo(vertexfifo, a, vertexfifooffset);

			if (feb == 0 || feb == 15)
				pushVertexFifo(vertexfifo, b, vertexfifooffset);

			if (fec == 0 || fec == 15)
				pushVertexFifo(vertexfifo, c, vertexfifooffset);

			pushEdgeFifo(edgefifo, b, a, edgefifooffset);
			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
	}

	assert(data <= buffer + buffer_size);
	(void)buffer_size;

	return data - buffer;
}

size_t meshopt_encodeIndexBufferBound(size_t index_count, size_t vertex_count)
{
	assert(index_count % 3 == 0);

	// compute number of bits required for each index
	unsigned int vertex_bits = 1;

	while (vertex_bits < 32 && vertex_count > size_t(1) << vertex_bits)
		vertex_bits++;

	// worst-case encoding is 2 header bytes + 3 varint-7 encoded indices
	unsigned int vertex_groups = (vertex_bits + 6) / 7;

	return (index_count / 3) * (2 + 3 * vertex_groups);
}

void meshopt_decodeIndexBuffer(unsigned int* destination, size_t index_count, const unsigned char* buffer, size_t buffer_size)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);

	EdgeFifo edgefifo;
	memset(edgefifo, -1, sizeof(edgefifo));

	VertexFifo vertexfifo;
	memset(vertexfifo, -1, sizeof(vertexfifo));

	size_t edgefifooffset = 0;
	size_t vertexfifooffset = 0;

	unsigned int next = 0;

	const unsigned char* data = buffer;

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned char code = *data++;

		int fe = code >> 4;

		if (fe < 15)
		{
			unsigned int a = edgefifo[(edgefifooffset - 1 - fe) & 15][0];
			unsigned int b = edgefifo[(edgefifooffset - 1 - fe) & 15][1];

			int fec = code & 15;

			unsigned int c = (fec == 0) ? next++ : (fec < 15) ? vertexfifo[(vertexfifooffset - 1 - fec) & 15] : next - decodeVarInt(data);

			destination[i + 0] = a;
			destination[i + 1] = b;
			destination[i + 2] = c;

			if (fec == 0 || fec == 15)
				pushVertexFifo(vertexfifo, c, vertexfifooffset);

			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
		else
		{
			int fea = code & 15;

			unsigned char codeaux = *data++;

			int feb = codeaux >> 4;
			int fec = codeaux & 15;

			unsigned int a = (fea == 0) ? next++ : (fea < 15) ? vertexfifo[(vertexfifooffset - fea) & 15] : 0;
			unsigned int b = (feb == 0) ? next++ : (feb < 15) ? vertexfifo[(vertexfifooffset - feb) & 15] : 0;
			unsigned int c = (fec == 0) ? next++ : (fec < 15) ? vertexfifo[(vertexfifooffset - fec) & 15] : 0;

			if (fea == 15)
				a = next - decodeVarInt(data);

			if (feb == 15)
				b = next - decodeVarInt(data);

			if (fec == 15)
				c = next - decodeVarInt(data);

			destination[i + 0] = a;
			destination[i + 1] = b;
			destination[i + 2] = c;

			if (fea == 0 || fea == 15)
				pushVertexFifo(vertexfifo, a, vertexfifooffset);

			if (feb == 0 || feb == 15)
				pushVertexFifo(vertexfifo, b, vertexfifooffset);

			if (fec == 0 || fec == 15)
				pushVertexFifo(vertexfifo, c, vertexfifooffset);

			pushEdgeFifo(edgefifo, b, a, edgefifooffset);
			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
	}

	assert(data == buffer + buffer_size);
	(void)buffer_size;
}
