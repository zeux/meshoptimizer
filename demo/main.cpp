#include "../src/meshoptimizer.hpp"

#include <algorithm>
#include <cstdio>
#include <ctime>

#include "objparser.hpp"

const size_t kCacheSize = 24;

struct Vertex
{
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;
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
			Vertex v = {float(x), float(y), 0, 0, 0, 1};

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

Mesh parseObj(const char* path)
{
	ObjFile file;

	if (!objParseFile(file, path))
	{
		printf("Error loading %s\n", path);
		return Mesh();
	}

	objTriangulate(file);

	size_t total_indices = file.f.size() / 3;

	std::vector<Vertex> vertices;
	vertices.reserve(total_indices);

	for (size_t i = 0; i < file.f.size(); i += 3)
	{
		int vi = file.f[i + 0];
		int vti = file.f[i + 1];
		int vni = file.f[i + 2];

		Vertex v =
		    {
		        file.v[vi * 4 + 0],
		        file.v[vi * 4 + 1],
		        file.v[vi * 4 + 2],

		        vni >= 0 ? file.vn[vni * 3 + 0] : 0,
		        vni >= 0 ? file.vn[vni * 3 + 1] : 0,
		        vni >= 0 ? file.vn[vni * 3 + 2] : 0,

		        vti >= 0 ? file.vt[vti * 3 + 0] : 0,
		        vti >= 0 ? file.vt[vti * 3 + 1] : 0,
		    };

		vertices.push_back(v);
	}

	Mesh result;

	result.indices.resize(total_indices);

	size_t total_vertices = generateIndexBuffer(&result.indices[0], &vertices[0], total_indices, sizeof(Vertex));

	result.vertices.resize(total_vertices);

	generateVertexBuffer(&result.vertices[0], &result.indices[0], &vertices[0], total_indices, sizeof(Vertex));

	return result;
}

void optNone(Mesh& mesh)
{
}

void optRandomShuffle(Mesh& mesh)
{
	std::vector<unsigned int> faces(mesh.indices.size() / 3);

	for (size_t i = 0; i < faces.size(); ++i)
		faces[i] = static_cast<unsigned int>(i);

	std::random_shuffle(faces.begin(), faces.end());

	std::vector<unsigned int> result(mesh.indices.size());

	for (size_t i = 0; i < faces.size(); ++i)
	{
		result[i * 3 + 0] = mesh.indices[faces[i] * 3 + 0];
		result[i * 3 + 1] = mesh.indices[faces[i] * 3 + 1];
		result[i * 3 + 2] = mesh.indices[faces[i] * 3 + 2];
	}

	mesh.indices.swap(result);
}

void optTransform(Mesh& mesh)
{
	std::vector<unsigned int> result(mesh.indices.size());
	optimizePostTransform(&result[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize);
	mesh.indices.swap(result);
}

void optOverdraw(Mesh& mesh)
{
	// use single input cluster encompassing the entire mesh and worst-case ACMR so that overdraw optimizer can sort *all* triangles
	// warning: this significantly deteriorates the vertex transform cache efficiency so it is not advised; look at optComplete for the recommended method
	std::vector<unsigned int> clusters(1);
	const float kThreshold = 3.f;

	std::vector<unsigned int> result(mesh.indices.size());
	optimizeOverdraw(&result[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, sizeof(Vertex), mesh.vertices.size(), clusters, kCacheSize, kThreshold);
	mesh.indices.swap(result);
}

void optFetch(Mesh& mesh)
{
	std::vector<Vertex> result(mesh.vertices.size());
	optimizePreTransform(&result[0], &mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex));
	mesh.vertices.swap(result);
}

void optComplete(Mesh& mesh)
{
	// post-transform optimization should go first as it provides data for overdraw
	std::vector<unsigned int> newindices(mesh.indices.size());
	std::vector<unsigned int> clusters;
	optimizePostTransform(&newindices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize, &clusters);

	// reorder indices for overdraw, balancing overdraw and vertex transform efficiency
	const float kThreshold = 1.05f; // allow up to 5% worse ACMR to get more reordering opportunities for overdraw
	optimizeOverdraw(&mesh.indices[0], &newindices[0], mesh.indices.size(), &mesh.vertices[0].px, sizeof(Vertex), mesh.vertices.size(), clusters, kCacheSize, kThreshold);

	// pre-transform optimization should go last as it depends on the final index order
	std::vector<Vertex> newvertices(mesh.vertices.size());
	optimizePreTransform(&newvertices[0], &mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex));
	mesh.vertices.swap(newvertices);
}

void optimize(const Mesh& mesh, const char* name, void (*optf)(Mesh& mesh))
{
	Mesh copy = mesh;

	clock_t start = clock();
	optf(copy);
	clock_t end = clock();

	PostTransformCacheStatistics vts = analyzePostTransform(&copy.indices[0], copy.indices.size(), copy.vertices.size(), kCacheSize);
	PreTransformCacheStatistics vfs = analyzePreTransform(&copy.indices[0], copy.indices.size(), copy.vertices.size(), sizeof(Vertex));
	OverdrawStatistics os = analyzeOverdraw(&copy.indices[0], copy.indices.size(), &copy.vertices[0].px, sizeof(Vertex), copy.vertices.size());

	printf("%-15s: ACMR %f ATVR %f Overfetch %f Overdraw %f in %f msec\n", name, vts.acmr, vts.atvr, vfs.overfetch, os.overdraw, double(end - start) / CLOCKS_PER_SEC * 1000);
}

int main(int argc, char** argv)
{
	Mesh mesh;

	if (argc > 1)
	{
		clock_t start = clock();
		mesh = parseObj(argv[1]);
		clock_t end = clock();

		if (mesh.vertices.empty())
		{
			printf("Mesh %s appears to be empty\n", argv[1]);
			return 0;
		}

		printf("Using %s (%d vertices, %d triangles); read in %f msec\n", argv[1], int(mesh.vertices.size()), int(mesh.indices.size() / 3), double(end - start) / CLOCKS_PER_SEC * 1000);
	}
	else
	{
		printf("Usage: %s [.obj file]\n", argv[0]);

		mesh = generatePlane(1000);

		printf("Using a tesselated plane (%d vertices, %d triangles)\n", int(mesh.vertices.size()), int(mesh.indices.size() / 3));
	}

	optimize(mesh, "Original", optNone);
	optimize(mesh, "Random Shuffle", optRandomShuffle);
	optimize(mesh, "Transform", optTransform);
	optimize(mesh, "Overdraw", optOverdraw);
	optimize(mesh, "Fetch", optFetch);
	optimize(mesh, "Cache+Overdraw", optComplete);
}
