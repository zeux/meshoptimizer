// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <cassert>
#include <vector>

namespace meshopt
{

static meshopt_VertexCacheStatistics analyzeVertexCacheFIFO(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	assert(index_count % 3 == 0);
	assert(cache_size >= 3);

	meshopt_VertexCacheStatistics result = {};

	std::vector<unsigned int> cache_timestamps(vertex_count, 0);
	unsigned int timestamp = cache_size + 1;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		if (timestamp - cache_timestamps[index] > cache_size)
		{
			// cache miss
			cache_timestamps[index] = timestamp++;
			result.vertices_transformed++;
		}
	}

	result.acmr = index_count == 0 ? 0 : float(result.vertices_transformed) / float(index_count / 3);
	result.atvr = vertex_count == 0 ? 0 : float(result.vertices_transformed) / float(vertex_count);

	return result;
}

static meshopt_VertexCacheStatistics analyzeVertexCacheLRU(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	assert(index_count % 3 == 0);
	assert(cache_size >= 3);

	meshopt_VertexCacheStatistics result = {};

	std::vector<unsigned int> cache_storage(2 * (cache_size + 1));
	unsigned int* cache = &cache_storage[0];
	unsigned int* cache_new = &cache_storage[cache_size + 1];
	size_t cache_count = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		size_t write = 0;
		cache_new[write++] = index;

		for (size_t j = 0; j < cache_count; ++j)
		{
			unsigned int cache_index = cache[j];

			cache_new[write] = cache_index;
			write += cache_index != index;
		}

		assert(write <= cache_count + 1);

		if (write == cache_count + 1)
		{
			result.vertices_transformed++;
		}

		std::swap(cache, cache_new);
		cache_count = write > cache_size ? cache_size : write;
	}

	result.acmr = index_count == 0 ? 0 : float(result.vertices_transformed) / float(index_count / 3);
	result.atvr = vertex_count == 0 ? 0 : float(result.vertices_transformed) / float(vertex_count);

	return result;
}

} // namespace meshopt

meshopt_VertexCacheStatistics meshopt_analyzeVertexCache(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	using namespace meshopt;

	if (cache_size == 0)
		return analyzeVertexCacheLRU(indices, index_count, vertex_count, 16);
	else
		return analyzeVertexCacheFIFO(indices, index_count, vertex_count, cache_size);
}
