// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

namespace
{

static unsigned int MurmurHash2(const void* key, size_t len, unsigned int seed)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h = seed ^ static_cast<unsigned int>(len);

	// Mix 4 bytes at a time into the hash
	const unsigned char* data = (const unsigned char*)key;

	while (len >= 4)
	{
		unsigned int k = *(unsigned int*)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array
	switch (len)
	{
	case 3:
		h ^= data[2] << 16;
	case 2:
		h ^= data[1] << 8;
	case 1:
		h ^= data[0];
		h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

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
		return MurmurHash2(vertices + index * vertex_size, vertex_size, 0);
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

template <typename T, typename Hash>
static void hashInit(std::vector<T>& table, const Hash& hash, size_t count)
{
	size_t buckets = 1;
	while (buckets < count)
		buckets *= 2;

	T dummy = T();
	dummy.key = hash.empty();

	table.clear();
	table.resize(buckets, dummy);
}

template <typename T, typename Hash, typename Key>
static T* hashLookup(std::vector<T>& table, const Hash& hash, const Key& key)
{
	assert((table.size() & (table.size() - 1)) == 0);

	size_t hashmod = table.size() - 1;
	size_t hashval = hash(key);
	size_t bucket = hashval & hashmod;

	for (size_t probe = 0; probe <= hashmod; ++probe)
	{
		T& item = table[bucket];

		if (item.key == hash.empty())
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
	assert(indices || index_count == vertex_count);
	assert(index_count % 3 == 0);
	assert(vertex_size > 0);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		destination[i] = ~0u;
	}

	VertexHasher hasher = {static_cast<const char*>(vertices), vertex_size};

	std::vector<VertexHashEntry> table;
	hashInit(table, hasher, vertex_count);

	unsigned int next_vertex = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices ? indices[i] : unsigned(i);
		assert(index < vertex_count);

		if (destination[index] == ~0u)
		{
			VertexHashEntry* entry = hashLookup(table, hasher, index);

			if (entry->key == ~0u)
			{
				entry->key = index;
				entry->value = next_vertex++;
			}

			destination[index] = entry->value;
		}
	}

	return next_vertex;
}

void meshopt_remapVertexBuffer(void* destination, const void* vertices, size_t vertex_count, size_t vertex_size, const unsigned int* remap)
{
	assert(destination != vertices);
	assert(vertex_size > 0);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		memcpy(static_cast<char*>(destination) + remap[i] * vertex_size, static_cast<const char*>(vertices) + i * vertex_size, vertex_size);
	}
}

void meshopt_remapIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const unsigned int* remap)
{
	assert(index_count % 3 == 0);

	if (indices)
	{
		for (size_t i = 0; i < index_count; ++i)
		{
			destination[i] = remap[indices[i]];
		}
	}
	else
	{
		for (size_t i = 0; i < index_count; ++i)
		{
			destination[i] = remap[i];
		}
	}
}
