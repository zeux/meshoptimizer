// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

#ifndef TRACE
#define TRACE 0
#endif

#if TRACE
#include <stdio.h>
#endif

// This work is based on:
// TODO
namespace meshopt
{

static float measureGrid(const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, int resolution, float* out_offset)
{
	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

	for (size_t i = 0; i < vertex_count; ++i)
	{
		const float* v = vertex_positions_data + i * vertex_stride_float;

		for (int j = 0; j < 3; ++j)
		{
			float vj = v[j];

			minv[j] = minv[j] > vj ? vj : minv[j];
			maxv[j] = maxv[j] < vj ? vj : maxv[j];
		}
	}

	float extent = 0.f;

	extent = (maxv[0] - minv[0]) < extent ? extent : (maxv[0] - minv[0]);
	extent = (maxv[1] - minv[1]) < extent ? extent : (maxv[1] - minv[1]);
	extent = (maxv[2] - minv[2]) < extent ? extent : (maxv[2] - minv[2]);

	// note: we rescale model extents to [0..resolution - 2], because the first and last voxel are used as empty padding
	float scale = extent == 0 ? 0.f : (resolution - 2) / extent;

	out_offset[0] = minv[0];
	out_offset[1] = minv[1];
	out_offset[2] = minv[2];

	return scale;
}

static void voxelize(unsigned char* grid, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, int resolution, float scale, const float offset[3])
{
}

} // namespace meshopt

size_t meshopt_remesh(float* destination, size_t max_triangle_count, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, int resolution, unsigned int options)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(resolution > 0 && resolution <= 250); // TBD

	meshopt_Allocator allocator;

	// measure voxel grid to compute position => voxel mapping
	float offset[3] = {};
	float scale = measureGrid(vertex_positions, vertex_count, vertex_positions_stride, resolution, offset);

	// rasterize triangles into the voxel grid: in the first pass, this simply tags occupied voxels
	unsigned char* grid = allocator.allocate<unsigned char>(size_t(resolution) * size_t(resolution) * size_t(resolution));
	memset(grid, 0, size_t(resolution) * size_t(resolution) * size_t(resolution));

	voxelize(grid, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, resolution, scale, offset);

#if TRACE
	size_t count = 0;
	for (size_t i = 0; i < size_t(resolution) * size_t(resolution) * size_t(resolution); ++i)
		count += grid[i] != 0;
	printf("remesher: %zu voxels occupied\n", count);
#endif

	return 0;
}
