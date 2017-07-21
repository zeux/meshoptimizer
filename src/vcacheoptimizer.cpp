// This file is part of meshoptimizer library; see meshoptimizer.hpp for version/license details
#include "meshoptimizer.hpp"

#include <cassert>
#include <vector>

// This work is based on:
// Pedro Sander, Diego Nehab and Joshua Barcz. Fast Triangle Reordering for Vertex Locality and Reduced Overdraw. 2007
namespace meshopt
{

struct Adjacency
{
	std::vector<unsigned int> triangle_counts;
	std::vector<unsigned int> offsets;
	std::vector<unsigned int> data;
};

static void buildAdjacency(Adjacency& adjacency, const unsigned int* indices, size_t index_count, size_t vertex_count)
{
	// fill triangle counts
	adjacency.triangle_counts.resize(vertex_count, 0);

	for (size_t i = 0; i < index_count; ++i)
	{
		assert(indices[i] < vertex_count);

		adjacency.triangle_counts[indices[i]]++;
	}

	// fill offset table
	adjacency.offsets.resize(vertex_count);

	unsigned int offset = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		adjacency.offsets[i] = offset;
		offset += adjacency.triangle_counts[i];
	}

	// fill triangle data
	adjacency.data.resize(offset);

	std::vector<unsigned int> offsets = adjacency.offsets;

	size_t face_count = index_count / 3;

	for (size_t i = 0; i < face_count; ++i)
	{
		unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

		adjacency.data[offsets[a]++] = unsigned(i);
		adjacency.data[offsets[b]++] = unsigned(i);
		adjacency.data[offsets[c]++] = unsigned(i);
	}
}

static unsigned int getNextVertexDeadEnd(const std::vector<unsigned int>& dead_end, unsigned int& dead_end_top, unsigned int& input_cursor, const std::vector<unsigned int>& live_triangles)
{
	// check dead-end stack
	while (dead_end_top)
	{
		unsigned int vertex = dead_end[--dead_end_top];

		if (live_triangles[vertex] > 0)
			return vertex;
	}

	// input order
	while (input_cursor < live_triangles.size())
	{
		if (live_triangles[input_cursor] > 0)
			return input_cursor;

		++input_cursor;
	}

	return static_cast<unsigned int>(-1);
}

static unsigned int getNextVertexNeighbour(const unsigned int* next_candidates_begin, const unsigned int* next_candidates_end, const std::vector<unsigned int>& live_triangles, const std::vector<unsigned int>& cache_timestamps, unsigned int timestamp, unsigned int cache_size)
{
	unsigned int best_candidate = static_cast<unsigned int>(-1);
	int best_priority = -1;

	for (const unsigned int* next_candidate = next_candidates_begin; next_candidate != next_candidates_end; ++next_candidate)
	{
		unsigned int vertex = *next_candidate;

		// otherwise we don't need to process it
		if (live_triangles[vertex] > 0)
		{
			int priority = 0;

			// will it be in cache after fanning?
			if (2 * live_triangles[vertex] + timestamp - cache_timestamps[vertex] <= cache_size)
			{
				priority = timestamp - cache_timestamps[vertex]; // position in cache
			}

			if (priority > best_priority)
			{
				best_candidate = vertex;
				best_priority = priority;
			}
		}
	}

	return best_candidate;
}

void optimizeVertexCache(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	assert(index_count % 3 == 0);
	assert(cache_size >= 3);

	// guard for empty meshes
	if (index_count == 0 || vertex_count == 0)
	{
		return;
	}

	// support in-place optimization
	std::vector<unsigned int> indices_copy;

	if (destination == indices)
	{
		indices_copy.assign(indices, indices + index_count);
		indices = &indices_copy[0];
	}

	// build adjacency information
	Adjacency adjacency;

	buildAdjacency(adjacency, indices, index_count, vertex_count);

	// live triangle counts
	std::vector<unsigned int> live_triangles = adjacency.triangle_counts;

	// cache time stamps
	std::vector<unsigned int> cache_timestamps(vertex_count, 0);

	// dead-end stack
	std::vector<unsigned int> dead_end(index_count);
	unsigned int dead_end_top = 0;

	// emitted flags
	std::vector<char> emitted_flags(index_count / 3, false);

	unsigned int current_vertex = 0;

	unsigned int timestamp = cache_size + 1;
	unsigned int input_cursor = 1; // vertex to restart from in case of dead-end

	unsigned int output_triangle = 0;

	while (current_vertex != static_cast<unsigned int>(-1))
	{
		const unsigned int* next_candidates_begin = &dead_end[0] + dead_end_top;

		// emit all vertex neighbours
		const unsigned int* neighbours_begin = &adjacency.data[0] + adjacency.offsets[current_vertex];
		const unsigned int* neighbours_end = neighbours_begin + adjacency.triangle_counts[current_vertex];

		for (const unsigned int* it = neighbours_begin; it != neighbours_end; ++it)
		{
			unsigned int triangle = *it;

			if (!emitted_flags[triangle])
			{
				unsigned int a = indices[triangle * 3 + 0], b = indices[triangle * 3 + 1], c = indices[triangle * 3 + 2];

				// output indices
				destination[output_triangle * 3 + 0] = a;
				destination[output_triangle * 3 + 1] = b;
				destination[output_triangle * 3 + 2] = c;
				output_triangle++;

				// update dead-end stack
				dead_end[dead_end_top + 0] = a;
				dead_end[dead_end_top + 1] = b;
				dead_end[dead_end_top + 2] = c;
				dead_end_top += 3;

				// update live triangle counts
				live_triangles[a]--;
				live_triangles[b]--;
				live_triangles[c]--;

				// update cache info
				// if vertex is not in cache, put it in cache
				if (timestamp - cache_timestamps[a] > cache_size)
					cache_timestamps[a] = timestamp++;

				if (timestamp - cache_timestamps[b] > cache_size)
					cache_timestamps[b] = timestamp++;

				if (timestamp - cache_timestamps[c] > cache_size)
					cache_timestamps[c] = timestamp++;

				// update emitted flags
				emitted_flags[triangle] = true;
			}
		}

		// next candidates are the ones we pushed to dead-end stack just now
		const unsigned int* next_candidates_end = &dead_end[0] + dead_end_top;

		// get next vertex
		current_vertex = getNextVertexNeighbour(next_candidates_begin, next_candidates_end, live_triangles, cache_timestamps, timestamp, cache_size);

		if (current_vertex == static_cast<unsigned int>(-1))
		{
			current_vertex = getNextVertexDeadEnd(dead_end, dead_end_top, input_cursor, live_triangles);
		}
	}

	assert(output_triangle == index_count / 3);
}

} // namespace meshopt
