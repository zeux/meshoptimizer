#pragma once

#include <stddef.h>

struct clodConfig
{
	size_t max_vertices;
	size_t min_triangles;
	size_t max_triangles;

	size_t partition_size;

	bool retry_queue;

	bool cluster_spatial;
	float cluster_fill_weight;
	float cluster_split_factor;

	bool optimize_raster;

	float simplify_ratio;
	float simplify_threshold;
	float simplify_sloppy_factor;
	bool simplify_permissive;
	bool simplify_fallback_permissive;
	bool simplify_fallback_sloppy;
};

struct clodMesh
{
	const unsigned int* indices;
	size_t index_count;

	size_t vertex_count;

	const float* vertex_positions;
	size_t vertex_positions_stride;

	const float* vertex_attributes;
	size_t vertex_attributes_stride;

	const float* attribute_weights;
	size_t attribute_count;

	unsigned int attribute_protect_mask;
};

struct clodCluster
{
	int depth;

	const unsigned int* indices;
	size_t index_count;
};

typedef void (*clodOutput)(void* context, clodCluster cluster);

#ifdef __cplusplus
extern "C"
{
#endif

clodConfig clodDefaultConfig(size_t max_triangles);
clodConfig clodDefaultConfigRT(size_t max_triangles);

size_t clodBuild(clodConfig config, clodMesh mesh, void* output_context, clodOutput output_callback);

#ifdef __cplusplus
}
#endif
