#include "posttloptimizertomf.hpp"

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

	template <typename T> class MeshOptimizerLinear
	{
	private:
		struct face_t
		{
			T a, b, c;
		};
		
		struct triangle_t: public face_t
		{
			float score;
		};
		
		struct vertex_t
		{
			vertex_t(): cache_position(-1), triangles_total(0), triangles_not_added(0)
			{
			}
			
			int cache_position;
			float score;
			unsigned int triangles_total;
			unsigned int triangles_not_added;
			triangle_t** triangles;
		};
		
		static float vertexScore(const vertex_t& vertex)
		{
			// check if there are any triangles needed
			if (vertex.triangles_not_added == 0) return -1;
		
			float score = vertex.cache_position == -1 ? 0 : cache_scores[vertex.cache_position];

			// Bonus points for having a low number of tris still to use the vert, so we get rid of lone verts quickly.
			// return score + valence_boost_scale * powf(vertex.triangles_not_added, -valence_boost_power);
			// for valence_boost_power = 0.5f
			return score + valence_boost_scale / sqrtf((float)vertex.triangles_not_added);
		}
		
	public:
		static void optimize(T* destination, const T* indices, size_t count, size_t vertex_count)
		{
			assert(destination != indices);

			const float m_inf = -1e32f;
			
			const face_t* faces = reinterpret_cast<const face_t*>(indices);
			
			assert(count % 3 == 0);
			size_t face_count = count / 3;
			
			std::vector<vertex_t> vertices(vertex_count);
			
			// initializing vertex data
			for (size_t i = 0; i < face_count; ++i)
			{
				const face_t& f = faces[i];
				
				vertices[f.a].triangles_total++;
				vertices[f.b].triangles_total++;
				vertices[f.c].triangles_total++;
			}
			
			// calculate total triangle number
			std::vector<triangle_t*> triangle_indices(count); // total triangle indices count == index count
			triangle_t** triangle_indices_ptr = &triangle_indices[0];
			
			// allocate storage for triangle indices
			for (size_t i = 0; i < vertex_count; ++i)
			{
				vertex_t& v = vertices[i];
				
				v.triangles = triangle_indices_ptr;
				triangle_indices_ptr += v.triangles_total;
			}

			std::vector<triangle_t> triangles(face_count);
			
			// fill triangle indices
			for (size_t i = 0; i < face_count; ++i)
			{
				const face_t& f = faces[i];
				triangle_t& t = triangles[i];
				
				t.a = f.a;
				t.b = f.b;
				t.c = f.c;
				
				vertex_t& a = vertices[f.a];
				a.triangles[a.triangles_not_added++] = &t;
				
				vertex_t& b = vertices[f.b];
				b.triangles[b.triangles_not_added++] = &t;
				
				vertex_t& c = vertices[f.c];
				c.triangles[c.triangles_not_added++] = &t;
			}
			
			// compute initial vertex scores
			for (size_t i = 0; i < vertex_count; ++i)
			{
				vertex_t& v = vertices[i];
				
				v.score = vertexScore(v);
			}
			
			// compute triangle scores
			for (size_t i = 0; i < face_count; ++i)
			{
				triangle_t& t = triangles[i];
				t.score = vertices[t.a].score + vertices[t.b].score + vertices[t.c].score;
			}
			
			T* destination_end = destination + count;
			
			unsigned int cache_holder[2 * (max_cache_size + 3)];
			unsigned int* cache = cache_holder;
			unsigned int* cache_end = cache;
			unsigned int* cache_new = cache + max_cache_size + 3;
			unsigned int* cache_new_end = cache_new;
			
			triangle_t* min_tri = 0;
			
			// main body
			while (destination != destination_end)
			{
				// find triangle with the best score if it was not found on previous step
				if (min_tri == 0)
				{
					float min_score = m_inf;
					
					for (size_t i = 0; i < face_count; ++i)
					{
						triangle_t& t = triangles[i];
						
						if (min_score < t.score)
						{
							min_tri = &t;
							min_score = t.score;
						}
					}
				}
				
				// add it to drawing sequence
				*destination++ = min_tri->a;
				*destination++ = min_tri->b;
				*destination++ = min_tri->c;
				
				min_tri->score = m_inf;
				
				// construct new cache
				cache_new_end = cache_new;
				
				// new triangle
				*cache_new_end++ = min_tri->a;
				*cache_new_end++ = min_tri->b;
				*cache_new_end++ = min_tri->c;
				
				// old parts
				int cp_a = vertices[min_tri->a].cache_position;
				int cp_b = vertices[min_tri->b].cache_position;
				int cp_c = vertices[min_tri->c].cache_position;
				
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
				
				size_t cache_size = cache_new_end - cache_new;
				
				if (cache_size > max_cache_size) cache_new_end = cache_new + max_cache_size;
				
				std::swap(cache, cache_new);
				std::swap(cache_end, cache_new_end);
				
				// modify vertices
				vertices[min_tri->a].triangles_not_added--;
				vertices[min_tri->b].triangles_not_added--;
				vertices[min_tri->c].triangles_not_added--;
				
				min_tri = 0;
				float min_score = m_inf;

				// update cache positions, vertices scores and triangle scores, and find new min_face
				for (size_t i = 0; i < cache_size; ++i)
				{
					vertex_t& v = vertices[cache[i]];
					v.cache_position = i >= max_cache_size ? -1 : (int)i;
					
					float score = vertexScore(v);
					float score_diff = score - v.score;
					
					// update scores of vertex triangles
					for (size_t j = 0; j < v.triangles_total; ++j)
					{
						triangle_t& t = *v.triangles[j];
						
						t.score += score_diff;
						
						if (min_score < t.score)
						{
							min_tri = &t;
							min_score = t.score;
						}
					}
					
					v.score = score;
				}
			}
		}
	};
}

void optimizePostTLTomF(unsigned short* destination, const unsigned short* indices, size_t index_count, size_t vertex_count)
{
	MeshOptimizerLinear<unsigned short>::optimize(destination, indices, index_count, vertex_count);
}

void optimizePostTLTomF(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count)
{
	MeshOptimizerLinear<unsigned int>::optimize(destination, indices, index_count, vertex_count);
}
