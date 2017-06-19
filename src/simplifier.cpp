#include "meshoptimizer.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace
{
	struct Vector3
	{
		float x, y, z;
	};

	struct Quadric
	{
		// TODO the matrix is symmetric, so we only really need 10 floats here
		float m[4][4];
	};

	struct Collapse
	{
		size_t v0;
		size_t v1;
		float error;

		bool operator<(const Collapse& other) const
		{
			return error < other.error;
		}
	};

	void quadricFromPlane(Quadric& Q, float a, float b, float c, float d)
	{
		Q.m[0][0] = a * a;
		Q.m[0][1] = a * b;
		Q.m[0][2] = a * c;
		Q.m[0][3] = a * d;
		Q.m[1][0] = b * a;
		Q.m[1][1] = b * b;
		Q.m[1][2] = b * c;
		Q.m[1][3] = b * d;
		Q.m[2][0] = c * a;
		Q.m[2][1] = c * b;
		Q.m[2][2] = c * c;
		Q.m[2][3] = c * d;
		Q.m[3][0] = d * a;
		Q.m[3][1] = d * b;
		Q.m[3][2] = d * c;
		Q.m[3][3] = d * d;
	}

	void quadricAdd(Quadric& Q, const Quadric& R)
	{
		Q.m[0][0] += R.m[0][0];
		Q.m[0][1] += R.m[0][1];
		Q.m[0][2] += R.m[0][2];
		Q.m[0][3] += R.m[0][3];
		Q.m[1][0] += R.m[1][0];
		Q.m[1][1] += R.m[1][1];
		Q.m[1][2] += R.m[1][2];
		Q.m[1][3] += R.m[1][3];
		Q.m[2][0] += R.m[2][0];
		Q.m[2][1] += R.m[2][1];
		Q.m[2][2] += R.m[2][2];
		Q.m[2][3] += R.m[2][3];
		Q.m[3][0] += R.m[3][0];
		Q.m[3][1] += R.m[3][1];
		Q.m[3][2] += R.m[3][2];
		Q.m[3][3] += R.m[3][3];
	}

	void quadricMul(Quadric& Q, float s)
	{
		Q.m[0][0] *= s;
		Q.m[0][1] *= s;
		Q.m[0][2] *= s;
		Q.m[0][3] *= s;
		Q.m[1][0] *= s;
		Q.m[1][1] *= s;
		Q.m[1][2] *= s;
		Q.m[1][3] *= s;
		Q.m[2][0] *= s;
		Q.m[2][1] *= s;
		Q.m[2][2] *= s;
		Q.m[2][3] *= s;
		Q.m[3][0] *= s;
		Q.m[3][1] *= s;
		Q.m[3][2] *= s;
		Q.m[3][3] *= s;
	}

	void quadricFromTriangle(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2)
	{
		Vector3 p10 = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
		Vector3 p20 = { p2.x - p0.x, p2.y - p0.y, p2.z - p0.z };
		Vector3 normal = { p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x };

		float area = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

		if (area > 0)
		{
			normal.x /= area;
			normal.y /= area;
			normal.z /= area;
		}

		float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

		quadricFromPlane(Q, normal.x, normal.y, normal.z, -distance);

		// Three classical weighting methods include weight=1, weight=area and weight=area^2
		// We use weight=area for now
		quadricMul(Q, area);
	}

	float quadricError(Quadric& Q, const Vector3& v)
	{
		float vTQv =
			(Q.m[0][0] * v.x + Q.m[0][1] * v.y + Q.m[0][2] * v.z + Q.m[0][3]) * v.x +
			(Q.m[1][0] * v.x + Q.m[1][1] * v.y + Q.m[1][2] * v.z + Q.m[1][3]) * v.y +
			(Q.m[2][0] * v.x + Q.m[2][1] * v.y + Q.m[2][2] * v.z + Q.m[2][3]) * v.z +
			(Q.m[3][0] * v.x + Q.m[3][1] * v.y + Q.m[3][2] * v.z + Q.m[3][3]);

		return fabsf(vTQv);
	}

	size_t simplifyEdgeCollapse(unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size, size_t target_index_count)
	{
		std::vector<Vector3> vertex_positions(vertex_count);

		for (size_t i = 0; i < vertex_count; ++i)
		{
			const float* v = reinterpret_cast<const float*>(static_cast<const char*>(vertices) + i * vertex_size);

			vertex_positions[i].x = v[0];
			vertex_positions[i].y = v[1];
			vertex_positions[i].z = v[2];
		}

		std::vector<Quadric> vertex_quadrics(vertex_count);

		for (size_t i = 0; i < index_count; i += 3)
		{
			Quadric Q;
			quadricFromTriangle(Q, vertex_positions[indices[i + 0]], vertex_positions[indices[i + 1]], vertex_positions[indices[i + 2]]);

			quadricAdd(vertex_quadrics[indices[i + 0]], Q);
			quadricAdd(vertex_quadrics[indices[i + 1]], Q);
			quadricAdd(vertex_quadrics[indices[i + 2]], Q);
		}

		while (index_count > target_index_count)
		{
			std::vector<Collapse> edge_collapses;
			edge_collapses.reserve(index_count);

			for (size_t i = 0; i < index_count; i += 3)
			{
				static const int next[3] = { 1, 2, 0 };

				for (int e = 0; e < 3; ++e)
				{
					unsigned int i0 = indices[i + e];
					unsigned int i1 = indices[i + next[e]];

					Collapse c01 = { i0, i1, quadricError(vertex_quadrics[i0], vertex_positions[i1]) };
					Collapse c10 = { i1, i0, quadricError(vertex_quadrics[i1], vertex_positions[i0]) };

					edge_collapses.push_back(c01.error <= c10.error ? c01 : c10);
				}
			}

			std::sort(edge_collapses.begin(), edge_collapses.end());

			std::vector<unsigned int> vertex_remap(vertex_count);

			for (size_t i = 0; i < vertex_remap.size(); ++i)
			{
				vertex_remap[i] = unsigned(i);
			}

			std::vector<char> vertex_locked(vertex_count);

			// each collapse removes 2 triangles
			size_t edge_collapse_goal = (index_count - target_index_count) / 6 + 1;

			size_t collapses = 0;
			float worst_error = 0;

			for (size_t i = 0; i < edge_collapses.size(); ++i)
			{
				const Collapse& c = edge_collapses[i];

				if (vertex_locked[c.v0] || vertex_locked[c.v1])
					continue;

				assert(vertex_remap[c.v0] == c.v0);
				assert(vertex_remap[c.v1] == c.v1);

				quadricAdd(vertex_quadrics[c.v1], vertex_quadrics[c.v0]);

				vertex_remap[c.v0] = unsigned(c.v1);

				vertex_locked[c.v0] = 1;
				vertex_locked[c.v1] = 1;

				collapses++;
				worst_error = c.error;

				if (collapses >= edge_collapse_goal)
					break;
			}

			printf("collapses: %d/%d, worst error: %e\n", int(collapses), int(edge_collapses.size()), worst_error);

			// no edges can be collapsed any more => bail out
			if (collapses == 0)
				break;

			size_t write = 0;

			for (size_t i = 0; i < index_count; i += 3)
			{
				unsigned int v0 = vertex_remap[indices[i + 0]];
				unsigned int v1 = vertex_remap[indices[i + 1]];
				unsigned int v2 = vertex_remap[indices[i + 2]];

				assert(vertex_remap[v0] == v0);
				assert(vertex_remap[v1] == v1);
				assert(vertex_remap[v2] == v2);

				if (v0 != v1 && v0 != v2 && v1 != v2)
				{
					indices[write + 0] = v0;
					indices[write + 1] = v1;
					indices[write + 2] = v2;
					write += 3;
				}
			}

			index_count = write;
		}

		return index_count;
	}
}

size_t simplify(unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size, size_t target_index_count)
{
	return simplifyEdgeCollapse(indices, index_count, vertices, vertex_count, vertex_size, target_index_count);

}
