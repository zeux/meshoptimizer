// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <cassert>
#include <cstring>

#include <vector>

size_t meshopt_optimizeVertexFetch(void* destination, unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	assert(index_count % 3 == 0);
	assert(vertex_size > 0 && vertex_size <= 256);

	// support in-place optimization
	std::vector<char> vertices_copy;

	if (destination == vertices)
	{
		vertices_copy.assign(static_cast<const char*>(vertices), static_cast<const char*>(vertices) + vertex_count * vertex_size);
		vertices = &vertices_copy[0];
	}

	// build vertex remap table
	const unsigned int no_remap = ~0u;
	std::vector<unsigned int> vertex_remap(vertex_count, no_remap);

	unsigned int next_vertex = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		unsigned int& remap = vertex_remap[index];

		if (remap == no_remap) // vertex was not added to destination VB
		{
			// add vertex
			memcpy(static_cast<char*>(destination) + next_vertex * vertex_size, static_cast<const char*>(vertices) + index * vertex_size, vertex_size);

			remap = next_vertex++;
		}

		// modify indices in place
		indices[i] = remap;
	}

	assert(next_vertex <= vertex_count);

	return next_vertex;
}
