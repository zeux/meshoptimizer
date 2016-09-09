#pragma once

#include <cstddef>
#include <vector>

void optimizePostTLTipsify(unsigned short* destination, const unsigned short* indices, size_t index_count, size_t vertex_count, unsigned int cache_size = 16, std::vector<unsigned int>* clusters = 0);
void optimizePostTLTipsify(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size = 16, std::vector<unsigned int>* clusters = 0);
