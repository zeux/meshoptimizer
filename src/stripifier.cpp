#include "meshoptimizer.h"

#include <cassert>

#include <algorithm>
#include <vector>

static unsigned int findStripFirst(const unsigned int buffer[][3], unsigned int buffer_size, const unsigned int* valence)
{
	unsigned int index = 0;
	unsigned int iv = ~0u;

	for (unsigned int i = 0; i < buffer_size; ++i)
	{
		unsigned int v = std::min(valence[buffer[i][0]], std::min(valence[buffer[i][1]], valence[buffer[i][2]]));

		if (v < iv)
		{
			index = i;
			iv = v;
		}
	}

	return index;
}

static int findStripNext(const unsigned int buffer[][3], unsigned int buffer_size, unsigned int e0, unsigned int e1)
{
	for (unsigned int i = 0; i < buffer_size; ++i)
	{
		unsigned int a = buffer[i][0], b = buffer[i][1], c = buffer[i][2];

		if (e0 == a && e1 == b)
			return (i << 2) | 2;
		else if (e0 == b && e1 == c)
			return (i << 2) | 0;
		else if (e0 == c && e1 == a)
			return (i << 2) | 1;
	}

	return -1;
}

size_t meshopt_stripify(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count)
{
	const size_t buffer_capacity = 16;

	unsigned int buffer[buffer_capacity][3] = {};
	unsigned int buffer_size = 0;

	size_t index_offset = 0;

	unsigned int strip[2] = {};
	unsigned int stripx = 0;

	size_t strip_size = 0;

	std::vector<unsigned int> valence(vertex_count, 0);

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		valence[index]++;
	}

	while (buffer_size > 0 || index_offset < index_count)
	{
		// fill triangle buffer
		while (buffer_size < buffer_capacity && index_offset < index_count)
		{
			buffer[buffer_size][0] = indices[index_offset + 0];
			buffer[buffer_size][1] = indices[index_offset + 1];
			buffer[buffer_size][2] = indices[index_offset + 2];

			buffer_size++;
			index_offset += 3;
		}

		assert(buffer_size > 0);

		// find next triangle
		// note that the order of last edge flips on every iteration
		int next = findStripNext(buffer, buffer_size, strip[stripx], strip[stripx ^ 1]);

		if (next >= 0)
		{
			assert((next & 3) < 3);

			unsigned int i = next >> 2;
			unsigned int a = buffer[i][0], b = buffer[i][1], c = buffer[i][2];
			unsigned int v = buffer[i][next & 3];

			// emit the next vertex in the strip
			destination[strip_size++] = v;

			// next strip has flipped winding
			strip[0] = strip[1];
			strip[1] = v;
			stripx ^= 1;

			// unordered removal from the buffer
			buffer[i][0] = buffer[buffer_size - 1][0];
			buffer[i][1] = buffer[buffer_size - 1][1];
			buffer[i][2] = buffer[buffer_size - 1][2];
			buffer_size--;

			// update vertex valences for strip start heuristic
			valence[a]--;
			valence[b]--;
			valence[c]--;
		}
		else
		{
			// if we didn't find anything, we need to find the next new triangle
			// we use a heuristic to maximize the strip length
			unsigned int i = findStripFirst(buffer, buffer_size, &valence[0]);
			unsigned int a = buffer[i][0], b = buffer[i][1], c = buffer[i][2];

			// unordered removal from the buffer
			buffer[i][0] = buffer[buffer_size - 1][0];
			buffer[i][1] = buffer[buffer_size - 1][1];
			buffer[i][2] = buffer[buffer_size - 1][2];
			buffer_size--;

			// we need to pre-rotate the triangle so that we will find a match in the existing buffer on the next iteration
			if (findStripNext(buffer, buffer_size, a, c) >= 0)
			{
				// abc -> bca
				unsigned int t = a;
				a = b, b = c, c = t;
			}
			else if (findStripNext(buffer, buffer_size, b, a) >= 0)
			{
				// abc -> cab
				unsigned int t = c;
				c = b, b = a, a = t;
			}

			// emit the new strip; we use restart indices
			if (strip_size)
				destination[strip_size++] = ~0u;

			destination[strip_size++] = a;
			destination[strip_size++] = b;
			destination[strip_size++] = c;

			// new strip always starts with the same edge winding
			strip[0] = b;
			strip[1] = c;
			stripx = 1;

			// update vertex valences for strip start heuristic
			valence[a]--;
			valence[b]--;
			valence[c]--;
		}
	}

	return strip_size;
}

size_t meshopt_unstripify(unsigned int* destination, const unsigned int* indices, size_t index_count)
{
	size_t offset = 0;
	size_t start = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		if (indices[i] == ~0u)
		{
			start = i + 1;
		}
		else if (i - start >= 2)
		{
			unsigned int a = indices[i - 2], b = indices[i - 1], c = indices[i];

			if ((i - start) % 2 == 1)
				std::swap(a, b);

			if (a != b && a != c && b != c)
			{
				destination[offset + 0] = a;
				destination[offset + 1] = b;
				destination[offset + 2] = c;
				offset += 3;
			}
		}
	}

	return offset;
}
