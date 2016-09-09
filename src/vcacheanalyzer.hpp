#pragma once

struct PostTLCacheStatistics
{
	unsigned int hits, misses;
	float hit_percent, miss_percent;
	float acmr; // transformed vertices / triangle count
};

// Analyze effectiveness for post T&L cache
PostTLCacheStatistics analyzePostTL(const unsigned short* indices, size_t index_count, size_t vertex_count, unsigned int cache_size = 32);
PostTLCacheStatistics analyzePostTL(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size = 32);
