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

	bool optimize_bounds;
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

// To compute approximate (perspective) projection error of a cluster in screen space (0..1; multiply by screen height to get pixels):
// - camera_proj is projection[1][1], or cot(fovy/2); camera_znear is *positive* near plane distance
// - for simplicity, we ignore perspective distortion and use rotationally invariant projection size estimation
// - return: bounds.error / max(distance(bounds.center, camera_position) - bounds.radius, camera_znear) * (camera_proj * 0.5f)
struct clodBounds
{
	float center[3];
	float radius;
	float error;
};

struct clodCluster
{
	int depth;

	int refined;

	clodBounds bounds;

	const unsigned int* indices;
	size_t index_count;
};

struct clodGroup
{
	clodBounds bounds;

	const int* clusters;
	size_t cluster_count;
};

typedef void (*clodOutputCluster)(void* context, clodCluster cluster);
typedef void (*clodOutputGroup)(void* context, clodGroup group);

#ifdef __cplusplus
extern "C"
{
#endif

clodConfig clodDefaultConfig(size_t max_triangles);
clodConfig clodDefaultConfigRT(size_t max_triangles);

size_t clodBuild(clodConfig config, clodMesh mesh, void* output_context, clodOutputCluster output_cluster, clodOutputGroup output_group);

#ifdef __cplusplus
}
#endif
