// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

#ifndef TRACE
#define TRACE 0
#endif

#if TRACE
#include <stdio.h>
#endif

// TODO: amalgamated build conflicts wrt indexcodec

// This work is based on:
// TODO
namespace meshopt
{

typedef unsigned int EdgeFifo[16][2];

static const unsigned int kTriangleIndexOrder[3][3] = {
    {0, 1, 2},
    {1, 2, 0},
    {2, 0, 1},
};

static int rotateTriangle(unsigned int a, unsigned int b, unsigned int c)
{
	return (a > b && a > c) ? 1 : (b > c ? 2 : 0);
}

static int getEdgeFifo(EdgeFifo fifo, unsigned int a, unsigned int b, unsigned int c, size_t offset)
{
	for (int i = 0; i < 16; ++i)
	{
		size_t index = (offset - 1 - i) & 15;

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

static void decodeTriangles(unsigned int* triangles, const unsigned char* codes, const unsigned char* extra, size_t triangle_count)
{
	// branchlessly read next or extra vertex and advance pointers
#define NEXT(var, ec) \
	e = *extra; \
	unsigned int var = (ec) ? e : next; \
	extra += (ec), next += 1 - (ec)

	unsigned int next = 0;
	unsigned int fifo[3] = {}; // two edge fifo entries in one uint: 0xcbac

	for (size_t i = 0; i < triangle_count; ++i)
	{
		unsigned int code = (codes[i / 2] >> ((i & 1) * 4)) & 0xF;
		unsigned int tri;

		if (code < 12)
		{
			// reuse
			unsigned int edge = fifo[code / 4];
			edge >>= (code << 3) & 16; // shift by 16 if bit 1 is set (odd edge for each triangle)

			// 0-1 extra vertices
			unsigned int e;
			NEXT(c, code & 1);

			// repack triangle into edge format (0xcbac)
			tri = ((edge & 0xff) << 16) | (edge & 0xff00) | c | (c << 24);
		}
		else
		{
			// restart
			int fea = code > 12;
			int feb = code > 13;
			int fec = code > 14;

			// 0-3 extra vertices
			unsigned int e;
			NEXT(a, fea);
			NEXT(b, feb);
			NEXT(c, fec);

			// repack triangle into edge format (0xcbac)
			tri = c | (a << 8) | (b << 16) | (c << 24);
		}

		// output triangle is stored without extra edge vertex (0xcbac => 0xcba)
		triangles[i] = tri >> 8;

		fifo[2] = fifo[1];
		fifo[1] = fifo[0];
		fifo[0] = tri;
	}

#undef NEXT
}

} // namespace meshopt

size_t meshopt_encodeMeshlet(unsigned char* buffer, size_t buffer_size, const unsigned int* vertices, const unsigned char* triangles, size_t triangle_count, size_t vertex_count)
{
	using namespace meshopt;

	(void)buffer_size; // TODO
	(void)vertices;
	(void)vertex_count;

	EdgeFifo edgefifo;
	memset(edgefifo, -1, sizeof(edgefifo));

	size_t edgefifooffset = 0;

	unsigned int next = 0;

	// 4-bit triangle codes give us 16 options that we use as follows:
	// 3*2 edge reuse (2 edges * 3 last triangles) * 2 next/explicit = 12 options
	// 4 remaining options = next bits; 000, 001, 011, 111.
	// triangles are rotated to make next bits line up.

	unsigned char* codes = buffer;
	unsigned char* extra = buffer + (triangle_count + 1) / 2;

	memset(codes, 0, (triangle_count + 1) / 2);

	for (size_t i = 0; i < triangle_count; ++i)
	{
#if TRACE
		unsigned int last = next;
#endif

		int fer = getEdgeFifo(edgefifo, triangles[i * 3 + 0], triangles[i * 3 + 1], triangles[i * 3 + 2], edgefifooffset);

		if (fer >= 0 && (fer >> 2) < 6)
		{
			// note: getEdgeFifo implicitly rotates triangles by matching a/b to existing edge
			const unsigned int* order = kTriangleIndexOrder[fer & 3];

			unsigned int a = triangles[i * 3 + order[0]], b = triangles[i * 3 + order[1]], c = triangles[i * 3 + order[2]];

			int fec = (c == next) ? (next++, 0) : 1;

#if TRACE
			printf("%3d+ | %3d %3d %3d | edge: e%d c%d\n", last, a, b, c, fer >> 2, fec);
#endif

			unsigned int code = (fer >> 2) * 2 + fec;

			codes[i / 2] |= (unsigned char)(code << ((i & 1) * 4));

			if (fec)
				*extra++ = (unsigned char)c;

			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
		else
		{
			int rotation = rotateTriangle(triangles[i * 3 + 0], triangles[i * 3 + 1], triangles[i * 3 + 2]);
			const unsigned int* order = kTriangleIndexOrder[rotation];

			unsigned int a = triangles[i * 3 + order[0]], b = triangles[i * 3 + order[1]], c = triangles[i * 3 + order[2]];

			// fe must be continuous: once a vertex is encoded with next, further vertices must also be encoded with next
			int fea = (a == next && b == next + 1 && c == next + 2) ? (next++, 0) : 1;
			int feb = (b == next && c == next + 1) ? (next++, 0) : 1;
			int fec = (c == next) ? (next++, 0) : 1;

			assert(fea == 1 || feb == 0);
			assert(feb == 1 || fec == 0);

#if TRACE
			printf("%3d+ | %3d %3d %3d | restart: %d%d%d\n", last, a, b, c, fea, feb, fec);
#endif

			unsigned int code = 12 + (fea + feb + fec);

			codes[i / 2] |= (unsigned char)(code << ((i & 1) * 4));

			if (fea)
				*extra++ = (unsigned char)a;
			if (feb)
				*extra++ = (unsigned char)b;
			if (fec)
				*extra++ = (unsigned char)c;

			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
	}

	assert(size_t(extra - buffer) <= buffer_size);
	return extra - buffer;
}

void meshopt_decodeMeshlet(unsigned int* triangles, size_t triangle_count, const unsigned char* buffer, size_t buffer_size)
{
	using namespace meshopt;

	(void)buffer_size; // TODO

	const unsigned char* codes = buffer;
	const unsigned char* extra = buffer + (triangle_count + 1) / 2;

	decodeTriangles(triangles, codes, extra, triangle_count);
}
