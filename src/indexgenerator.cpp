#include "meshoptimizer.hpp"

#include <cassert>
#include <cstring>
#include <unordered_map>

namespace
{
	struct Hash1
	{
		size_t size;

		size_t operator()(const void* ptr) const
		{
			unsigned int result = 2166136261;

			for (size_t i = 0; i < size; ++i)
			{
				result ^= static_cast<const unsigned char*>(ptr)[i];
				result *= 16777619;
			}

			return result;
		}

		size_t operator()(const void* lhs, const void* rhs) const
		{
			return memcmp(lhs, rhs, size) == 0;
		}
	};

	template <typename Hasher>
	size_t generateIndexBufferImpl(unsigned int* destination, const void* vertices, size_t vertex_count, size_t vertex_size, Hasher hasher)
	{
		std::unordered_map<const void*, unsigned int, Hasher, Hasher> hm(vertex_count, hasher, hasher);

		for (size_t i = 0; i < vertex_count; ++i)
		{
			const void* vertex = static_cast<const char*>(vertices) + i * vertex_size;

			unsigned int& idx = hm[vertex];

			if (!idx)
				idx = hm.size();

			destination[i] = idx - 1;
		}

		return hm.size();
	}
}

size_t generateIndexBuffer(unsigned int* destination, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	Hash1 hasher = { vertex_size };
	return generateIndexBufferImpl(destination, vertices, vertex_count, vertex_size, hasher);
}

void generateVertexBuffer(void* destination, const unsigned int* indices, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	unsigned int next_vertex = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		assert(indices[i] <= next_vertex);

		if (indices[i] == next_vertex)
		{
			memcpy(static_cast<char*>(destination) + next_vertex * vertex_size, static_cast<const char*>(vertices) + i * vertex_size, vertex_size);

			next_vertex++;
		}
	}
}