// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <math.h>
#include <string.h>

// This work is based on:
// Morten Mikkelsen. Simulation of wrinkled surfaces revisited. 2008
namespace meshopt
{

struct Tangent
{
	float x, y, z, w;
};

static void computeFaceTangents(Tangent* result, size_t triangle_count, const unsigned int* indices, const float* vertex_positions, size_t vertex_positions_stride, const float* vertex_uvs, size_t vertex_uvs_stride)
{
	size_t vertex_position_stride_float = vertex_positions_stride / sizeof(float);
	size_t vertex_uv_stride_float = vertex_uvs_stride / sizeof(float);

	for (size_t i = 0; i < triangle_count; ++i)
	{
		unsigned int a = indices ? indices[i * 3 + 0] : unsigned(i * 3 + 0);
		unsigned int b = indices ? indices[i * 3 + 1] : unsigned(i * 3 + 1);
		unsigned int c = indices ? indices[i * 3 + 2] : unsigned(i * 3 + 2);

		const float* pa = &vertex_positions[a * vertex_position_stride_float];
		const float* pb = &vertex_positions[b * vertex_position_stride_float];
		const float* pc = &vertex_positions[c * vertex_position_stride_float];

		const float* ta = &vertex_uvs[a * vertex_uv_stride_float];
		const float* tb = &vertex_uvs[b * vertex_uv_stride_float];
		const float* tc = &vertex_uvs[c * vertex_uv_stride_float];

		// compute face tangents using chain rule, see eq. 42
		float dp1x = pb[0] - pa[0], dp1y = pb[1] - pa[1], dp1z = pb[2] - pa[2];
		float dp2x = pc[0] - pa[0], dp2y = pc[1] - pa[1], dp2z = pc[2] - pa[2];
		float dt1x = tb[0] - ta[0], dt1y = tb[1] - ta[1];
		float dt2x = tc[0] - ta[0], dt2y = tc[1] - ta[1];

		float rx = dt2y * dp1x - dt1y * dp2x;
		float ry = dt2y * dp1y - dt1y * dp2y;
		float rz = dt2y * dp1z - dt1y * dp2z;

		// .w stores orientation (+1/-1) or 0 for degenerate triangles, see eq. 48
		float det = dt1x * dt2y - dt1y * dt2x;
		float rw = det == 0.f ? 0.f : (det > 0.f ? 1.f : -1.f);

		float rl = sqrtf(rx * rx + ry * ry + rz * rz);
		float rs = (det != 0.f && rl != 0.f) ? rw / rl : 0.f;

		result[i].x = rx * rs;
		result[i].y = ry * rs;
		result[i].z = rz * rs;
		result[i].w = rw;
	}
}

static unsigned int hashUpdate4(unsigned int h, const unsigned char* key, size_t len)
{
	// MurmurHash2
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	while (len >= 4)
	{
		unsigned int k = *reinterpret_cast<const unsigned int*>(key);

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		key += 4;
		len -= 4;
	}

	return h;
}

struct VertexHasherF
{
	const float* vertex_positions;
	size_t vertex_positions_stride_float;
	const float* vertex_normals;
	size_t vertex_normals_stride_float;
	const float* vertex_uvs;
	size_t vertex_uvs_stride_float;

	size_t hash(unsigned int index) const
	{
		const float* p = vertex_positions + index * vertex_positions_stride_float;
		const float* n = vertex_normals + index * vertex_normals_stride_float;
		const float* t = vertex_uvs + index * vertex_uvs_stride_float;

		// TODO: use a simpler / more direct hash function + handle negative zero
		unsigned int hash = 0;
		hash = hashUpdate4(hash, reinterpret_cast<const unsigned char*>(p), 3 * sizeof(float));
		hash = hashUpdate4(hash, reinterpret_cast<const unsigned char*>(n), 3 * sizeof(float));
		hash = hashUpdate4(hash, reinterpret_cast<const unsigned char*>(t), 2 * sizeof(float));
		return hash;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		const float* lp = vertex_positions + lhs * vertex_positions_stride_float;
		const float* ln = vertex_normals + lhs * vertex_normals_stride_float;
		const float* lt = vertex_uvs + lhs * vertex_uvs_stride_float;
		const float* rp = vertex_positions + rhs * vertex_positions_stride_float;
		const float* rn = vertex_normals + rhs * vertex_normals_stride_float;
		const float* rt = vertex_uvs + rhs * vertex_uvs_stride_float;
		return lp[0] == rp[0] && lp[1] == rp[1] && lp[2] == rp[2] && ln[0] == rn[0] && ln[1] == rn[1] && ln[2] == rn[2] && lt[0] == rt[0] && lt[1] == rt[1];
	}
};

static size_t hashBuckets4(size_t count)
{
	size_t buckets = 1;
	while (buckets < count + count / 4)
		buckets *= 2;

	return buckets;
}

template <typename T, typename Hash>
static T* hashLookup4(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty)
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

	assert(false && "Hash table is full"); // unreachable
	return NULL;
}

static void buildVertexRemap(unsigned int* remap, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, const float* vertex_normals, size_t vertex_normals_stride, const float* vertex_uvs, size_t vertex_uvs_stride, meshopt_Allocator& allocator)
{
	VertexHasherF vertex_hasher = {vertex_positions, vertex_positions_stride / sizeof(float), vertex_normals, vertex_normals_stride / sizeof(float), vertex_uvs, vertex_uvs_stride / sizeof(float)};

	size_t vertex_table_size = hashBuckets4(vertex_count);
	unsigned int* vertex_table = allocator.allocate<unsigned int>(vertex_table_size);
	memset(vertex_table, -1, vertex_table_size * sizeof(unsigned int));

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int index = unsigned(i);
		unsigned int* entry = hashLookup4(vertex_table, vertex_table_size, vertex_hasher, index, ~0u);

		if (*entry == ~0u)
			*entry = index;

		remap[index] = *entry;
	}

	allocator.deallocate(vertex_table);
}

} // namespace meshopt

// TODO: argument order?
MESHOPTIMIZER_EXPERIMENTAL void meshopt_generateTangentsMikkT(float* result, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, const float* vertex_normals, size_t vertex_normals_stride, const float* vertex_uvs, size_t vertex_uvs_stride)
{
	using namespace meshopt;

	assert(indices || index_count == vertex_count);
	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_normals_stride >= 12 && vertex_normals_stride <= 256);
	assert(vertex_uvs_stride >= 12 && vertex_uvs_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0 && vertex_normals_stride % sizeof(float) == 0 && vertex_uvs_stride % sizeof(float) == 0);

	meshopt_Allocator allocator;

	size_t face_count = index_count / 3;

	(void)vertex_count;
	(void)vertex_normals;
	(void)vertex_normals_stride;

	// compute vertex remap to unique vertex index
	unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
	buildVertexRemap(remap, vertex_positions, vertex_count, vertex_positions_stride, vertex_normals, vertex_normals_stride, vertex_uvs, vertex_uvs_stride, allocator);

	// compute per-triangle tangents
	Tangent* face_tangents = allocator.allocate<Tangent>(face_count);
	computeFaceTangents(face_tangents, face_count, indices, vertex_positions, vertex_positions_stride, vertex_uvs, vertex_uvs_stride);

	// TODO: for now output per face tangents
	for (size_t i = 0; i < index_count; ++i)
	{
		float* r = &result[i * 4];
		const Tangent& t = face_tangents[i / 3];
		r[0] = t.x, r[1] = t.y, r[2] = t.z, r[3] = t.w;
	}
}
