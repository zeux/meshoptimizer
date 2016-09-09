#pragma once

void optimizeOverdrawTipsify(unsigned short* destination, const unsigned short* indices, size_t index_count, const void* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const vector<unsigned int>& clusters, unsigned int cache_size = 16, float threshold = 1);
void optimizeOverdrawTipsify(unsigned int* destination, const unsigned int* indices, size_t index_count, const void* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const vector<unsigned int>& clusters, unsigned int cache_size = 16, float threshold = 1);
