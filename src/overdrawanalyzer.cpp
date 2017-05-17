#include "meshoptimizer.hpp"

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cstring>

namespace
{
	const int kViewport = 256;

	struct OverdrawBuffer
	{
		float z[kViewport][kViewport];
		unsigned int overdraw[kViewport][kViewport];
	};

	inline float det2x2(float a, float b, float c, float d)
	{
		// (a b)
		// (c d)
		return a * d - b * c;
	}

	inline float computeDepthGradients(float& dzdx, float& dzdy, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3)
	{
		// z2 = z1 + dzdx * (x2 - x1) + dzdy * (y2 - y1)
		// z3 = z1 + dzdx * (x3 - x1) + dzdy * (y3 - y1)
		// (x2-x1 y2-y1)(dzdx) = (z2-z1)
		// (x3-x1 y3-y1)(dzdy)   (z3-z1)
		// we'll solve it with Cramer's rule
		float det = det2x2(x2 - x1, y2 - y1, x3 - x1, y3 - y1);
		float invdet = (det == 0) ? 0 : 1 / det;

		dzdx = det2x2(z2 - z1, y2 - y1, z3 - z1, y3 - y1) * invdet;
		dzdy = det2x2(x2 - x1, z2 - z1, x3 - x1, z3 - z1) * invdet;

		return det;
	}

	// half-space fixed point triangle rasterizer
	// adapted from http://forum.devmaster.net/t/advanced-rasterization/6145
	void rasterize(OverdrawBuffer& buffer, float v1x, float v1y, float v1z, float v2x, float v2y, float v2z, float v3x, float v3y, float v3z)
	{
		// compute depth gradients
		float DZx, DZy;
		float det = computeDepthGradients(DZx, DZy, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z);

		// cull backface/degenerate triangles
		if (det >= 0)
			return;

		// 28.4 fixed-point coordinates
		const int X1 = int(16.0f * v1x + 0.5f);
		const int X2 = int(16.0f * v2x + 0.5f);
		const int X3 = int(16.0f * v3x + 0.5f);

		const int Y1 = int(16.0f * v1y + 0.5f);
		const int Y2 = int(16.0f * v2y + 0.5f);
		const int Y3 = int(16.0f * v3y + 0.5f);

		// deltas
		const int DX12 = X1 - X2;
		const int DX23 = X2 - X3;
		const int DX31 = X3 - X1;

		const int DY12 = Y1 - Y2;
		const int DY23 = Y2 - Y3;
		const int DY31 = Y3 - Y1;

		// bounding rectangle
		int minx = std::min(X1, std::min(X2, X3)) >> 4;
		int maxx = (std::max(X1, std::max(X2, X3)) + 0xF) >> 4;
		int miny = std::min(Y1, std::min(Y2, Y3)) >> 4;
		int maxy = (std::max(Y1, std::max(Y2, Y3)) + 0xF) >> 4;

		assert(minx >= 0 && maxx <= kViewport);
		assert(miny >= 0 && maxy <= kViewport);

		// half-edge constants
		int C1 = DY12 * X1 - DX12 * Y1;
		int C2 = DY23 * X2 - DX23 * Y2;
		int C3 = DY31 * X3 - DX31 * Y3;

		// correct for fill convention
		if (DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
		if (DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
		if (DY31 < 0 || (DY31 == 0 && DX31 > 0)) C3++;

		int CY1 = C1 + DX12 * (miny << 4) - DY12 * (minx << 4);
		int CY2 = C2 + DX23 * (miny << 4) - DY23 * (minx << 4);
		int CY3 = C3 + DX31 * (miny << 4) - DY31 * (minx << 4);

		float ZY = v1z + DZx * (minx - v1x) + DZy * (miny - v1y);

		for (int y = miny; y < maxy; y++)
		{
			int CX1 = CY1;
			int CX2 = CY2;
			int CX3 = CY3;
			float ZX = ZY;

			for (int x = minx; x < maxx; x++)
			{
				if (CX1 > 0 && CX2 > 0 && CX3 > 0)
				{
					if (ZX >= buffer.z[y][x])
					{
						buffer.z[y][x] = ZX;
						buffer.overdraw[y][x]++;
					}
				}

				CX1 -= DY12 << 4;
				CX2 -= DY23 << 4;
				CX3 -= DY31 << 4;
				ZX += DZx;
			}

			CY1 += DX12 << 4;
			CY2 += DX23 << 4;
			CY3 += DX31 << 4;
			ZY += DZy;
		}
	}

	template <typename T>
	OverdrawStatistics analyzeOverdrawImpl(const T* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, size_t vertex_count)
	{
		assert(vertex_positions_stride > 0);
		assert(vertex_positions_stride % sizeof(float) == 0);

		size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

		OverdrawStatistics result = {};

		float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
		float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

		for (size_t i = 0; i < vertex_count; ++i)
		{
			const float* v = vertex_positions + i * vertex_stride_float;

			for (int j = 0; j < 3; ++j)
			{
				minv[j] = std::min(minv[j], v[j]);
				maxv[j] = std::max(maxv[j], v[j]);
			}
		}

		float radius = std::max(maxv[0] - minv[0], std::max(maxv[1] - minv[1], maxv[2] - minv[2]));
		float scale = kViewport / radius;

		OverdrawBuffer* buffer = new OverdrawBuffer();

		for (int axis = 0; axis < 6; ++axis)
		{
			memset(buffer, 0, sizeof(OverdrawBuffer));

			for (size_t i = 0; i < index_count; i += 3)
			{
				const float* v0 = vertex_positions + indices[i + 0] * vertex_stride_float;
				const float* v1 = vertex_positions + indices[i + 1] * vertex_stride_float;
				const float* v2 = vertex_positions + indices[i + 2] * vertex_stride_float;

				float vn0[3] = {(v0[0] - minv[0]) * scale, (v0[1] - minv[1]) * scale, (v0[2] - minv[2]) * scale};
				float vn1[3] = {(v1[0] - minv[0]) * scale, (v1[1] - minv[1]) * scale, (v1[2] - minv[2]) * scale};
				float vn2[3] = {(v2[0] - minv[0]) * scale, (v2[1] - minv[1]) * scale, (v2[2] - minv[2]) * scale};

				switch (axis)
				{
				case 0: /* +X */
					rasterize(*buffer, kViewport - vn0[2], kViewport - vn0[1], vn0[0], kViewport - vn1[2], kViewport - vn1[1], vn1[0], kViewport - vn2[2], kViewport - vn2[1], vn2[0]);
					break;
				case 1: /* -X */
					rasterize(*buffer, vn0[2], kViewport - vn0[1], kViewport - vn0[0], vn1[2], kViewport - vn1[1], kViewport - vn1[0], vn2[2], kViewport - vn2[1], kViewport - vn2[0]);
					break;
				case 2: /* +Y */
					rasterize(*buffer, vn0[0], vn0[2], vn0[1], vn1[0], vn1[2], vn1[1], vn2[0], vn2[2], vn2[1]);
					break;
				case 3: /* -Y */
					rasterize(*buffer, vn0[2], vn0[0], kViewport - vn0[1], vn1[2], vn1[0], kViewport - vn1[1], vn2[2], vn2[0], kViewport - vn2[1]);
					break;
				case 4: /* +Z */
					rasterize(*buffer, vn0[0], kViewport - vn0[1], vn0[2], vn1[0], kViewport - vn1[1], vn1[2], vn2[0], kViewport - vn2[1], vn2[2]);
					break;
				case 5: /* -Z */
					rasterize(*buffer, kViewport - vn0[0], kViewport - vn0[1], kViewport - vn0[2], kViewport - vn1[0], kViewport - vn1[1], kViewport - vn1[2], kViewport - vn2[0], kViewport - vn2[1], kViewport - vn2[2]);
					break;
				}
			}

			for (int y = 0; y < kViewport; ++y)
				for (int x = 0; x < kViewport; ++x)
					if (buffer->overdraw[y][x])
					{
						result.pixels_covered += 1;
						result.pixels_shaded += buffer->overdraw[y][x];
					}
		}

		delete buffer;

		result.overdraw = static_cast<float>(result.pixels_shaded) / static_cast<float>(result.pixels_covered);

		return result;
	}
}

OverdrawStatistics analyzeOverdraw(const unsigned short* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, size_t vertex_count)
{
	return analyzeOverdrawImpl(indices, index_count, vertex_positions, vertex_positions_stride, vertex_count);
}

OverdrawStatistics analyzeOverdraw(const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, size_t vertex_count)
{
	return analyzeOverdrawImpl(indices, index_count, vertex_positions, vertex_positions_stride, vertex_count);
}
