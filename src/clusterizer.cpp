// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <limits>
#include <math.h>
#include <string.h>

// This work is based on:
// Graham Wihlidal. Optimizing the Graphics Pipeline with Compute. 2016
// Matthaeus Chajdas. GeometryFX 1.2 - Cluster Culling. 2016
// Jack Ritter. An Efficient Bounding Sphere. 1990
namespace meshopt
{

// This must be <= 255 since index 0xff is used internally to indice a vertex that doesn't belong to a meshlet
const size_t kMeshletMaxVertices = 255;

// A reasonable limit is around 2*max_vertices or less
const size_t kMeshletMaxTriangles = 512;

struct TriangleAdjacency2
{
	datatype_t* counts;
	datatype_t* offsets;
	datatype_t* data;
};

static void buildTriangleAdjacency(TriangleAdjacency2& adjacency, const datatype_t* indices, size_t index_count, size_t vertex_count, meshopt_Allocator& allocator)
{
	size_t face_count = index_count / 3;

	// allocate arrays
	adjacency.counts = allocator.allocate<datatype_t>(vertex_count);
	adjacency.offsets = allocator.allocate<datatype_t>(vertex_count);
	adjacency.data = allocator.allocate<datatype_t>(index_count);

	// fill triangle counts
	memset(adjacency.counts, 0, vertex_count * sizeof(datatype_t));

	for (size_t i = 0; i < index_count; ++i)
	{
		assert(indices[i] < vertex_count);

		adjacency.counts[indices[i]]++;
	}

	// fill offset table
	datatype_t offset = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		adjacency.offsets[i] = offset;
		offset += adjacency.counts[i];
	}

	assert(offset == index_count);

	// fill triangle data
	for (size_t i = 0; i < face_count; ++i)
	{
		datatype_t a = indices[i * 3 + 0];
		datatype_t b = indices[i * 3 + 1];
		datatype_t c = indices[i * 3 + 2];

		adjacency.data[adjacency.offsets[a]++] = i;
		adjacency.data[adjacency.offsets[b]++] = i;
		adjacency.data[adjacency.offsets[c]++] = i;
	}

	// fix offsets that have been disturbed by the previous pass
	for (size_t i = 0; i < vertex_count; ++i)
	{
		assert(adjacency.offsets[i] >= adjacency.counts[i]);

		adjacency.offsets[i] -= adjacency.counts[i];
	}
}

static void computeBoundingSphere(real_t result[4], const real_t points[][3], size_t count)
{
	assert(count > 0);

	// find extremum points along all 3 axes; for each axis we get a pair of points with min/max coordinates
	size_t pmin[3] = {0, 0, 0};
	size_t pmax[3] = {0, 0, 0};

	for (size_t i = 0; i < count; ++i)
	{
		const real_t* p = points[i];

		for (int axis = 0; axis < 3; ++axis)
		{
			pmin[axis] = (p[axis] < points[pmin[axis]][axis]) ? i : pmin[axis];
			pmax[axis] = (p[axis] > points[pmax[axis]][axis]) ? i : pmax[axis];
		}
	}

	// find the pair of points with largest distance
	real_t paxisd2 = 0;
	int paxis = 0;

	for (int axis = 0; axis < 3; ++axis)
	{
		const real_t* p1 = points[pmin[axis]];
		const real_t* p2 = points[pmax[axis]];

		real_t d2 = (p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]) + (p2[2] - p1[2]) * (p2[2] - p1[2]);

		if (d2 > paxisd2)
		{
			paxisd2 = d2;
			paxis = axis;
		}
	}

	// use the longest segment as the initial sphere diameter
	const real_t* p1 = points[pmin[paxis]];
	const real_t* p2 = points[pmax[paxis]];

	real_t center[3] = {(p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2, (p1[2] + p2[2]) / 2};
	real_t radius = sqrtf(paxisd2) / 2;

	// iteratively adjust the sphere up until all points fit
	for (size_t i = 0; i < count; ++i)
	{
		const real_t* p = points[i];
		real_t d2 = (p[0] - center[0]) * (p[0] - center[0]) + (p[1] - center[1]) * (p[1] - center[1]) + (p[2] - center[2]) * (p[2] - center[2]);

		if (d2 > radius * radius)
		{
			real_t d = sqrtf(d2);
			assert(d > 0);

			real_t k = 0.5f + (radius / d) / 2;

			center[0] = center[0] * k + p[0] * (1 - k);
			center[1] = center[1] * k + p[1] * (1 - k);
			center[2] = center[2] * k + p[2] * (1 - k);
			radius = (radius + d) / 2;
		}
	}

	result[0] = center[0];
	result[1] = center[1];
	result[2] = center[2];
	result[3] = radius;
}

struct Cone
{
	real_t px, py, pz;
	real_t nx, ny, nz;
};

static real_t getMeshletScore(real_t distance2, real_t spread, real_t cone_weight, real_t expected_radius)
{
	real_t cone = 1.f - spread * cone_weight;
	real_t cone_clamped = cone < 1e-3f ? 1e-3f : cone;

	return (1 + sqrtf(distance2) / expected_radius * (1 - cone_weight)) * cone_clamped;
}

static Cone getMeshletCone(const Cone& acc, datatype_t triangle_count)
{
	Cone result = acc;

	real_t center_scale = triangle_count == 0 ? 0.f : 1.f / real_t(triangle_count);

	result.px *= center_scale;
	result.py *= center_scale;
	result.pz *= center_scale;

	real_t axis_length = result.nx * result.nx + result.ny * result.ny + result.nz * result.nz;
	real_t axis_scale = axis_length == 0.f ? 0.f : 1.f / sqrtf(axis_length);

	result.nx *= axis_scale;
	result.ny *= axis_scale;
	result.nz *= axis_scale;

	return result;
}

static real_t computeTriangleCones(Cone* triangles, const datatype_t* indices, size_t index_count, const real_t* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	(void)vertex_count;

	size_t vertex_stride_float = vertex_positions_stride / sizeof(real_t);
	size_t face_count = index_count / 3;

	real_t mesh_area = 0;

	for (size_t i = 0; i < face_count; ++i)
	{
		datatype_t a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		const real_t* p0 = vertex_positions + vertex_stride_float * a;
		const real_t* p1 = vertex_positions + vertex_stride_float * b;
		const real_t* p2 = vertex_positions + vertex_stride_float * c;

		real_t p10[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
		real_t p20[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};

		real_t normalx = p10[1] * p20[2] - p10[2] * p20[1];
		real_t normaly = p10[2] * p20[0] - p10[0] * p20[2];
		real_t normalz = p10[0] * p20[1] - p10[1] * p20[0];

		real_t area = sqrtf(normalx * normalx + normaly * normaly + normalz * normalz);
		real_t invarea = (area == 0.f) ? 0.f : 1.f / area;

		triangles[i].px = (p0[0] + p1[0] + p2[0]) / 3.f;
		triangles[i].py = (p0[1] + p1[1] + p2[1]) / 3.f;
		triangles[i].pz = (p0[2] + p1[2] + p2[2]) / 3.f;

		triangles[i].nx = normalx * invarea;
		triangles[i].ny = normaly * invarea;
		triangles[i].nz = normalz * invarea;

		mesh_area += area;
	}

	return mesh_area;
}

static void finishMeshlet(meshopt_Meshlet& meshlet, unsigned char* meshlet_triangles)
{
	size_t offset = meshlet.triangle_offset + meshlet.triangle_count * 3;

	// fill 4b padding with 0
	while (offset & 3)
		meshlet_triangles[offset++] = 0;
}

static bool appendMeshlet(meshopt_Meshlet& meshlet, datatype_t a, datatype_t b, datatype_t c, unsigned char* used, meshopt_Meshlet* meshlets, datatype_t* meshlet_vertices, unsigned char* meshlet_triangles, size_t meshlet_offset, size_t max_vertices, size_t max_triangles)
{
	unsigned char& av = used[a];
	unsigned char& bv = used[b];
	unsigned char& cv = used[c];

	bool result = false;

	datatype_t used_extra = (av == 0xff) + (bv == 0xff) + (cv == 0xff);

	if (meshlet.vertex_count + used_extra > max_vertices || meshlet.triangle_count >= max_triangles)
	{
		meshlets[meshlet_offset] = meshlet;

		for (size_t j = 0; j < meshlet.vertex_count; ++j)
			used[meshlet_vertices[meshlet.vertex_offset + j]] = 0xff;

		finishMeshlet(meshlet, meshlet_triangles);

		meshlet.vertex_offset += meshlet.vertex_count;
		meshlet.triangle_offset += (meshlet.triangle_count * 3 + 3) & ~3; // 4b padding
		meshlet.vertex_count = 0;
		meshlet.triangle_count = 0;

		result = true;
	}

	if (av == 0xff)
	{
		av = (unsigned char)meshlet.vertex_count;
		meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = a;
	}

	if (bv == 0xff)
	{
		bv = (unsigned char)meshlet.vertex_count;
		meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = b;
	}

	if (cv == 0xff)
	{
		cv = (unsigned char)meshlet.vertex_count;
		meshlet_vertices[meshlet.vertex_offset + meshlet.vertex_count++] = c;
	}

	meshlet_triangles[meshlet.triangle_offset + meshlet.triangle_count * 3 + 0] = av;
	meshlet_triangles[meshlet.triangle_offset + meshlet.triangle_count * 3 + 1] = bv;
	meshlet_triangles[meshlet.triangle_offset + meshlet.triangle_count * 3 + 2] = cv;
	meshlet.triangle_count++;

	return result;
}

struct KDNode
{
	union
	{
		real_t split;
		datatype_t index;
	};

	// leaves: axis = 3, children = number of extra points after this one (0 if 'index' is the only point)
	// branches: axis != 3, left subtree = skip 1, right subtree = skip 1+children
	datatype_t axis : 2;
	datatype_t children : 30;
};

static size_t kdtreePartition(datatype_t* indices, size_t count, const real_t* points, size_t stride, datatype_t axis, real_t pivot)
{
	size_t m = 0;

	// invariant: elements in range [0, m) are < pivot, elements in range [m, i) are >= pivot
	for (size_t i = 0; i < count; ++i)
	{
		real_t v = points[indices[i] * stride + axis];

		// swap(m, i) unconditionally
		datatype_t t = indices[m];
		indices[m] = indices[i];
		indices[i] = t;

		// when v >= pivot, we swap i with m without advancing it, preserving invariants
		m += v < pivot;
	}

	return m;
}

static size_t kdtreeBuildLeaf(size_t offset, KDNode* nodes, size_t node_count, datatype_t* indices, size_t count)
{
	assert(offset + count <= node_count);
	(void)node_count;

	KDNode& result = nodes[offset];

	result.index = indices[0];
	result.axis = 3;
	result.children = count - 1;

	// all remaining points are stored in nodes immediately following the leaf
	for (size_t i = 1; i < count; ++i)
	{
		KDNode& tail = nodes[offset + i];

		tail.index = indices[i];
		tail.axis = 3;
		tail.children = ~0u >> 2; // bogus value to prevent misuse
	}

	return offset + count;
}

static size_t kdtreeBuild(size_t offset, KDNode* nodes, size_t node_count, const real_t* points, size_t stride, datatype_t* indices, size_t count, size_t leaf_size)
{
	assert(count > 0);
	assert(offset < node_count);

	if (count <= leaf_size)
		return kdtreeBuildLeaf(offset, nodes, node_count, indices, count);

	real_t mean[3] = {};
	real_t vars[3] = {};
	real_t runc = 1, runs = 1;

	// gather statistics on the points in the subtree using Welford's algorithm
	for (size_t i = 0; i < count; ++i, runc += 1.f, runs = 1.f / runc)
	{
		const real_t* point = points + indices[i] * stride;

		for (int k = 0; k < 3; ++k)
		{
			real_t delta = point[k] - mean[k];
			mean[k] += delta * runs;
			vars[k] += delta * (point[k] - mean[k]);
		}
	}

	// split axis is one where the variance is largest
	datatype_t axis = vars[0] >= vars[1] && vars[0] >= vars[2] ? 0 : vars[1] >= vars[2] ? 1 : 2;

	real_t split = mean[axis];
	size_t middle = kdtreePartition(indices, count, points, stride, axis, split);

	// when the partition is degenerate simply consolidate the points into a single node
	if (middle <= leaf_size / 2 || middle >= count - leaf_size / 2)
		return kdtreeBuildLeaf(offset, nodes, node_count, indices, count);

	KDNode& result = nodes[offset];

	result.split = split;
	result.axis = axis;

	// left subtree is right after our node
	size_t next_offset = kdtreeBuild(offset + 1, nodes, node_count, points, stride, indices, middle, leaf_size);

	// distance to the right subtree is represented explicitly
	result.children = next_offset - offset - 1;

	return kdtreeBuild(next_offset, nodes, node_count, points, stride, indices + middle, count - middle, leaf_size);
}

static void kdtreeNearest(KDNode* nodes, datatype_t root, const real_t* points, size_t stride, const unsigned char* emitted_flags, const real_t* position, datatype_t& result, real_t& limit)
{
	const KDNode& node = nodes[root];

	if (node.axis == 3)
	{
		// leaf
		for (datatype_t i = 0; i <= node.children; ++i)
		{
			datatype_t index = nodes[root + i].index;

			if (emitted_flags[index])
				continue;

			const real_t* point = points + index * stride;

			real_t distance2 =
			    (point[0] - position[0]) * (point[0] - position[0]) +
			    (point[1] - position[1]) * (point[1] - position[1]) +
			    (point[2] - position[2]) * (point[2] - position[2]);
			real_t distance = sqrtf(distance2);

			if (distance < limit)
			{
				result = index;
				limit = distance;
			}
		}
	}
	else
	{
		// branch; we order recursion to process the node that search position is in first
		real_t delta = position[node.axis] - node.split;
		datatype_t first = (delta <= 0) ? 0 : node.children;
		datatype_t second = first ^ node.children;

		kdtreeNearest(nodes, root + 1 + first, points, stride, emitted_flags, position, result, limit);

		// only process the other node if it can have a match based on closest distance so far
		if (fabsf(delta) <= limit)
			kdtreeNearest(nodes, root + 1 + second, points, stride, emitted_flags, position, result, limit);
	}
}

} // namespace meshopt

size_t meshopt_buildMeshletsBound(size_t index_count, size_t max_vertices, size_t max_triangles)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(max_vertices >= 3 && max_vertices <= kMeshletMaxVertices);
	assert(max_triangles >= 1 && max_triangles <= kMeshletMaxTriangles);
	assert(max_triangles % 4 == 0); // ensures the caller will compute output space properly as index data is 4b aligned

	(void)kMeshletMaxVertices;
	(void)kMeshletMaxTriangles;

	// meshlet construction is limited by max vertices and max triangles per meshlet
	// the worst case is that the input is an unindexed stream since this equally stresses both limits
	// note that we assume that in the worst case, we leave 2 vertices unpacked in each meshlet - if we have space for 3 we can pack any triangle
	size_t max_vertices_conservative = max_vertices - 2;
	size_t meshlet_limit_vertices = (index_count + max_vertices_conservative - 1) / max_vertices_conservative;
	size_t meshlet_limit_triangles = (index_count / 3 + max_triangles - 1) / max_triangles;

	return meshlet_limit_vertices > meshlet_limit_triangles ? meshlet_limit_vertices : meshlet_limit_triangles;
}

size_t meshopt_buildMeshlets(meshopt_Meshlet* meshlets, datatype_t* meshlet_vertices, unsigned char* meshlet_triangles, const datatype_t* indices, size_t index_count, const real_t* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, size_t max_vertices, size_t max_triangles, real_t cone_weight)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(real_t) == 0);

	assert(max_vertices >= 3 && max_vertices <= kMeshletMaxVertices);
	assert(max_triangles >= 1 && max_triangles <= kMeshletMaxTriangles);
	assert(max_triangles % 4 == 0); // ensures the caller will compute output space properly as index data is 4b aligned

	meshopt_Allocator allocator;

	TriangleAdjacency2 adjacency = {};
	buildTriangleAdjacency(adjacency, indices, index_count, vertex_count, allocator);

	datatype_t* live_triangles = allocator.allocate<datatype_t>(vertex_count);
	memcpy(live_triangles, adjacency.counts, vertex_count * sizeof(datatype_t));

	size_t face_count = index_count / 3;

	unsigned char* emitted_flags = allocator.allocate<unsigned char>(face_count);
	memset(emitted_flags, 0, face_count);

	// for each triangle, precompute centroid & normal to use for scoring
	Cone* triangles = allocator.allocate<Cone>(face_count);
	real_t mesh_area = computeTriangleCones(triangles, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride);

	// assuming each meshlet is a square patch, expected radius is sqrt(expected area)
	real_t triangle_area_avg = face_count == 0 ? 0.f : mesh_area / real_t(face_count) * 0.5f;
	real_t meshlet_expected_radius = sqrtf(triangle_area_avg * max_triangles) * 0.5f;

	// build a kd-tree for nearest neighbor lookup
	datatype_t* kdindices = allocator.allocate<datatype_t>(face_count);
	for (size_t i = 0; i < face_count; ++i)
		kdindices[i] = i;

	KDNode* nodes = allocator.allocate<KDNode>(face_count * 2);
	kdtreeBuild(0, nodes, face_count * 2, &triangles[0].px, sizeof(Cone) / sizeof(real_t), kdindices, face_count, /* leaf_size= */ 8);

	// index of the vertex in the meshlet, 0xff if the vertex isn't used
	unsigned char* used = allocator.allocate<unsigned char>(vertex_count);
	memset(used, -1, vertex_count);

	meshopt_Meshlet meshlet = {};
	size_t meshlet_offset = 0;

	Cone meshlet_cone_acc = {};

	for (;;)
	{
		datatype_t best_triangle = ~0u;
		datatype_t best_extra = 5;
		real_t best_score = std::numeric_limits<real_t>::max();

		Cone meshlet_cone = getMeshletCone(meshlet_cone_acc, meshlet.triangle_count);

		for (size_t i = 0; i < meshlet.vertex_count; ++i)
		{
			datatype_t index = meshlet_vertices[meshlet.vertex_offset + i];

			datatype_t* neighbours = &adjacency.data[0] + adjacency.offsets[index];
			size_t neighbours_size = adjacency.counts[index];

			for (size_t j = 0; j < neighbours_size; ++j)
			{
				datatype_t triangle = neighbours[j];
				assert(!emitted_flags[triangle]);

				datatype_t a = indices[triangle * 3 + 0], b = indices[triangle * 3 + 1], c = indices[triangle * 3 + 2];
				assert(a < vertex_count && b < vertex_count && c < vertex_count);

				datatype_t extra = (used[a] == 0xff) + (used[b] == 0xff) + (used[c] == 0xff);

				// triangles that don't add new vertices to meshlets are max. priority
				if (extra != 0)
				{
					// artificially increase the priority of dangling triangles as they're expensive to add to new meshlets
					if (live_triangles[a] == 1 || live_triangles[b] == 1 || live_triangles[c] == 1)
						extra = 0;

					extra++;
				}

				// since topology-based priority is always more important than the score, we can skip scoring in some cases
				if (extra > best_extra)
					continue;

				const Cone& tri_cone = triangles[triangle];

				real_t distance2 =
				    (tri_cone.px - meshlet_cone.px) * (tri_cone.px - meshlet_cone.px) +
				    (tri_cone.py - meshlet_cone.py) * (tri_cone.py - meshlet_cone.py) +
				    (tri_cone.pz - meshlet_cone.pz) * (tri_cone.pz - meshlet_cone.pz);

				real_t spread = tri_cone.nx * meshlet_cone.nx + tri_cone.ny * meshlet_cone.ny + tri_cone.nz * meshlet_cone.nz;

				real_t score = getMeshletScore(distance2, spread, cone_weight, meshlet_expected_radius);

				// note that topology-based priority is always more important than the score
				// this helps maintain reasonable effectiveness of meshlet data and reduces scoring cost
				if (extra < best_extra || score < best_score)
				{
					best_triangle = triangle;
					best_extra = extra;
					best_score = score;
				}
			}
		}

		if (best_triangle == ~0u)
		{
			real_t position[3] = {meshlet_cone.px, meshlet_cone.py, meshlet_cone.pz};
			datatype_t index = ~0u;
			real_t limit = std::numeric_limits<real_t>::max();

			kdtreeNearest(nodes, 0, &triangles[0].px, sizeof(Cone) / sizeof(real_t), emitted_flags, position, index, limit);

			best_triangle = index;
		}

		if (best_triangle == ~0u)
			break;

		datatype_t a = indices[best_triangle * 3 + 0], b = indices[best_triangle * 3 + 1], c = indices[best_triangle * 3 + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		// add meshlet to the output; when the current meshlet is full we reset the accumulated bounds
		if (appendMeshlet(meshlet, a, b, c, used, meshlets, meshlet_vertices, meshlet_triangles, meshlet_offset, max_vertices, max_triangles))
		{
			meshlet_offset++;
			memset(&meshlet_cone_acc, 0, sizeof(meshlet_cone_acc));
		}

		live_triangles[a]--;
		live_triangles[b]--;
		live_triangles[c]--;

		// remove emitted triangle from adjacency data
		// this makes sure that we spend less time traversing these lists on subsequent iterations
		for (size_t k = 0; k < 3; ++k)
		{
			datatype_t index = indices[best_triangle * 3 + k];

			datatype_t* neighbours = &adjacency.data[0] + adjacency.offsets[index];
			size_t neighbours_size = adjacency.counts[index];

			for (size_t i = 0; i < neighbours_size; ++i)
			{
				datatype_t tri = neighbours[i];

				if (tri == best_triangle)
				{
					neighbours[i] = neighbours[neighbours_size - 1];
					adjacency.counts[index]--;
					break;
				}
			}
		}

		// update aggregated meshlet cone data for scoring subsequent triangles
		meshlet_cone_acc.px += triangles[best_triangle].px;
		meshlet_cone_acc.py += triangles[best_triangle].py;
		meshlet_cone_acc.pz += triangles[best_triangle].pz;
		meshlet_cone_acc.nx += triangles[best_triangle].nx;
		meshlet_cone_acc.ny += triangles[best_triangle].ny;
		meshlet_cone_acc.nz += triangles[best_triangle].nz;

		emitted_flags[best_triangle] = 1;
	}

	if (meshlet.triangle_count)
	{
		finishMeshlet(meshlet, meshlet_triangles);

		meshlets[meshlet_offset++] = meshlet;
	}

	assert(meshlet_offset <= meshopt_buildMeshletsBound(index_count, max_vertices, max_triangles));
	return meshlet_offset;
}

size_t meshopt_buildMeshletsScan(meshopt_Meshlet* meshlets, datatype_t* meshlet_vertices, unsigned char* meshlet_triangles, const datatype_t* indices, size_t index_count, size_t vertex_count, size_t max_vertices, size_t max_triangles)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);

	assert(max_vertices >= 3 && max_vertices <= kMeshletMaxVertices);
	assert(max_triangles >= 1 && max_triangles <= kMeshletMaxTriangles);
	assert(max_triangles % 4 == 0); // ensures the caller will compute output space properly as index data is 4b aligned

	meshopt_Allocator allocator;

	// index of the vertex in the meshlet, 0xff if the vertex isn't used
	unsigned char* used = allocator.allocate<unsigned char>(vertex_count);
	memset(used, -1, vertex_count);

	meshopt_Meshlet meshlet = {};
	size_t meshlet_offset = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		datatype_t a = indices[i + 0];
		datatype_t b = indices[i + 1];
		datatype_t c = indices[i + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		// appends triangle to the meshlet and writes previous meshlet to the output if full
		meshlet_offset += appendMeshlet(meshlet, a, b, c, used, meshlets, meshlet_vertices, meshlet_triangles, meshlet_offset, max_vertices, max_triangles);
	}

	if (meshlet.triangle_count)
	{
		finishMeshlet(meshlet, meshlet_triangles);

		meshlets[meshlet_offset++] = meshlet;
	}

	assert(meshlet_offset <= meshopt_buildMeshletsBound(index_count, max_vertices, max_triangles));
	return meshlet_offset;
}

meshopt_Bounds meshopt_computeClusterBounds(const datatype_t* indices, size_t index_count, const real_t* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(index_count / 3 <= kMeshletMaxTriangles);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(real_t) == 0);

	(void)vertex_count;

	size_t vertex_stride_float = vertex_positions_stride / sizeof(real_t);

	// compute triangle normals and gather triangle corners
	real_t normals[kMeshletMaxTriangles][3];
	real_t corners[kMeshletMaxTriangles][3][3];
	size_t triangles = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		datatype_t a = indices[i + 0];
		datatype_t b = indices[i + 1];
		datatype_t c = indices[i + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		const real_t* p0 = vertex_positions + vertex_stride_float * a;
		const real_t* p1 = vertex_positions + vertex_stride_float * b;
		const real_t* p2 = vertex_positions + vertex_stride_float * c;

		real_t p10[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
		real_t p20[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};

		real_t normalx = p10[1] * p20[2] - p10[2] * p20[1];
		real_t normaly = p10[2] * p20[0] - p10[0] * p20[2];
		real_t normalz = p10[0] * p20[1] - p10[1] * p20[0];

		real_t area = sqrtf(normalx * normalx + normaly * normaly + normalz * normalz);

		// no need to include degenerate triangles - they will be invisible anyway
		if (area == 0.f)
			continue;

		// record triangle normals & corners for future use; normal and corner 0 define a plane equation
		normals[triangles][0] = normalx / area;
		normals[triangles][1] = normaly / area;
		normals[triangles][2] = normalz / area;
		memcpy(corners[triangles][0], p0, 3 * sizeof(real_t));
		memcpy(corners[triangles][1], p1, 3 * sizeof(real_t));
		memcpy(corners[triangles][2], p2, 3 * sizeof(real_t));
		triangles++;
	}

	meshopt_Bounds bounds = {};

	// degenerate cluster, no valid triangles => trivial reject (cone data is 0)
	if (triangles == 0)
		return bounds;

	// compute cluster bounding sphere; we'll use the center to determine normal cone apex as well
	real_t psphere[4] = {};
	computeBoundingSphere(psphere, corners[0], triangles * 3);

	real_t center[3] = {psphere[0], psphere[1], psphere[2]};

	// treating triangle normals as points, find the bounding sphere - the sphere center determines the optimal cone axis
	real_t nsphere[4] = {};
	computeBoundingSphere(nsphere, normals, triangles);

	real_t axis[3] = {nsphere[0], nsphere[1], nsphere[2]};
	real_t axislength = sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
	real_t invaxislength = axislength == 0.f ? 0.f : 1.f / axislength;

	axis[0] *= invaxislength;
	axis[1] *= invaxislength;
	axis[2] *= invaxislength;

	// compute a tight cone around all normals, mindp = cos(angle/2)
	real_t mindp = 1.f;

	for (size_t i = 0; i < triangles; ++i)
	{
		real_t dp = normals[i][0] * axis[0] + normals[i][1] * axis[1] + normals[i][2] * axis[2];

		mindp = (dp < mindp) ? dp : mindp;
	}

	// fill bounding sphere info; note that below we can return bounds without cone information for degenerate cones
	bounds.center[0] = center[0];
	bounds.center[1] = center[1];
	bounds.center[2] = center[2];
	bounds.radius = psphere[3];

	// degenerate cluster, normal cone is larger than a hemisphere => trivial accept
	// note that if mindp is positive but close to 0, the triangle intersection code below gets less stable
	// we arbitrarily decide that if a normal cone is ~168 degrees wide or more, the cone isn't useful
	if (mindp <= 0.1f)
	{
		bounds.cone_cutoff = 1;
		bounds.cone_cutoff_s8 = 127;
		return bounds;
	}

	real_t maxt = 0;

	// we need to find the point on center-t*axis ray that lies in negative half-space of all triangles
	for (size_t i = 0; i < triangles; ++i)
	{
		// dot(center-t*axis-corner, trinormal) = 0
		// dot(center-corner, trinormal) - t * dot(axis, trinormal) = 0
		real_t cx = center[0] - corners[i][0][0];
		real_t cy = center[1] - corners[i][0][1];
		real_t cz = center[2] - corners[i][0][2];

		real_t dc = cx * normals[i][0] + cy * normals[i][1] + cz * normals[i][2];
		real_t dn = axis[0] * normals[i][0] + axis[1] * normals[i][1] + axis[2] * normals[i][2];

		// dn should be larger than mindp cutoff above
		assert(dn > 0.f);
		real_t t = dc / dn;

		maxt = (t > maxt) ? t : maxt;
	}

	// cone apex should be in the negative half-space of all cluster triangles by construction
	bounds.cone_apex[0] = center[0] - axis[0] * maxt;
	bounds.cone_apex[1] = center[1] - axis[1] * maxt;
	bounds.cone_apex[2] = center[2] - axis[2] * maxt;

	// note: this axis is the axis of the normal cone, but our test for perspective camera effectively negates the axis
	bounds.cone_axis[0] = axis[0];
	bounds.cone_axis[1] = axis[1];
	bounds.cone_axis[2] = axis[2];

	// cos(a) for normal cone is mindp; we need to add 90 degrees on both sides and invert the cone
	// which gives us -cos(a+90) = -(-sin(a)) = sin(a) = sqrt(1 - cos^2(a))
	bounds.cone_cutoff = sqrtf(1 - mindp * mindp);

	// quantize axis & cutoff to 8-bit SNORM format
	bounds.cone_axis_s8[0] = (signed char)(meshopt_quantizeSnorm(bounds.cone_axis[0], 8));
	bounds.cone_axis_s8[1] = (signed char)(meshopt_quantizeSnorm(bounds.cone_axis[1], 8));
	bounds.cone_axis_s8[2] = (signed char)(meshopt_quantizeSnorm(bounds.cone_axis[2], 8));

	// for the 8-bit test to be conservative, we need to adjust the cutoff by measuring the max. error
	real_t cone_axis_s8_e0 = fabsf(bounds.cone_axis_s8[0] / 127.f - bounds.cone_axis[0]);
	real_t cone_axis_s8_e1 = fabsf(bounds.cone_axis_s8[1] / 127.f - bounds.cone_axis[1]);
	real_t cone_axis_s8_e2 = fabsf(bounds.cone_axis_s8[2] / 127.f - bounds.cone_axis[2]);

	// note that we need to round this up instead of rounding to nearest, hence +1
	int cone_cutoff_s8 = int(127 * (bounds.cone_cutoff + cone_axis_s8_e0 + cone_axis_s8_e1 + cone_axis_s8_e2) + 1);

	bounds.cone_cutoff_s8 = (cone_cutoff_s8 > 127) ? 127 : (signed char)(cone_cutoff_s8);

	return bounds;
}

meshopt_Bounds meshopt_computeMeshletBounds(const datatype_t* meshlet_vertices, const unsigned char* meshlet_triangles, size_t triangle_count, const real_t* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(triangle_count <= kMeshletMaxTriangles);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(real_t) == 0);

	datatype_t indices[kMeshletMaxTriangles * 3];

	for (size_t i = 0; i < triangle_count * 3; ++i)
	{
		datatype_t index = meshlet_vertices[meshlet_triangles[i]];
		assert(index < vertex_count);

		indices[i] = index;
	}

	return meshopt_computeClusterBounds(indices, triangle_count * 3, vertex_positions, vertex_count, vertex_positions_stride);
}
