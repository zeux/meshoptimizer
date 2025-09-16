#pragma once

#include <stddef.h>
#include <vector>

struct Vertex;

struct clodConfig
{
	size_t max_vertices;
	size_t min_triangles;
	size_t max_triangles;

	size_t partition_size;

	bool retry_queue;

	bool cluster_spatial;
	float cluster_fill; // fill_factor when !spatial, fill_weight when spatial

	bool optimize_raster;

	float simplify_threshold;
	bool simplify_fallback_permissive;
	bool simplify_fallback_sloppy;
};

struct clodCluster
{
	int depth;

	const unsigned int* indices;
	size_t index_count;
};

typedef void (*clodOutput)(void* context, clodCluster cluster);

clodConfig clodDefaultConfig(size_t max_triangles);
clodConfig clodDefaultConfigRT(size_t max_triangles);

size_t clodBuild(clodConfig config, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, void* output_context, clodOutput output_callback);
