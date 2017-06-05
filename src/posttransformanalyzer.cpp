#include "meshoptimizer.hpp"

#include <vector>

namespace
{
	template <typename T>
	PostTransformCacheStatistics analyzePostTransformImpl(const T* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
	{
		PostTransformCacheStatistics result = {};

		std::vector<unsigned int> cache_time_stamps(vertex_count, 0);
		unsigned int time_stamp = cache_size + 1;

		for (const T* indices_end = indices + index_count; indices != indices_end; ++indices)
		{
			T index = *indices;

			if (time_stamp - cache_time_stamps[index] > cache_size)
			{
				// cache miss
				cache_time_stamps[index] = time_stamp++;
				result.vertices_transformed++;
			}
		}

		result.acmr = float(result.vertices_transformed) / float(index_count / 3);
		result.atvr = float(result.vertices_transformed) / float(vertex_count);

		return result;
	}
}

PostTransformCacheStatistics analyzePostTransform(const unsigned short* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	return analyzePostTransformImpl(indices, index_count, vertex_count, cache_size);
}

PostTransformCacheStatistics analyzePostTransform(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	return analyzePostTransformImpl(indices, index_count, vertex_count, cache_size);
}
