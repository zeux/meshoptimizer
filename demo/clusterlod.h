#pragma once

#include <stddef.h>

struct clodConfig
{
	// configuration of each cluster; maps to meshopt_buildMeshlets* parameters
	size_t max_vertices;
	size_t min_triangles;
	size_t max_triangles;

	// partition size; maps to meshopt_partitionClusters parameter
	// note: this is the target size; actual partitions may be up to 1.5x larger
	size_t partition_size;

	// clusterization setup; maps to meshopt_buildMeshletsSpatial / meshopt_buildMeshletsFlex
	bool cluster_spatial;
	float cluster_fill_weight;
	float cluster_split_factor;

	// every level aims to reduce the number of triangles by ratio, and considers clusters that don't reach the threshold stuck
	float simplify_ratio;
	float simplify_threshold;

	// amplify the error of clusters that go through sloppy simplification to account for appearance degradation
	float simplify_sloppy_factor;

	// use permissive simplification instead of regular simplification (make sure to use attribute_protect_mask if this is set!)
	bool simplify_permissive;

	// use permissive or sloppy simplification but only if regular simplification gets stuck
	bool simplify_fallback_permissive;
	bool simplify_fallback_sloppy;

	// should clodCluster::bounds be computed based on the geometry of each cluster
	bool optimize_bounds;

	// should clodCluster::indices be optimized for rasterization
	bool optimize_raster;

	// should we retry processing of clusters that get stuck? can be useful when not using simplification fallbacks
	bool retry_queue;
};

struct clodMesh
{
	// input triangle indices
	const unsigned int* indices;
	size_t index_count;

	// total vertex count
	size_t vertex_count;

	// input vertex positions; must be 3 floats per vertex
	const float* vertex_positions;
	size_t vertex_positions_stride;

	// input vertex attributes; used for attribute-aware simplification and permissive simplification
	const float* vertex_attributes;
	size_t vertex_attributes_stride;

	// attribute weights for attribute-aware simplification; maps to meshopt_simplifyWithAttributes parameters
	const float* attribute_weights;
	size_t attribute_count;

	// attribute mask to flag attribute discontinuities for permissive simplification; mask (1<<K) corresponds to attribute K
	unsigned int attribute_protect_mask;
};

// To compute approximate (perspective) projection error of a cluster in screen space (0..1; multiply by screen height to get pixels):
// - camera_proj is projection[1][1], or cot(fovy/2); camera_znear is *positive* near plane distance
// - for simplicity, we ignore perspective distortion and use rotationally invariant projection size estimation
// - return: bounds.error / max(distance(bounds.center, camera_position) - bounds.radius, camera_znear) * (camera_proj * 0.5f)
struct clodBounds
{
	// sphere bounds, in mesh coordinate space
	float center[3];
	float radius;

	// combined simplification error, in mesh coordinate space
	float error;
};

struct clodCluster
{
	// DAG level the cluster was generated from
	int depth;

	// index of more refined group (with more triangles) that produced this cluster during simplification
	int refined;

	// cluster bounds; should only be used for culling, as bounds.error is not monotonic across DAG
	clodBounds bounds;

	// cluster indices; refer to the original mesh vertex buffer
	const unsigned int* indices;
	size_t index_count;
};

struct clodGroup
{
	// simplified group bounds (reflects error for clusters with clodCluster::refined == group id)
	// cluster should be rendered if:
	// 1. cluster is in no groups (leaf) *or* clodGroup::simplified for the group it's in is over error threshold
	// 2. cluster.refined is -1 *or* clodGroup::simplified for groups[cluster.refined].simplified is at or under error threshold
	clodBounds simplified;

	// clusters that belong to the group
	const int* clusters;
	size_t cluster_count;
};

// gets called for each cluster generated in sequence (starting from id 0)
typedef void (*clodOutputCluster)(void* context, clodCluster cluster);

// gets called for each group generated in sequence (starting from id 0)
typedef void (*clodOutputGroup)(void* context, clodGroup group);

#ifdef __cplusplus
extern "C"
{
#endif

// default configuration optimized for rasterization / raytracing
clodConfig clodDefaultConfig(size_t max_triangles);
clodConfig clodDefaultConfigRT(size_t max_triangles);

// build cluster LOD hierarchy, calling output callbacks as new clusters and groups are generated
// returns the number of triangles in the lowest LOD level
size_t clodBuild(clodConfig config, clodMesh mesh, void* output_context, clodOutputCluster output_cluster, clodOutputGroup output_group);

#ifdef __cplusplus
}
#endif
