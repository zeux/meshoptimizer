// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <math.h>

void meshopt_decodeFilterOctS8(void* buffer, size_t vertex_count, size_t vertex_size)
{
	(void)buffer;
	(void)vertex_count;
	(void)vertex_size;
}

void meshopt_decodeFilterOctS12(void* buffer, size_t vertex_count, size_t vertex_size)
{
	(void)buffer;
	(void)vertex_count;
	(void)vertex_size;
}

void meshopt_decodeFilterQuatR12(void* buffer, size_t vertex_count, size_t vertex_size)
{
	assert(vertex_count % 4 == 0);
	assert(vertex_size == 8);
	(void)vertex_size;

	static const int order[4][4] =
	{
		{ 1, 2, 3, 0 },
		{ 2, 3, 0, 1 },
		{ 3, 0, 1, 2 },
		{ 0, 1, 2, 3 },
	};

#ifdef __EMSCRIPTEN__
	volatile // workaround to prevent autovectorization
#endif
	float scale = 2047.f * sqrtf(2.f);

	short* data = static_cast<short*>(buffer);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		float x = float(data[i * 4 + 0]) / scale;
		float y = float(data[i * 4 + 1]) / scale;
		float z = float(data[i * 4 + 2]) / scale;

		float ww = 1.f - x * x - y * y - z * z;
		float w = sqrtf(ww >= 0.f ? ww : 0.f);

		int xf = int(x * 32767.f + (x >= 0.f ? 0.5f : -0.5f));
		int yf = int(y * 32767.f + (y >= 0.f ? 0.5f : -0.5f));
		int zf = int(z * 32767.f + (z >= 0.f ? 0.5f : -0.5f));
		int wf = int(w * 32767.f + 0.5f);

		int qc = data[i * 4 + 3] & 3;

		data[i * 4 + order[qc][0]] = xf;
		data[i * 4 + order[qc][1]] = yf;
		data[i * 4 + order[qc][2]] = zf;
		data[i * 4 + order[qc][3]] = wf;
	}
}
