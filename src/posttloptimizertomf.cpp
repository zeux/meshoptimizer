#include "meshoptimizer.hpp"

#include <cassert>
#include <cmath>
#include <vector>

namespace
{
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

	struct Vertex
	{
		Vertex(): cache_position(-1), score(0), triangles_total(0), triangles_not_added(0), triangles(0)
		{
		}
		
		int cache_position;
		float score;
		unsigned int triangles_total;
		unsigned int triangles_not_added;
		unsigned int* triangles;
	};

	static float vertexScore(const Vertex& vertex)
	{
		// check if there are any triangles needed
		if (vertex.triangles_not_added == 0) return -1;

		float score = vertex.cache_position == -1 ? 0 : cache_scores[vertex.cache_position];

		// Bonus points for having a low number of tris still to use the vert, so we get rid of lone verts quickly.
		// return score + valence_boost_scale * powf(vertex.triangles_not_added, -valence_boost_power);
		// for valence_boost_power = 0.5f
		return score + valence_boost_scale / sqrtf((float)vertex.triangles_not_added);
	}

	template <typename T> static void optimizeLinear(T* destination, const T* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
	{
		assert(destination != indices);
		assert(cache_size <= max_cache_size);

		const float m_inf = -1e32f;

		assert(index_count % 3 == 0);
		size_t face_count = index_count / 3;

		std::vector<Vertex> vertices(vertex_count);

		// initializing vertex data
		for (size_t i = 0; i < index_count; ++i)
		{
			vertices[indices[i]].triangles_total++;
		}

		// calculate total triangle number
		std::vector<unsigned int> triangle_indices(index_count); // total triangle indices count == index count
		unsigned int* triangle_indices_ptr = &triangle_indices[0];

		// allocate storage for triangle indices
		for (size_t i = 0; i < vertex_count; ++i)
		{
			Vertex& v = vertices[i];

			v.triangles = triangle_indices_ptr;
			triangle_indices_ptr += v.triangles_total;
		}

		// fill triangle indices
		for (size_t i = 0; i < face_count; ++i)
		{
			Vertex& a = vertices[indices[i * 3 + 0]];
			a.triangles[a.triangles_not_added++] = static_cast<unsigned int>(i);

			Vertex& b = vertices[indices[i * 3 + 1]];
			b.triangles[b.triangles_not_added++] = static_cast<unsigned int>(i);

			Vertex& c = vertices[indices[i * 3 + 2]];
			c.triangles[c.triangles_not_added++] = static_cast<unsigned int>(i);
		}

		// compute initial vertex scores
		for (size_t i = 0; i < vertex_count; ++i)
		{
			Vertex& v = vertices[i];

			v.score = vertexScore(v);
		}

		// compute triangle scores
		std::vector<float> triangle_scores(face_count);

		for (size_t i = 0; i < face_count; ++i)
		{
			unsigned int tri_a = indices[i * 3 + 0];
			unsigned int tri_b = indices[i * 3 + 1];
			unsigned int tri_c = indices[i * 3 + 2];

			triangle_scores[i] = vertices[tri_a].score + vertices[tri_b].score + vertices[tri_c].score;
		}

		T* destination_end = destination + index_count;

		unsigned int cache_holder[2 * (max_cache_size + 3)];
		unsigned int* cache = cache_holder;
		unsigned int* cache_end = cache;
		unsigned int* cache_new = cache + max_cache_size + 3;
		unsigned int* cache_new_end = cache_new;

		int min_tri = -1;

		// main body
		while (destination != destination_end)
		{
			// find triangle with the best score if it was not found on previous step
			if (min_tri < 0)
			{
				float min_score = m_inf;
				
				for (size_t i = 0; i < face_count; ++i)
				{
					float tri_score = triangle_scores[i];

					if (min_score < tri_score)
					{
						min_tri = int(i);
						min_score = tri_score;
					}
				}
			}

			unsigned int tri_a = indices[min_tri * 3 + 0];
			unsigned int tri_b = indices[min_tri * 3 + 1];
			unsigned int tri_c = indices[min_tri * 3 + 2];

			// add it to drawing sequence
			*destination++ = tri_a;
			*destination++ = tri_b;
			*destination++ = tri_c;

			triangle_scores[min_tri] = m_inf;

			// construct new cache
			cache_new_end = cache_new;

			// new triangle
			*cache_new_end++ = tri_a;
			*cache_new_end++ = tri_b;
			*cache_new_end++ = tri_c;

			// old parts
			int cp_a = vertices[tri_a].cache_position;
			int cp_b = vertices[tri_b].cache_position;
			int cp_c = vertices[tri_c].cache_position;

			// order indices
			if (cp_a > cp_b) std::swap(cp_a, cp_b);
			if (cp_a > cp_c) std::swap(cp_a, cp_c);
			if (cp_b > cp_c) std::swap(cp_b, cp_c);

			if (cp_c == -1) // largest is -1 => all 3 are -1
			{
				cache_new_end = std::copy(cache, cache_end, cache_new_end);
			}
			else if (cp_b == -1) // cp_a == -1, cp_b == -1
			{
				cache_new_end = std::copy(cache, cache + cp_c, cache_new_end);
				cache_new_end = std::copy(cache + cp_c + 1, cache_end, cache_new_end);
			}
			else if (cp_a == -1) // only cp_a == -1
			{
				cache_new_end = std::copy(cache, cache + cp_b, cache_new_end);
				cache_new_end = std::copy(cache + cp_b + 1, cache + cp_c, cache_new_end);
				cache_new_end = std::copy(cache + cp_c + 1, cache_end, cache_new_end);
			}
			else
			{
				cache_new_end = std::copy(cache, cache + cp_a, cache_new_end);
				cache_new_end = std::copy(cache + cp_a + 1, cache + cp_b, cache_new_end);
				cache_new_end = std::copy(cache + cp_b + 1, cache + cp_c, cache_new_end);
				cache_new_end = std::copy(cache + cp_c + 1, cache_end, cache_new_end);
			}

			size_t cache_new_size = cache_new_end - cache_new;

			if (cache_new_size > cache_size) cache_new_end = cache_new + cache_size;

			std::swap(cache, cache_new);
			std::swap(cache_end, cache_new_end);

			// modify vertices
			vertices[tri_a].triangles_not_added--;
			vertices[tri_b].triangles_not_added--;
			vertices[tri_c].triangles_not_added--;

			min_tri = -1;
			float min_score = m_inf;

			// update cache positions, vertices scores and triangle scores, and find new min_face
			for (size_t i = 0; i < cache_new_size; ++i)
			{
				Vertex& v = vertices[cache[i]];

				v.cache_position = i >= cache_size ? -1 : (int)i;

				float score = vertexScore(v);
				float score_diff = score - v.score;
				
				// update scores of vertex triangles
				for (size_t j = 0; j < v.triangles_total; ++j)
				{
					unsigned int tri = v.triangles[j];

					triangle_scores[tri] += score_diff;

					float tri_score = triangle_scores[tri];

					if (min_score < tri_score)
					{
						min_tri = tri;
						min_score = tri_score;
					}
				}

				v.score = score;
			}
		}
	}
}

void optimizePostTransformTomF(unsigned short* destination, const unsigned short* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	optimizeLinear(destination, indices, index_count, vertex_count, cache_size);
}

void optimizePostTransformTomF(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	optimizeLinear(destination, indices, index_count, vertex_count, cache_size);
}
