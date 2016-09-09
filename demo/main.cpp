#include <cstdio>
#include <ctime>

#include "../src/math.hpp"
#include "../src/meshoptimizer.hpp"

#include "tiny_obj_loader.h"

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

Mesh readOBJ(const char* path)
{
	using namespace tinyobj;

	attrib_t attrib;
	std::vector<shape_t> shapes;
	std::vector<material_t> materials;
	std::string error;

	if (!LoadObj(&attrib, &shapes, &materials, &error, path))
	{
		printf("Error loading %s: %s\n", path, error.c_str());
		return Mesh();
	}

	Mesh result;

	result.vertices.reserve(attrib.vertices.size() / 3);

	for (size_t i = 0; i < attrib.vertices.size(); i += 3)
	{
		Vertex v = { vec3(&attrib.vertices[i]), vec3(0,0,1) };

		result.vertices.push_back(v);
	}

	size_t total_indices = 0;

	for (auto& s: shapes)
		total_indices += s.mesh.indices.size();

	result.indices.reserve(total_indices);

	for (auto& s: shapes)
		for (size_t i = 0; i < s.mesh.indices.size(); ++i)
			result.indices.push_back(s.mesh.indices[i].vertex_index);

	return result;
}

void optimizeNone(Mesh& mesh)
{
}

void optimizeTomF(Mesh& mesh)
{
	std::vector<unsigned int> result(mesh.indices.size());

	optimizePostTransformTomF(&result[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), std::min(int(kCacheSize), 32));

	mesh.indices.swap(result);
}

void optimizeTipsify(Mesh& mesh)
{
	std::vector<unsigned int> result(mesh.indices.size());

	optimizePostTransformTipsify(&result[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize);

	mesh.indices.swap(result);
}

void optimizeTipsifyOverdraw(Mesh& mesh)
{
	std::vector<unsigned int> result(mesh.indices.size());

	std::vector<unsigned int> clusters;
	optimizePostTransformTipsify(&result[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize, &clusters);

	const float kThreshold = 1.05f; // allow up to 5% worse ACMR to get more reordering opportunities for overdraw

	optimizeOverdrawTipsify(&mesh.indices[0], &result[0], mesh.indices.size(), &mesh.vertices[0].position, sizeof(Vertex), mesh.vertices.size(), clusters, kCacheSize, kThreshold);
}

void optimize(const Mesh& mesh, const char* name, void (*optf)(Mesh& mesh))
{
	Mesh copy = mesh;

	clock_t start = clock();
	optf(copy);
	clock_t end = clock();

	PostTransformCacheStatistics stats = analyzePostTransform(&copy.indices[0], copy.indices.size(), copy.vertices.size(), kCacheSize);

	printf("%s: ACMR %f in %f msec\n", name, stats.acmr, double(end - start) / CLOCKS_PER_SEC * 1000);
}

int main(int argc, char** argv)
{
	Mesh mesh;

	if (argc > 1)
	{
		mesh = readOBJ(argv[1]);

		if (mesh.vertices.empty())
		{
			printf("Mesh %s appears to be empty\n", argv[1]);
			return 0;
		}

		printf("Using %s (%d vertices, %d triangles)\n", argv[1], int(mesh.vertices.size()), int(mesh.indices.size() / 3));
	}
	else
	{
		printf("Usage: %s [.obj file]\n", argv[0]);

		mesh = generatePlane(1000);

		printf("Using a tesselated plane (%d vertices, %d triangles)\n", int(mesh.vertices.size()), int(mesh.indices.size() / 3));
	}

	optimize(mesh, "Original", optimizeNone);
	optimize(mesh, "Tipsify", optimizeTipsify);
	optimize(mesh, "Tipsify + overdraw", optimizeTipsifyOverdraw);
	optimize(mesh, "TomF", optimizeTomF);
}
