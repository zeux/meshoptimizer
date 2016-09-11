#include "meshoptimizer.hpp"

#include <cassert>
#include <cstring>
#include <vector>

namespace
{
	template <typename T> void optimizePreTransformImpl(void* destination, const void* vertices, T* indices, size_t index_count, size_t vertex_count, size_t vertex_size)
	{
		assert(destination != vertices);

		// build vertex remap table
		std::vector<unsigned int> vertex_remap(vertex_count, static_cast<unsigned int>(-1));

		size_t vertex = 0;

		for (T* indices_end = indices + index_count; indices != indices_end; ++indices)
		{
			unsigned int& index = vertex_remap[*indices];

			if (index == static_cast<unsigned int>(-1)) // vertex was not added to destination VB
			{
				// add vertex
				memcpy(static_cast<char*>(destination) + vertex * vertex_size, static_cast<const char*>(vertices) + *indices * vertex_size, vertex_size);
				
				index = static_cast<unsigned int>(vertex++);
			}

			*indices = static_cast<T>(index);
		}
	}
}

void optimizePreTransform(void* destination, const void* vertices, unsigned short* indices, size_t index_count, size_t vertex_count, size_t vertex_size)
{
	optimizePreTransformImpl(destination, vertices, indices, index_count, vertex_count, vertex_size);
}

void optimizePreTransform(void* destination, const void* vertices, unsigned int* indices, size_t index_count, size_t vertex_count, size_t vertex_size)
{
	optimizePreTransformImpl(destination, vertices, indices, index_count, vertex_count, vertex_size);
}
