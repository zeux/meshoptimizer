#include "meshoptimizer.hpp"

#include <vector>
#include <cassert>

namespace
{
	template <typename T>
	PreTransformCacheStatistics analyzePreTransformImpl(const T* indices, size_t index_count, size_t vertex_count, size_t vertex_size)
	{
		PreTransformCacheStatistics result = {};

		const size_t kCacheLine = 64;
		const size_t kCacheSize = 128*1024;

		// simple direct mapped cache; on typical mesh data this is close to 4-way cache, and this model is a gross approximation anyway
		size_t cache[kCacheSize / kCacheLine] = {};
		size_t fetched = 0;

		for (size_t i = 0; i < index_count; ++i)
		{
			T index = indices[i];

			size_t start_address = index * vertex_size;
			size_t end_address = start_address + vertex_size;

			size_t start_tag = start_address / kCacheLine;
			size_t end_tag = (end_address + kCacheLine - 1) / kCacheLine;

			assert(start_tag < end_tag);

			for (size_t tag = start_tag; tag < end_tag; ++tag)
			{
				size_t line = tag % (sizeof(cache) / sizeof(cache[0]));

				// we store +1 since cache is filled with 0 by default
				fetched += cache[line] != tag + 1;
				cache[line] = tag + 1;
			}
		}

		result.overfetch = static_cast<float>(fetched * kCacheLine) / static_cast<float>(vertex_count * vertex_size);

		return result;
	}
}

PreTransformCacheStatistics analyzePreTransform(const unsigned short* indices, size_t index_count, size_t vertex_count, size_t vertex_size)
{
	return analyzePreTransformImpl(indices, index_count, vertex_count, vertex_size);
}

PreTransformCacheStatistics analyzePreTransform(const unsigned int* indices, size_t index_count, size_t vertex_count, size_t vertex_size)
{
	return analyzePreTransformImpl(indices, index_count, vertex_count, vertex_size);
}
