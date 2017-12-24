// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

namespace meshopt
{

static unsigned int murmurHash(const char* key, size_t len, unsigned int h)
{
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

struct VertexHasher
{
	const char* vertices;
	size_t vertex_size;

	unsigned int empty() const
	{
		return ~0u;
	}

	size_t operator()(unsigned int index) const
	{
		return murmurHash(vertices + index * vertex_size, vertex_size, 0);
	}

	size_t operator()(unsigned int lhs, unsigned int rhs) const
	{
		return memcmp(vertices + lhs * vertex_size, vertices + rhs * vertex_size, vertex_size) == 0;
	}
};

struct VertexHashEntry
{
	unsigned int key;
	unsigned int value;
};

static size_t hashBuckets(size_t count)
{
	size_t buckets = 1;
	while (buckets < count)
		buckets *= 2;

	return buckets;
}

template <typename T, typename Hash, typename Key>
static T* hashLookup(T* table, size_t buckets, const Hash& hash, const Key& key, const Key& empty)
{
	assert(buckets > 0);
	assert((buckets & (buckets - 1)) == 0);

	size_t hashmod = buckets - 1;
	size_t bucket = hash(key) & hashmod;

	for (size_t probe = 0; probe <= hashmod; ++probe)
	{
		T& item = table[bucket];

		if (item.key == empty)
			return &item;

		if (hash(item.key, key))
			return &item;

		// hash collision, quadratic probing
		bucket = (bucket + probe + 1) & hashmod;
	}

	assert(false && "Hash table is full");
	return 0;
}

} // namespace meshopt

size_t meshopt_generateVertexRemap(unsigned int* destination, const unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	using namespace meshopt;

	assert(indices || index_count == vertex_count);
	assert(index_count % 3 == 0);
	assert(vertex_size > 0 && vertex_size <= 256);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		destination[i] = ~0u;
	}

	VertexHasher hasher = {static_cast<const char*>(vertices), vertex_size};

	meshopt_Buffer<VertexHashEntry> table(hashBuckets(vertex_count));
	memset(table.data, -1, table.size * sizeof(VertexHashEntry));

	unsigned int next_vertex = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices ? indices[i] : unsigned(i);
		assert(index < vertex_count);

		if (destination[index] == ~0u)
		{
			VertexHashEntry* entry = hashLookup(table.data, table.size, hasher, index, ~0u);

			if (entry->key == ~0u)
			{
				entry->key = index;
				entry->value = next_vertex++;
			}

			destination[index] = entry->value;
		}
	}

	assert(next_vertex <= vertex_count);

	return next_vertex;
}

void meshopt_remapVertexBuffer(void* destination, const void* vertices, size_t vertex_count, size_t vertex_size, const unsigned int* remap)
{
	assert(destination != vertices);
	assert(vertex_size > 0 && vertex_size <= 256);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (remap[i] != ~0u)
		{
			assert(remap[i] < vertex_count);

			memcpy(static_cast<char*>(destination) + remap[i] * vertex_size, static_cast<const char*>(vertices) + i * vertex_size, vertex_size);
		}
	}
}

void meshopt_remapIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const unsigned int* remap)
{
	assert(index_count % 3 == 0);

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices ? indices[i] : unsigned(i);
		assert(remap[index] != ~0u);

		destination[i] = remap[index];
	}
}
