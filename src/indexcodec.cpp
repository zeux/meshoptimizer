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

static const unsigned char kCodeAuxEncodingTable[16] =
    {
        // clang-format array formatting is broken :/
        0x00, 0x01, 0x10, 0x02, 0x20, 0x03, 0x30, 0x67, 0x76, 0x78, 0x87, 0x89, 0x98, 0xff, 0, 0,
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

static void encodeVByte(unsigned char*& data, unsigned int v)
{
	do
	{
		*data++ = (v & 127) | (v > 127 ? 128 : 0);
		v >>= 7;
	} while (v);
}

static unsigned int decodeVByte(const unsigned char*& data)
{
	unsigned char lead = *data++;

	if (lead < 128)
		return lead;

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

static void encodeIndex(unsigned char*& data, unsigned int index, unsigned int next, unsigned int last)
{
	(void)next;

	unsigned int d = index - last;
	unsigned int v = (d << 1) ^ (int(d) >> 31);

	encodeVByte(data, v);
}

static unsigned int decodeIndex(const unsigned char*& data, unsigned int next, unsigned int last)
{
	(void)next;

	unsigned int v = decodeVByte(data);
	unsigned int d = (v >> 1) ^ -int(v & 1);

	return last + d;
}

static int getCodeAuxIndex(unsigned char v, const unsigned char* table)
{
	for (int i = 0; i < 16; ++i)
		if (table[i] == v)
			return i;

	return -1;
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
	unsigned int last = 0;

	unsigned char* code = buffer;
	unsigned char* data = buffer + index_count / 3;

	const unsigned char* codeaux_table = kCodeAuxEncodingTable;

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

			*code++ = static_cast<unsigned char>((fe << 4) | fec);

			if (fec == 15)
				encodeIndex(data, c, next, last), last = c;

			if (fec == 0 || fec == 15)
				pushVertexFifo(vertexfifo, c, vertexfifooffset);

			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
		else
		{
			const unsigned int* order = kTriangleIndexOrder[rotateTriangle(indices[i + 0], indices[i + 1], indices[i + 2], next)];

			unsigned int a = indices[i + order[0]], b = indices[i + order[1]], c = indices[i + order[2]];

			int fb = getVertexFifo(vertexfifo, b, vertexfifooffset);
			int fc = getVertexFifo(vertexfifo, c, vertexfifooffset);

			// after rotation, a is almost always equal to next, so we don't waste bits on FIFO encoding for a
			int fea = (a == next) ? (next++, 0) : 15;
			int feb = (fb >= 0 && fb < 14) ? (fb + 1) : (b == next) ? (next++, 0) : 15;
			int fec = (fc >= 0 && fc < 14) ? (fc + 1) : (c == next) ? (next++, 0) : 15;

			// we encode feb & fec in 4 bits using a table if possible, and as a full byte otherwise
			unsigned char codeaux = static_cast<unsigned char>((feb << 4) | fec);
			int codeauxindex = getCodeAuxIndex(codeaux, codeaux_table);

			// <14 encodes an index into codeaux table, 14 encodes fea=0, 15 encodes fea=15
			if (fea == 0 && codeauxindex >= 0 && codeauxindex < 14)
			{
				*code++ = static_cast<unsigned char>((15 << 4) | codeauxindex);
			}
			else
			{
				*code++ = static_cast<unsigned char>((15 << 4) | 14 | fea);
				*data++ = codeaux;
			}

			if (fea == 15)
				encodeIndex(data, a, next, last), last = a;

			if (feb == 15)
				encodeIndex(data, b, next, last), last = b;

			if (fec == 15)
				encodeIndex(data, c, next, last), last = c;

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

	// add codeaux encoding table to the end of the stream; this is used for decoding codeaux *and* as padding
	// we need padding for decoding to be able to assume that each triangle is encoded as <= 16 bytes of extra data
	// this is enough space for aux byte + 5 bytes per varint index which is the absolute worst case for any input
	for (size_t i = 0; i < 16; ++i)
	{
		*data++ = codeaux_table[i];
	}

	assert(data >= buffer + index_count / 3 + 16);
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

	// worst-case encoding is 2 header bytes + 3 varint-7 encoded index deltas
	unsigned int vertex_groups = (vertex_bits + 1 + 6) / 7;

	return (index_count / 3) * (2 + 3 * vertex_groups) + 16;
}

int meshopt_decodeIndexBuffer(unsigned int* destination, size_t index_count, const unsigned char* buffer, size_t buffer_size)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);

	if (buffer_size < index_count / 3 + 16)
		return -1;

	EdgeFifo edgefifo;
	memset(edgefifo, -1, sizeof(edgefifo));

	VertexFifo vertexfifo;
	memset(vertexfifo, -1, sizeof(vertexfifo));

	size_t edgefifooffset = 0;
	size_t vertexfifooffset = 0;

	unsigned int next = 0;
	unsigned int last = 0;

	const unsigned char* code = buffer;
	const unsigned char* data = buffer + index_count / 3;
	const unsigned char* data_safe_end = buffer + buffer_size - 16;

	const unsigned char* codeaux_table = data_safe_end;

	for (size_t i = 0; i < index_count; i += 3)
	{
		if (data > data_safe_end)
			return -2;

		unsigned char codetri = *code++;

		int fe = codetri >> 4;

		if (fe < 15)
		{
			unsigned int a = edgefifo[(edgefifooffset - 1 - fe) & 15][0];
			unsigned int b = edgefifo[(edgefifooffset - 1 - fe) & 15][1];

			int fec = codetri & 15;

			unsigned int c = (fec == 0) ? next++ : vertexfifo[(vertexfifooffset - 1 - fec) & 15];

			if (fec == 15)
				last = c = decodeIndex(data, next, last);

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
			unsigned char codeaux = codeaux_table[codetri & 15];

			int fea = 0;
			int feb = codeaux >> 4;
			int fec = codeaux & 15;

			if ((codetri & 15) >= 14)
			{
				fea = (codetri & 15) == 14 ? 0 : 15;

				codeaux = *data++;

				feb = codeaux >> 4;
				fec = codeaux & 15;
			}

			unsigned int a = (fea == 0) ? next++ : 0;
			unsigned int b = (feb == 0) ? next++ : vertexfifo[(vertexfifooffset - feb) & 15];
			unsigned int c = (fec == 0) ? next++ : vertexfifo[(vertexfifooffset - fec) & 15];

			if (fea == 15)
				last = a = decodeIndex(data, next, last);

			if (feb == 15)
				last = b = decodeIndex(data, next, last);

			if (fec == 15)
				last = c = decodeIndex(data, next, last);

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

	if (data != data_safe_end)
		return -3;

	return 0;
}
