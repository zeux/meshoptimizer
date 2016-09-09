#include "vcacheanalyzer.hpp"

#include <vector>

namespace
{
	template <typename T> PostTLCacheStatistics analyzePostTLImpl(const T* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
	{
		PostTLCacheStatistics result = {};

		std::vector<unsigned int> cache_time_stamps(vertex_count, 0);
		unsigned int time_stamp = cache_size + 1;
		
		for (const T* indices_end = indices + index_count; indices != indices_end; ++indices)
		{
			T index = *indices;
			
			if (time_stamp - cache_time_stamps[index] > cache_size)
			{
				// cache miss
				cache_time_stamps[index] = time_stamp++;
				result.misses++;
			}
		}
		
		result.hits = static_cast<unsigned int>(index_count) - result.misses;

		result.hit_percent = 100 * static_cast<float>(result.hits) / index_count;
		result.miss_percent = 100 * static_cast<float>(result.misses) / index_count;

		result.acmr = static_cast<float>(result.misses) / (index_count / 3);

		return result;
	}
}

PostTLCacheStatistics analyzePostTL(const unsigned short* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	return analyzePostTLImpl(indices, index_count, vertex_count, cache_size);
}

PostTLCacheStatistics analyzePostTL(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	return analyzePostTLImpl(indices, index_count, vertex_count, cache_size);
}
