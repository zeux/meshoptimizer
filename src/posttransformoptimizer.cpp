#include "meshoptimizer.hpp"

#include <cassert>

namespace
{
	struct Adjacency
	{
		std::vector<unsigned int> triangle_counts;
		std::vector<unsigned int> offsets;
		std::vector<unsigned int> data;
	};

	template <typename T> void buildAdjacency(Adjacency& adjacency, const T* indices, size_t index_count, size_t vertex_count)
	{
		// fill triangle counts
		adjacency.triangle_counts.resize(vertex_count, 0);

		for (size_t i = 0; i < index_count; ++i)
		{
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

		std::vector<unsigned int> filled_triangle_counts(vertex_count, 0);
		
		for (size_t i = 0; i < index_count / 3; ++i)
		{
			unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];
			
			adjacency.data[adjacency.offsets[a] + filled_triangle_counts[a]] = static_cast<unsigned int>(i);
			filled_triangle_counts[a]++;
			
			adjacency.data[adjacency.offsets[b] + filled_triangle_counts[b]] = static_cast<unsigned int>(i);
			filled_triangle_counts[b]++;
			
			adjacency.data[adjacency.offsets[c] + filled_triangle_counts[c]] = static_cast<unsigned int>(i);
			filled_triangle_counts[c]++;
		}
	}

	unsigned int getNextVertexDeadEnd(const std::vector<unsigned int>& dead_end, unsigned int& dead_end_top, unsigned int& input_cursor, const std::vector<unsigned int>& live_triangles)
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

	unsigned int getNextVertexNeighbour(const unsigned int* next_candidates_begin, const unsigned int* next_candidates_end, const std::vector<unsigned int>& live_triangles, const std::vector<unsigned int>& cache_time_stamps, unsigned int time_stamp, unsigned int cache_size)
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
				if (2 * live_triangles[vertex] + time_stamp - cache_time_stamps[vertex] <= cache_size)
				{
					priority = time_stamp - cache_time_stamps[vertex]; // position in cache
				}

				if (priority > best_priority)
				{
					best_candidate = static_cast<int>(vertex);
					best_priority = priority;
				}
			}
		}

		return best_candidate;
	}

	template <typename T> void optimizePostTransformTipsify(T* destination, const T* indices, size_t index_count, size_t vertex_count, unsigned int cache_size, std::vector<unsigned int>* clusters)
	{
		assert(destination != indices);

		// guard for empty meshes
		if (index_count == 0 || vertex_count == 0)
		{
			return;
		}
		
		// build adjacency information
		Adjacency adjacency;

		buildAdjacency(adjacency, indices, index_count, vertex_count);

		// live triangle counts
		std::vector<unsigned int> live_triangles = adjacency.triangle_counts;

		// cache time stamps
		std::vector<unsigned int> cache_time_stamps(vertex_count, 0);

		// dead-end stack
		std::vector<unsigned int> dead_end(index_count);
		unsigned int dead_end_top = 0;

		// emitted flags
		std::vector<char> emitted_flags(index_count / 3, false);
		
		// prepare clusters
		if (clusters)
		{
			clusters->reserve(index_count);
			
			clusters->push_back(0);
		}

		unsigned int current_vertex = 0;
		
		unsigned int time_stamp = cache_size + 1;
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
					destination[output_triangle * 3 + 0] = static_cast<T>(a);
					destination[output_triangle * 3 + 1] = static_cast<T>(b);
					destination[output_triangle * 3 + 2] = static_cast<T>(c);
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
					if (time_stamp - cache_time_stamps[a] > cache_size)
						cache_time_stamps[a] = time_stamp++;
					
					if (time_stamp - cache_time_stamps[b] > cache_size)
						cache_time_stamps[b] = time_stamp++;
					
					if (time_stamp - cache_time_stamps[c] > cache_size)
						cache_time_stamps[c] = time_stamp++;
				
					// update emitted flags
					emitted_flags[triangle] = true;
				}
			}

			// next candidates are the ones we pushed to dead-end stack just now
			const unsigned int* next_candidates_end = &dead_end[0] + dead_end_top;
			
			// get next vertex
			current_vertex = getNextVertexNeighbour(next_candidates_begin, next_candidates_end, live_triangles, cache_time_stamps, time_stamp, cache_size);

			if (current_vertex == static_cast<unsigned int>(-1))
			{
				current_vertex = getNextVertexDeadEnd(dead_end, dead_end_top, input_cursor, live_triangles);
				
				if (clusters && current_vertex != static_cast<unsigned int>(-1))
				{
					// hard boundary, add cluster information
					clusters->push_back(output_triangle);
				}
			}
		}

		assert(output_triangle == index_count / 3);
	}
}

void optimizePostTransform(unsigned short* destination, const unsigned short* indices, size_t index_count, size_t vertex_count, unsigned int cache_size, std::vector<unsigned int>* clusters)
{
	optimizePostTransformTipsify(destination, indices, index_count, vertex_count, cache_size, clusters);
}

void optimizePostTransform(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size, std::vector<unsigned int>* clusters)
{
	optimizePostTransformTipsify(destination, indices, index_count, vertex_count, cache_size, clusters);
}
