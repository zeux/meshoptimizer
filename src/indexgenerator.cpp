// This file is part of meshoptimizer library; see meshoptimizer.hpp for version/license details
#include "meshoptimizer.hpp"

#include <cassert>
#include <cstring>
#include <vector>

namespace meshopt
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

struct Hasher
{
	size_t size;

	size_t operator()(const void* ptr) const
	{
		return MurmurHash2(ptr, size, 0);
	}

	size_t operator()(const void* lhs, const void* rhs) const
	{
		return memcmp(lhs, rhs, size) == 0;
	}
};

struct HashEntry
{
	const void* key;
	unsigned int value;
};

static void hashInit(std::vector<HashEntry>& table, size_t count)
{
	size_t buckets = 1;
	while (buckets < count)
		buckets *= 2;

	HashEntry dummy = {0, 0};

	table.clear();
	table.resize(buckets, dummy);
}

static HashEntry* hashLookup(std::vector<HashEntry>& table, const Hasher& hasher, const void* key)
{
	assert((table.size() & (table.size() - 1)) == 0);

	size_t hash = hasher(key);

	size_t hashmod = table.size() - 1;
	size_t bucket = hash & hashmod;

	for (size_t probe = 0; probe <= hashmod; ++probe)
	{
		HashEntry& item = table[bucket];

		if (item.key == 0)
			return &item;

		if (hasher(item.key, key))
			return &item;

		// hash collision, quadratic probing
		bucket = (bucket + probe + 1) & hashmod;
	}

	assert(false && "Hash table is full");
	return 0;
}

size_t generateVertexRemap(unsigned int* destination, const unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	assert(indices || index_count == vertex_count);
	assert(index_count % 3 == 0);
	assert(vertex_size > 0);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		destination[i] = static_cast<unsigned int>(-1);
	}

	Hasher hasher = {vertex_size};

	std::vector<HashEntry> table;
	hashInit(table, vertex_count);

	unsigned int next_vertex = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices ? indices[i] : unsigned(i);
		assert(index < vertex_count);

		if (destination[index] == static_cast<unsigned int>(-1))
		{
			const void* vertex = static_cast<const char*>(vertices) + index * vertex_size;

			HashEntry* entry = hashLookup(table, hasher, vertex);

			if (!entry->key)
			{
				entry->key = vertex;
				entry->value = next_vertex++;
			}

			destination[index] = entry->value;
		}
	}

	return next_vertex;
}

void remapVertexBuffer(void* destination, const void* vertices, size_t vertex_count, size_t vertex_size, const unsigned int* remap)
{
	assert(destination != vertices);
	assert(vertex_size > 0);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		memcpy(static_cast<char*>(destination) + remap[i] * vertex_size, static_cast<const char*>(vertices) + i * vertex_size, vertex_size);
	}
}

void remapIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const unsigned int* remap)
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

} // namespace meshopt
