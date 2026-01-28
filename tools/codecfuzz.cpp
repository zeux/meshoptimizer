#include "../src/meshoptimizer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void fuzzDecoder(const uint8_t* data, size_t size, size_t stride, int (*decode)(void*, size_t, size_t, const unsigned char*, size_t))
{
	size_t count = 66; // must be divisible by 3 for decodeIndexBuffer; should be >=64 to cover large vertex blocks

	void* destination = malloc(count * stride);
	assert(destination);

	int rc = decode(destination, count, stride, reinterpret_cast<const unsigned char*>(data), size);
	(void)rc;

	free(destination);
}

void fuzzRoundtrip(const uint8_t* data, size_t size, size_t stride, int level)
{
	size_t count = size / stride;

	size_t bound = meshopt_encodeVertexBufferBound(count, stride);
	void* encoded = malloc(bound);
	void* decoded = malloc(count * stride);
	assert(encoded && decoded);

	size_t res = meshopt_encodeVertexBufferLevel(static_cast<unsigned char*>(encoded), bound, data, count, stride, level, -1);
	assert(res > 0 && res <= bound);

	// encode again at the boundary to check for memory safety
	// this should produce the same output because encoder is deterministic
	size_t rese = meshopt_encodeVertexBufferLevel(static_cast<unsigned char*>(encoded) + bound - res, res, data, count, stride, level, -1);
	assert(rese == res);

	int rc = meshopt_decodeVertexBuffer(decoded, count, stride, static_cast<unsigned char*>(encoded) + bound - res, res);
	assert(rc == 0);

	assert(memcmp(data, decoded, count * stride) == 0);

	free(decoded);
	free(encoded);
}

size_t align(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void fuzzDecodeMeshlet(size_t vertex_count, size_t triangle_count, const unsigned char* data, size_t size)
{
	// raw decoding: allowed to write align(count, 4) elements
	unsigned int rt[256];
	unsigned int rv[256];
	meshopt_decodeMeshletRaw(rv + 256 - align(vertex_count, 4), vertex_count, rt + 256 - align(triangle_count, 4), triangle_count, data, size);

	// regular decoding: allowed to write align(count * size, 4) bytes
	// with variations for 3-byte triangles and 2-byte vertex references
	unsigned short rsv[256];
	unsigned char rbt[256 * 3];

	meshopt_decodeMeshlet(rv + 256 - vertex_count, vertex_count, 4, rt + 256 - triangle_count, triangle_count, 4, data, size);
	meshopt_decodeMeshlet(rsv + 256 - align(vertex_count, 2), vertex_count, 2, rt + 256 - triangle_count, triangle_count, 4, data, size);
	meshopt_decodeMeshlet(rv + 256 - vertex_count, vertex_count, 4, rbt + 256 * 3 - align(triangle_count * 3, 4), triangle_count, 3, data, size);
	meshopt_decodeMeshlet(rsv + 256 - align(vertex_count, 2), vertex_count, 2, rbt + 256 * 3 - align(triangle_count * 3, 4), triangle_count, 3, data, size);
}

void fuzzRoundtripMeshlet(const uint8_t* data, size_t size)
{
	size_t triangle_count = size / 3;
	if (triangle_count > 256)
		triangle_count = 256;

	unsigned char buf[4096];
	size_t enc = meshopt_encodeMeshlet(buf, sizeof(buf), NULL, 0, reinterpret_cast<const unsigned char*>(data), triangle_count);
	assert(enc > 0);
	assert(enc <= meshopt_encodeMeshletBound(0, triangle_count));

	unsigned int rt4[256];
	int rc4 = meshopt_decodeMeshlet(static_cast<unsigned int*>(NULL), 0, rt4, triangle_count, buf, enc);
	assert(rc4 == 0);

	for (size_t i = 0; i < triangle_count; ++i)
	{
		unsigned char a = data[i * 3 + 0], b = data[i * 3 + 1], c = data[i * 3 + 2];

		unsigned int abc = (a << 0) | (b << 8) | (c << 16);
		unsigned int bca = (b << 0) | (c << 8) | (a << 16);
		unsigned int cba = (c << 0) | (a << 8) | (b << 16);

		unsigned int tri = rt4[i];

		assert(tri == abc || tri == bca || tri == cba);
	}

	unsigned char rt3[256 * 3];
	int rc3 = meshopt_decodeMeshlet(static_cast<unsigned int*>(NULL), 0, rt3, triangle_count, buf, enc);
	assert(rc3 == 0);

	for (size_t i = 0; i < triangle_count; ++i)
	{
		unsigned char a = data[i * 3 + 0], b = data[i * 3 + 1], c = data[i * 3 + 2];

		unsigned int abc = (a << 0) | (b << 8) | (c << 16);
		unsigned int bca = (b << 0) | (c << 8) | (a << 16);
		unsigned int cba = (c << 0) | (a << 8) | (b << 16);

		unsigned int tri = rt3[i * 3 + 0] | (rt3[i * 3 + 1] << 8) | (rt3[i * 3 + 2] << 16);

		assert(tri == abc || tri == bca || tri == cba);
	}
}

void fuzzRoundtripMeshletV(const uint8_t* data, size_t size)
{
	size_t vertex_count = size / 4;
	if (vertex_count > 256)
		vertex_count = 256;

	unsigned char tri[4] = {0, 1, 2};

	unsigned char buf[4096];
	size_t enc = meshopt_encodeMeshlet(buf, sizeof(buf), reinterpret_cast<const uint32_t*>(data), vertex_count, tri, 1);
	assert(enc > 0);
	assert(enc <= meshopt_encodeMeshletBound(vertex_count, 1));

	unsigned int rv4[256];
	int rc4 = meshopt_decodeMeshlet(rv4, vertex_count, tri, 1, buf, enc);
	assert(rc4 == 0);

	for (size_t i = 0; i < vertex_count; ++i)
		assert(rv4[i] == reinterpret_cast<const uint32_t*>(data)[i]);

	unsigned short rv2[256];
	int rc2 = meshopt_decodeMeshlet(rv2, vertex_count, tri, 1, buf, enc);
	assert(rc2 == 0);

	for (size_t i = 0; i < vertex_count; ++i)
		assert(rv2[i] == uint16_t(reinterpret_cast<const uint32_t*>(data)[i]));
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// decodeIndexBuffer supports 2 and 4-byte indices
	fuzzDecoder(data, size, 2, meshopt_decodeIndexBuffer);
	fuzzDecoder(data, size, 4, meshopt_decodeIndexBuffer);

	// decodeIndexSequence supports 2 and 4-byte indices
	fuzzDecoder(data, size, 2, meshopt_decodeIndexSequence);
	fuzzDecoder(data, size, 4, meshopt_decodeIndexSequence);

	// decodeVertexBuffer supports any strides divisible by 4 in 4-256 interval
	// it's a waste of time to check all of them, so we'll just check a few with different alignment mod 16
	fuzzDecoder(data, size, 4, meshopt_decodeVertexBuffer);
	fuzzDecoder(data, size, 16, meshopt_decodeVertexBuffer);
	fuzzDecoder(data, size, 24, meshopt_decodeVertexBuffer);
	fuzzDecoder(data, size, 32, meshopt_decodeVertexBuffer);

	// encodeVertexBuffer/decodeVertexBuffer should roundtrip for any stride, check a few with different alignment mod 16
	// this also checks memory safety properties of the encoder
	// to conserve time, we only check one version/level combination, biased towards version 1
	uint8_t data0 = size > 0 ? data[0] : 0;
	int level = data0 % 5;

	meshopt_encodeVertexVersion(level < 4 ? 1 : 0);

	fuzzRoundtrip(data, size, 4, level);
	fuzzRoundtrip(data, size, 16, level);
	fuzzRoundtrip(data, size, 24, level);
	fuzzRoundtrip(data, size, 32, level);

	// validate that decodeMeshlet works on untrusted data and is memory safe within documented limits
	if (size > 2)
		fuzzDecodeMeshlet(data[0] + 1, data[1] + 1, reinterpret_cast<const unsigned char*>(data + 2), size - 2);

	// validate that index data roundtrips in meshlet encoding modulo rotation
	fuzzRoundtripMeshlet(data, size);

	// validate that vertex data roundtrips in meshlet encoding
	fuzzRoundtripMeshletV(data, size);

	return 0;
}
