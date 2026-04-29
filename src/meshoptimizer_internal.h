// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#ifndef MESHOPTIMIZER_INTERNAL_H
#define MESHOPTIMIZER_INTERNAL_H

#include <assert.h>
#include <stddef.h>
#include <string.h>

namespace meshopt
{

inline unsigned int hashUpdate4(unsigned int h, const unsigned char* key, size_t len)
{
	// MurmurHash2
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	while (len >= 4)
	{
		unsigned int k;
		memcpy(&k, key, sizeof(k));

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

inline unsigned int hashFinalize(unsigned int h)
{
	// MurmurHash2 finalizer
	h ^= h >> 13;
	h *= 0x5bd1e995;
	h ^= h >> 15;
	return h;
}

inline unsigned int hashFloatBits(unsigned int v)
{
	// replace negative zero with zero, then scramble bits to improve integer-coordinate hashes
	v = (v == 0x80000000) ? 0 : v;
	return v ^ (v >> 17);
}

inline unsigned int hashSpatial3(unsigned int x, unsigned int y, unsigned int z)
{
	// Optimized Spatial Hashing for Collision Detection of Deformable Objects
	return (hashFloatBits(x) * 73856093) ^ (hashFloatBits(y) * 19349663) ^ (hashFloatBits(z) * 83492791);
}

inline size_t hashBuckets(size_t count)
{
	size_t buckets = 1;
	while (buckets < count + count / 4)
		buckets *= 2;

	return buckets;
}

template <typename T, typename Hash>
inline T* hashLookup(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty)
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

} // namespace meshopt

#endif
