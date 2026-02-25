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
static void rasterizeOpacityRec(unsigned char* result, size_t index, int level, float u0, float v0, float a0, float u1, float v1, float a1, float u2, float v2, float a2, const Texture& texture)
{
	if (level == 0)
	{
		// compute triangle center & sample
		float uc = (u0 + u1 + u2) / 3;
		float vc = (v0 + v1 + v2) / 3;
		float ac = sampleTexture(texture, uc, vc);

		// rasterize opacity state based on alpha values in corners and center
		(States == 2) ? rasterizeOpacity2(result, index, a0, a1, a2, ac) : rasterizeOpacity4(result, index, a0, a1, a2, ac);
		return;
	}

	// compute each edge midpoint & sample
	float u01 = (u0 + u1) / 2, v01 = (v0 + v1) / 2;
	float u02 = (u0 + u2) / 2, v02 = (v0 + v2) / 2;
	float u12 = (u1 + u2) / 2, v12 = (v1 + v2) / 2;

	float a01 = sampleTexture(texture, u01, v01);
	float a02 = sampleTexture(texture, u02, v02);
	float a12 = sampleTexture(texture, u12, v12);

	// recursively rasterize each triangle
	// note: triangles 1 and 3 have flipped winding, and 1 is additionally flipped
	rasterizeOpacityRec<States>(result, index * 4 + 0, level - 1, u0, v0, a0, u01, v01, a01, u02, v02, a02, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 1, level - 1, u02, v02, a02, u12, v12, a12, u01, v01, a01, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 2, level - 1, u01, v01, a01, u1, v1, a1, u12, v12, a12, texture);
	rasterizeOpacityRec<States>(result, index * 4 + 3, level - 1, u12, v12, a12, u02, v02, a02, u2, v2, a2, texture);
}

} // namespace meshopt

void meshopt_opacityMapRasterize(unsigned char* result, int level, int states, const float* uv0, const float* uv1, const float* uv2, const unsigned char* texture_data, size_t texture_stride, size_t texture_pitch, unsigned int texture_width, unsigned int texture_height)
{
	using namespace meshopt;

	assert(level >= 0 && level <= 12);
	assert(states == 2 || states == 4);

	Texture texture = {texture_data, texture_stride, texture_pitch, texture_width, texture_height};

	float alpha0 = sampleTexture(texture, uv0[0], uv0[1]);
	float alpha1 = sampleTexture(texture, uv1[0], uv1[1]);
	float alpha2 = sampleTexture(texture, uv2[0], uv2[1]);

	memset(result, 0, ((1 << (level * 2)) * (states / 2) + 7) / 8);
	(states == 2 ? rasterizeOpacityRec<2> : rasterizeOpacityRec<4>)(result, 0, level, uv0[0], uv0[1], alpha0, uv1[0], uv1[1], alpha1, uv2[0], uv2[1], alpha2, texture);
}
