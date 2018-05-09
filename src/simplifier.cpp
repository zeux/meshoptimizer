// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#define TRACE 0

#if TRACE
#include <stdio.h>
#endif

// This work is based on:
// Michael Garland and Paul S. Heckbert. Surface simplification using quadric error metrics. 1997
namespace meshopt
{

static size_t hash(unsigned long long key)
{
	key = (~key) + (key << 18);
	key = key ^ (key >> 31);
	key = key * 21;
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);
	return size_t(key);
}

// TODO: figure out naming conflicts between indexgenerator.cpp and this file
static size_t hashBuckets2(size_t count)
{
	size_t buckets = 1;
	while (buckets < count)
		buckets *= 2;

	return buckets;
}

template <typename T>
static T* hashLookup(T* table, size_t buckets, const T& key, const T& empty)
{
	assert(buckets > 0);
	assert((buckets & (buckets - 1)) == 0);

	size_t hashmod = buckets - 1;
	size_t bucket = hash(key) & hashmod;

	for (size_t probe = 0; probe <= hashmod; ++probe)
	{
		T& item = table[bucket];

		if (item == empty || item == key)
			return &item;

		// hash collision, quadratic probing
		bucket = (bucket + probe + 1) & hashmod;
	}

	assert(false && "Hash table is full");
	return 0;
}

struct Vector3
{
	float x, y, z;
};

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

	// Three classical weighting methods include weight=1, weight=area and weight=area^2
	// We use weight=area for now
	quadricMul(Q, area);
}

static void quadricFromTriangleEdge(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	float length = normalize(p10);

	Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};
	float p20p = p20.x * p10.x + p20.y * p10.y + p20.z * p10.z;

	Vector3 normal = {p20.x - p10.x * p20p, p20.y - p10.y * p20p, p20.z - p10.z * p20p};
	normalize(normal);

	float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

	quadricFromPlane(Q, normal.x, normal.y, normal.z, -distance);

	quadricMul(Q, length * 1000);
}

static unsigned long long edgeId(unsigned int a, unsigned int b)
{
	return (static_cast<unsigned long long>(a) << 32) | b;
}

static void sortEdgeCollapses(unsigned int* sort_order, const Collapse* collapses, size_t collapse_count)
{
	const int sort_bits = 11;

	// fill histogram for counting sort
	unsigned int histogram[1 << sort_bits];
	memset(histogram, 0, sizeof(histogram));

	for (size_t i = 0; i < collapse_count; ++i)
	{
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

static size_t simplifyEdgeCollapse(unsigned int* result, const unsigned int* indices, size_t index_count, const float* vertex_positions_data, size_t vertex_positions_stride, size_t vertex_count, size_t target_index_count)
{
	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	meshopt_Buffer<Vector3> vertex_positions(vertex_count);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		const float* v = vertex_positions_data + i * vertex_stride_float;

		vertex_positions[i].x = v[0];
		vertex_positions[i].y = v[1];
		vertex_positions[i].z = v[2];
	}

	meshopt_Buffer<Quadric> vertex_quadrics(vertex_count);
	memset(vertex_quadrics.data, 0, vertex_count * sizeof(Quadric));

	// face quadrics
	for (size_t i = 0; i < index_count; i += 3)
	{
		Quadric Q;
		quadricFromTriangle(Q, vertex_positions[indices[i + 0]], vertex_positions[indices[i + 1]], vertex_positions[indices[i + 2]]);

		quadricAdd(vertex_quadrics[indices[i + 0]], Q);
		quadricAdd(vertex_quadrics[indices[i + 1]], Q);
		quadricAdd(vertex_quadrics[indices[i + 2]], Q);
	}

	// edge quadrics for boundary edges
	meshopt_Buffer<unsigned long long> edges(hashBuckets2(index_count));
	memset(edges.data, 0, edges.size * sizeof(unsigned long long));

	for (size_t i = 0; i < index_count; i += 3)
	{
		static const int next[3] = {1, 2, 0};

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];

			unsigned long long edge = edgeId(i0, i1);

			*hashLookup(edges.data, edges.size, edge, 0ull) = edge;
		}
	}

	for (size_t i = 0; i < index_count; i += 3)
	{
		static const int next[3] = {1, 2, 0};

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];

			unsigned long long edge = edgeId(i1, i0);

			if (*hashLookup(edges.data, edges.size, edge, 0ull) != edge)
			{
				unsigned int i2 = indices[i + next[next[e]]];

				Quadric Q;
				quadricFromTriangleEdge(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2]);

				quadricAdd(vertex_quadrics[i0], Q);
				quadricAdd(vertex_quadrics[i1], Q);
			}
		}
	}

	if (result != indices)
	{
		for (size_t i = 0; i < index_count; ++i)
		{
			result[i] = indices[i];
		}
	}

	size_t pass_count = 0;
	float worst_error = 0;

	meshopt_Buffer<Collapse> edge_collapses(index_count);
	meshopt_Buffer<unsigned int> collapse_order(index_count);
	meshopt_Buffer<unsigned int> vertex_remap(vertex_count);
	meshopt_Buffer<char> vertex_locked(vertex_count);

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

				Collapse c01 = {i0, i1, {quadricError(vertex_quadrics[i0], vertex_positions[i1])}};
				Collapse c10 = {i1, i0, {quadricError(vertex_quadrics[i1], vertex_positions[i0])}};
				Collapse c = c01.error <= c10.error ? c01 : c10;
				assert(c.error >= 0);

				edge_collapses[edge_collapse_count++] = c;
			}
		}

		sortEdgeCollapses(collapse_order.data, edge_collapses.data, edge_collapse_count);

		for (size_t i = 0; i < vertex_count; ++i)
		{
			vertex_remap[i] = unsigned(i);
		}

		memset(vertex_locked.data, 0, vertex_count);

		// each collapse removes 2 triangles
		size_t edge_collapse_goal = (index_count - target_index_count) / 6 + 1;

		size_t collapses = 0;
		float pass_error = 0;

		float error_goal = edge_collapses[collapse_order[edge_collapse_goal < edge_collapse_count ? edge_collapse_goal : edge_collapse_count - 1]].error;
		float error_limit = error_goal * 1.5f;

		for (size_t i = 0; i < edge_collapse_count; ++i)
		{
			const Collapse& c = edge_collapses[collapse_order[i]];

			if (vertex_locked[c.v0] || vertex_locked[c.v1])
				continue;

			if (c.error > error_limit)
				break;

			assert(vertex_remap[c.v0] == c.v0);
			assert(vertex_remap[c.v1] == c.v1);

			quadricAdd(vertex_quadrics[c.v1], vertex_quadrics[c.v0]);

			vertex_remap[c.v0] = unsigned(c.v1);

			vertex_locked[c.v0] = 1;
			vertex_locked[c.v1] = 1;

			collapses++;
			pass_error = c.error;

			if (collapses >= edge_collapse_goal)
				break;
		}

#if TRACE
		printf("pass %d: collapses: %d/%d (target %d), error: %e\n", int(pass_count), int(collapses), int(edge_collapse_count), int(edge_collapse_goal), pass_error);
#endif

		pass_count++;
		worst_error = (worst_error < pass_error) ? pass_error : worst_error;

		// no edges can be collapsed any more => bail out
		if (collapses == 0)
			break;

		size_t write = 0;

		for (size_t i = 0; i < index_count; i += 3)
		{
			unsigned int v0 = vertex_remap[result[i + 0]];
			unsigned int v1 = vertex_remap[result[i + 1]];
			unsigned int v2 = vertex_remap[result[i + 2]];

			assert(vertex_remap[v0] == v0);
			assert(vertex_remap[v1] == v1);
			assert(vertex_remap[v2] == v2);

			if (v0 != v1 && v0 != v2 && v1 != v2)
			{
				result[write + 0] = v0;
				result[write + 1] = v1;
				result[write + 2] = v2;
				write += 3;
			}
		}

		index_count = write;
	}

#if TRACE
	printf("passes: %d, worst error: %e\n", int(pass_count), worst_error);
#endif

	return index_count;
}

} // namespace meshopt

size_t meshopt_simplify(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, size_t target_index_count)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride > 0 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(target_index_count <= index_count);

	return simplifyEdgeCollapse(destination, indices, index_count, vertex_positions, vertex_positions_stride, vertex_count, target_index_count);
}
