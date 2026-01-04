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

} // namespace meshopt

size_t meshopt_encodeMeshlet(unsigned char* buffer, size_t buffer_size, const unsigned int* vertices, const unsigned char* triangles, size_t triangle_count, size_t vertex_count)
{
	return 0;
}
