// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

// This work is based on:
// TODO: references
namespace meshopt
{

const size_t kVertexBlockSize = 256;

static unsigned char* encodeVertexBlock(unsigned char* data, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size)
{
	assert(vertex_count > 0);

	for (size_t k = 0; k < vertex_size; ++k)
	{
		*data++ = vertex_data[k];
	}

	for (size_t k = 0; k < vertex_size; ++k)
	{
		size_t vertex_offset = k;

		for (size_t i = 1; i < vertex_count; ++i)
		{
			*data++ = vertex_data[vertex_offset + vertex_size] - vertex_data[vertex_offset];

			vertex_offset += vertex_size;
		}
	}

	return data;
}

}

size_t meshopt_encodeVertexBuffer(unsigned char* buffer, size_t buffer_size, const void* vertices, size_t vertex_count, size_t vertex_size, size_t index_count, const unsigned char* index_buffer, size_t index_buffer_size)
{
	using namespace meshopt;

	assert(vertex_size > 0 && vertex_size <= 256);
	assert(index_count % 3 == 0);

	(void)index_buffer;
	(void)index_buffer_size;

	const unsigned char* vertex_data = static_cast<const unsigned char*>(vertices);

	unsigned char* data = buffer;

	for (size_t i = 0; i < vertex_count; i += kVertexBlockSize)
	{
		size_t block_size = (i + kVertexBlockSize < vertex_count) ? kVertexBlockSize : vertex_count - i;

		data = encodeVertexBlock(data, vertex_data + i * vertex_size, block_size, vertex_size);
	}

	assert(size_t(data - buffer) <= buffer_size);

	return data - buffer;
}

size_t meshopt_encodeVertexBufferBound(size_t vertex_count, size_t vertex_size)
{
	return vertex_count * vertex_size * 2;
}
