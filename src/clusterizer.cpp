// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <math.h>
#include <string.h>

size_t meshopt_buildMeshletsBound(size_t index_count, size_t max_vertices, size_t max_triangles)
{
	assert(index_count % 3 == 0);
	assert(max_vertices >= 3);
	assert(max_triangles >= 1);

	// meshlet construction is limited by max vertices and max triangles per meshlet
	// the worst case is that the input is an unindexed stream since this equally stresses both limits
	// note that we assume that in the worst case, we leave 2 vertices unpacked in each meshlet - if we have space for 3 we can pack any triangle
	size_t max_vertices_conservative = max_vertices - 2;
	size_t meshlet_limit_vertices = (index_count + max_vertices_conservative - 1) / max_vertices_conservative;
	size_t meshlet_limit_triangles = (index_count / 3 + max_triangles - 1) / max_triangles; 

	return meshlet_limit_vertices > meshlet_limit_triangles ? meshlet_limit_vertices : meshlet_limit_triangles;
}

size_t meshopt_buildMeshlets(meshopt_Meshlet* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, size_t max_vertices, size_t max_triangles)
{
	assert(index_count % 3 == 0);
	assert(max_vertices >= 3);
	assert(max_triangles >= 1);

	meshopt_Meshlet meshlet;
	memset(&meshlet, 0, sizeof(meshlet));

	assert(max_vertices <= sizeof(meshlet.vertices) / sizeof(meshlet.vertices[0]));
	assert(max_triangles <= sizeof(meshlet.indices) / 3);

	// index of the vertex in the meshlet, 0xff if the vertex isn't used
	meshopt_Buffer<unsigned char> used(vertex_count);
	memset(used.data, -1, vertex_count);

	size_t offset = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		unsigned char& av = used[a];
		unsigned char& bv = used[b];
		unsigned char& cv = used[c];

		unsigned int used_extra = (av == 0xff) + (bv == 0xff) + (cv == 0xff);

		if (meshlet.vertex_count + used_extra > max_vertices || meshlet.triangle_count >= max_triangles)
		{
			destination[offset++] = meshlet;

			for (size_t j = 0; j < meshlet.vertex_count; ++j)
				used[meshlet.vertices[j]] = 0xff;

			memset(&meshlet, 0, sizeof(meshlet));
		}

		if (av == 0xff)
		{
			av = meshlet.vertex_count;
			meshlet.vertices[meshlet.vertex_count++] = a;
		}

		if (bv == 0xff)
		{
			bv = meshlet.vertex_count;
			meshlet.vertices[meshlet.vertex_count++] = b;
		}

		if (cv == 0xff)
		{
			cv = meshlet.vertex_count;
			meshlet.vertices[meshlet.vertex_count++] = c;
		}

		meshlet.indices[meshlet.triangle_count][0] = av;
		meshlet.indices[meshlet.triangle_count][1] = bv;
		meshlet.indices[meshlet.triangle_count][2] = cv;
		meshlet.triangle_count++;
	}

	if (meshlet.triangle_count)
		destination[offset++] = meshlet;

	assert(offset <= meshopt_buildMeshletsBound(index_count, max_vertices, max_triangles));

	return offset;
}

meshopt_Cone meshopt_computeClusterCone(const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	assert(index_count % 3 == 0);
	assert(vertex_positions_stride > 0 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	assert(index_count / 3 <= 256);

	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	float normals[256][3];
	float corners[256][3];
	unsigned int triangles = 0;

	for (unsigned int i = 0; i < index_count; i += 3)
	{
		unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		const float* p0 = vertex_positions + vertex_stride_float * a;
		const float* p1 = vertex_positions + vertex_stride_float * b;
		const float* p2 = vertex_positions + vertex_stride_float * c;

		float p10[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
		float p20[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };

		float normalx = p10[1] * p20[2] - p10[2] * p20[1];
		float normaly = p10[2] * p20[0] - p10[0] * p20[2];
		float normalz = p10[0] * p20[1] - p10[1] * p20[0];

		float area = sqrtf(normalx * normalx + normaly * normaly + normalz * normalz);

		// no need to include degenerate triangles - they will be invisible anyway
		if (area == 0.f)
			continue;

		normals[triangles][0] = normalx / area;
		normals[triangles][1] = normaly / area;
		normals[triangles][2] = normalz / area;
		corners[triangles][0] = p0[0];
		corners[triangles][1] = p0[1];
		corners[triangles][2] = p0[2];
		triangles++;
	}

	float avgnormal[3] = {};

	for (unsigned int i = 0; i < triangles; ++i)
	{
		avgnormal[0] += normals[i][0];
		avgnormal[1] += normals[i][1];
		avgnormal[2] += normals[i][2];
	}

	float avglength = sqrtf(avgnormal[0] * avgnormal[0] + avgnormal[1] * avgnormal[1] + avgnormal[2] * avgnormal[2]);
	// TODO: robustness
	float invavglength = avglength == 0.f ? 0.f : 1.f / avglength;

	avgnormal[0] *= invavglength;
	avgnormal[1] *= invavglength;
	avgnormal[2] *= invavglength;

	float mindp = 1.f;

	for (unsigned int i = 0; i < triangles; ++i)
	{
		float dp = normals[i][0] * avgnormal[0] + normals[i][1] * avgnormal[1] + normals[i][2] * avgnormal[2];

		mindp = (dp < mindp) ? dp : mindp;
	}

	if (1)
	{
		meshopt_Cone cone;

		cone.apex[0] = 0.f;
		cone.apex[1] = 0.f;
		cone.apex[2] = 0.f;
		cone.direction[0] = avgnormal[0];
		cone.direction[1] = avgnormal[1];
		cone.direction[2] = avgnormal[2];
		cone.cutoff = (mindp <= 0.f) ? 1 : sqrtf(1 - mindp * mindp);

		return cone;
	}
	else
	{
		avgnormal[0] *= -1;
		avgnormal[1] *= -1;
		avgnormal[2] *= -1;

		float center[3] = {};

		// probably not a great idea to just average a single corner
		for (unsigned int i = 0; i < triangles; ++i)
		{
			center[0] += corners[i][0];
			center[1] += corners[i][1];
			center[2] += corners[i][2];
		}

		// TODO: triangles==0
		center[0] /= triangles;
		center[1] /= triangles;
		center[2] /= triangles;

		// for each triangle:
		// intersect center+t*avgnormal with triangle
		// record maximum t
		// if any triangles are parallel, we need a degen cone
		float maxt = 0;

		for (unsigned int i = 0; i < triangles; ++i)
		{
			// dot(center+t*avgnormal-corner, trinormal) = 0
			// dot(center-corner, trinormal) + t * dot(avgnormal, trinormal) = 0
			float dc = (center[0] - corners[i][0]) * normals[i][0] + (center[1] - corners[i][1]) * normals[i][1] + (center[2] - corners[i][2]) * normals[i][2];
			float dn = avgnormal[0] * normals[i][0] + avgnormal[1] * normals[i][1] + avgnormal[2] * normals[i][2];

			// TODO: if (dn == 0.f) maxt = inf
			float t = -dc / dn;

			maxt = (t > maxt) ? t : maxt;
		}

		meshopt_Cone cone;

		// TODO: the signs are potentially wrong here
		cone.apex[0] = center[0] + avgnormal[0] * maxt;
		cone.apex[1] = center[1] + avgnormal[1] * maxt;
		cone.apex[2] = center[2] + avgnormal[2] * maxt;
		cone.direction[0] = -avgnormal[0];
		cone.direction[1] = -avgnormal[1];
		cone.direction[2] = -avgnormal[2];
		// TODO: this is potentially wrong as well :)
		cone.cutoff = (mindp <= 0.f) ? 1 : sqrtf(1 - mindp * mindp);

		return cone;
	}
}

meshopt_Cone meshopt_computeMeshletCone(const meshopt_Meshlet* meshlet, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	assert(vertex_positions_stride > 0 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	unsigned int indices[sizeof(meshlet->indices) / sizeof(meshlet->indices[0][0])];

	for (unsigned int i = 0; i < meshlet->triangle_count; ++i)
	{
		unsigned int a = meshlet->vertices[meshlet->indices[i][0]];
		unsigned int b = meshlet->vertices[meshlet->indices[i][1]];
		unsigned int c = meshlet->vertices[meshlet->indices[i][2]];

		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		indices[i * 3 + 0] = a;
		indices[i * 3 + 1] = b;
		indices[i * 3 + 2] = c;
	}

	return meshopt_computeClusterCone(indices, meshlet->triangle_count * 3, vertex_positions, vertex_count, vertex_positions_stride);
}
