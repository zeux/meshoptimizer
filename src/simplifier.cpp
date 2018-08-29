// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

#ifndef TRACE
#define TRACE 0
#endif

#if TRACE
#include <stdio.h>
#endif

// This work is based on:
// Michael Garland and Paul S. Heckbert. Surface simplification using quadric error metrics. 1997
// Michael Garland. Quadric-based polygonal surface simplification. 1999
namespace meshopt
{

struct EdgeAdjacency
{
	meshopt_Buffer<unsigned int> counts;
	meshopt_Buffer<unsigned int> offsets;
	meshopt_Buffer<unsigned int> data;

	EdgeAdjacency(size_t index_count, size_t vertex_count)
	    : counts(vertex_count)
	    , offsets(vertex_count)
	    , data(index_count)
	{
	}
};

static void buildEdgeAdjacency(EdgeAdjacency& adjacency, const unsigned int* indices, size_t index_count, size_t vertex_count)
{
	size_t face_count = index_count / 3;

	// fill edge counts
	memset(adjacency.counts.data, 0, vertex_count * sizeof(unsigned int));

	for (size_t i = 0; i < index_count; ++i)
	{
		assert(indices[i] < vertex_count);

		adjacency.counts[indices[i]]++;
	}

	// fill offset table
	unsigned int offset = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		adjacency.offsets[i] = offset;
		offset += adjacency.counts[i];
	}

	assert(offset == index_count);

	// fill edge data
	for (size_t i = 0; i < face_count; ++i)
	{
		unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

		adjacency.data[adjacency.offsets[a]++] = b;
		adjacency.data[adjacency.offsets[b]++] = c;
		adjacency.data[adjacency.offsets[c]++] = a;
	}

	// fix offsets that have been disturbed by the previous pass
	for (size_t i = 0; i < vertex_count; ++i)
	{
		assert(adjacency.offsets[i] >= adjacency.counts[i]);

		adjacency.offsets[i] -= adjacency.counts[i];
	}
}

static bool findEdge(const EdgeAdjacency& adjacency, unsigned int a, unsigned int b)
{
	unsigned int count = adjacency.counts[a];
	const unsigned int* data = adjacency.data.data + adjacency.offsets[a];

	for (size_t i = 0; i < count; ++i)
		if (data[i] == b)
			return true;

	return false;
}

struct PositionHasher
{
	const float* vertex_positions;
	size_t vertex_stride_float;

	size_t hash(unsigned int index) const
	{
		// MurmurHash2
		const unsigned int m = 0x5bd1e995;
		const int r = 24;

		unsigned int h = 0;
		const unsigned int* key = reinterpret_cast<const unsigned int*>(vertex_positions + index * vertex_stride_float);

		for (size_t i = 0; i < 3; ++i)
		{
			unsigned int k = key[i];

			k *= m;
			k ^= k >> r;
			k *= m;

			h *= m;
			h ^= k;
		}

		return h;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		return memcmp(vertex_positions + lhs * vertex_stride_float, vertex_positions + rhs * vertex_stride_float, sizeof(float) * 3) == 0;
	}
};

// TODO: is there a better way to resolve naming conflicts with indexgenerator.cpp?
static size_t hashBuckets2(size_t count)
{
	size_t buckets = 1;
	while (buckets < count)
		buckets *= 2;

	return buckets;
}

// TODO: is there a better way to resolve naming conflicts with indexgenerator.cpp?
template <typename T, typename Hash>
static T* hashLookup2(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty)
{
	assert(buckets > 0);
	assert((buckets & (buckets - 1)) == 0);

	size_t hashmod = buckets - 1;
	size_t bucket = hash.hash(key) & hashmod;

	for (size_t probe = 0; probe <= hashmod; ++probe)
	{
		T& item = table[bucket];

		if (item == empty)
			return &item;

		if (hash.equal(item, key))
			return &item;

		// hash collision, quadratic probing
		bucket = (bucket + probe + 1) & hashmod;
	}

	assert(false && "Hash table is full");
	return 0;
}

static void buildPositionRemap(unsigned int* remap, unsigned int* reverse_remap, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride)
{
	PositionHasher hasher = {vertex_positions_data, vertex_positions_stride / sizeof(float)};

	size_t table_size = hashBuckets2(vertex_count);
	meshopt_Buffer<unsigned int> table(table_size);
	memset(table.data, -1, table_size * sizeof(unsigned int));

	// build forward remap: for each vertex, which other (canonical) vertex does it map to?
	// we use position equivalence for this, and remap vertices to other existing vertices
	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int index = unsigned(i);
		unsigned int* entry = hashLookup2(table.data, table_size, hasher, index, ~0u);

		if (*entry == ~0u)
		{
			*entry = index;

			remap[index] = index;
		}
		else
		{
			assert(remap[*entry] != ~0u);

			remap[index] = remap[*entry];
		}
	}

	// build reverse remap table: for each vertex, which other vertex remaps to this one?
	// this is a many-to-one relationship, but we only identify vertices for which it's 2-1 (so a pair of vertices)
	for (size_t i = 0; i < vertex_count; ++i)
		reverse_remap[i] = unsigned(i);

	for (size_t i = 0; i < vertex_count; ++i)
		if (remap[i] != i)
		{
			// first vertex to remap to remap[i]: keep
			if (reverse_remap[remap[i]] == remap[i])
				reverse_remap[remap[i]] = unsigned(i);
			// more than one vertex, invalidate entry
			else
				reverse_remap[remap[i]] = ~0u;
		}
}

enum VertexKind
{
	Kind_Manifold, // not on an attribute seam, not on any boundary
	Kind_Border,   // not on an attribute seam, has exactly two open edges
	Kind_Seam,     // on an attribute seam with exactly two attribute seam edges
	Kind_Locked,   // none of the above; these vertices can't move

	Kind_Count
};

// manifold vertices can collapse on anything except locked
// border/seam vertices can only be collapsed onto border/seam respectively
// TODO: seam->seam collapses don't make sure the collapse is along a seam edge
const char kCanCollapse[Kind_Count][Kind_Count] = {
    {1, 1, 1, 1},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 0},
};

static size_t countOpenEdges(const EdgeAdjacency& adjacency, unsigned int vertex, unsigned int* last = 0)
{
	size_t result = 0;

	unsigned int count = adjacency.counts[vertex];
	const unsigned int* data = adjacency.data.data + adjacency.offsets[vertex];

	for (size_t i = 0; i < count; ++i)
		if (!findEdge(adjacency, data[i], vertex))
		{
			result++;

			if (last)
				*last = data[i];
		}

	return result;
}

static void classifyVertices(unsigned char* result, size_t vertex_count, const EdgeAdjacency& adjacency, const unsigned int* remap, const unsigned int* reverse_remap, size_t lockedstats[4])
{
	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (remap[i] == i)
		{
			if (reverse_remap[i] == i)
			{
				// no attribute seam, need to check if it's manifold
				size_t edges = countOpenEdges(adjacency, unsigned(i));

				// note: we classify any vertices with no open edges as manifold
				// this is technically incorrect - if 4 triangles share an edge, we'll classify vertices as manifold
				// it's unclear if this is a problem in practice
				// also note that we classify vertices as border if they have *one* open edge, not two
				// this is because we only have half-edges - so a border vertex would have one incoming and one outgoing edge
				if (edges == 0)
					result[i] = Kind_Manifold;
				else if (edges == 1)
					result[i] = Kind_Border;
				else
					result[i] = Kind_Locked, lockedstats[0]++;
			}
			else if (reverse_remap[i] != ~0u)
			{
				// attribute seam; need to distinguish between Seam and Locked
				unsigned int a = 0;
				size_t a_count = countOpenEdges(adjacency, unsigned(i), &a);
				unsigned int b = 0;
				size_t b_count = countOpenEdges(adjacency, reverse_remap[i], &b);

				// seam should have one open half-edge for each vertex, and the edges need to "connect" - point to the same vertex post-remap
				if (a_count == 1 && b_count == 1)
				{
					// "flip" a & b between one side of the seam and the other
					unsigned int af = (remap[a] == a) ? reverse_remap[a] : remap[a];
					unsigned int bf = (remap[b] == b) ? reverse_remap[b] : remap[b];

					if (af != ~0u && findEdge(adjacency, af, reverse_remap[i]) &&
					    bf != ~0u && findEdge(adjacency, bf, unsigned(i)))
						result[i] = Kind_Seam;
					else
						result[i] = Kind_Locked, lockedstats[1]++;
				}
				else
				{
					result[i] = Kind_Locked, lockedstats[2]++;
				}
			}
			else
			{
				// more than one vertex maps to this one; we don't have classification available
				result[i] = Kind_Locked, lockedstats[3]++;
			}
		}
		else
		{
			assert(remap[i] < i);

			result[i] = result[remap[i]];
		}
	}
}

struct Vector3
{
	float x, y, z;
};

static void rescalePositions(Vector3* result, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride)
{
	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

	for (size_t i = 0; i < vertex_count; ++i)
	{
		const float* v = vertex_positions_data + i * vertex_stride_float;

		result[i].x = v[0];
		result[i].y = v[1];
		result[i].z = v[2];

		for (int j = 0; j < 3; ++j)
		{
			float vj = v[j];

			minv[j] = minv[j] > vj ? vj : minv[j];
			maxv[j] = maxv[j] < vj ? vj : maxv[j];
		}
	}

	float extent = 0.f;

	extent = (maxv[0] - minv[0]) < extent ? extent : (maxv[0] - minv[0]);
	extent = (maxv[1] - minv[1]) < extent ? extent : (maxv[1] - minv[1]);
	extent = (maxv[2] - minv[2]) < extent ? extent : (maxv[2] - minv[2]);

	float scale = extent == 0 ? 0.f : 1.f / extent;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		result[i].x = (result[i].x - minv[0]) * scale;
		result[i].y = (result[i].y - minv[1]) * scale;
		result[i].z = (result[i].z - minv[2]) * scale;
	}
}

struct Quadric
{
	float a00;
	float a10, a11;
	float a20, a21, a22;
	float b0, b1, b2, c;
};

struct Collapse
{
	unsigned int v0;
	unsigned int v1;

	union {
		float error;
		unsigned int errorui;
	};
};

static float normalize(Vector3& v)
{
	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

	if (length > 0)
	{
		v.x /= length;
		v.y /= length;
		v.z /= length;
	}

	return length;
}

static void quadricAdd(Quadric& Q, const Quadric& R)
{
	Q.a00 += R.a00;
	Q.a10 += R.a10;
	Q.a11 += R.a11;
	Q.a20 += R.a20;
	Q.a21 += R.a21;
	Q.a22 += R.a22;
	Q.b0 += R.b0;
	Q.b1 += R.b1;
	Q.b2 += R.b2;
	Q.c += R.c;
}

static void quadricMul(Quadric& Q, float s)
{
	Q.a00 *= s;
	Q.a10 *= s;
	Q.a11 *= s;
	Q.a20 *= s;
	Q.a21 *= s;
	Q.a22 *= s;
	Q.b0 *= s;
	Q.b1 *= s;
	Q.b2 *= s;
	Q.c *= s;
}

static float quadricError(Quadric& Q, const Vector3& v)
{
	float xx = v.x * v.x;
	float xy = v.x * v.y;
	float xz = v.x * v.z;
	float yy = v.y * v.y;
	float yz = v.y * v.z;
	float zz = v.z * v.z;

	float vTQv = Q.a00 * xx + Q.a10 * xy * 2 + Q.a11 * yy + Q.a20 * xz * 2 + Q.a21 * yz * 2 + Q.a22 * zz + Q.b0 * v.x * 2 + Q.b1 * v.y * 2 + Q.b2 * v.z * 2 + Q.c;

	return fabsf(vTQv);
}

static void quadricFromPlane(Quadric& Q, float a, float b, float c, float d)
{
	Q.a00 = a * a;
	Q.a10 = b * a;
	Q.a11 = b * b;
	Q.a20 = c * a;
	Q.a21 = c * b;
	Q.a22 = c * c;
	Q.b0 = d * a;
	Q.b1 = d * b;
	Q.b2 = d * c;
	Q.c = d * d;
}

static void quadricFromTriangle(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

	Vector3 normal = {p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x};
	float area = normalize(normal);

	float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

	quadricFromPlane(Q, normal.x, normal.y, normal.z, -distance);

	quadricMul(Q, area);
}

static void quadricFromTriangleEdge(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2, float weight)
{
	Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	float length = normalize(p10);

	Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};
	float p20p = p20.x * p10.x + p20.y * p10.y + p20.z * p10.z;

	Vector3 normal = {p20.x - p10.x * p20p, p20.y - p10.y * p20p, p20.z - p10.z * p20p};
	normalize(normal);

	float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

	quadricFromPlane(Q, normal.x, normal.y, normal.z, -distance);

	quadricMul(Q, length * length * weight);
}

static void sortEdgeCollapses(unsigned int* sort_order, const Collapse* collapses, size_t collapse_count)
{
	const int sort_bits = 11;

	// fill histogram for counting sort
	unsigned int histogram[1 << sort_bits];
	memset(histogram, 0, sizeof(histogram));

	for (size_t i = 0; i < collapse_count; ++i)
	{
		// skip sign bit since error is non-negative
		unsigned int key = (collapses[i].errorui << 1) >> (32 - sort_bits);

		histogram[key]++;
	}

	// compute offsets based on histogram data
	size_t histogram_sum = 0;

	for (size_t i = 0; i < 1 << sort_bits; ++i)
	{
		size_t count = histogram[i];
		histogram[i] = unsigned(histogram_sum);
		histogram_sum += count;
	}

	assert(histogram_sum == collapse_count);

	// compute sort order based on offsets
	for (size_t i = 0; i < collapse_count; ++i)
	{
		unsigned int key = (collapses[i].errorui << 1) >> (32 - sort_bits);

		sort_order[histogram[key]++] = unsigned(i);
	}
}

} // namespace meshopt

// TODO: this is necessary for lodviewer but will go away at some point
unsigned char* meshopt_simplifyDebugKind = 0;

size_t meshopt_simplify(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, size_t target_index_count, float target_error)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride > 0 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(target_index_count <= index_count);

	unsigned int* result = destination;

	// build adjacency information
	EdgeAdjacency adjacency(index_count, vertex_count);
	buildEdgeAdjacency(adjacency, indices, index_count, vertex_count);

	// build position remap that maps each vertex to the one with identical position
	meshopt_Buffer<unsigned int> remap(vertex_count);
	meshopt_Buffer<unsigned int> reverse_remap(vertex_count);
	buildPositionRemap(remap.data, reverse_remap.data, vertex_positions_data, vertex_count, vertex_positions_stride);

	// TODO: maybe make this an option? this disables seam awareness
	// for (size_t i = 0; i < vertex_count; ++i) remap[i] = reverse_remap[i] = unsigned(i);

	size_t lockedstats[4] = {};

	// classify vertices; vertex kind determines collapse rules, see kCanCollapse
	meshopt_Buffer<unsigned char> vertex_kind(vertex_count);
	classifyVertices(vertex_kind.data, vertex_count, adjacency, remap.data, reverse_remap.data, lockedstats);

	if (meshopt_simplifyDebugKind)
		memcpy(meshopt_simplifyDebugKind, vertex_kind.data, vertex_count);

#if TRACE
	size_t unique_positions = 0;
	for (size_t i = 0; i < vertex_count; ++i)
		unique_positions += remap[i] == i;

	printf("position remap: %d vertices => %d positions\n", int(vertex_count), int(unique_positions));

	size_t kinds[Kind_Count] = {};
	for (size_t i = 0; i < vertex_count; ++i)
		kinds[vertex_kind[i]] += remap[i] == i;

	printf("kinds: manifold %d, border %d, seam %d, locked %d\n",
	       int(kinds[Kind_Manifold]), int(kinds[Kind_Border]), int(kinds[Kind_Seam]), int(kinds[Kind_Locked]));
	printf("locked: 0 %d, 1 %d, 2 %d, 3 %d\n",
	       int(lockedstats[0]), int(lockedstats[1]), int(lockedstats[2]), int(lockedstats[3]));
#endif

	meshopt_Buffer<Vector3> vertex_positions(vertex_count);
	rescalePositions(vertex_positions.data, vertex_positions_data, vertex_count, vertex_positions_stride);

	meshopt_Buffer<Quadric> vertex_quadrics(vertex_count);
	memset(vertex_quadrics.data, 0, vertex_count * sizeof(Quadric));

	// face quadrics
	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int i0 = indices[i + 0];
		unsigned int i1 = indices[i + 1];
		unsigned int i2 = indices[i + 2];

		// TODO: degenerate triangles can produce degenerate quadrics
		// assert(i0 != i1 && i0 != i2 && i1 != i2);

		Quadric Q;
		quadricFromTriangle(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2]);

		quadricAdd(vertex_quadrics[remap[i0]], Q);
		quadricAdd(vertex_quadrics[remap[i1]], Q);
		quadricAdd(vertex_quadrics[remap[i2]], Q);
	}

	// edge quadrics for boundary edges
	size_t boundary = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		static const int next[3] = {1, 2, 0};

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];

			if (vertex_kind[i0] != Kind_Border && vertex_kind[i0] != Kind_Seam)
				continue;

			if (findEdge(adjacency, i1, i0))
				continue;

			unsigned int i2 = indices[i + next[next[e]]];

			// TODO: degenerate triangles can produce degenerate quadrics
			// assert(i0 != i1 && i0 != i2 && i1 != i2);

			float edgeWeight = vertex_kind[i0] == Kind_Seam ? 1.f : 10.f;

			Quadric Q;
			quadricFromTriangleEdge(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], edgeWeight);

			quadricAdd(vertex_quadrics[remap[i0]], Q);
			quadricAdd(vertex_quadrics[remap[i1]], Q);

			boundary++;
		}
	}

#if TRACE
	printf("vertices: %d, half-edges: %d, boundary: %d\n", int(vertex_count), int(index_count), int(boundary));
#endif

	if (result != indices)
	{
		memcpy(result, indices, index_count * sizeof(unsigned int));
	}

	size_t pass_count = 0;
	float worst_error = 0;

	meshopt_Buffer<Collapse> edge_collapses(index_count);
	meshopt_Buffer<unsigned int> collapse_order(index_count);
	meshopt_Buffer<unsigned int> collapse_remap(vertex_count);
	meshopt_Buffer<char> collapse_locked(vertex_count);

	while (index_count > target_index_count)
	{
		size_t edge_collapse_count = 0;

		for (size_t i = 0; i < index_count; i += 3)
		{
			static const int next[3] = {1, 2, 0};

			for (int e = 0; e < 3; ++e)
			{
				unsigned int i0 = result[i + e];
				unsigned int i1 = result[i + next[e]];

				// TODO: i0==i1 is an interesting condition... it seems like it can arise when collapsing a seam loop
				if (remap[i0] == remap[i1])
					continue;

				if (!kCanCollapse[vertex_kind[i0]][vertex_kind[i1]])
					continue;

				// TODO: we currently push the same collapse twice for manifold edges
				// TODO: needs cleanup in general...
				if (vertex_kind[i0] == vertex_kind[i1])
				{
					// TODO: Garland97 uses Q1+Q2 for error estimation; here we asssume that V1 produces zero error for Q1 but it actually gets larger with more collapses
					Collapse c01 = {i0, i1, {quadricError(vertex_quadrics[remap[i0]], vertex_positions[i1])}};
					Collapse c10 = {i1, i0, {quadricError(vertex_quadrics[remap[i1]], vertex_positions[i0])}};
					Collapse c = c01.error <= c10.error ? c01 : c10;
					assert(c.error >= 0);

					edge_collapses[edge_collapse_count++] = c;
				}
				else
				{
					Collapse c = {i0, i1, {quadricError(vertex_quadrics[remap[i0]], vertex_positions[i1])}};
					assert(c.error >= 0);

					edge_collapses[edge_collapse_count++] = c;
				}
			}
		}

		// no edges can be collapsed any more due to topology restrictions => bail out
		if (edge_collapse_count == 0)
			break;

		sortEdgeCollapses(collapse_order.data, edge_collapses.data, edge_collapse_count);

		for (size_t i = 0; i < vertex_count; ++i)
		{
			collapse_remap[i] = unsigned(i);
		}

		memset(collapse_locked.data, 0, vertex_count);

		// each collapse removes 2 triangles
		size_t edge_collapse_goal = (index_count - target_index_count) / 6 + 1;

		size_t collapses = 0;
		float pass_error = 0;

		float error_goal = edge_collapse_goal < edge_collapse_count ? edge_collapses[collapse_order[edge_collapse_goal]].error * 1.5f : FLT_MAX;
		float error_limit = error_goal > target_error ? target_error : error_goal;

		// no edges can be collapsed any more due to hitting the error limit => bail out
		if (edge_collapses[collapse_order[0]].error > error_limit)
			break;

		for (size_t i = 0; i < edge_collapse_count; ++i)
		{
			const Collapse& c = edge_collapses[collapse_order[i]];

			unsigned int r0 = remap[c.v0];
			unsigned int r1 = remap[c.v1];

			if (collapse_locked[r0] || collapse_locked[r1])
				continue;

			if (c.error > error_limit)
				break;

#if TRACE > 1
			printf("collapse: %d (kind %d r %d rr %d) => %d (kind %d r %d rr %d); error %e\n",
			       c.v0, vertex_kind[c.v0], remap[c.v0], reverse_remap[c.v0],
			       c.v1, vertex_kind[c.v1], remap[c.v1], reverse_remap[c.v1],
			       c.error);
#endif

			assert(collapse_remap[r0] == r0);
			assert(collapse_remap[r1] == r1);

			quadricAdd(vertex_quadrics[r1], vertex_quadrics[r0]);

			if (vertex_kind[c.v0] == Kind_Seam)
			{
				// remap v0 to v1 and seam pair of v0 to seam pair of v1
				unsigned int s0 = c.v0 == r0 ? reverse_remap[r0] : r0;
				unsigned int s1 = c.v1 == r1 ? reverse_remap[r1] : r1;

				assert(s0 != ~0u && s0 != c.v0);
				assert(s1 != ~0u && s1 != c.v1);

				collapse_remap[c.v0] = c.v1;
				collapse_remap[s0] = s1;
			}
			else
			{
				assert(c.v0 == r0 && reverse_remap[r0] == r0);

				collapse_remap[c.v0] = c.v1;
			}

			collapse_locked[r0] = 1;
			collapse_locked[r1] = 1;

			collapses++;
			pass_error = c.error;

			if (collapses >= edge_collapse_goal)
				break;
		}

#if TRACE
		printf("pass %d: triangles: %d, collapses: %d/%d (target %d), error: %e\n", int(pass_count), int(index_count / 3), int(collapses), int(edge_collapse_count), int(edge_collapse_goal), pass_error);
#endif

		pass_count++;
		worst_error = (worst_error < pass_error) ? pass_error : worst_error;

		// no edges can be collapsed any more => bail out
		if (collapses == 0)
			break;

		size_t write = 0;

		for (size_t i = 0; i < index_count; i += 3)
		{
			unsigned int v0 = collapse_remap[result[i + 0]];
			unsigned int v1 = collapse_remap[result[i + 1]];
			unsigned int v2 = collapse_remap[result[i + 2]];

			assert(collapse_remap[v0] == v0);
			assert(collapse_remap[v1] == v1);
			assert(collapse_remap[v2] == v2);

			if (v0 != v1 && v0 != v2 && v1 != v2)
			{
				result[write + 0] = v0;
				result[write + 1] = v1;
				result[write + 2] = v2;
				write += 3;
			}
		}

		assert(write < index_count);
		index_count = write;
	}

#if TRACE
	printf("passes: %d, worst error: %e\n", int(pass_count), worst_error);

	size_t locked_collapses[Kind_Count][Kind_Count] = {};

	for (size_t i = 0; i < index_count; i += 3)
	{
		static const int next[3] = {1, 2, 0};

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = result[i + e];
			unsigned int i1 = result[i + next[e]];

			unsigned char k0 = vertex_kind[i0];
			unsigned char k1 = vertex_kind[i1];

			locked_collapses[k0][k1] += !kCanCollapse[k0][k1];
		}
	}

	for (int k0 = 0; k0 < Kind_Count; ++k0)
		for (int k1 = 0; k1 < Kind_Count; ++k1)
			if (locked_collapses[k0][k1])
				printf("locked collapses %d -> %d: %d\n", k0, k1, int(locked_collapses[k0][k1]));
#endif

	return index_count;
}
