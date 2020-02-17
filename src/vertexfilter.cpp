// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <math.h>

#if defined(__wasm_simd128__)
#define SIMD_WASM
#endif

#ifdef SIMD_WASM
#include <wasm_simd128.h>
#endif

void meshopt_decodeFilterOct8(void* buffer, size_t vertex_count, size_t vertex_size)
{
	assert(vertex_count % 4 == 0);
	assert(vertex_size == 4);
	(void)vertex_size;

	signed char* data = static_cast<signed char*>(buffer);

#ifdef SIMD_WASM
	const v128_t sign = wasm_f32x4_splat(-0.f);

	for (size_t i = 0; i < vertex_count; i += 4)
	{
		v128_t n4 = wasm_v128_load(&data[i * 4]);

		// sign-extends each of x,y in [x y ? ?] with arithmetic shifts
		v128_t xf = wasm_i32x4_shr(wasm_i32x4_shl(n4, 24), 24);
		v128_t yf = wasm_i32x4_shr(wasm_i32x4_shl(n4, 16), 24);

		// convert x and y to [-1..1] and reconstruct z
		v128_t x = wasm_f32x4_mul(wasm_f32x4_convert_i32x4(xf), wasm_f32x4_splat(1.f / 127.f));
		v128_t y = wasm_f32x4_mul(wasm_f32x4_convert_i32x4(yf), wasm_f32x4_splat(1.f / 127.f));
		v128_t z = wasm_f32x4_sub(wasm_f32x4_splat(1.f), wasm_f32x4_add(wasm_f32x4_abs(x), wasm_f32x4_abs(y)));

		// fixup octahedral coordinates for z<0
		v128_t t = wasm_v128_and(z, wasm_f32x4_lt(z, wasm_f32x4_splat(0.f)));

		x = wasm_f32x4_add(x, wasm_v128_xor(t, wasm_v128_and(x, sign)));
		y = wasm_f32x4_add(y, wasm_v128_xor(t, wasm_v128_and(y, sign)));

		// compute normal length & scale
		v128_t l = wasm_f32x4_sqrt(wasm_f32x4_add(wasm_f32x4_mul(x, x), wasm_f32x4_add(wasm_f32x4_mul(y, y), wasm_f32x4_mul(z, z))));
		v128_t s = wasm_f32x4_div(wasm_f32x4_splat(127.f), l);

		// fast rounded signed float->int: addition triggers renormalization after which mantissa stores the integer value
		const v128_t fsnap = wasm_f32x4_splat(3 << 22);
		const v128_t fmask = wasm_i32x4_splat(0x7fffff);
		const v128_t fbase = wasm_i32x4_splat(0x400000);

		v128_t xr = wasm_i32x4_sub(wasm_v128_and(wasm_f32x4_add(wasm_f32x4_mul(x, s), fsnap), fmask), fbase);
		v128_t yr = wasm_i32x4_sub(wasm_v128_and(wasm_f32x4_add(wasm_f32x4_mul(y, s), fsnap), fmask), fbase);
		v128_t zr = wasm_i32x4_sub(wasm_v128_and(wasm_f32x4_add(wasm_f32x4_mul(z, s), fsnap), fmask), fbase);

		// combine xr/yr/zr into final value
		v128_t res = wasm_v128_and(n4, wasm_i32x4_splat(0xff000000));
		res = wasm_v128_or(res, wasm_v128_and(xr, wasm_i32x4_splat(0xff)));
		res = wasm_v128_or(res, wasm_i32x4_shl(wasm_v128_and(yr, wasm_i32x4_splat(0xff)), 8));
		res = wasm_v128_or(res, wasm_i32x4_shl(wasm_v128_and(zr, wasm_i32x4_splat(0xff)), 16));

		wasm_v128_store(&data[i * 4], res);
	}
#else
	const float scale = 1.f / 127.f;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		// convert x and y to [-1..1] and reconstruct z
		float x = float(data[i * 4 + 0]) * scale;
		float y = float(data[i * 4 + 1]) * scale;
		float z = 1.f - fabsf(x) - fabsf(y);

		// fixup octahedral coordinates for z<0
		float t = (z >= 0.f) ? 0.f : z;

		x += (x >= 0.f) ? t : -t;
		y += (y >= 0.f) ? t : -t;

		// compute normal length & scale
		float l = sqrtf(x * x + y * y + z * z);
		float s = 127.f / l;

		// rounded signed float->int
		int xf = int(x * s + (x >= 0.f ? 0.5f : -0.5f));
		int yf = int(y * s + (y >= 0.f ? 0.5f : -0.5f));
		int zf = int(z * s + (z >= 0.f ? 0.5f : -0.5f));

		data[i * 4 + 0] = (signed char)(xf);
		data[i * 4 + 1] = (signed char)(yf);
		data[i * 4 + 2] = (signed char)(zf);
	}
#endif
}

void meshopt_decodeFilterOct12(void* buffer, size_t vertex_count, size_t vertex_size)
{
	assert(vertex_count % 4 == 0);
	assert(vertex_size == 8);
	(void)vertex_size;

	const float scale = 1.f / 2047.f;

	short* data = static_cast<short*>(buffer);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		// convert x and y to [-1..1] and reconstruct z
		float x = float(data[i * 4 + 0]) * scale;
		float y = float(data[i * 4 + 1]) * scale;
		float z = 1.f - fabsf(x) - fabsf(y);

		// fixup octahedral coordinates for z<0
		float t = z >= 0.f ? 0.f : z;

		x += (x >= 0.f) ? t : -t;
		y += (y >= 0.f) ? t : -t;

		// compute normal length & scale
		float l = sqrtf(x * x + y * y + z * z);
		float s = 32767.f / l;

		// rounded signed float->int
		int xf = int(x * s + (x >= 0.f ? 0.5f : -0.5f));
		int yf = int(y * s + (y >= 0.f ? 0.5f : -0.5f));
		int zf = int(z * s + (z >= 0.f ? 0.5f : -0.5f));

		data[i * 4 + 0] = short(xf);
		data[i * 4 + 1] = short(yf);
		data[i * 4 + 2] = short(zf);
	}
}

void meshopt_decodeFilterQuat12(void* buffer, size_t vertex_count, size_t vertex_size)
{
	assert(vertex_count % 4 == 0);
	assert(vertex_size == 8);
	(void)vertex_size;

	static const int order[4][4] = {
	    {1, 2, 3, 0},
	    {2, 3, 0, 1},
	    {3, 0, 1, 2},
	    {0, 1, 2, 3},
	};

	const float scale = 1.f / (2047.f * sqrtf(2.f));

	short* data = static_cast<short*>(buffer);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		// convert x/y/z to [-1..1] (scaled...)
		float x = float(data[i * 4 + 0]) * scale;
		float y = float(data[i * 4 + 1]) * scale;
		float z = float(data[i * 4 + 2]) * scale;

		// reconstruct w as a square root; we clamp to 0.f to avoid NaN due to precision errors
		float ww = 1.f - x * x - y * y - z * z;
		float w = sqrtf(ww >= 0.f ? ww : 0.f);

		// rounded signed float->int
		int xf = int(x * 32767.f + (x >= 0.f ? 0.5f : -0.5f));
		int yf = int(y * 32767.f + (y >= 0.f ? 0.5f : -0.5f));
		int zf = int(z * 32767.f + (z >= 0.f ? 0.5f : -0.5f));
		int wf = int(w * 32767.f + 0.5f);

		int qc = data[i * 4 + 3] & 3;

		// output order is dictated by input index
		data[i * 4 + order[qc][0]] = short(xf);
		data[i * 4 + order[qc][1]] = short(yf);
		data[i * 4 + order[qc][2]] = short(zf);
		data[i * 4 + order[qc][3]] = short(wf);
	}
}

#undef SIMD_WASM
