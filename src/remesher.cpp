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
	(void)vertex_count;

	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		const float* va = vertex_positions + a * vertex_stride_float;
		const float* vb = vertex_positions + b * vertex_stride_float;
		const float* vc = vertex_positions + c * vertex_stride_float;

		float ex = vb[0] - va[0], ey = vb[1] - va[1], ez = vb[2] - va[2];
		float fx = vc[0] - va[0], fy = vc[1] - va[1], fz = vc[2] - va[2];
		float gx = vc[0] - vb[0], gy = vc[1] - vb[1], gz = vc[2] - vb[2];

		// use maximum edge length to establish sampling rate
		// TODO: this is wasteful for thin triangles
		float el = sqrtf(ex * ex + ey * ey + ez * ez);
		float fl = sqrtf(fx * fx + fy * fy + fz * fz);
		float gl = sqrtf(gx * gx + gy * gy + gz * gz);

		float max_edge = el > fl ? el : fl;
		max_edge = max_edge > gl ? max_edge : gl;

		// we target 2 samples per voxel edge which should be enough to hit all voxels at any rotation
		int samples = int(max_edge * scale * 2.f);
		samples = samples > 1 ? samples : 1;
		samples = samples < resolution * 2 ? samples : resolution * 2;

		float sx = va[0] - offset[0], sy = va[1] - offset[1], sz = va[2] - offset[2];
		float sr = 1.f / float(samples);

		for (int u = 0; u <= samples; ++u)
			for (int v = 0; v <= samples - u; ++v)
			{
				float su = float(u) * sr, sv = float(v) * sr;
				int x = int((sx + su * ex + sv * fx) * scale);
				int y = int((sy + su * ey + sv * fy) * scale);
				int z = int((sz + su * ez + sv * fz) * scale);

				// safety: rounding errors and non-finite inputs may produce out of bounds coordinates, so we clamp them
				// TODO: codegen
				x = (x < 0) ? 0 : (x > resolution - 2 ? resolution - 2 : x);
				y = (y < 0) ? 0 : (y > resolution - 2 ? resolution - 2 : y);
				z = (z < 0) ? 0 : (z > resolution - 2 ? resolution - 2 : z);

				size_t index = (x + 1) + size_t(resolution) * ((y + 1) + size_t(resolution) * (z + 1));
				grid[index] = 1;
			}
	}
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
