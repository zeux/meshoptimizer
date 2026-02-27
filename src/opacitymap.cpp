// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

namespace meshopt
{

// opacity micromaps use a "bird" traversal order which recursively subdivides the triangles:
// https://docs.vulkan.org/spec/latest/_images/micromap-subd.svg
// note that triangles 0 and 2 have the same winding as the source triangle, however triangles 1 (flipped)
// and 3 (upright) have flipped winding; this is obvious from the level 2 subdivision in the diagram above

struct TriangleOMM
{
	int uvs[6];
	int level;
};

struct Texture
{
	const unsigned char* data;
	size_t stride, pitch;
	unsigned int width, height;
};

static float sampleTexture(const Texture& texture, float u, float v)
{
	// wrap texture coordinates; floor is expensive so only call it if we're outside of [0, 1] range (+eps)
	u = fabsf(u - 0.5f) > 0.5f ? u - floorf(u) : u;
	v = fabsf(v - 0.5f) > 0.5f ? v - floorf(v) : v;

	// convert from [0, 1] to pixel grid and shift so that texel centers are on an integer grid
	u = u * float(int(texture.width)) - 0.5f;
	v = v * float(int(texture.height)) - 0.5f;

	// clamp u/v along the left/top edge to avoid extrapolation since we don't interpolate across the edge
	u = u < 0 ? 0.f : u;
	v = v < 0 ? 0.f : v;

	// note: u/v is now in [0, size-0.5]; float->int->float is usually less expensive than floor
	int x = int(u);
	int y = int(v);
	float rx = u - float(x);
	float ry = v - float(y);

	// safeguard: this should not happen but if it ever does, ensure the accesses are inbounds
	if (unsigned(x) >= texture.width || unsigned(y) >= texture.height)
		return 0.f;

	// clamp the offsets instead of wrapping for simplicity and performance
	size_t offset = size_t(y) * texture.pitch + x * texture.stride;
	size_t offsetx = (x + 1 < int(texture.width)) ? texture.stride : 0;
	size_t offsety = (y + 1 < int(texture.height)) ? texture.pitch : 0;

	unsigned char a00 = texture.data[offset];
	unsigned char a10 = texture.data[offset + offsetx];
	unsigned char a01 = texture.data[offset + offsety];
	unsigned char a11 = texture.data[offset + offsetx + offsety];

	// blinear interpolation; we do it partially in integer space, deferring full conversion to [0, 1] until the end
	float ax0 = float(a00) + float(a10 - a00) * rx;
	float ax1 = float(a01) + float(a11 - a01) * rx;
	return (ax0 + (ax1 - ax0) * ry) * (1.f / 255.f);
}

struct TriangleOMMHasher
{
	const TriangleOMM* data;

	size_t hash(unsigned int index) const
	{
		const TriangleOMM& tri = data[index];
		unsigned int h = tri.level;

		const unsigned int m = 0x5bd1e995;
		const int r = 24;

		// MurmurHash2
		const unsigned int* uvs = reinterpret_cast<const unsigned int*>(tri.uvs);

		for (int i = 0; i < 6; ++i)
		{
			unsigned int k = uvs[i];

			k *= m;
			k ^= k >> r;
			k *= m;

			h *= m;
			h ^= k;
		}

		// MurmurHash2 finalizer
		h ^= h >> 13;
		h *= 0x5bd1e995;
		h ^= h >> 15;
		return h;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		const TriangleOMM& lt = data[lhs];
		const TriangleOMM& rt = data[rhs];

		return lt.level == rt.level && memcmp(lt.uvs, rt.uvs, sizeof(lt.uvs)) == 0;
	}
};

struct OMMHasher
{
	const unsigned char* data;
	const unsigned int* offsets;
	const int* levels;
	int states;

	size_t hash(unsigned int index) const
	{
		size_t size = ((1 << (levels[index] * 2)) * (states / 2) + 7) / 8;
		const unsigned char* key = data + offsets[index];

		unsigned int h = levels[index];

		const unsigned int m = 0x5bd1e995;
		const int r = 24;

		// MurmurHash2
		for (size_t i = 0; i < size / 4; ++i)
		{
			unsigned int k = *reinterpret_cast<const unsigned int*>(key + i * 4);

			k *= m;
			k ^= k >> r;
			k *= m;

			h *= m;
			h ^= k;
		}

		// MurmurHash2 tail
		switch (size & 3)
		{
		case 3:
			h ^= key[size - 3] << 16;
			// fallthrough
		case 2:
			h ^= key[size - 2] << 8;
			// fallthrough
		case 1:
			h ^= key[size - 1];
			h *= m;
		}

		// MurmurHash2 finalizer
		h ^= h >> 13;
		h *= 0x5bd1e995;
		h ^= h >> 15;
		return h;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		size_t size = ((1 << (levels[lhs] * 2)) * (states / 2) + 7) / 8;

		return levels[lhs] == levels[rhs] && memcmp(data + offsets[lhs], data + offsets[rhs], size) == 0;
	}
};

static size_t hashBuckets3(size_t count)
{
	size_t buckets = 1;
	while (buckets < count + count / 4)
		buckets *= 2;

	return buckets;
}

template <typename T, typename Hash>
static T* hashLookup3(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty)
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

inline int quantizeSubpixel(float v, unsigned int size)
{
	return int(v * float(int(size) * 4) + (v >= 0 ? 0.5f : -0.5f));
}

static void rasterizeOpacity2(unsigned char* result, size_t index, float a0, float a1, float a2, float ac)
{
	// basic coverage estimator from center and corner values; trained to minimize error
	float coverage = (a0 + a1 + a2) * 0.12f + ac * 0.64f;

	result[index / 8] |= (coverage >= 0.5f) << (index % 8);
}

static void rasterizeOpacity4(unsigned char* result, size_t index, float a0, float a1, float a2, float ac)
{
	int transp = (a0 < 0.25f) & (a1 < 0.25f) & (a2 < 0.25f) & (ac < 0.25f);
	int opaque = (a0 > 0.75f) & (a1 > 0.75f) & (a2 > 0.75f) & (ac > 0.75f);
	float coverage = (a0 + a1 + a2) * 0.12f + ac * 0.64f;

	// treat state as known if thresholding of corners & centers against wider bounds is consistent
	// for unknown states, we currently use the same formula as the 2-state opacity for better consistency with forced 2-state
	int state = (transp | opaque) ? opaque : (2 + (coverage >= 0.5f));

	result[index / 4] |= state << ((index % 4) * 2);
}

template <int States>
static void rasterizeOpacityRec(unsigned char* result, size_t index, int level, const float* c0, const float* c1, const float* c2, const Texture& texture)
{
	if (level == 0)
	{
		// compute triangle center & sample
		float uc = (c0[0] + c1[0] + c2[0]) / 3;
		float vc = (c0[1] + c1[1] + c2[1]) / 3;
		float ac = sampleTexture(texture, uc, vc);

		// rasterize opacity state based on alpha values in corners and center
		(States == 2) ? rasterizeOpacity2(result, index, c0[2], c1[2], c2[2], ac) : rasterizeOpacity4(result, index, c0[2], c1[2], c2[2], ac);
		return;
	}

	// compute each edge midpoint & sample
	float c01[3] = {(c0[0] + c1[0]) / 2, (c0[1] + c1[1]) / 2, 0.f};
	float c12[3] = {(c1[0] + c2[0]) / 2, (c1[1] + c2[1]) / 2, 0.f};
	float c02[3] = {(c0[0] + c2[0]) / 2, (c0[1] + c2[1]) / 2, 0.f};

	c01[2] = sampleTexture(texture, c01[0], c01[1]);
	c12[2] = sampleTexture(texture, c12[0], c12[1]);
	c02[2] = sampleTexture(texture, c02[0], c02[1]);

	// recursively rasterize each triangle
	// note: triangles 1 and 3 have flipped winding, and 1 is flipped upside down
	rasterizeOpacityRec<States>(result, index * 4 + 0, level - 1, c0, c01, c02, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 1, level - 1, c02, c12, c01, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 2, level - 1, c01, c1, c12, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 3, level - 1, c12, c02, c2, texture);
}

static int getSpecialIndex(const unsigned char* data, int level, int states)
{
	int first = data[0] & (states == 2 ? 1 : 3);
	int special = -(1 + first);

	// at level 0, every micromap can be converted to a special index
	if (level == 0)
		return special;

	// at level 1 with 2 states, the byte is partially filled so we need a separate check
	if (level == 1 && states == 2)
		return (data[0] & 15) == ((-first) & 15) ? special : 0;

	// otherwise we need to check that all bytes are consistent with the first value and we can do this byte-wise
	int expected = first * (states == 2 ? 0xff : 0x55);
	size_t size = (1 << (level * 2)) * (states / 2) / 8;

	for (size_t i = 0; i < size; ++i)
		if (data[i] != expected)
			return 0;

	return special;
}

} // namespace meshopt

size_t meshopt_opacityMapMeasure(int* levels, unsigned int* sources, int* omm_indices, const unsigned int* indices, size_t index_count, const float* vertex_uvs, size_t vertex_count, size_t vertex_uvs_stride, unsigned int texture_width, unsigned int texture_height, int max_level, float target_edge)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_uvs_stride >= 8 && vertex_uvs_stride <= 256);
	assert(vertex_uvs_stride % sizeof(float) == 0);
	assert(unsigned(texture_width - 1) < 16384 && unsigned(texture_height - 1) < 16384);
	assert(max_level > 0 && max_level <= 12);
	assert(target_edge >= 0);

	(void)vertex_count;

	meshopt_Allocator allocator;

	size_t vertex_stride_float = vertex_uvs_stride / sizeof(float);
	float texture_area = float(texture_width) * float(texture_height);

	// hash map used to deduplicate triangle rasterization requests based on UV
	size_t table_size = hashBuckets3(index_count / 3);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);
	memset(table, -1, table_size * sizeof(unsigned int));

	TriangleOMM* triangles = allocator.allocate<TriangleOMM>(index_count / 3);
	TriangleOMMHasher hasher = {triangles};

	size_t result = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		float u0 = vertex_uvs[a * vertex_stride_float + 0], v0 = vertex_uvs[a * vertex_stride_float + 1];
		float u1 = vertex_uvs[b * vertex_stride_float + 0], v1 = vertex_uvs[b * vertex_stride_float + 1];
		float u2 = vertex_uvs[c * vertex_stride_float + 0], v2 = vertex_uvs[c * vertex_stride_float + 1];

		int level = max_level;

		if (target_edge > 0)
		{
			// compute ratio of edge length (in texels) to target and determine subdivision level
			float uvarea = fabsf((u1 - u0) * (v2 - v0) - (u2 - u0) * (v1 - v0)) * 0.5f * texture_area;
			float ratio = sqrtf(uvarea) / target_edge;
			float levelf = log2f(ratio > 1 ? ratio : 1);

			// round to nearest and clamp
			level = int(levelf + 0.5f);
			level = level < 0 ? 0 : level;
			level = level < max_level ? level : max_level;
		}

		// deduplicate rasterization requests based on UV
		int su0 = quantizeSubpixel(u0, texture_width), sv0 = quantizeSubpixel(v0, texture_height);
		int su1 = quantizeSubpixel(u1, texture_width), sv1 = quantizeSubpixel(v1, texture_height);
		int su2 = quantizeSubpixel(u2, texture_width), sv2 = quantizeSubpixel(v2, texture_height);

		TriangleOMM tri = {{su0, sv0, su1, sv1, su2, sv2}, level};
		triangles[result] = tri; // speculatively write triangle data to give hasher a way to compare it

		unsigned int* entry = hashLookup3(table, table_size, hasher, unsigned(result), ~0u);

		if (*entry == ~0u)
		{
			*entry = unsigned(result);
			levels[result] = level;
			sources[result] = int(i / 3);
			result++;
		}

		omm_indices[i / 3] = int(*entry);
	}

	return result;
}

size_t meshopt_opacityMapTriangleSize(int level, int states)
{
	assert(level >= 0 && level <= 12);
	assert(states == 2 || states == 4);

	return ((1 << (level * 2)) * (states / 2) + 7) / 8;
}

int meshopt_opacityMapPreferredMip(int level, const float* uv0, const float* uv1, const float* uv2, unsigned int texture_width, unsigned int texture_height)
{
	assert(level >= 0 && level <= 12);
	assert(unsigned(texture_width - 1) < 16384 && unsigned(texture_height - 1) < 16384);

	float texture_area = float(texture_width) * float(texture_height);

	// compute log2 of edge length (in texels)
	float uvarea = fabsf((uv1[0] - uv0[0]) * (uv2[1] - uv0[1]) - (uv2[0] - uv0[0]) * (uv1[1] - uv0[1])) * 0.5f * texture_area;
	float ratio = sqrtf(uvarea) / float(1 << level);
	float levelf = log2f(ratio > 1 ? ratio : 1);

	// round down and clamp
	int mip = int(levelf - 0.5f);
	mip = mip < 0 ? 0 : mip;
	mip = mip < 16 ? mip : 16;

	// ensure the selected mip is in range
	while (mip > 0 && (texture_width >> mip) == 0 && (texture_height >> mip) == 0)
		mip--;

	return mip;
}

void meshopt_opacityMapRasterize(unsigned char* result, int level, int states, const float* uv0, const float* uv1, const float* uv2, const unsigned char* texture_data, size_t texture_stride, size_t texture_pitch, unsigned int texture_width, unsigned int texture_height)
{
	using namespace meshopt;

	assert(level >= 0 && level <= 12);
	assert(states == 2 || states == 4);
	assert(unsigned(texture_width - 1) < 16384 && unsigned(texture_height - 1) < 16384);
	assert(texture_stride >= 1 && texture_stride <= 4);
	assert(texture_pitch >= texture_stride * texture_width);

	Texture texture = {texture_data, texture_stride, texture_pitch, texture_width, texture_height};

	// 1-bit 2-state or 2-bit 4-state per micro triangle, rounded up to whole bytes
	memset(result, 0, ((1 << (level * 2)) * (states / 2) + 7) / 8);

	// rasterize all micro triangles recursively, passing corner data down to reduce redundant sampling
	float c0[3] = {uv0[0], uv0[1], sampleTexture(texture, uv0[0], uv0[1])};
	float c1[3] = {uv1[0], uv1[1], sampleTexture(texture, uv1[0], uv1[1])};
	float c2[3] = {uv2[0], uv2[1], sampleTexture(texture, uv2[0], uv2[1])};

	(states == 2 ? rasterizeOpacityRec<2> : rasterizeOpacityRec<4>)(result, 0, level, c0, c1, c2, texture);
}

size_t meshopt_opacityMapCompact(unsigned char* data, size_t data_size, int* levels, unsigned int* offsets, size_t omm_count, int* omm_indices, size_t triangle_count, int states)
{
	using namespace meshopt;

	assert(states == 2 || states == 4);

	meshopt_Allocator allocator;

	unsigned char* data_old = allocator.allocate<unsigned char>(data_size);
	memcpy(data_old, data, data_size);

	size_t table_size = hashBuckets3(omm_count);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);
	memset(table, -1, table_size * sizeof(unsigned int));

	OMMHasher hasher = {data, offsets, levels, states};

	int* remap = allocator.allocate<int>(omm_count);

	size_t next = 0;
	size_t offset = 0;

	for (size_t i = 0; i < omm_count; ++i)
	{
		int level = levels[i];
		assert(level >= 0 && level <= 12);

		const unsigned char* old = data_old + offsets[i];
		size_t size = ((1 << (level * 2)) * (states / 2) + 7) / 8;
		assert(offsets[i] + size <= data_size);

		// try to convert to a special index if all micro-triangle states are the same
		int special = getSpecialIndex(old, level, states);
		if (special < 0)
		{
			remap[i] = special;
			continue;
		}

		// speculatively write data to give hasher a way to compare it
		memcpy(data + offset, old, size);
		offsets[next] = unsigned(offset);
		levels[next] = level;

		unsigned int* entry = hashLookup3(table, table_size, hasher, unsigned(next), ~0u);

		if (*entry == ~0u)
		{
			*entry = unsigned(next);
			next++;
			offset += size;
		}

		remap[i] = int(*entry);
	}

	// remap triangle indices to new indices or special indices
	for (size_t i = 0; i < triangle_count; ++i)
	{
		assert(unsigned(omm_indices[i]) < omm_count);
		omm_indices[i] = remap[omm_indices[i]];
	}

	return next;
}
