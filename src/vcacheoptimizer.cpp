// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <cassert>
#include <cmath>

#include <vector>

// This work is based on:
// Tom Forsyth. Linear-Speed Vertex Cache Optimisation. 2006
// Pedro Sander, Diego Nehab and Joshua Barczak. Fast Triangle Reordering for Vertex Locality and Reduced Overdraw. 2007
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

static void optimizeVertexCacheTipsify(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
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

	assert(output_triangle == index_count / 3);
}

const size_t max_cache_size = 32;
const size_t max_valence = 32;

#if 0
static float vertex_score_table_cache[1 + max_cache_size];
static float vertex_score_table_live[max_valence];

void vertexScoreInit(float last_triangle_score, float valence_boost_scale, float valence_boost_power, float cache_decay_power)
{
	vertex_score_table_cache[0] = 0;

	for (size_t i = 0; i < max_cache_size; ++i)
	{
		vertex_score_table_cache[1 + i] = (i < 3) ? last_triangle_score : powf(float(max_cache_size - i) / float(max_cache_size - 3), cache_decay_power);
	}

	vertex_score_table_live[0] = 0;

	for (size_t i = 1; i < max_valence; ++i)
	{
		vertex_score_table_live[i] = valence_boost_scale / powf(float(i), valence_boost_power);
	}
}
#else
// last_triangle_score = 0.8, cache_decay_power = 1.5
static const float vertex_score_table_cache[1 + max_cache_size] =
    {
        // clang-format array formatting is broken :/
        0.000000f,
        0.800000f, 0.800000f, 0.800000f, 1.000000f, 0.948724f, 0.898356f, 0.848913f, 0.800411f,
        0.752870f, 0.706309f, 0.660750f, 0.616215f, 0.572727f, 0.530314f, 0.489003f, 0.448824f,
        0.409810f, 0.371997f, 0.335425f, 0.300136f, 0.266180f, 0.233610f, 0.202490f, 0.172889f,
        0.144890f, 0.118591f, 0.094109f, 0.071591f, 0.051226f, 0.033272f, 0.018111f, 0.006403f,
};

// valence_boost_scale = 3.2, valence_boost_power = 0.9
static const float vertex_score_table_live[1 + max_valence] =
    {
        // clang-format array formatting is broken :/
        0.000000f, 3.200000f, 1.714838f, 1.190531f, 0.918959f, 0.751756f, 0.637990f, 0.555344f,
        0.492458f, 0.442927f, 0.402856f, 0.369740f, 0.341890f, 0.318127f, 0.297601f, 0.279684f,
        0.263902f, 0.249888f, 0.237358f, 0.226085f, 0.215885f, 0.206611f, 0.198139f, 0.190368f,
        0.183215f, 0.176605f, 0.170480f, 0.164787f, 0.159481f, 0.154523f, 0.149879f, 0.145521f,
};
#endif

static float vertexScore(int cache_position, unsigned int live_triangles)
{
	assert(cache_position >= -1 && cache_position < int(max_cache_size));

	unsigned int live_triangles_clamped = live_triangles < max_valence ? live_triangles : max_valence - 1;

	return vertex_score_table_cache[1 + cache_position] + vertex_score_table_live[live_triangles_clamped];
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

static void optimizeVertexCacheForsyth(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
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
	unsigned int* cache_new = cache_holder + max_cache_size + 3;
	size_t cache_count = 0;

	unsigned int current_triangle = 0;
	unsigned int input_cursor = 1;

	unsigned int output_triangle = 0;

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
		triangle_scores[current_triangle] = 0;

		// new triangle
		size_t cache_write = 0;
		cache_new[cache_write++] = a;
		cache_new[cache_write++] = b;
		cache_new[cache_write++] = c;

		// old triangles
		for (size_t i = 0; i < cache_count; ++i)
		{
			unsigned int index = cache[i];

			if (index != a && index != b && index != c)
			{
				cache_new[cache_write++] = index;
			}
		}

		std::swap(cache, cache_new);
		cache_count = cache_write > cache_size ? cache_size : cache_write;

		// update live triangle counts
		live_triangles[a]--;
		live_triangles[b]--;
		live_triangles[c]--;

		// remove emitted triangle from adjacency data
		// this makes sure that we spend less time traversing these lists on subsequent iterations
		for (size_t k = 0; k < 3; ++k)
		{
			unsigned int index = indices[current_triangle * 3 + k];

			unsigned int* neighbours = &adjacency.data[0] + adjacency.offsets[index];
			size_t neighbours_size = adjacency.triangle_counts[index];

			for (size_t i = 0; i < neighbours_size; ++i)
			{
				unsigned int tri = neighbours[i];

				if (tri == current_triangle)
				{
					neighbours[i] = neighbours[neighbours_size - 1];
					adjacency.triangle_counts[index]--;
					break;
				}
			}
		}

		unsigned int best_triangle = ~0u;
		float best_score = 0;

		// update cache positions, vertex scores and triangle scores, and find next best triangle
		for (size_t i = 0; i < cache_write; ++i)
		{
			unsigned int index = cache[i];

			int cache_position = i >= cache_size ? -1 : int(i);

			// update vertex score
			float score = vertexScore(cache_position, live_triangles[index]);
			float score_diff = score - vertex_scores[index];

			vertex_scores[index] = score;

			// update scores of vertex triangles
			const unsigned int* neighbours_begin = &adjacency.data[0] + adjacency.offsets[index];
			const unsigned int* neighbours_end = neighbours_begin + adjacency.triangle_counts[index];

			for (const unsigned int* it = neighbours_begin; it != neighbours_end; ++it)
			{
				unsigned int tri = *it;
				assert(!emitted_flags[tri]);

				float tri_score = triangle_scores[tri] + score_diff;
				assert(tri_score > 0);

				if (best_score < tri_score)
				{
					best_triangle = tri;
					best_score = tri_score;
				}

				triangle_scores[tri] = tri_score;
			}
		}

		// step through input triangles in order if we hit a dead-end
		current_triangle = best_triangle;

		if (current_triangle == ~0u)
		{
			current_triangle = getNextTriangleDeadEnd(input_cursor, emitted_flags);
		}
	}

	assert(input_cursor == index_count / 3);
	assert(output_triangle == index_count / 3);
}

} // namespace meshopt

void meshopt_optimizeVertexCache(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	using namespace meshopt;

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
		optimizeVertexCacheForsyth(destination, indices, index_count, vertex_count, 16);
	else
		optimizeVertexCacheTipsify(destination, indices, index_count, vertex_count, cache_size);
}
