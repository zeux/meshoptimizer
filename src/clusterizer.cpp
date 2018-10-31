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

	float cluster_area = 0;
	float cluster_centroid[3] = {};

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

		// record triangle normals & corners for future use (this is a plane eqn)
		normals[triangles][0] = normalx / area;
		normals[triangles][1] = normaly / area;
		normals[triangles][2] = normalz / area;
		corners[triangles][0] = p0[0];
		corners[triangles][1] = p0[1];
		corners[triangles][2] = p0[2];
		triangles++;

		// area-weighted centroid
		cluster_centroid[0] += (p0[0] + p1[0] + p2[0]) * (area / 3);
		cluster_centroid[1] += (p0[1] + p1[1] + p2[1]) * (area / 3);
		cluster_centroid[2] += (p0[2] + p1[2] + p2[2]) * (area / 3);
		cluster_area += area;
	}

	float inv_cluster_area = cluster_area == 0 ? 0 : 1 / cluster_area;

	cluster_centroid[0] *= inv_cluster_area;
	cluster_centroid[1] *= inv_cluster_area;
	cluster_centroid[2] *= inv_cluster_area;

	// build a cone around triangle normals
	float axis[3] = {};

	for (unsigned int i = 0; i < triangles; ++i)
	{
		axis[0] += normals[i][0];
		axis[1] += normals[i][1];
		axis[2] += normals[i][2];
	}

	float avglength = sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
	float invavglength = avglength == 0.f ? 0.f : 1.f / avglength;

	axis[0] *= invavglength;
	axis[1] *= invavglength;
	axis[2] *= invavglength;

	float mindp = 1.f;

	for (unsigned int i = 0; i < triangles; ++i)
	{
		float dp = normals[i][0] * axis[0] + normals[i][1] * axis[1] + normals[i][2] * axis[2];

		mindp = (dp < mindp) ? dp : mindp;
	}

	// degenerate cluster, no valid triangles or normal cone is larger than a hemisphere
	if (triangles == 0 || mindp <= 0.f)
	{
		meshopt_Cone cone = {};
		cone.cutoff = 1;
		return cone;
	}

	float maxt = 0;

	// we need to find the point on centroid-t*axis ray that lies in negative half-space of all triangles
	for (unsigned int i = 0; i < triangles; ++i)
	{
		// dot(centroid-t*axis-corner, trinormal) = 0
		// dot(centroid-corner, trinormal) - t * dot(axis, trinormal) = 0
		float cx = cluster_centroid[0] - corners[i][0];
		float cy = cluster_centroid[1] - corners[i][1];
		float cz = cluster_centroid[2] - corners[i][2];

		float dc = cx * normals[i][0] + cy * normals[i][1] + cz * normals[i][2];
		float dn = axis[0] * normals[i][0] + axis[1] * normals[i][1] + axis[2] * normals[i][2];

		// dn should be more than mindp cutoff above
		assert(dn > 0.f);
		float t = dc / dn;

		maxt = (t > maxt) ? t : maxt;
	}

	meshopt_Cone cone;

	// cone apex should be in the negative half-space of all cluster triangles by construction
	cone.apex[0] = cluster_centroid[0] - axis[0] * maxt;
	cone.apex[1] = cluster_centroid[1] - axis[1] * maxt;
	cone.apex[2] = cluster_centroid[2] - axis[2] * maxt;

	// note: this axis is the axis of the normal cone, but our test for perspective camera effectively negates the axis
	cone.axis[0] = axis[0];
	cone.axis[1] = axis[1];
	cone.axis[2] = axis[2];

	// cos(a) for normal cone is mindp; we need to add 90 degrees on both sides and invert the cone
	// which gives us -cos(a+90) = -(-sin(a)) = sin(a) = sqrt(1 - cos^2(a))
	cone.cutoff = sqrtf(1 - mindp * mindp);

	return cone;
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
