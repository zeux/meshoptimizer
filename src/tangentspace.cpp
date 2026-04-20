// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <math.h>
#include <string.h>

// This work is based on:
// Morten Mikkelsen. Simulation of wrinkled surfaces revisited. 2008
namespace meshopt
{
}

// TODO: argument order?
MESHOPTIMIZER_EXPERIMENTAL void meshopt_generateTangentsMikkT(float* result, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, const float* vertex_normals, size_t vertex_normals_stride, const float* vertex_uvs, size_t vertex_uvs_stride)
{
	(void)result;
	(void)indices;
	(void)index_count;
	(void)vertex_positions;
	(void)vertex_count;
	(void)vertex_positions_stride;
	(void)vertex_normals;
	(void)vertex_normals_stride;
	(void)vertex_uvs;
	(void)vertex_uvs_stride;
}
