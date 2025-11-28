// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <string.h> // memcpy

namespace meshopt
{

/**
 *  Copyright (C) 2011 by Morten S. Mikkelsen
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */

static void mergeVerticesFast(size_t* unique_vertices_count, size_t* remap_indices, float* destination, size_t* vertex_ids, const float* vertices, size_t vertex_count, int num_floats_per_vertex, size_t left, size_t right, int channel)
{
	const size_t count = right - left + 1;
	assert(count > 0);

	float min_val = vertices[vertex_ids[left] * num_floats_per_vertex + channel];
	float max_val = min_val;

	for (size_t i = left + 1; i <= right; ++i)
	{
		const size_t index = vertex_ids[i] * num_floats_per_vertex + channel;
		const float val = vertices[index];

		if (min_val > val)
		{
			min_val = val;
		}
		else if (max_val < val)
		{
			max_val = val;
		}
	}

	const float avg = 0.5f * (max_val + min_val);

	if (avg <= min_val || avg >= max_val || count == 1)
	{
		if (channel + 1 == num_floats_per_vertex || count == 1)
		{
			size_t unique_new_vertices = 0;
			float* new_uni_verts = &destination[*unique_vertices_count * num_floats_per_vertex];

			for (size_t i = left; i <= right; ++i)
			{
				const size_t index = vertex_ids[i] * num_floats_per_vertex;

				int found = 0;
				size_t j = 0;

				while (j < unique_new_vertices && found == 0)
				{
					const size_t index2 = j * num_floats_per_vertex;
					int all_same = 1;

					for (int c = 0; all_same != 0 && c < num_floats_per_vertex; ++c)
					{
						all_same &= (vertices[index + c] == new_uni_verts[index2 + c] ? 1 : 0);
					}

					found = all_same;

					if (found == 0)
					{
						++j;
					}
				}

				if (found == 0)
				{
					memcpy(new_uni_verts + unique_new_vertices * num_floats_per_vertex, vertices + index, sizeof(float) * num_floats_per_vertex);
					++unique_new_vertices;
				}

				assert(remap_indices[vertex_ids[i]] == size_t(-1) );
				remap_indices[vertex_ids[i]] = (int)(*unique_vertices_count + j);
			}

			*unique_vertices_count += unique_new_vertices;
		}
		else
		{
			mergeVerticesFast(unique_vertices_count, remap_indices, destination, vertex_ids, vertices, vertex_count, num_floats_per_vertex, left, right, channel + 1);
		}
	}
	else
	{
		size_t l = left, r = right;

		while (l < r)
		{
			int ready_left_swap = 0;
			int ready_right_swap = 0;

			while (ready_left_swap == 0 && l < r)
			{
				const size_t index = vertex_ids[l] * num_floats_per_vertex + channel;
				ready_left_swap = !(vertices[index] < avg) ? 1 : 0;

				if (ready_left_swap == 0)
				{
					++l;
				}
			}

			while (ready_right_swap == 0 && l < r)
			{
				const size_t index = vertex_ids[r] * num_floats_per_vertex + channel;
				ready_right_swap = vertices[index] < avg ? 1 : 0;

				if (ready_right_swap == 0)
				{
					--r;
				}
			}

			if (ready_left_swap != 0 && ready_right_swap != 0)
			{
				const int id = vertex_ids[l];

				vertex_ids[l] = vertex_ids[r];
				vertex_ids[r] = id;
				++l;
				--r;
			}
		}

		if (l == r)
		{
			const size_t index = vertex_ids[r] * num_floats_per_vertex + channel;
			const int ready_right_swap = vertices[index] < avg ? 1 : 0;

			if (ready_right_swap != 0)
			{
				++l;
			}
			else
			{
				--r;
			}
		}

		if (left <= r)
		{
			mergeVerticesFast(unique_vertices_count, remap_indices, destination, vertex_ids, vertices, vertex_count, num_floats_per_vertex, left, r, channel);
		}

		if (l <= right)
		{
			mergeVerticesFast(unique_vertices_count, remap_indices, destination, vertex_ids, vertices, vertex_count, num_floats_per_vertex, l, right, channel);
		}
	}
}

} // namespace meshopt

size_t meshopt_weldMesh(size_t* remap_indices, float* destination, const float* vertices, size_t vertex_count, int num_floats_per_vertex)
{
	size_t unique_vertices = 0;

	if (vertex_count > 0)
	{
		using namespace meshopt;

		meshopt_Allocator allocator;

		size_t* vertex_ids = allocator.allocate<size_t>(vertex_count);

		if (vertex_ids != NULL)
		{
			for (size_t i = 0; i < vertex_count; ++i)
			{
				remap_indices[i] = size_t(-1);
				vertex_ids[i] = i;
			}

			mergeVerticesFast(&unique_vertices, remap_indices, destination, vertex_ids, vertices, vertex_count, num_floats_per_vertex, 0, vertex_count - 1, 0);

			allocator.deallocate(vertex_ids);

#if _DEBUG
			for(size_t i = 0; i < unique_vertices; ++i)
			{
				assert(remap_indices[i] >= 0);
			}
#endif // _DEBUG
		}
	}

	return unique_vertices;
}
