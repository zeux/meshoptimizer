#pragma once

#include <stddef.h>
#include <vector>

struct Vertex;

struct clodCluster
{
	int depth;

	const unsigned int* indices;
	size_t index_count;
};

typedef void (*clodOutput)(void* context, clodCluster cluster);

size_t clod(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, void* output_context, clodOutput output_callback);
