#include <cstdio>
#include <ctime>

#include "math.hpp"
#include "overdrawoptimizertipsify.hpp"
#include "posttloptimizertipsify.hpp"
#include "posttloptimizertomf.hpp"
#include "pretloptimizer.hpp"
#include "vcacheanalyzer.hpp"

const size_t kCacheSize = 24;

struct Vertex
{
	vec3 position;
	vec3 normal;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};

Mesh generatePlane(unsigned int N)
{
	Mesh result;

	result.vertices.reserve((N + 1) * (N + 1));
	result.indices.reserve(N * N * 6);

	for (unsigned int y = 0; y <= N; ++y)
		for (unsigned int x = 0; x <= N; ++x)
		{
			Vertex v = { vec3(x, y, 0), vec3(0, 0, 1) };

			result.vertices.push_back(v);
		}

	for (unsigned int y = 0; y < N; ++y)
		for (unsigned int x = 0; x < N; ++x)
		{
			result.indices.push_back((y + 0) * N + (x + 0));
			result.indices.push_back((y + 0) * N + (x + 1));
			result.indices.push_back((y + 1) * N + (x + 0));

			result.indices.push_back((y + 1) * N + (x + 0));
			result.indices.push_back((y + 0) * N + (x + 1));
			result.indices.push_back((y + 1) * N + (x + 1));
		}

	return result;
}

void optimizeNone(Mesh& mesh)
{
}

void optimizeTomF(Mesh& mesh)
{
	std::vector<unsigned int> result(mesh.indices.size());

	optimizePostTLTomF(&result[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size());

	mesh.indices.swap(result);
}

void optimizeTipsify(Mesh& mesh)
{
	std::vector<unsigned int> result(mesh.indices.size());

	optimizePostTLTipsify(&result[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize);

	mesh.indices.swap(result);
}

void optimizeTipsifyOverdraw(Mesh& mesh)
{
	std::vector<unsigned int> result(mesh.indices.size());

	std::vector<unsigned int> clusters;
	optimizePostTLTipsify(&result[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize, &clusters);

	const float kThreshold = 1;

	optimizeOverdrawTipsify(&mesh.indices[0], &result[0], mesh.indices.size(), &mesh.vertices[0].position, sizeof(Vertex), mesh.vertices.size(), clusters, kCacheSize, kThreshold);

	mesh.indices.swap(result);
}


void optimize(const Mesh& mesh, const char* name, void (*optf)(Mesh& mesh))
{
	Mesh copy = mesh;

	clock_t start = clock();
	optf(copy);
	clock_t end = clock();

	PostTLCacheStatistics stats = analyzePostTL(&copy.indices[0], copy.indices.size(), copy.vertices.size(), kCacheSize);

	printf("%s: ACMR %f in %f msec (%d triangles)\n", name, stats.acmr, double(end - start) / CLOCKS_PER_SEC * 1000, int(mesh.indices.size() / 3));
}

int main()
{
	Mesh mesh = generatePlane(1000);

	optimize(mesh, "none", optimizeNone);
	optimize(mesh, "TomF", optimizeTomF);
	optimize(mesh, "Tipsify", optimizeTipsify);
	optimize(mesh, "Tipsify + overdraw", optimizeTipsifyOverdraw);
}
