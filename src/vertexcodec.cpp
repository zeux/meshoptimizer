// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

// This work is based on:
// TODO: references
namespace meshopt
{
}

size_t meshopt_encodeVertexBuffer(unsigned char* buffer, size_t buffer_size, const void* vertices, size_t vertex_count, size_t vertex_size, size_t index_count, const unsigned char* index_buffer, size_t index_buffer_size)
{
	assert(vertex_size > 0 && vertex_size <= 256);
	assert(index_count % 3 == 0);

	(void)index_buffer;
	(void)index_buffer_size;

	const unsigned char* vertex_data = static_cast<const unsigned char*>(vertices);

	unsigned char* data = buffer;

	for (size_t k = 0; k < vertex_size; ++k)
	{
		for (size_t i = 0; i < vertex_count; ++i)
		{
			*data++ = vertex_data[i * vertex_size + k];
		}
	}

	assert(size_t(data - buffer) <= buffer_size);

	return data - buffer;
}

size_t meshopt_encodeVertexBufferBound(size_t vertex_count, size_t vertex_size)
{
	return vertex_count * vertex_size * 2;
}
