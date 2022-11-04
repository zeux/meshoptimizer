// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

// This work is based on:
// John McDonald, Mark Kilgard. Crack-Free Point-Normal Triangles using Adjacent Edge Normals. 2010
namespace meshopt
{

static datatype_t hashUpdate4(datatype_t h, const unsigned char* key, size_t len)
{
	// MurmurHash2
	const datatype_t m = 0x5bd1e995;
	const int r = 24;

	while (len >= 4)
	{
		datatype_t k = *reinterpret_cast<const datatype_t*>(key);

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

struct VertexHasher
{
	const unsigned char* vertices;
	size_t vertex_size;
	size_t vertex_stride;

	size_t hash(datatype_t index) const
	{
		return hashUpdate4(0, vertices + index * vertex_stride, vertex_size);
	}

	bool equal(datatype_t lhs, datatype_t rhs) const
	{
		return memcmp(vertices + lhs * vertex_stride, vertices + rhs * vertex_stride, vertex_size) == 0;
	}
};

struct VertexStreamHasher
{
	const meshopt_Stream* streams;
	size_t stream_count;

	size_t hash(datatype_t index) const
	{
		datatype_t h = 0;

		for (size_t i = 0; i < stream_count; ++i)
		{
			const meshopt_Stream& s = streams[i];
			const unsigned char* data = static_cast<const unsigned char*>(s.data);

			h = hashUpdate4(h, data + index * s.stride, s.size);
		}

		return h;
	}

	bool equal(datatype_t lhs, datatype_t rhs) const
	{
		for (size_t i = 0; i < stream_count; ++i)
		{
			const meshopt_Stream& s = streams[i];
			const unsigned char* data = static_cast<const unsigned char*>(s.data);

			if (memcmp(data + lhs * s.stride, data + rhs * s.stride, s.size) != 0)
				return false;
		}

		return true;
	}
};

struct EdgeHasher
{
	const datatype_t* remap;

	size_t hash(unsigned long long edge) const
	{
		datatype_t e0 = datatype_t(edge >> 32);
		datatype_t e1 = datatype_t(edge);

		datatype_t h1 = remap[e0];
		datatype_t h2 = remap[e1];

		const datatype_t m = 0x5bd1e995;

		// MurmurHash64B finalizer
		h1 ^= h2 >> 18;
		h1 *= m;
		h2 ^= h1 >> 22;
		h2 *= m;
		h1 ^= h2 >> 17;
		h1 *= m;
		h2 ^= h1 >> 19;
		h2 *= m;

		return h2;
	}

	bool equal(unsigned long long lhs, unsigned long long rhs) const
	{
		datatype_t l0 = datatype_t(lhs >> 32);
		datatype_t l1 = datatype_t(lhs);

		datatype_t r0 = datatype_t(rhs >> 32);
		datatype_t r1 = datatype_t(rhs);

		return remap[l0] == remap[r0] && remap[l1] == remap[r1];
	}
};

static size_t hashBuckets(size_t count)
{
	size_t buckets = 1;
	while (buckets < count + count / 4)
		buckets *= 2;

	return buckets;
}

template <typename T, typename Hash>
static T* hashLookup(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty)
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
	return 0;
}

static void buildPositionRemap(datatype_t* remap, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, meshopt_Allocator& allocator)
{
	VertexHasher vertex_hasher = {reinterpret_cast<const unsigned char*>(vertex_positions), 3 * sizeof(float), vertex_positions_stride};

	size_t vertex_table_size = hashBuckets(vertex_count);
	datatype_t* vertex_table = allocator.allocate<datatype_t>(vertex_table_size);
	memset(vertex_table, -1, vertex_table_size * sizeof(datatype_t));

	for (size_t i = 0; i < vertex_count; ++i)
	{
		datatype_t index = datatype_t(i);
		datatype_t* entry = hashLookup(vertex_table, vertex_table_size, vertex_hasher, index, ~0u);

		if (*entry == ~0u)
			*entry = index;

		remap[index] = *entry;
	}
}

} // namespace meshopt

size_t meshopt_generateVertexRemap(datatype_t* destination, const datatype_t* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	using namespace meshopt;

	assert(indices || index_count == vertex_count);
	assert(!indices || index_count % 3 == 0);
	assert(vertex_size > 0 && vertex_size <= 256);

	meshopt_Allocator allocator;

	memset(destination, -1, vertex_count * sizeof(datatype_t));

	VertexHasher hasher = {static_cast<const unsigned char*>(vertices), vertex_size, vertex_size};

	size_t table_size = hashBuckets(vertex_count);
	datatype_t* table = allocator.allocate<datatype_t>(table_size);
	memset(table, -1, table_size * sizeof(datatype_t));

	datatype_t next_vertex = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		datatype_t index = indices ? indices[i] : datatype_t(i);
		assert(index < vertex_count);

		if (destination[index] == ~0u)
		{
			datatype_t* entry = hashLookup(table, table_size, hasher, index, ~0u);

			if (*entry == ~0u)
			{
				*entry = index;

				destination[index] = next_vertex++;
			}
			else
			{
				assert(destination[*entry] != ~0u);

				destination[index] = destination[*entry];
			}
		}
	}

	assert(next_vertex <= vertex_count);

	return next_vertex;
}

size_t meshopt_generateVertexRemapMulti(datatype_t* destination, const datatype_t* indices, size_t index_count, size_t vertex_count, const struct meshopt_Stream* streams, size_t stream_count)
{
	using namespace meshopt;

	assert(indices || index_count == vertex_count);
	assert(index_count % 3 == 0);
	assert(stream_count > 0 && stream_count <= 16);

	for (size_t i = 0; i < stream_count; ++i)
	{
		assert(streams[i].size > 0 && streams[i].size <= 256);
		assert(streams[i].size <= streams[i].stride);
	}

	meshopt_Allocator allocator;

	memset(destination, -1, vertex_count * sizeof(datatype_t));

	VertexStreamHasher hasher = {streams, stream_count};

	size_t table_size = hashBuckets(vertex_count);
	datatype_t* table = allocator.allocate<datatype_t>(table_size);
	memset(table, -1, table_size * sizeof(datatype_t));

	datatype_t next_vertex = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		datatype_t index = indices ? indices[i] : datatype_t(i);
		assert(index < vertex_count);

		if (destination[index] == ~0u)
		{
			datatype_t* entry = hashLookup(table, table_size, hasher, index, ~0u);

			if (*entry == ~0u)
			{
				*entry = index;

				destination[index] = next_vertex++;
			}
			else
			{
				assert(destination[*entry] != ~0u);

				destination[index] = destination[*entry];
			}
		}
	}

	assert(next_vertex <= vertex_count);

	return next_vertex;
}

void meshopt_remapVertexBuffer(void* destination, const void* vertices, size_t vertex_count, size_t vertex_size, const datatype_t* remap)
{
	assert(vertex_size > 0 && vertex_size <= 256);

	meshopt_Allocator allocator;

	// support in-place remap
	if (destination == vertices)
	{
		unsigned char* vertices_copy = allocator.allocate<unsigned char>(vertex_count * vertex_size);
		memcpy(vertices_copy, vertices, vertex_count * vertex_size);
		vertices = vertices_copy;
	}

	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (remap[i] != ~0u)
		{
			assert(remap[i] < vertex_count);

			memcpy(static_cast<unsigned char*>(destination) + remap[i] * vertex_size, static_cast<const unsigned char*>(vertices) + i * vertex_size, vertex_size);
		}
	}
}

void meshopt_remapIndexBuffer(datatype_t* destination, const datatype_t* indices, size_t index_count, const datatype_t* remap)
{
	assert(index_count % 3 == 0);

	for (size_t i = 0; i < index_count; ++i)
	{
		datatype_t index = indices ? indices[i] : datatype_t(i);
		assert(remap[index] != ~0u);

		destination[i] = remap[index];
	}
}

void meshopt_generateShadowIndexBuffer(datatype_t* destination, const datatype_t* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size, size_t vertex_stride)
{
	using namespace meshopt;

	assert(indices);
	assert(index_count % 3 == 0);
	assert(vertex_size > 0 && vertex_size <= 256);
	assert(vertex_size <= vertex_stride);

	meshopt_Allocator allocator;

	datatype_t* remap = allocator.allocate<datatype_t>(vertex_count);
	memset(remap, -1, vertex_count * sizeof(datatype_t));

	VertexHasher hasher = {static_cast<const unsigned char*>(vertices), vertex_size, vertex_stride};

	size_t table_size = hashBuckets(vertex_count);
	datatype_t* table = allocator.allocate<datatype_t>(table_size);
	memset(table, -1, table_size * sizeof(datatype_t));

	for (size_t i = 0; i < index_count; ++i)
	{
		datatype_t index = indices[i];
		assert(index < vertex_count);

		if (remap[index] == ~0u)
		{
			datatype_t* entry = hashLookup(table, table_size, hasher, index, ~0u);

			if (*entry == ~0u)
				*entry = index;

			remap[index] = *entry;
		}

		destination[i] = remap[index];
	}
}

void meshopt_generateShadowIndexBufferMulti(datatype_t* destination, const datatype_t* indices, size_t index_count, size_t vertex_count, const struct meshopt_Stream* streams, size_t stream_count)
{
	using namespace meshopt;

	assert(indices);
	assert(index_count % 3 == 0);
	assert(stream_count > 0 && stream_count <= 16);

	for (size_t i = 0; i < stream_count; ++i)
	{
		assert(streams[i].size > 0 && streams[i].size <= 256);
		assert(streams[i].size <= streams[i].stride);
	}

	meshopt_Allocator allocator;

	datatype_t* remap = allocator.allocate<datatype_t>(vertex_count);
	memset(remap, -1, vertex_count * sizeof(datatype_t));

	VertexStreamHasher hasher = {streams, stream_count};

	size_t table_size = hashBuckets(vertex_count);
	datatype_t* table = allocator.allocate<datatype_t>(table_size);
	memset(table, -1, table_size * sizeof(datatype_t));

	for (size_t i = 0; i < index_count; ++i)
	{
		datatype_t index = indices[i];
		assert(index < vertex_count);

		if (remap[index] == ~0u)
		{
			datatype_t* entry = hashLookup(table, table_size, hasher, index, ~0u);

			if (*entry == ~0u)
				*entry = index;

			remap[index] = *entry;
		}

		destination[i] = remap[index];
	}
}

void meshopt_generateAdjacencyIndexBuffer(datatype_t* destination, const datatype_t* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	meshopt_Allocator allocator;

	static const int next[4] = {1, 2, 0, 1};

	// build position remap: for each vertex, which other (canonical) vertex does it map to?
	datatype_t* remap = allocator.allocate<datatype_t>(vertex_count);
	buildPositionRemap(remap, vertex_positions, vertex_count, vertex_positions_stride, allocator);

	// build edge set; this stores all triangle edges but we can look these up by any other wedge
	EdgeHasher edge_hasher = {remap};

	size_t edge_table_size = hashBuckets(index_count);
	unsigned long long* edge_table = allocator.allocate<unsigned long long>(edge_table_size);
	datatype_t* edge_vertex_table = allocator.allocate<datatype_t>(edge_table_size);

	memset(edge_table, -1, edge_table_size * sizeof(unsigned long long));
	memset(edge_vertex_table, -1, edge_table_size * sizeof(datatype_t));

	for (size_t i = 0; i < index_count; i += 3)
	{
		for (int e = 0; e < 3; ++e)
		{
			datatype_t i0 = indices[i + e];
			datatype_t i1 = indices[i + next[e]];
			datatype_t i2 = indices[i + next[e + 1]];
			assert(i0 < vertex_count && i1 < vertex_count && i2 < vertex_count);

			unsigned long long edge = ((unsigned long long)i0 << 32) | i1;
			unsigned long long* entry = hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			if (*entry == ~0ull)
			{
				*entry = edge;

				// store vertex opposite to the edge
				edge_vertex_table[entry - edge_table] = i2;
			}
		}
	}

	// build resulting index buffer: 6 indices for each input triangle
	for (size_t i = 0; i < index_count; i += 3)
	{
		datatype_t patch[6];

		for (int e = 0; e < 3; ++e)
		{
			datatype_t i0 = indices[i + e];
			datatype_t i1 = indices[i + next[e]];
			assert(i0 < vertex_count && i1 < vertex_count);

			// note: this refers to the opposite edge!
			unsigned long long edge = ((unsigned long long)i1 << 32) | i0;
			unsigned long long* oppe = hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			patch[e * 2 + 0] = i0;
			patch[e * 2 + 1] = (*oppe == ~0ull) ? i0 : edge_vertex_table[oppe - edge_table];
		}

		memcpy(destination + i * 2, patch, sizeof(patch));
	}
}

void meshopt_generateTessellationIndexBuffer(datatype_t* destination, const datatype_t* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	meshopt_Allocator allocator;

	static const int next[3] = {1, 2, 0};

	// build position remap: for each vertex, which other (canonical) vertex does it map to?
	datatype_t* remap = allocator.allocate<datatype_t>(vertex_count);
	buildPositionRemap(remap, vertex_positions, vertex_count, vertex_positions_stride, allocator);

	// build edge set; this stores all triangle edges but we can look these up by any other wedge
	EdgeHasher edge_hasher = {remap};

	size_t edge_table_size = hashBuckets(index_count);
	unsigned long long* edge_table = allocator.allocate<unsigned long long>(edge_table_size);
	memset(edge_table, -1, edge_table_size * sizeof(unsigned long long));

	for (size_t i = 0; i < index_count; i += 3)
	{
		for (int e = 0; e < 3; ++e)
		{
			datatype_t i0 = indices[i + e];
			datatype_t i1 = indices[i + next[e]];
			assert(i0 < vertex_count && i1 < vertex_count);

			unsigned long long edge = ((unsigned long long)i0 << 32) | i1;
			unsigned long long* entry = hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			if (*entry == ~0ull)
				*entry = edge;
		}
	}

	// build resulting index buffer: 12 indices for each input triangle
	for (size_t i = 0; i < index_count; i += 3)
	{
		datatype_t patch[12];

		for (int e = 0; e < 3; ++e)
		{
			datatype_t i0 = indices[i + e];
			datatype_t i1 = indices[i + next[e]];
			assert(i0 < vertex_count && i1 < vertex_count);

			// note: this refers to the opposite edge!
			unsigned long long edge = ((unsigned long long)i1 << 32) | i0;
			unsigned long long oppe = *hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			// use the same edge if opposite edge doesn't exist (border)
			oppe = (oppe == ~0ull) ? edge : oppe;

			// triangle index (0, 1, 2)
			patch[e] = i0;

			// opposite edge (3, 4; 5, 6; 7, 8)
			patch[3 + e * 2 + 0] = datatype_t(oppe);
			patch[3 + e * 2 + 1] = datatype_t(oppe >> 32);

			// dominant vertex (9, 10, 11)
			patch[9 + e] = remap[i0];
		}

		memcpy(destination + i * 4, patch, sizeof(patch));
	}
}
