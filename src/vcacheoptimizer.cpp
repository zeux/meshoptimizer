// This file is part of meshoptimizer library; see meshoptimizer.hpp for version/license details
#include "meshoptimizer.hpp"

#include <cassert>
#include <cmath>
#include <vector>

// This work is based on:
// Tom Forsyth. Linear-Speed Vertex Cache Optimisation. 2006
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

	return ~0u;
}

static unsigned int getNextVertexNeighbour(const unsigned int* next_candidates_begin, const unsigned int* next_candidates_end, const std::vector<unsigned int>& live_triangles, const std::vector<unsigned int>& cache_timestamps, unsigned int timestamp, unsigned int cache_size)
{
	unsigned int best_candidate = ~0u;
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

static void optimizeVertexCacheFIFO(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
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

	while (current_vertex != ~0u)
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

		if (current_vertex == ~0u)
		{
			current_vertex = getNextVertexDeadEnd(dead_end, dead_end_top, input_cursor, live_triangles);
		}
	}

	assert(input_cursor == index_count / 3);
	assert(output_triangle == index_count / 3);
}

const size_t max_cache_size = 32;

const float valence_boost_scale = 2.0f;
const float last_triangle_score = 0.75f;

// The table is for max_cache_size = 32, cache_decay_power = 1.5,
// Equation is ((position - 3) / (max_cache_size - 3)) ^ cache_decay_power
static const float cache_scores[max_cache_size] =
{
	last_triangle_score, last_triangle_score, last_triangle_score,
	// perl -e "for ($i = 32; $i > 3; --$i) {print sqrt(($i - 3) / 29)*(($i-3)/29);print 'f, ';}"
	1, 0.948724356160999f, 0.898356365398566f, 0.84891268884993f, 0.800410940418327f,
	0.752869781209394f, 0.706309027617917f, 0.660749775712085f, 0.616214545218346f,
	0.572727447267172f, 0.530314381193352f, 0.489003267201725f, 0.448824323769283f,
	0.409810401494183f, 0.371997389083488f, 0.335424712859142f, 0.300135959460546f,
	0.266179663821797f, 0.233610323536753f, 0.202489730867139f, 0.172888763130359f,
	0.144889856948659f, 0.118590544520125f, 0.0941087226511742f, 0.0715909309083965f,
	0.0512263001867729f, 0.0332724579777247f, 0.0181112321185824f, 0.00640328752334662f,
};

static float vertexScore(int cache_position, unsigned int live_triangles)
{
	// check if there are any triangles needed
	if (live_triangles == 0)
		return -1;

	float score = cache_position == -1 ? 0 : cache_scores[cache_position];

	// Bonus points for having a low number of tris still to use the vert, so we get rid of lone verts quickly.
	// return score + valence_boost_scale * powf(vertex.triangles_not_added, -valence_boost_power);
	// for valence_boost_power = 0.5f
	return score + valence_boost_scale / sqrtf(float(live_triangles));
}

static unsigned int getNextTriangleDeadEnd(unsigned int& input_cursor, const std::vector<char>& emitted_flags)
{
	// input order
	while (input_cursor < emitted_flags.size())
	{
		if (!emitted_flags[input_cursor])
			return input_cursor;

		++input_cursor;
	}

	return ~0u;
}

static void optimizeVertexCacheLRU(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	assert(index_count % 3 == 0);
	assert(cache_size <= max_cache_size);

	// build adjacency information
	Adjacency adjacency;

	buildAdjacency(adjacency, indices, index_count, vertex_count);

	// live triangle counts
	std::vector<unsigned int> live_triangles = adjacency.triangle_counts;

	// emitted flags
	std::vector<char> emitted_flags(index_count / 3, false);

	// compute initial vertex scores
	std::vector<float> vertex_scores(vertex_count);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		vertex_scores[i] = vertexScore(-1, live_triangles[i]);
	}

	// compute triangle scores
	std::vector<float> triangle_scores(index_count / 3);

	for (size_t i = 0; i < triangle_scores.size(); ++i)
	{
		unsigned int a = indices[i * 3 + 0];
		unsigned int b = indices[i * 3 + 1];
		unsigned int c = indices[i * 3 + 2];

		triangle_scores[i] = vertex_scores[a] + vertex_scores[b] + vertex_scores[c];
	}

	unsigned int cache_holder[2 * (max_cache_size + 3)];
	unsigned int* cache = cache_holder;
	unsigned int* cache_end = cache;
	unsigned int* cache_new = cache + max_cache_size + 3;
	unsigned int* cache_new_end = cache_new;

	unsigned int current_triangle = 0;
	unsigned int input_cursor = 1;

	unsigned int output_triangle = 0;

	float min_score = -1e22f;

	while (current_triangle != ~0u)
	{
		assert(output_triangle < index_count / 3);

		unsigned int a = indices[current_triangle * 3 + 0];
		unsigned int b = indices[current_triangle * 3 + 1];
		unsigned int c = indices[current_triangle * 3 + 2];

		// output indices
		destination[output_triangle * 3 + 0] = a;
		destination[output_triangle * 3 + 1] = b;
		destination[output_triangle * 3 + 2] = c;
		output_triangle++;

		// update emitted flags
		emitted_flags[current_triangle] = true;
		triangle_scores[current_triangle] = min_score;

		// construct new cache
		cache_new_end = cache_new;

		// new triangle
		*cache_new_end++ = a;
		*cache_new_end++ = b;
		*cache_new_end++ = c;

		// old triangles
		for (const unsigned int* it = cache; it != cache_end; ++it)
		{
			if (*it != a && *it != b && *it != c)
			{
				*cache_new_end++ = *it;
			}
		}

		size_t cache_new_size = cache_new_end - cache_new;

		if (cache_new_size > cache_size) cache_new_end = cache_new + cache_size;

		std::swap(cache, cache_new);
		std::swap(cache_end, cache_new_end);

		// update live triangle counts
		live_triangles[a]--;
		live_triangles[b]--;
		live_triangles[c]--;

		current_triangle = ~0u;

		float best_score = min_score;

		// update cache positions, vertices scores and triangle scores, and find next best triangle
		for (size_t i = 0; i < cache_new_size; ++i)
		{
			unsigned int index = cache[i];

			int cache_position = i >= cache_size ? -1 : int(i);

			float score = vertexScore(cache_position, live_triangles[index]);
			float score_diff = score - vertex_scores[index];

			// update scores of vertex triangles
			const unsigned int* neighbours_begin = &adjacency.data[0] + adjacency.offsets[index];
			const unsigned int* neighbours_end = neighbours_begin + adjacency.triangle_counts[index];

			for (const unsigned int* it = neighbours_begin; it != neighbours_end; ++it)
			{
				unsigned int tri = *it;

				triangle_scores[tri] += score_diff;

				float tri_score = triangle_scores[tri];

				if (best_score < tri_score)
				{
					current_triangle = tri;
					best_score = tri_score;
				}
			}

			vertex_scores[index] = score;
		}

		// step through input triangles in order if we hit a dead-end
		if (current_triangle == ~0u)
		{
			current_triangle = getNextTriangleDeadEnd(input_cursor, emitted_flags);
		}
	}

	assert(input_cursor == index_count / 3);
	assert(output_triangle == index_count / 3);
}

void optimizeVertexCache(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	assert(index_count % 3 == 0);
	assert(cache_size == 0 || cache_size >= 3);

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

	if (cache_size == 0)
		optimizeVertexCacheLRU(destination, indices, index_count, vertex_count, 16);
	else
		optimizeVertexCacheFIFO(destination, indices, index_count, vertex_count, cache_size);
}

} // namespace meshopt
