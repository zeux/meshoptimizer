#include "../src/meshoptimizer.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

// This file uses assert() to verify algorithm correctness
#undef NDEBUG
#include <assert.h>

struct PV
{
	unsigned short px, py, pz;
	unsigned char nu, nv; // octahedron encoded normal, aliases .pw
	unsigned short tx, ty;
};

// note: 4 6 5 triangle here is a combo-breaker:
// we encode it without rotating, a=next, c=next - this means we do *not* bump next to 6
// which means that the next triangle can't be encoded via next sequencing!
static const unsigned int kIndexBuffer[] = {0, 1, 2, 2, 1, 3, 4, 6, 5, 7, 8, 9};

static const unsigned char kIndexDataV0[] = {
    0xe0, 0xf0, 0x10, 0xfe, 0xff, 0xf0, 0x0c, 0xff, 0x02, 0x02, 0x02, 0x00, 0x76, 0x87, 0x56, 0x67,
    0x78, 0xa9, 0x86, 0x65, 0x89, 0x68, 0x98, 0x01, 0x69, 0x00, 0x00, // clang-format :-/
};

// note: this exercises two features of v1 format, restarts (0 1 2) and last
static const unsigned int kIndexBufferTricky[] = {0, 1, 2, 2, 1, 3, 0, 1, 2, 2, 1, 5, 2, 1, 4};

static const unsigned char kIndexDataV1[] = {
    0xe1, 0xf0, 0x10, 0xfe, 0x1f, 0x3d, 0x00, 0x0a, 0x00, 0x76, 0x87, 0x56, 0x67, 0x78, 0xa9, 0x86,
    0x65, 0x89, 0x68, 0x98, 0x01, 0x69, 0x00, 0x00, // clang-format :-/
};

static const unsigned int kIndexSequence[] = {0, 1, 51, 2, 49, 1000};

static const unsigned char kIndexSequenceV1[] = {
    0xd1, 0x00, 0x04, 0xcd, 0x01, 0x04, 0x07, 0x98, 0x1f, 0x00, 0x00, 0x00, 0x00, // clang-format :-/
};

static const PV kVertexBuffer[] = {
    {0, 0, 0, 0, 0, 0, 0},
    {300, 0, 0, 0, 0, 500, 0},
    {0, 300, 0, 0, 0, 0, 500},
    {300, 300, 0, 0, 0, 500, 500},
};

static const unsigned char kVertexDataV0[] = {
    0xa0, 0x01, 0x3f, 0x00, 0x00, 0x00, 0x58, 0x57, 0x58, 0x01, 0x26, 0x00, 0x00, 0x00, 0x01, 0x0c,
    0x00, 0x00, 0x00, 0x58, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x3f, 0x00,
    0x00, 0x00, 0x17, 0x18, 0x17, 0x01, 0x26, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x00, 0x00, 0x00, 0x17,
    0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, // clang-format :-/
};

static const unsigned char kVertexDataV1[] = {
    0xa1, 0xee, 0xaa, 0xee, 0x00, 0x4b, 0x4b, 0x4b, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x7d, 0x7d, 0x7d,
    0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0x62, // clang-format :-/
};

// This binary blob is a valid v1 encoding of vertex buffer but it used a custom version of
// the encoder that exercised all features of the format; because of this it is much larger
// and will never be produced by the encoder itself.
static const unsigned char kVertexDataV1Custom[] = {
    0xa1, 0xd4, 0x94, 0xd4, 0x01, 0x0e, 0x00, 0x58, 0x57, 0x58, 0x02, 0x02, 0x12, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0e, 0x00, 0x7d, 0x7d, 0x7d, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x62, // clang-format :-/
};

static void decodeIndexV0()
{
	const size_t index_count = sizeof(kIndexBuffer) / sizeof(kIndexBuffer[0]);

	std::vector<unsigned char> buffer(kIndexDataV0, kIndexDataV0 + sizeof(kIndexDataV0));

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexBuffer(decoded, index_count, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, kIndexBuffer, sizeof(kIndexBuffer)) == 0);
}

static void decodeIndexV1()
{
	const size_t index_count = sizeof(kIndexBufferTricky) / sizeof(kIndexBufferTricky[0]);

	std::vector<unsigned char> buffer(kIndexDataV1, kIndexDataV1 + sizeof(kIndexDataV1));

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexBuffer(decoded, index_count, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, kIndexBufferTricky, sizeof(kIndexBufferTricky)) == 0);
}

static void decodeIndex16()
{
	const size_t index_count = sizeof(kIndexBuffer) / sizeof(kIndexBuffer[0]);
	const size_t vertex_count = 10;

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), kIndexBuffer, index_count));

	unsigned short decoded[index_count];
	assert(meshopt_decodeIndexBuffer(decoded, index_count, &buffer[0], buffer.size()) == 0);

	for (size_t i = 0; i < index_count; ++i)
		assert(decoded[i] == kIndexBuffer[i]);
}

static void encodeIndexMemorySafe()
{
	const size_t index_count = sizeof(kIndexBuffer) / sizeof(kIndexBuffer[0]);
	const size_t vertex_count = 10;

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), kIndexBuffer, index_count));

	// check that encode is memory-safe; note that we reallocate the buffer for each try to make sure ASAN can verify buffer access
	for (size_t i = 0; i <= buffer.size(); ++i)
	{
		std::vector<unsigned char> shortbuffer(i);
		size_t result = meshopt_encodeIndexBuffer(i == 0 ? NULL : &shortbuffer[0], i, kIndexBuffer, index_count);

		if (i == buffer.size())
			assert(result == buffer.size());
		else
			assert(result == 0);
	}
}

static void decodeIndexMemorySafe()
{
	const size_t index_count = sizeof(kIndexBuffer) / sizeof(kIndexBuffer[0]);
	const size_t vertex_count = 10;

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), kIndexBuffer, index_count));

	// check that decode is memory-safe; note that we reallocate the buffer for each try to make sure ASAN can verify buffer access
	unsigned int decoded[index_count];

	for (size_t i = 0; i <= buffer.size(); ++i)
	{
		std::vector<unsigned char> shortbuffer(buffer.begin(), buffer.begin() + i);
		int result = meshopt_decodeIndexBuffer(decoded, index_count, i == 0 ? NULL : &shortbuffer[0], i);

		if (i == buffer.size())
			assert(result == 0);
		else
			assert(result < 0);
	}
}

static void decodeIndexRejectExtraBytes()
{
	const size_t index_count = sizeof(kIndexBuffer) / sizeof(kIndexBuffer[0]);
	const size_t vertex_count = 10;

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), kIndexBuffer, index_count));

	// check that decoder doesn't accept extra bytes after a valid stream
	std::vector<unsigned char> largebuffer(buffer);
	largebuffer.push_back(0);

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexBuffer(decoded, index_count, &largebuffer[0], largebuffer.size()) < 0);
}

static void decodeIndexRejectMalformedHeaders()
{
	const size_t index_count = sizeof(kIndexBuffer) / sizeof(kIndexBuffer[0]);
	const size_t vertex_count = 10;

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), kIndexBuffer, index_count));

	// check that decoder doesn't accept malformed headers
	std::vector<unsigned char> brokenbuffer(buffer);
	brokenbuffer[0] = 0;

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexBuffer(decoded, index_count, &brokenbuffer[0], brokenbuffer.size()) < 0);
}

static void decodeIndexRejectInvalidVersion()
{
	const size_t index_count = sizeof(kIndexBuffer) / sizeof(kIndexBuffer[0]);
	const size_t vertex_count = 10;

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), kIndexBuffer, index_count));

	// check that decoder doesn't accept invalid version
	std::vector<unsigned char> brokenbuffer(buffer);
	brokenbuffer[0] |= 0x0f;

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexBuffer(decoded, index_count, &brokenbuffer[0], brokenbuffer.size()) < 0);
}

static void decodeIndexMalformedVByte()
{
	const unsigned char input[] = {
	    0xe1, 0x20, 0x20, 0x20, 0xff, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	    0xff, 0xff, 0xff, 0xff, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	    0x20, 0x20, 0x20, // clang-format :-/
	};

	unsigned int decoded[66];
	assert(meshopt_decodeIndexBuffer(decoded, 66, input, sizeof(input)) < 0);
}

static void roundtripIndexTricky()
{
	const size_t index_count = sizeof(kIndexBufferTricky) / sizeof(kIndexBufferTricky[0]);
	const size_t vertex_count = 6;

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), kIndexBufferTricky, index_count));

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexBuffer(decoded, index_count, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, kIndexBufferTricky, sizeof(kIndexBufferTricky)) == 0);
}

static void encodeIndexEmpty()
{
	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(0, 0));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), NULL, 0));

	assert(meshopt_decodeIndexBuffer(static_cast<unsigned int*>(NULL), 0, &buffer[0], buffer.size()) == 0);
}

static void decodeIndexSequence()
{
	const size_t index_count = sizeof(kIndexSequence) / sizeof(kIndexSequence[0]);

	std::vector<unsigned char> buffer(kIndexSequenceV1, kIndexSequenceV1 + sizeof(kIndexSequenceV1));

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexSequence(decoded, index_count, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, kIndexSequence, sizeof(kIndexSequence)) == 0);
}

static void decodeIndexSequence16()
{
	const size_t index_count = sizeof(kIndexSequence) / sizeof(kIndexSequence[0]);
	const size_t vertex_count = 1001;

	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), kIndexSequence, index_count));

	unsigned short decoded[index_count];
	assert(meshopt_decodeIndexSequence(decoded, index_count, &buffer[0], buffer.size()) == 0);

	for (size_t i = 0; i < index_count; ++i)
		assert(decoded[i] == kIndexSequence[i]);
}

static void encodeIndexSequenceMemorySafe()
{
	const size_t index_count = sizeof(kIndexSequence) / sizeof(kIndexSequence[0]);
	const size_t vertex_count = 1001;

	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), kIndexSequence, index_count));

	// check that encode is memory-safe; note that we reallocate the buffer for each try to make sure ASAN can verify buffer access
	for (size_t i = 0; i <= buffer.size(); ++i)
	{
		std::vector<unsigned char> shortbuffer(i);
		size_t result = meshopt_encodeIndexSequence(i == 0 ? NULL : &shortbuffer[0], i, kIndexSequence, index_count);

		if (i == buffer.size())
			assert(result == buffer.size());
		else
			assert(result == 0);
	}
}

static void decodeIndexSequenceMemorySafe()
{
	const size_t index_count = sizeof(kIndexSequence) / sizeof(kIndexSequence[0]);
	const size_t vertex_count = 1001;

	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), kIndexSequence, index_count));

	// check that decode is memory-safe; note that we reallocate the buffer for each try to make sure ASAN can verify buffer access
	unsigned int decoded[index_count];

	for (size_t i = 0; i <= buffer.size(); ++i)
	{
		std::vector<unsigned char> shortbuffer(buffer.begin(), buffer.begin() + i);
		int result = meshopt_decodeIndexSequence(decoded, index_count, i == 0 ? NULL : &shortbuffer[0], i);

		if (i == buffer.size())
			assert(result == 0);
		else
			assert(result < 0);
	}
}

static void decodeIndexSequenceRejectExtraBytes()
{
	const size_t index_count = sizeof(kIndexSequence) / sizeof(kIndexSequence[0]);
	const size_t vertex_count = 1001;

	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), kIndexSequence, index_count));

	// check that decoder doesn't accept extra bytes after a valid stream
	std::vector<unsigned char> largebuffer(buffer);
	largebuffer.push_back(0);

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexSequence(decoded, index_count, &largebuffer[0], largebuffer.size()) < 0);
}

static void decodeIndexSequenceRejectMalformedHeaders()
{
	const size_t index_count = sizeof(kIndexSequence) / sizeof(kIndexSequence[0]);
	const size_t vertex_count = 1001;

	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), kIndexSequence, index_count));

	// check that decoder doesn't accept malformed headers
	std::vector<unsigned char> brokenbuffer(buffer);
	brokenbuffer[0] = 0;

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexSequence(decoded, index_count, &brokenbuffer[0], brokenbuffer.size()) < 0);
}

static void decodeIndexSequenceRejectInvalidVersion()
{
	const size_t index_count = sizeof(kIndexSequence) / sizeof(kIndexSequence[0]);
	const size_t vertex_count = 1001;

	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(index_count, vertex_count));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), kIndexSequence, index_count));

	// check that decoder doesn't accept invalid version
	std::vector<unsigned char> brokenbuffer(buffer);
	brokenbuffer[0] |= 0x0f;

	unsigned int decoded[index_count];
	assert(meshopt_decodeIndexSequence(decoded, index_count, &brokenbuffer[0], brokenbuffer.size()) < 0);
}

static void encodeIndexSequenceEmpty()
{
	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(0, 0));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), NULL, 0));

	assert(meshopt_decodeIndexSequence(static_cast<unsigned int*>(NULL), 0, &buffer[0], buffer.size()) == 0);
}

static void decodeVertexV0()
{
	const size_t vertex_count = sizeof(kVertexBuffer) / sizeof(kVertexBuffer[0]);

	std::vector<unsigned char> buffer(kVertexDataV0, kVertexDataV0 + sizeof(kVertexDataV0));

	PV decoded[vertex_count];
	assert(meshopt_decodeVertexBuffer(decoded, vertex_count, sizeof(PV), &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, kVertexBuffer, sizeof(kVertexBuffer)) == 0);
}

static void decodeVertexV1()
{
	const size_t vertex_count = sizeof(kVertexBuffer) / sizeof(kVertexBuffer[0]);

	std::vector<unsigned char> buffer(kVertexDataV1, kVertexDataV1 + sizeof(kVertexDataV1));

	PV decoded[vertex_count];
	assert(meshopt_decodeVertexBuffer(decoded, vertex_count, sizeof(PV), &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, kVertexBuffer, sizeof(kVertexBuffer)) == 0);
}

static void decodeVertexV1Custom()
{
	const size_t vertex_count = sizeof(kVertexBuffer) / sizeof(kVertexBuffer[0]);

	std::vector<unsigned char> buffer(kVertexDataV1Custom, kVertexDataV1Custom + sizeof(kVertexDataV1Custom));

	PV decoded[vertex_count];
	assert(meshopt_decodeVertexBuffer(decoded, vertex_count, sizeof(PV), &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, kVertexBuffer, sizeof(kVertexBuffer)) == 0);
}

static void encodeVertexMemorySafe()
{
	const size_t vertex_count = sizeof(kVertexBuffer) / sizeof(kVertexBuffer[0]);

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(vertex_count, sizeof(PV)));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), kVertexBuffer, vertex_count, sizeof(PV)));

	// check that encode is memory-safe; note that we reallocate the buffer for each try to make sure ASAN can verify buffer access
	for (size_t i = 0; i <= buffer.size(); ++i)
	{
		std::vector<unsigned char> shortbuffer(i);
		size_t result = meshopt_encodeVertexBuffer(i == 0 ? NULL : &shortbuffer[0], i, kVertexBuffer, vertex_count, sizeof(PV));

		if (i == buffer.size())
			assert(result == buffer.size());
		else
			assert(result == 0);
	}
}

static void decodeVertexMemorySafe()
{
	const size_t vertex_count = sizeof(kVertexBuffer) / sizeof(kVertexBuffer[0]);

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(vertex_count, sizeof(PV)));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), kVertexBuffer, vertex_count, sizeof(PV)));

	// check that decode is memory-safe; note that we reallocate the buffer for each try to make sure ASAN can verify buffer access
	PV decoded[vertex_count];

	for (size_t i = 0; i <= buffer.size(); ++i)
	{
		std::vector<unsigned char> shortbuffer(buffer.begin(), buffer.begin() + i);
		int result = meshopt_decodeVertexBuffer(decoded, vertex_count, sizeof(PV), i == 0 ? NULL : &shortbuffer[0], i);
		(void)result;

		if (i == buffer.size())
			assert(result == 0);
		else
			assert(result < 0);
	}
}

static void decodeVertexRejectExtraBytes()
{
	const size_t vertex_count = sizeof(kVertexBuffer) / sizeof(kVertexBuffer[0]);

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(vertex_count, sizeof(PV)));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), kVertexBuffer, vertex_count, sizeof(PV)));

	// check that decoder doesn't accept extra bytes after a valid stream
	std::vector<unsigned char> largebuffer(buffer);
	largebuffer.push_back(0);

	PV decoded[vertex_count];
	assert(meshopt_decodeVertexBuffer(decoded, vertex_count, sizeof(PV), &largebuffer[0], largebuffer.size()) < 0);
}

static void decodeVertexRejectMalformedHeaders()
{
	const size_t vertex_count = sizeof(kVertexBuffer) / sizeof(kVertexBuffer[0]);

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(vertex_count, sizeof(PV)));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), kVertexBuffer, vertex_count, sizeof(PV)));

	// check that decoder doesn't accept malformed headers
	std::vector<unsigned char> brokenbuffer(buffer);
	brokenbuffer[0] = 0;

	PV decoded[vertex_count];
	assert(meshopt_decodeVertexBuffer(decoded, vertex_count, sizeof(PV), &brokenbuffer[0], brokenbuffer.size()) < 0);
}

static void decodeVertexBitGroups()
{
	unsigned char data[16 * 4];

	// this tests 0/2/4/8 bit groups in one stream
	for (size_t i = 0; i < 16; ++i)
	{
		data[i * 4 + 0] = 0;
		data[i * 4 + 1] = (unsigned char)(i * 1);
		data[i * 4 + 2] = (unsigned char)(i * 2);
		data[i * 4 + 3] = (unsigned char)(i * 8);
	}

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(16, 4));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), data, 16, 4));

	unsigned char decoded[16 * 4];
	assert(meshopt_decodeVertexBuffer(decoded, 16, 4, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, data, sizeof(data)) == 0);
}

static void decodeVertexBitGroupSentinels()
{
	unsigned char data[16 * 4];

	// this tests 0/2/4/8 bit groups and sentinels in one stream
	for (size_t i = 0; i < 16; ++i)
	{
		if (i == 7 || i == 13)
		{
			data[i * 4 + 0] = 42;
			data[i * 4 + 1] = 42;
			data[i * 4 + 2] = 42;
			data[i * 4 + 3] = 42;
		}
		else
		{
			data[i * 4 + 0] = 0;
			data[i * 4 + 1] = (unsigned char)(i * 1);
			data[i * 4 + 2] = (unsigned char)(i * 2);
			data[i * 4 + 3] = (unsigned char)(i * 8);
		}
	}

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(16, 4));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), data, 16, 4));

	unsigned char decoded[16 * 4];
	assert(meshopt_decodeVertexBuffer(decoded, 16, 4, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, data, sizeof(data)) == 0);
}

static void decodeVertexDeltas()
{
	unsigned short data[16 * 4];

	// this forces wider deltas by using values that cross byte boundary
	for (size_t i = 0; i < 16; ++i)
	{
		data[i * 4 + 0] = (unsigned short)(0xf8 + i * 1);
		data[i * 4 + 1] = (unsigned short)(0xf8 + i * 2);
		data[i * 4 + 2] = (unsigned short)(0xf0 + i * 3);
		data[i * 4 + 3] = (unsigned short)(0xf0 + i * 4);
	}

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(16, 8));
	buffer.resize(meshopt_encodeVertexBufferLevel(&buffer[0], buffer.size(), data, 16, 8, 2));

	unsigned short decoded[16 * 4];
	assert(meshopt_decodeVertexBuffer(decoded, 16, 8, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, data, sizeof(data)) == 0);
}

static void decodeVertexBitXor()
{
	unsigned int data[16 * 4];

	// this forces xors by using bit values at an offset
	for (size_t i = 0; i < 16; ++i)
	{
		data[i * 4 + 0] = unsigned(i << 0);
		data[i * 4 + 1] = unsigned(i << 2);
		data[i * 4 + 2] = unsigned(i << 15);
		data[i * 4 + 3] = unsigned(i << 28);
	}

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(16, 16));
	buffer.resize(meshopt_encodeVertexBufferLevel(&buffer[0], buffer.size(), data, 16, 16, 3));

	unsigned int decoded[16 * 4];
	assert(meshopt_decodeVertexBuffer(decoded, 16, 16, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, data, sizeof(data)) == 0);
}

static void decodeVertexLarge()
{
	unsigned char data[128 * 4];

	// this tests 0/2/4/8 bit groups in one stream
	for (size_t i = 0; i < 128; ++i)
	{
		data[i * 4 + 0] = 0;
		data[i * 4 + 1] = (unsigned char)(i * 1);
		data[i * 4 + 2] = (unsigned char)(i * 2);
		data[i * 4 + 3] = (unsigned char)(i * 8);
	}

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(128, 4));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), data, 128, 4));

	unsigned char decoded[128 * 4];
	assert(meshopt_decodeVertexBuffer(decoded, 128, 4, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, data, sizeof(data)) == 0);
}

static void decodeVertexSmall()
{
	unsigned char data[13 * 4];

	// this tests 0/2/4/8 bit groups in one stream
	for (size_t i = 0; i < 13; ++i)
	{
		data[i * 4 + 0] = 0;
		data[i * 4 + 1] = (unsigned char)(i * 1);
		data[i * 4 + 2] = (unsigned char)(i * 2);
		data[i * 4 + 3] = (unsigned char)(i * 8);
	}

	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(13, 4));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), data, 13, 4));

	unsigned char decoded[13 * 4];
	assert(meshopt_decodeVertexBuffer(decoded, 13, 4, &buffer[0], buffer.size()) == 0);
	assert(memcmp(decoded, data, sizeof(data)) == 0);
}

static void encodeVertexEmpty()
{
	std::vector<unsigned char> buffer(meshopt_encodeVertexBufferBound(0, 16));
	buffer.resize(meshopt_encodeVertexBuffer(&buffer[0], buffer.size(), NULL, 0, 16));

	assert(meshopt_decodeVertexBuffer(NULL, 0, 16, &buffer[0], buffer.size()) == 0);
}

static void decodeFilterOct8()
{
	const unsigned char data[4 * 4] = {
	    0, 1, 127, 0,
	    0, 187, 127, 1,
	    255, 1, 127, 0,
	    14, 130, 127, 1, // clang-format :-/
	};

	const unsigned char expected[4 * 4] = {
	    0, 1, 127, 0,
	    0, 159, 82, 1,
	    255, 1, 127, 0,
	    1, 130, 241, 1, // clang-format :-/
	};

	// Aligned by 4
	unsigned char full[4 * 4];
	memcpy(full, data, sizeof(full));
	meshopt_decodeFilterOct(full, 4, 4);
	assert(memcmp(full, expected, sizeof(full)) == 0);

	// Tail processing for unaligned data
	unsigned char tail[3 * 4];
	memcpy(tail, data, sizeof(tail));
	meshopt_decodeFilterOct(tail, 3, 4);
	assert(memcmp(tail, expected, sizeof(tail)) == 0);
}

static void decodeFilterOct12()
{
	const unsigned short data[4 * 4] = {
	    0, 1, 2047, 0,
	    0, 1870, 2047, 1,
	    2017, 1, 2047, 0,
	    14, 1300, 2047, 1, // clang-format :-/
	};

	const unsigned short expected[4 * 4] = {
	    0, 16, 32767, 0,
	    0, 32621, 3088, 1,
	    32764, 16, 471, 0,
	    307, 28541, 16093, 1, // clang-format :-/
	};

	// Aligned by 4
	unsigned short full[4 * 4];
	memcpy(full, data, sizeof(full));
	meshopt_decodeFilterOct(full, 4, 8);
	assert(memcmp(full, expected, sizeof(full)) == 0);

	// Tail processing for unaligned data
	unsigned short tail[3 * 4];
	memcpy(tail, data, sizeof(tail));
	meshopt_decodeFilterOct(tail, 3, 8);
	assert(memcmp(tail, expected, sizeof(tail)) == 0);
}

static void decodeFilterQuat12()
{
	const unsigned short data[4 * 4] = {
	    0, 1, 0, 0x7fc,
	    0, 1870, 0, 0x7fd,
	    2017, 1, 0, 0x7fe,
	    14, 1300, 0, 0x7ff, // clang-format :-/
	};

	const unsigned short expected[4 * 4] = {
	    32767, 0, 11, 0,
	    0, 25013, 0, 21166,
	    11, 0, 23504, 22830,
	    158, 14715, 0, 29277, // clang-format :-/
	};

	// Aligned by 4
	unsigned short full[4 * 4];
	memcpy(full, data, sizeof(full));
	meshopt_decodeFilterQuat(full, 4, 8);
	assert(memcmp(full, expected, sizeof(full)) == 0);

	// Tail processing for unaligned data
	unsigned short tail[3 * 4];
	memcpy(tail, data, sizeof(tail));
	meshopt_decodeFilterQuat(tail, 3, 8);
	assert(memcmp(tail, expected, sizeof(tail)) == 0);
}

static void decodeFilterExp()
{
	const unsigned int data[4] = {
	    0,
	    0xff000003,
	    0x02fffff7,
	    0xfe7fffff, // clang-format :-/
	};

	const unsigned int expected[4] = {
	    0,
	    0x3fc00000,
	    0xc2100000,
	    0x49fffffe, // clang-format :-/
	};

	// Aligned by 4
	unsigned int full[4];
	memcpy(full, data, sizeof(full));
	meshopt_decodeFilterExp(full, 4, 4);
	assert(memcmp(full, expected, sizeof(full)) == 0);

	// Tail processing for unaligned data
	unsigned int tail[3];
	memcpy(tail, data, sizeof(tail));
	meshopt_decodeFilterExp(tail, 3, 4);
	assert(memcmp(tail, expected, sizeof(tail)) == 0);
}

static void encodeFilterOct8()
{
	const float data[4 * 4] = {
	    1, 0, 0, 0,
	    0, -1, 0, 0,
	    0.7071068f, 0, 0.707168f, 1,
	    -0.7071068f, 0, -0.707168f, 1, // clang-format :-/
	};

	const unsigned char expected[4 * 4] = {
	    0x7f, 0, 0x7f, 0,
	    0, 0x81, 0x7f, 0,
	    0x3f, 0, 0x7f, 0x7f,
	    0x81, 0x40, 0x7f, 0x7f, // clang-format :-/
	};

	unsigned char encoded[4 * 4];
	meshopt_encodeFilterOct(encoded, 4, 4, 8, data);

	assert(memcmp(encoded, expected, sizeof(expected)) == 0);

	signed char decoded[4 * 4];
	memcpy(decoded, encoded, sizeof(decoded));
	meshopt_decodeFilterOct(decoded, 4, 4);

	for (size_t i = 0; i < 4 * 4; ++i)
		assert(fabsf(decoded[i] / 127.f - data[i]) < 1e-2f);
}

static void encodeFilterOct12()
{
	const float data[4 * 4] = {
	    1, 0, 0, 0,
	    0, -1, 0, 0,
	    0.7071068f, 0, 0.707168f, 1,
	    -0.7071068f, 0, -0.707168f, 1, // clang-format :-/
	};

	const unsigned short expected[4 * 4] = {
	    0x7ff, 0, 0x7ff, 0,
	    0x0, 0xf801, 0x7ff, 0,
	    0x3ff, 0, 0x7ff, 0x7fff,
	    0xf801, 0x400, 0x7ff, 0x7fff, // clang-format :-/
	};

	unsigned short encoded[4 * 4];
	meshopt_encodeFilterOct(encoded, 4, 8, 12, data);

	assert(memcmp(encoded, expected, sizeof(expected)) == 0);

	short decoded[4 * 4];
	memcpy(decoded, encoded, sizeof(decoded));
	meshopt_decodeFilterOct(decoded, 4, 8);

	for (size_t i = 0; i < 4 * 4; ++i)
		assert(fabsf(decoded[i] / 32767.f - data[i]) < 1e-3f);
}

static void encodeFilterQuat12()
{
	const float data[4 * 4] = {
	    1, 0, 0, 0,
	    0, -1, 0, 0,
	    0.7071068f, 0, 0, 0.707168f,
	    -0.7071068f, 0, 0, -0.707168f, // clang-format :-/
	};

	const unsigned short expected[4 * 4] = {
	    0, 0, 0, 0x7fc,
	    0, 0, 0, 0x7fd,
	    0x7ff, 0, 0, 0x7ff,
	    0x7ff, 0, 0, 0x7ff, // clang-format :-/
	};

	unsigned short encoded[4 * 4];
	meshopt_encodeFilterQuat(encoded, 4, 8, 12, data);

	assert(memcmp(encoded, expected, sizeof(expected)) == 0);

	short decoded[4 * 4];
	memcpy(decoded, encoded, sizeof(decoded));
	meshopt_decodeFilterQuat(decoded, 4, 8);

	for (size_t i = 0; i < 4; ++i)
	{
		float dx = decoded[i * 4 + 0] / 32767.f;
		float dy = decoded[i * 4 + 1] / 32767.f;
		float dz = decoded[i * 4 + 2] / 32767.f;
		float dw = decoded[i * 4 + 3] / 32767.f;

		float dp =
		    data[i * 4 + 0] * dx +
		    data[i * 4 + 1] * dy +
		    data[i * 4 + 2] * dz +
		    data[i * 4 + 3] * dw;

		assert(fabsf(fabsf(dp) - 1.f) < 1e-4f);
	}
}

static void encodeFilterExp()
{
	const float data[4] = {
	    1,
	    -23.4f,
	    -0.1f,
	    11.0f,
	};

	// separate exponents: each component gets its own value
	const unsigned int expected1[4] = {
	    0xf3002000,
	    0xf7ffd133,
	    0xefffcccd,
	    0xf6002c00,
	};

	// shared exponents (vector): all components of each vector get the same value
	const unsigned int expected2[4] = {
	    0xf7000200,
	    0xf7ffd133,
	    0xf6ffff9a,
	    0xf6002c00,
	};

	// shared exponents (component): each component gets the same value across all vectors
	const unsigned int expected3[4] = {
	    0xf3002000,
	    0xf7ffd133,
	    0xf3fffccd,
	    0xf7001600,
	};

	unsigned int encoded1[4];
	meshopt_encodeFilterExp(encoded1, 2, 8, 15, data, meshopt_EncodeExpSeparate);

	unsigned int encoded2[4];
	meshopt_encodeFilterExp(encoded2, 2, 8, 15, data, meshopt_EncodeExpSharedVector);

	unsigned int encoded3[4];
	meshopt_encodeFilterExp(encoded3, 2, 8, 15, data, meshopt_EncodeExpSharedComponent);

	assert(memcmp(encoded1, expected1, sizeof(expected1)) == 0);
	assert(memcmp(encoded2, expected2, sizeof(expected2)) == 0);
	assert(memcmp(encoded3, expected3, sizeof(expected3)) == 0);

	float decoded1[4];
	memcpy(decoded1, encoded1, sizeof(decoded1));
	meshopt_decodeFilterExp(decoded1, 2, 8);

	float decoded2[4];
	memcpy(decoded2, encoded2, sizeof(decoded2));
	meshopt_decodeFilterExp(decoded2, 2, 8);

	float decoded3[4];
	memcpy(decoded3, encoded3, sizeof(decoded3));
	meshopt_decodeFilterExp(decoded3, 2, 8);

	for (size_t i = 0; i < 4; ++i)
	{
		assert(fabsf(decoded1[i] - data[i]) < 1e-3f);
		assert(fabsf(decoded2[i] - data[i]) < 1e-3f);
		assert(fabsf(decoded3[i] - data[i]) < 1e-3f);
	}
}

static void encodeFilterExpZero()
{
	const float data[4] = {
	    0.f,
	    -0.f,
	    1.1754944e-38f,
	    -1.1754944e-38f,
	};
	const unsigned int expected[4] = {
	    0xf2000000,
	    0xf2000000,
	    0x8e000000,
	    0x8e000000,
	};

	unsigned int encoded[4];
	meshopt_encodeFilterExp(encoded, 4, 4, 15, data, meshopt_EncodeExpSeparate);

	assert(memcmp(encoded, expected, sizeof(expected)) == 0);

	float decoded[4];
	memcpy(decoded, encoded, sizeof(decoded));
	meshopt_decodeFilterExp(&decoded, 4, 4);

	for (size_t i = 0; i < 4; ++i)
		assert(decoded[i] == 0);
}

static void encodeFilterExpAlias()
{
	const float data[4] = {
	    1,
	    -23.4f,
	    -0.1f,
	    11.0f,
	};

	// separate exponents: each component gets its own value
	const unsigned int expected1[4] = {
	    0xf3002000,
	    0xf7ffd133,
	    0xefffcccd,
	    0xf6002c00,
	};

	// shared exponents (vector): all components of each vector get the same value
	const unsigned int expected2[4] = {
	    0xf7000200,
	    0xf7ffd133,
	    0xf6ffff9a,
	    0xf6002c00,
	};

	// shared exponents (component): each component gets the same value across all vectors
	const unsigned int expected3[4] = {
	    0xf3002000,
	    0xf7ffd133,
	    0xf3fffccd,
	    0xf7001600,
	};

	unsigned int encoded1[4];
	memcpy(encoded1, data, sizeof(data));
	meshopt_encodeFilterExp(encoded1, 2, 8, 15, reinterpret_cast<float*>(encoded1), meshopt_EncodeExpSeparate);

	unsigned int encoded2[4];
	memcpy(encoded2, data, sizeof(data));
	meshopt_encodeFilterExp(encoded2, 2, 8, 15, reinterpret_cast<float*>(encoded2), meshopt_EncodeExpSharedVector);

	unsigned int encoded3[4];
	memcpy(encoded3, data, sizeof(data));
	meshopt_encodeFilterExp(encoded3, 2, 8, 15, reinterpret_cast<float*>(encoded3), meshopt_EncodeExpSharedComponent);

	assert(memcmp(encoded1, expected1, sizeof(expected1)) == 0);
	assert(memcmp(encoded2, expected2, sizeof(expected2)) == 0);
	assert(memcmp(encoded3, expected3, sizeof(expected3)) == 0);
}

static void encodeFilterExpClamp()
{
	const float data[4] = {
	    1,
	    -23.4f,
	    -0.1f,
	    11.0f,
	};

	// separate exponents: each component gets its own value
	// note: third value is exponent clamped
	const unsigned int expected[4] = {
	    0xf3002000,
	    0xf7ffd133,
	    0xf2fff99a,
	    0xf6002c00,
	};

	unsigned int encoded[4];
	meshopt_encodeFilterExp(encoded, 2, 8, 15, data, meshopt_EncodeExpClamped);

	assert(memcmp(encoded, expected, sizeof(expected)) == 0);

	float decoded[4];
	memcpy(decoded, encoded, sizeof(decoded));
	meshopt_decodeFilterExp(decoded, 2, 8);

	for (size_t i = 0; i < 4; ++i)
		assert(fabsf(decoded[i] - data[i]) < 1e-3f);
}

static void clusterBoundsDegenerate()
{
	const float vbd[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	const unsigned int ibd[] = {0, 0, 0};
	const unsigned int ib1[] = {0, 1, 2};

	// all of the bounds below are degenerate as they use 0 triangles, one topology-degenerate triangle and one position-degenerate triangle respectively
	meshopt_Bounds bounds0 = meshopt_computeClusterBounds(NULL, 0, NULL, 0, 12);
	meshopt_Bounds boundsd = meshopt_computeClusterBounds(ibd, 3, vbd, 3, 12);
	meshopt_Bounds bounds1 = meshopt_computeClusterBounds(ib1, 3, vbd, 3, 12);

	assert(bounds0.center[0] == 0 && bounds0.center[1] == 0 && bounds0.center[2] == 0 && bounds0.radius == 0);
	assert(boundsd.center[0] == 0 && boundsd.center[1] == 0 && boundsd.center[2] == 0 && boundsd.radius == 0);
	assert(bounds1.center[0] == 0 && bounds1.center[1] == 0 && bounds1.center[2] == 0 && bounds1.radius == 0);

	const float vb1[] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
	const unsigned int ib2[] = {0, 1, 2, 0, 2, 1};

	// these bounds have a degenerate cone since the cluster has two triangles with opposite normals
	meshopt_Bounds bounds2 = meshopt_computeClusterBounds(ib2, 6, vb1, 3, 12);

	assert(bounds2.cone_apex[0] == 0 && bounds2.cone_apex[1] == 0 && bounds2.cone_apex[2] == 0);
	assert(bounds2.cone_axis[0] == 0 && bounds2.cone_axis[1] == 0 && bounds2.cone_axis[2] == 0);
	assert(bounds2.cone_cutoff == 1);
	assert(bounds2.cone_axis_s8[0] == 0 && bounds2.cone_axis_s8[1] == 0 && bounds2.cone_axis_s8[2] == 0);
	assert(bounds2.cone_cutoff_s8 == 127);

	// however, the bounding sphere needs to be in tact (here we only check bbox for simplicity)
	assert(bounds2.center[0] - bounds2.radius <= 0 && bounds2.center[0] + bounds2.radius >= 1);
	assert(bounds2.center[1] - bounds2.radius <= 0 && bounds2.center[1] + bounds2.radius >= 1);
	assert(bounds2.center[2] - bounds2.radius <= 0 && bounds2.center[2] + bounds2.radius >= 1);
}

static void meshletsDense()
{
	const float vbd[4 * 3] = {};
	const unsigned int ibd[6] = {0, 2, 1, 1, 2, 3};

	meshopt_Meshlet ml[1];
	unsigned int mv[4];
	unsigned char mt[8];
	size_t mc = meshopt_buildMeshlets(ml, mv, mt, ibd, 6, vbd, 4, sizeof(float) * 3, 64, 64, 0.f);

	assert(mc == 1);
	assert(ml[0].triangle_count == 2);
	assert(ml[0].vertex_count == 4);

	unsigned int tri0[3] = {mv[mt[0]], mv[mt[1]], mv[mt[2]]};
	unsigned int tri1[3] = {mv[mt[3]], mv[mt[4]], mv[mt[5]]};

	// technically triangles could also be flipped in the meshlet but for now just assume they aren't
	assert(memcmp(tri0, ibd + 0, 3 * sizeof(unsigned int)) == 0);
	assert(memcmp(tri1, ibd + 3, 3 * sizeof(unsigned int)) == 0);
}

static void meshletsSparse()
{
	const float vbd[16 * 3] = {};
	const unsigned int ibd[6] = {0, 7, 15, 15, 7, 3};

	meshopt_Meshlet ml[1];
	unsigned int mv[4];
	unsigned char mt[8];
	size_t mc = meshopt_buildMeshlets(ml, mv, mt, ibd, 6, vbd, 16, sizeof(float) * 3, 64, 64, 0.f);

	assert(mc == 1);
	assert(ml[0].triangle_count == 2);
	assert(ml[0].vertex_count == 4);

	unsigned int tri0[3] = {mv[mt[0]], mv[mt[1]], mv[mt[2]]};
	unsigned int tri1[3] = {mv[mt[3]], mv[mt[4]], mv[mt[5]]};

	// technically triangles could also be flipped in the meshlet but for now just assume they aren't
	assert(memcmp(tri0, ibd + 0, 3 * sizeof(unsigned int)) == 0);
	assert(memcmp(tri1, ibd + 3, 3 * sizeof(unsigned int)) == 0);
}

static size_t allocCount;
static size_t freeCount;

static void* customAlloc(size_t size)
{
	allocCount++;

	return malloc(size);
}

static void customFree(void* ptr)
{
	freeCount++;

	free(ptr);
}

static void customAllocator()
{
	meshopt_setAllocator(customAlloc, customFree);

	assert(allocCount == 0 && freeCount == 0);

	float vb[] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
	unsigned int ib[] = {0, 1, 2};
	unsigned short ibs[] = {0, 1, 2};

	// meshopt_computeClusterBounds doesn't allocate
	meshopt_computeClusterBounds(ib, 3, vb, 3, 12);
	assert(allocCount == 0 && freeCount == 0);

	// ... unless IndexAdapter is used
	meshopt_computeClusterBounds(ibs, 3, vb, 3, 12);
	assert(allocCount == 1 && freeCount == 1);

	// meshopt_optimizeVertexFetch allocates internal remap table and temporary storage for in-place remaps
	meshopt_optimizeVertexFetch(vb, ib, 3, vb, 3, 12);
	assert(allocCount == 3 && freeCount == 3);

	// ... plus one for IndexAdapter
	meshopt_optimizeVertexFetch(vb, ibs, 3, vb, 3, 12);
	assert(allocCount == 6 && freeCount == 6);

	meshopt_setAllocator(operator new, operator delete);

	// customAlloc & customFree should not get called anymore
	meshopt_optimizeVertexFetch(vb, ib, 3, vb, 3, 12);
	assert(allocCount == 6 && freeCount == 6);

	allocCount = freeCount = 0;
}

static void emptyMesh()
{
	meshopt_optimizeVertexCache(NULL, NULL, 0, 0);
	meshopt_optimizeVertexCacheFifo(NULL, NULL, 0, 0, 16);
	meshopt_optimizeOverdraw(NULL, NULL, 0, NULL, 0, 12, 1.f);
}

static void simplify()
{
	// 0
	// 1 2
	// 3 4 5
	unsigned int ib[] = {
	    0, 2, 1,
	    1, 2, 3,
	    3, 2, 4,
	    2, 5, 4, // clang-format :-/
	};

	float vb[] = {
	    0, 4, 0,
	    0, 1, 0,
	    2, 2, 0,
	    0, 0, 0,
	    1, 0, 0,
	    4, 0, 0, // clang-format :-/
	};

	unsigned int expected[] = {
	    0,
	    5,
	    3,
	};

	float error;
	assert(meshopt_simplify(ib, ib, 12, vb, 6, 12, 3, 1e-2f, 0, &error) == 3);
	assert(error == 0.f);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyStuck()
{
	// tetrahedron can't be simplified due to collapse error restrictions
	float vb1[] = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1};
	unsigned int ib1[] = {0, 1, 2, 0, 2, 3, 0, 3, 1, 2, 1, 3};

	assert(meshopt_simplify(ib1, ib1, 12, vb1, 4, 12, 6, 1e-3f) == 12);

	// 5-vertex strip can't be simplified due to topology restriction since middle triangle has flipped winding
	float vb2[] = {0, 0, 0, 1, 0, 0, 2, 0, 0, 0.5f, 1, 0, 1.5f, 1, 0};
	unsigned int ib2[] = {0, 1, 3, 3, 1, 4, 1, 2, 4}; // ok
	unsigned int ib3[] = {0, 1, 3, 1, 3, 4, 1, 2, 4}; // flipped

	assert(meshopt_simplify(ib2, ib2, 9, vb2, 5, 12, 6, 1e-3f) == 6);
	assert(meshopt_simplify(ib3, ib3, 9, vb2, 5, 12, 6, 1e-3f) == 9);

	// 4-vertex quad with a locked corner can't be simplified due to border error-induced restriction
	float vb4[] = {0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0};
	unsigned int ib4[] = {0, 1, 3, 0, 3, 2};

	assert(meshopt_simplify(ib4, ib4, 6, vb4, 4, 12, 3, 1e-3f) == 6);

	// 4-vertex quad with a locked corner can't be simplified due to border error-induced restriction
	float vb5[] = {0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0};
	unsigned int ib5[] = {0, 1, 4, 0, 3, 2};

	assert(meshopt_simplify(ib5, ib5, 6, vb5, 5, 12, 3, 1e-3f) == 6);
}

static void simplifySloppyStuck()
{
	const float vb[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	const unsigned int ib[] = {0, 1, 2, 0, 1, 2};

	unsigned int* target = NULL;

	// simplifying down to 0 triangles results in 0 immediately
	assert(meshopt_simplifySloppy(target, ib, 3, vb, 3, 12, 0, 0.f) == 0);

	// simplifying down to 2 triangles given that all triangles are degenerate results in 0 as well
	assert(meshopt_simplifySloppy(target, ib, 6, vb, 3, 12, 6, 0.f) == 0);
}

static void simplifyPointsStuck()
{
	const float vb[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

	// simplifying down to 0 points results in 0 immediately
	assert(meshopt_simplifyPoints(NULL, vb, 3, 12, NULL, 0, 0, 0) == 0);
}

static void simplifyFlip()
{
	// this mesh has been constructed by taking a tessellated irregular grid with a square cutout
	// and progressively collapsing edges until the only ones left violate border or flip constraints.
	// there is only one valid non-flip collapse, so we validate that we take it; when flips are allowed,
	// the wrong collapse is picked instead.
	float vb[] = {
	    1.000000f, 1.000000f, -1.000000f,
	    1.000000f, 1.000000f, 1.000000f,
	    1.000000f, -1.000000f, 1.000000f,
	    1.000000f, -0.200000f, -0.200000f,
	    1.000000f, 0.200000f, -0.200000f,
	    1.000000f, -0.200000f, 0.200000f,
	    1.000000f, 0.200000f, 0.200000f,
	    1.000000f, 0.500000f, -0.500000f,
	    1.000000f, -1.000000f, 0.000000f, // clang-format :-/
	};

	// the collapse we expect is 7 -> 0
	unsigned int ib[] = {
	    7, 4, 3,
	    1, 2, 5,
	    7, 1, 6,
	    7, 8, 0, // gets removed
	    7, 6, 4,
	    8, 5, 2,
	    8, 7, 3,
	    8, 3, 5,
	    5, 6, 1,
	    7, 0, 1, // gets removed
	};

	unsigned int expected[] = {
	    0, 4, 3,
	    1, 2, 5,
	    0, 1, 6,
	    0, 6, 4,
	    8, 5, 2,
	    8, 0, 3,
	    8, 3, 5,
	    5, 6, 1, // clang-format :-/
	};

	assert(meshopt_simplify(ib, ib, 30, vb, 9, 12, 3, 1e-3f) == 24);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyScale()
{
	const float vb[] = {0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3};

	assert(meshopt_simplifyScale(vb, 4, 12) == 3.f);
}

static void simplifyDegenerate()
{
	float vb[] = {
	    0.000000f, 0.000000f, 0.000000f,
	    0.000000f, 1.000000f, 0.000000f,
	    0.000000f, 2.000000f, 0.000000f,
	    1.000000f, 0.000000f, 0.000000f,
	    2.000000f, 0.000000f, 0.000000f,
	    1.000000f, 1.000000f, 0.000000f, // clang-format :-/
	};

	// 0 1 2
	// 3 5
	// 4

	unsigned int ib[] = {
	    0, 1, 3,
	    3, 1, 5,
	    1, 2, 5,
	    3, 5, 4,
	    1, 0, 1, // these two degenerate triangles create a fake reverse edge
	    0, 3, 0, // which breaks border classification
	};

	unsigned int expected[] = {
	    0, 1, 4,
	    4, 1, 2, // clang-format :-/
	};

	assert(meshopt_simplify(ib, ib, 18, vb, 6, 12, 3, 1e-3f) == 6);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyLockBorder()
{
	float vb[] = {
	    0.000000f, 0.000000f, 0.000000f,
	    0.000000f, 1.000000f, 0.000000f,
	    0.000000f, 2.000000f, 0.000000f,
	    1.000000f, 0.000000f, 0.000000f,
	    1.000000f, 1.000000f, 0.000000f,
	    1.000000f, 2.000000f, 0.000000f,
	    2.000000f, 0.000000f, 0.000000f,
	    2.000000f, 1.000000f, 0.000000f,
	    2.000000f, 2.000000f, 0.000000f, // clang-format :-/
	};

	// 0 1 2
	// 3 4 5
	// 6 7 8

	unsigned int ib[] = {
	    0, 1, 3,
	    3, 1, 4,
	    1, 2, 4,
	    4, 2, 5,
	    3, 4, 6,
	    6, 4, 7,
	    4, 5, 7,
	    7, 5, 8, // clang-format :-/
	};

	unsigned int expected[] = {
	    0, 1, 3,
	    1, 2, 3,
	    3, 2, 5,
	    6, 3, 7,
	    3, 5, 7,
	    7, 5, 8, // clang-format :-/
	};

	assert(meshopt_simplify(ib, ib, 24, vb, 9, 12, 3, 1e-3f, meshopt_SimplifyLockBorder) == 18);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyAttr(bool skip_g)
{
	float vb[8 * 3][6];

	for (int y = 0; y < 8; ++y)
	{
		// first four rows are a blue gradient, next four rows are a yellow gradient
		float r = (y < 4) ? 0.8f + y * 0.05f : 0.f;
		float g = (y < 4) ? 0.8f + y * 0.05f : 0.f;
		float b = (y < 4) ? 0.f : 0.8f + (7 - y) * 0.05f;

		for (int x = 0; x < 3; ++x)
		{
			vb[y * 3 + x][0] = float(x);
			vb[y * 3 + x][1] = float(y);
			vb[y * 3 + x][2] = 0.03f * x + 0.03f * (y % 2) + (x == 2 && y == 7) * 0.03f;
			vb[y * 3 + x][3] = r;
			vb[y * 3 + x][4] = g;
			vb[y * 3 + x][5] = b;
		}
	}

	unsigned int ib[7 * 2][6];

	for (int y = 0; y < 7; ++y)
	{
		for (int x = 0; x < 2; ++x)
		{
			ib[y * 2 + x][0] = (y + 0) * 3 + (x + 0);
			ib[y * 2 + x][1] = (y + 0) * 3 + (x + 1);
			ib[y * 2 + x][2] = (y + 1) * 3 + (x + 0);
			ib[y * 2 + x][3] = (y + 1) * 3 + (x + 0);
			ib[y * 2 + x][4] = (y + 0) * 3 + (x + 1);
			ib[y * 2 + x][5] = (y + 1) * 3 + (x + 1);
		}
	}

	float attr_weights[3] = {0.5f, skip_g ? 0.f : 0.5f, 0.5f};

	// *0  1   *2
	//  3  4    5
	//  6  7    8
	// *9  10 *11
	// *12 13 *14
	//  15 16  17
	//  18 19  20
	// *21 22 *23
	unsigned int expected[3][6] = {
	    {0, 2, 11, 0, 11, 9},
	    {9, 11, 12, 12, 11, 14},
	    {12, 14, 23, 12, 23, 21},
	};

	assert(meshopt_simplifyWithAttributes(ib[0], ib[0], 7 * 2 * 6, vb[0], 8 * 3, 6 * sizeof(float), vb[0] + 3, 6 * sizeof(float), attr_weights, 3, NULL, 6 * 3, 1e-2f) == 18);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyLockFlags()
{
	float vb[] = {
	    0, 0, 0,
	    0, 1, 0,
	    0, 2, 0,
	    1, 0, 0,
	    1, 1, 0,
	    1, 2, 0,
	    2, 0, 0,
	    2, 1, 0,
	    2, 2, 0, // clang-format :-/
	};

	unsigned char lock[9] = {
	    1, 1, 1,
	    1, 0, 1,
	    1, 1, 1, // clang-format :-/
	};

	// 0 1 2
	// 3 4 5
	// 6 7 8

	unsigned int ib[] = {
	    0, 1, 3,
	    3, 1, 4,
	    1, 2, 4,
	    4, 2, 5,
	    3, 4, 6,
	    6, 4, 7,
	    4, 5, 7,
	    7, 5, 8, // clang-format :-/
	};

	unsigned int expected[] = {
	    0, 1, 3,
	    1, 2, 3,
	    3, 2, 5,
	    6, 3, 7,
	    3, 5, 7,
	    7, 5, 8, // clang-format :-/
	};

	assert(meshopt_simplifyWithAttributes(ib, ib, 24, vb, 9, 12, NULL, 0, NULL, 0, lock, 3, 1e-3f, 0) == 18);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyLockFlagsSeam()
{
	float vb[] = {
	    0, 0, 0,
	    0, 1, 0,
	    0, 1, 0,
	    0, 2, 0,
	    1, 0, 0,
	    1, 1, 0,
	    1, 1, 0,
	    1, 2, 0,
	    2, 0, 0,
	    2, 1, 0,
	    2, 1, 0,
	    2, 2, 0, // clang-format :-/
	};

	unsigned char lock0[12] = {
	    1, 0, 0, 1,
	    0, 0, 0, 0,
	    1, 0, 0, 1, // clang-format :-/
	};

	unsigned char lock1[12] = {
	    1, 0, 0, 1,
	    1, 0, 0, 1,
	    1, 0, 0, 1, // clang-format :-/
	};

	unsigned char lock2[12] = {
	    1, 0, 1, 1,
	    1, 0, 1, 1,
	    1, 0, 1, 1, // clang-format :-/
	};

	unsigned char lock3[12] = {
	    1, 1, 0, 1,
	    1, 1, 0, 1,
	    1, 1, 0, 1, // clang-format :-/
	};

	// 0 1-2 3
	// 4 5-6 7
	// 8 9-10 11

	unsigned int ib[] = {
	    0, 1, 4,
	    4, 1, 5,
	    4, 5, 8,
	    8, 5, 9,
	    2, 3, 6,
	    6, 3, 7,
	    6, 7, 10,
	    10, 7, 11, // clang-format :-/
	};

	unsigned int res[24];
	// with no locks, we should be able to collapse the entire mesh (vertices 1-2 and 9-10 are locked but others can move towards them)
	assert(meshopt_simplifyWithAttributes(res, ib, 24, vb, 12, 12, NULL, 0, NULL, 0, NULL, 0, 1.f, 0) == 0);

	// with corners locked, we should get two quads
	assert(meshopt_simplifyWithAttributes(res, ib, 24, vb, 12, 12, NULL, 0, NULL, 0, lock0, 0, 1.f, 0) == 12);

	// with both sides locked, we can only collapse the seam spine
	assert(meshopt_simplifyWithAttributes(res, ib, 24, vb, 12, 12, NULL, 0, NULL, 0, lock1, 0, 1.f, 0) == 18);

	// with seam spine locked, we can collapse nothing; note that we intentionally test two different lock configurations
	// they each lock only one side of the seam spine, which should be equivalent
	assert(meshopt_simplifyWithAttributes(res, ib, 24, vb, 12, 12, NULL, 0, NULL, 0, lock2, 0, 1.f, 0) == 24);
	assert(meshopt_simplifyWithAttributes(res, ib, 24, vb, 12, 12, NULL, 0, NULL, 0, lock3, 0, 1.f, 0) == 24);
}

static void simplifySparse()
{
	float vb[] = {
	    0, 0, 100,
	    0, 1, 0,
	    0, 2, 100,
	    1, 0, 0.1f,
	    1, 1, 0.1f,
	    1, 2, 0.1f,
	    2, 0, 100,
	    2, 1, 0,
	    2, 2, 100, // clang-format :-/
	};

	float vba[] = {
	    100,
	    0.5f,
	    100,
	    0.5f,
	    0.5f,
	    0,
	    100,
	    0.5f,
	    100, // clang-format :-/
	};

	float aw[] = {
	    0.5f};

	unsigned char lock[9] = {
	    8, 1, 8,
	    1, 0, 1,
	    8, 1, 8, // clang-format :-/
	};

	//   1
	// 3 4 5
	//   7

	unsigned int ib[] = {
	    3, 1, 4,
	    1, 5, 4,
	    3, 4, 7,
	    4, 5, 7, // clang-format :-/
	};

	unsigned int res[12];

	// vertices 3-4-5 are slightly elevated along Z which guides the collapses when only using geometry
	unsigned int expected[] = {
	    1, 5, 3,
	    3, 5, 7, // clang-format :-/
	};

	assert(meshopt_simplify(res, ib, 12, vb, 9, 12, 6, 1e-3f, meshopt_SimplifySparse) == 6);
	assert(memcmp(res, expected, sizeof(expected)) == 0);

	// vertices 1-4-7 have a crease in the attribute value which guides the collapses the opposite way when weighing attributes sufficiently
	unsigned int expecteda[] = {
	    3, 1, 7,
	    1, 5, 7, // clang-format :-/
	};

	assert(meshopt_simplifyWithAttributes(res, ib, 12, vb, 9, 12, vba, sizeof(float), aw, 1, lock, 6, 1e-1f, meshopt_SimplifySparse) == 6);
	assert(memcmp(res, expecteda, sizeof(expecteda)) == 0);

	// a final test validates that destination can alias when using sparsity
	assert(meshopt_simplify(ib, ib, 12, vb, 9, 12, 6, 1e-3f, meshopt_SimplifySparse) == 6);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyErrorAbsolute()
{
	float vb[] = {
	    0, 0, 0,
	    0, 1, 0,
	    0, 2, 0,
	    1, 0, 0,
	    1, 1, 1,
	    1, 2, 0,
	    2, 0, 0,
	    2, 1, 0,
	    2, 2, 0, // clang-format :-/
	};

	// 0 1 2
	// 3 4 5
	// 6 7 8

	unsigned int ib[] = {
	    0, 1, 3,
	    3, 1, 4,
	    1, 2, 4,
	    4, 2, 5,
	    3, 4, 6,
	    6, 4, 7,
	    4, 5, 7,
	    7, 5, 8, // clang-format :-/
	};

	float error = 0.f;
	assert(meshopt_simplify(ib, ib, 24, vb, 9, 12, 18, 2.f, meshopt_SimplifyLockBorder | meshopt_SimplifyErrorAbsolute, &error) == 18);
	assert(fabsf(error - 0.85f) < 0.01f);
}

static void simplifySeam()
{
	// xyz+attr
	float vb[] = {
	    0, 0, 0, 0,
	    0, 1, 0, 0,
	    0, 1, 0, 1,
	    0, 2, 0, 1,
	    1, 0, 0, 0,
	    1, 1, 0.3f, 0,
	    1, 1, 0.3f, 1,
	    1, 2, 0, 1,
	    2, 0, 0, 0,
	    2, 1, 0.1f, 0,
	    2, 1, 0.1f, 1,
	    2, 2, 0, 1,
	    3, 0, 0, 0,
	    3, 1, 0, 0,
	    3, 1, 0, 1,
	    3, 2, 0, 1, // clang-format :-/
	};

	// 0   1-2   3
	// 4   5-6   7
	// 8   9-10 11
	// 12 13-14 15

	unsigned int ib[] = {
	    0, 1, 4,
	    4, 1, 5,
	    2, 3, 6,
	    6, 3, 7,
	    4, 5, 8,
	    8, 5, 9,
	    6, 7, 10,
	    10, 7, 11,
	    8, 9, 12,
	    12, 9, 13,
	    10, 11, 14,
	    14, 11, 15, // clang-format :-/
	};

	// note: vertices 1-2 and 13-14 are classified as locked, because they are on a seam & a border
	// 0   1-2   3
	//     5-6
	//     9-10
	// 12 13-14 15
	unsigned int expected[] = {
	    0, 1, 13,
	    2, 3, 14,
	    0, 13, 12,
	    14, 3, 15, // clang-format :-/
	};

	unsigned int res[36];
	float error = 0.f;

	assert(meshopt_simplify(res, ib, 36, vb, 16, 16, 12, 1.f, 0, &error) == 12);
	assert(memcmp(res, expected, sizeof(expected)) == 0);
	assert(fabsf(error - 0.09f) < 0.01f); // note: the error is not zero because there is a difference in height between the seam vertices

	float aw = 1;
	assert(meshopt_simplifyWithAttributes(res, ib, 36, vb, 16, 16, vb + 3, 16, &aw, 1, NULL, 12, 2.f, 0, &error) == 12);
	assert(memcmp(res, expected, sizeof(expected)) == 0);
	assert(fabsf(error - 0.09f) < 0.01f); // note: this is the same error as above because the attribute is constant on either side of the seam
}

static void simplifySeamFake()
{
	// xyz+attr
	float vb[] = {
	    0, 0, 0, 0,
	    1, 0, 0, 1,
	    1, 0, 0, 2,
	    0, 0, 0, 3, // clang-format :-/
	};

	unsigned int ib[] = {
	    0, 1, 2,
	    2, 1, 3, // clang-format :-/
	};

	assert(meshopt_simplify(ib, ib, 6, vb, 4, 16, 0, 1.f, 0, NULL) == 6);
}

static void simplifySeamAttr()
{
	// xyz+attr
	float vb[] = {
	    0, 0, 0, 0,
	    0, 1, 0, 0,
	    0, 1, 0, 0,
	    0, 2, 0, 0,
	    1, 0, 0, 1,
	    1, 1, 0, 1,
	    1, 1, 0, 1,
	    1, 2, 0, 1,
	    4, 0, 0, 2,
	    4, 1, 0, 2,
	    4, 1, 0, 2,
	    4, 2, 0, 2, // clang-format :-/
	};

	// 0   1-2   3
	// 4   5-6   7
	// 8   9-10 11

	unsigned int ib[] = {
	    0, 1, 4,
	    4, 1, 5,
	    2, 3, 6,
	    6, 3, 7,
	    4, 5, 8,
	    8, 5, 9,
	    6, 7, 10,
	    10, 7, 11, // clang-format :-/
	};

	// note: vertices 1-2 and 9-10 are classified as locked, because they are on a seam & a border
	// 0   1-2   3
	// 4         7
	// 8   9-10 11
	unsigned int expected[] = {
	    0, 1, 4,
	    2, 3, 7,
	    4, 1, 8,
	    8, 1, 9,
	    2, 7, 10,
	    10, 7, 11, // clang-format :-/
	};

	unsigned int res[24];
	float error = 0.f;

	float aw = 1;
	assert(meshopt_simplifyWithAttributes(res, ib, 24, vb, 12, 16, vb + 3, 16, &aw, 1, NULL, 12, 2.f, meshopt_SimplifyLockBorder, &error) == 18);
	assert(memcmp(res, expected, sizeof(expected)) == 0);
	assert(fabsf(error - 0.35f) < 0.01f);
}

static void simplifyDebug()
{
	// 0
	// 1 2
	// 3 4 5
	unsigned int ib[] = {
	    0, 2, 1,
	    1, 2, 3,
	    3, 2, 4,
	    2, 5, 4, // clang-format :-/
	};

	float vb[] = {
	    0, 4, 0,
	    0, 1, 0,
	    2, 2, 0,
	    0, 0, 0,
	    1, 0, 0,
	    4, 0, 0, // clang-format :-/
	};

	unsigned int expected[] = {
	    0 | (9u << 28),
	    5 | (9u << 28),
	    3 | (9u << 28),
	};

	const unsigned int meshopt_SimplifyInternalDebug = 1 << 30;

	float error;
	assert(meshopt_simplify(ib, ib, 12, vb, 6, 12, 3, 1e-2f, meshopt_SimplifyInternalDebug, &error) == 3);
	assert(error == 0.f);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyPrune()
{
	// 0
	// 1 2
	// 3 4 5
	// +
	// 6 7 8 (same position)
	unsigned int ib[] = {
	    0, 2, 1,
	    1, 2, 3,
	    3, 2, 4,
	    2, 5, 4,
	    6, 7, 8, // clang-format :-/
	};

	float vb[] = {
	    0, 4, 0,
	    0, 1, 0,
	    2, 2, 0,
	    0, 0, 0,
	    1, 0, 0,
	    4, 0, 0,
	    1, 1, 1,
	    1, 1, 1,
	    1, 1, 1, // clang-format :-/
	};

	unsigned int expected[] = {
	    0,
	    5,
	    3,
	};

	float error;
	assert(meshopt_simplify(ib, ib, 15, vb, 9, 12, 3, 1e-2f, meshopt_SimplifyPrune, &error) == 3);
	assert(error == 0.f);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);

	// re-run prune with and without sparsity on a small subset to make sure the component code correctly handles sparse subsets
	assert(meshopt_simplify(ib, ib, 3, vb, 9, 12, 3, 1e-2f, meshopt_SimplifyPrune, &error) == 3);
	assert(meshopt_simplify(ib, ib, 3, vb, 9, 12, 3, 1e-2f, meshopt_SimplifyPrune | meshopt_SimplifySparse, &error) == 3);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void simplifyPruneCleanup()
{
	unsigned int ib[] = {
	    0, 1, 2,
	    3, 4, 5,
	    6, 7, 8, // clang-format :-/
	};

	float vb[] = {
	    0, 0, 0,
	    0, 1, 0,
	    1, 0, 0,
	    0, 0, 1,
	    0, 2, 1,
	    2, 0, 1,
	    0, 0, 2,
	    0, 4, 2,
	    4, 0, 2, // clang-format :-/
	};

	unsigned int expected[] = {
	    6,
	    7,
	    8,
	};

	float error;
	assert(meshopt_simplify(ib, ib, 9, vb, 9, 12, 3, 1.f, meshopt_SimplifyLockBorder | meshopt_SimplifyPrune, &error) == 3);
	assert(fabsf(error - 0.37f) < 0.01f);
	assert(memcmp(ib, expected, sizeof(expected)) == 0);
}

static void adjacency()
{
	// 0 1/4
	// 2/5 3
	const float vb[] = {0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0};
	const unsigned int ib[] = {0, 1, 2, 5, 4, 3};

	unsigned int adjib[12];
	meshopt_generateAdjacencyIndexBuffer(adjib, ib, 6, vb, 6, 12);

	unsigned int expected[] = {
	    // patch 0
	    0, 0,
	    1, 3,
	    2, 2,

	    // patch 1
	    5, 0,
	    4, 4,
	    3, 3,

	    // clang-format :-/
	};

	assert(memcmp(adjib, expected, sizeof(expected)) == 0);
}

static void tessellation()
{
	// 0 1/4
	// 2/5 3
	const float vb[] = {0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0};
	const unsigned int ib[] = {0, 1, 2, 5, 4, 3};

	unsigned int tessib[24];
	meshopt_generateTessellationIndexBuffer(tessib, ib, 6, vb, 6, 12);

	unsigned int expected[] = {
	    // patch 0
	    0, 1, 2,
	    0, 1,
	    4, 5,
	    2, 0,
	    0, 1, 2,

	    // patch 1
	    5, 4, 3,
	    2, 1,
	    4, 3,
	    3, 5,
	    2, 1, 3,

	    // clang-format :-/
	};

	assert(memcmp(tessib, expected, sizeof(expected)) == 0);
}

static void provoking()
{
	// 0 1 2
	// 3 4 5
	const unsigned int ib[] = {
	    0, 1, 3,
	    3, 1, 4,
	    1, 2, 4,
	    4, 2, 5,
	    0, 2, 4,
	    // clang-format :-/
	};

	unsigned int pib[15];
	unsigned int pre[6 + 5]; // limit is vertex count + triangle count
	size_t res = meshopt_generateProvokingIndexBuffer(pib, pre, ib, 15, 6);

	unsigned int expectedib[] = {
	    0, 5, 1,
	    1, 4, 0,
	    2, 4, 1,
	    3, 4, 2,
	    4, 5, 2,
	    // clang-format :-/
	};

	unsigned int expectedre[] = {
	    3, 1, 2, 5, 4, 0,
	    // clang-format :-/
	};

	assert(res == 6);
	assert(memcmp(pib, expectedib, sizeof(expectedib)) == 0);
	assert(memcmp(pre, expectedre, sizeof(expectedre)) == 0);
}

static void quantizeFloat()
{
	volatile float zero = 0.f; // avoids div-by-zero warnings

	assert(meshopt_quantizeFloat(1.2345f, 23) == 1.2345f);

	assert(meshopt_quantizeFloat(1.2345f, 16) == 1.2344971f);
	assert(meshopt_quantizeFloat(1.2345f, 8) == 1.2343750f);
	assert(meshopt_quantizeFloat(1.2345f, 4) == 1.25f);
	assert(meshopt_quantizeFloat(1.2345f, 1) == 1.0);

	assert(meshopt_quantizeFloat(1.f, 0) == 1.0f);

	assert(meshopt_quantizeFloat(1.f / zero, 0) == 1.f / zero);
	assert(meshopt_quantizeFloat(-1.f / zero, 0) == -1.f / zero);

	float nanf = meshopt_quantizeFloat(zero / zero, 8);
	assert(nanf != nanf);
}

static void quantizeHalf()
{
	volatile float zero = 0.f; // avoids div-by-zero warnings

	// normal
	assert(meshopt_quantizeHalf(1.2345f) == 0x3cf0);

	// overflow
	assert(meshopt_quantizeHalf(65535.f) == 0x7c00);
	assert(meshopt_quantizeHalf(-65535.f) == 0xfc00);

	// large
	assert(meshopt_quantizeHalf(65000.f) == 0x7bef);
	assert(meshopt_quantizeHalf(-65000.f) == 0xfbef);

	// small
	assert(meshopt_quantizeHalf(0.125f) == 0x3000);
	assert(meshopt_quantizeHalf(-0.125f) == 0xb000);

	// very small
	assert(meshopt_quantizeHalf(1e-4f) == 0x068e);
	assert(meshopt_quantizeHalf(-1e-4f) == 0x868e);

	// underflow
	assert(meshopt_quantizeHalf(1e-5f) == 0x0000);
	assert(meshopt_quantizeHalf(-1e-5f) == 0x8000);

	// exponent underflow
	assert(meshopt_quantizeHalf(1e-20f) == 0x0000);
	assert(meshopt_quantizeHalf(-1e-20f) == 0x8000);

	// exponent overflow
	assert(meshopt_quantizeHalf(1e20f) == 0x7c00);
	assert(meshopt_quantizeHalf(-1e20f) == 0xfc00);

	// inf
	assert(meshopt_quantizeHalf(1.f / zero) == 0x7c00);
	assert(meshopt_quantizeHalf(-1.f / zero) == 0xfc00);

	// nan
	unsigned short nanh = meshopt_quantizeHalf(zero / zero);
	assert(nanh == 0x7e00 || nanh == 0xfe00);
}

static void dequantizeHalf()
{
	volatile float zero = 0.f; // avoids div-by-zero warnings

	// normal
	assert(meshopt_dequantizeHalf(0x3cf0) == 1.234375f);

	// large
	assert(meshopt_dequantizeHalf(0x7bef) == 64992.f);
	assert(meshopt_dequantizeHalf(0xfbef) == -64992.f);

	// small
	assert(meshopt_dequantizeHalf(0x3000) == 0.125f);
	assert(meshopt_dequantizeHalf(0xb000) == -0.125f);

	// very small
	assert(meshopt_dequantizeHalf(0x068e) == 1.00016594e-4f);
	assert(meshopt_dequantizeHalf(0x868e) == -1.00016594e-4f);

	// denormal
	assert(meshopt_dequantizeHalf(0x00ff) == 0.f);
	assert(meshopt_dequantizeHalf(0x80ff) == 0.f); // actually this is -0.f
	assert(1.f / meshopt_dequantizeHalf(0x80ff) == -1.f / zero);

	// inf
	assert(meshopt_dequantizeHalf(0x7c00) == 1.f / zero);
	assert(meshopt_dequantizeHalf(0xfc00) == -1.f / zero);

	// nan
	float nanf = meshopt_dequantizeHalf(0x7e00);
	assert(nanf != nanf);
}

void runTests()
{
	decodeIndexV0();
	decodeIndexV1();
	decodeIndex16();
	encodeIndexMemorySafe();
	decodeIndexMemorySafe();
	decodeIndexRejectExtraBytes();
	decodeIndexRejectMalformedHeaders();
	decodeIndexRejectInvalidVersion();
	decodeIndexMalformedVByte();
	roundtripIndexTricky();
	encodeIndexEmpty();

	decodeIndexSequence();
	decodeIndexSequence16();
	encodeIndexSequenceMemorySafe();
	decodeIndexSequenceMemorySafe();
	decodeIndexSequenceRejectExtraBytes();
	decodeIndexSequenceRejectMalformedHeaders();
	decodeIndexSequenceRejectInvalidVersion();
	encodeIndexSequenceEmpty();

	decodeVertexV0();
	decodeVertexV1();
	decodeVertexV1Custom();

	for (int version = 0; version <= 1; ++version)
	{
		meshopt_encodeVertexVersion(version);

		decodeVertexMemorySafe();
		decodeVertexRejectExtraBytes();
		decodeVertexRejectMalformedHeaders();
		decodeVertexBitGroups();
		decodeVertexBitGroupSentinels();
		decodeVertexDeltas();
		decodeVertexBitXor();
		decodeVertexLarge();
		decodeVertexSmall();
		encodeVertexEmpty();
		encodeVertexMemorySafe();
	}

	decodeFilterOct8();
	decodeFilterOct12();
	decodeFilterQuat12();
	decodeFilterExp();

	encodeFilterOct8();
	encodeFilterOct12();
	encodeFilterQuat12();
	encodeFilterExp();
	encodeFilterExpZero();
	encodeFilterExpAlias();
	encodeFilterExpClamp();

	clusterBoundsDegenerate();

	meshletsDense();
	meshletsSparse();

	customAllocator();

	emptyMesh();

	simplify();
	simplifyStuck();
	simplifySloppyStuck();
	simplifyPointsStuck();
	simplifyFlip();
	simplifyScale();
	simplifyDegenerate();
	simplifyLockBorder();
	simplifyAttr(/* skip_g= */ false);
	simplifyAttr(/* skip_g= */ true);
	simplifyLockFlags();
	simplifyLockFlagsSeam();
	simplifySparse();
	simplifyErrorAbsolute();
	simplifySeam();
	simplifySeamFake();
	simplifySeamAttr();
	simplifyDebug();
	simplifyPrune();
	simplifyPruneCleanup();

	adjacency();
	tessellation();
	provoking();

	quantizeFloat();
	quantizeHalf();
	dequantizeHalf();
}
