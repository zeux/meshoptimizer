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

struct Texture
{
	const unsigned char* data;
	size_t stride, pitch;
	unsigned int width, height;
};

static float sampleTexture(const Texture& texture, float u, float v)
{
	int x = int(u * float(int(texture.width)));
	int y = int(v * float(int(texture.height)));

	// TODO: clamp/wrap
	if (unsigned(x) >= texture.width || unsigned(y) >= texture.height)
		return 0.f;

	// TODO: bilinear
	return texture.data[y * texture.pitch + x * texture.stride] * (1.f / 255.f);
}

static void rasterizeOpacity2(unsigned char* result, size_t index, float a0, float a1, float a2, float ac)
{
	// for now threshold average value; this could use a more sophisticated heuristic in the future
	float average = (a0 + a1 + a2 + ac) * 0.25f;
	int state = average >= 0.5f;

	result[index / 8] |= state << (index % 8);
}

static void rasterizeOpacity4(unsigned char* result, size_t index, float a0, float a1, float a2, float ac)
{
	int transp = (a0 < 0.25f) & (a1 < 0.25f) & (a2 < 0.25f) & (ac < 0.25f);
	int opaque = (a0 > 0.75f) & (a1 > 0.75f) & (a2 > 0.75f) & (ac > 0.75f);
	float average = (a0 + a1 + a2 + ac) * 0.25f;

	// treat state as known if thresholding of corners & centers against wider bounds is consistent
	// for unknown states, we currently use the same formula as the 2-state opacity for better consistency with forced 2-state
	int state = (transp | opaque) ? opaque : (2 + (average >= 0.5f));

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
	c02[2] = sampleTexture(texture, c02[0], c02[1]);
	c12[2] = sampleTexture(texture, c12[0], c12[1]);

	// recursively rasterize each triangle
	// note: triangles 1 and 3 have flipped winding, and 1 is flipped upside down
	rasterizeOpacityRec<States>(result, index * 4 + 0, level - 1, c0, c01, c02, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 1, level - 1, c02, c12, c01, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 2, level - 1, c01, c1, c12, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 3, level - 1, c12, c02, c2, texture);
}

} // namespace meshopt

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
