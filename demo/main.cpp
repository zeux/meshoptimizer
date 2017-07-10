#include "../src/meshoptimizer.hpp"

#include <algorithm>
#include <cstdio>
#include <ctime>

#include "objparser.hpp"

using namespace meshopt;

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
			Vertex v = {float(x), float(y), 0, 0, 0, 1, float(x) / float(N), float(y) / float(N)};

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
		printf("Error loading %s: file not found\n", path);
		return Mesh();
	}

	if (!objValidate(file))
	{
		printf("Error loading %s: invalid file data\n", path);
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
		        file.v[vi * 3 + 0],
		        file.v[vi * 3 + 1],
		        file.v[vi * 3 + 2],

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
	(void)mesh;
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

void optCache(Mesh& mesh)
{
	optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize);
}

void optOverdraw(Mesh& mesh)
{
	// use single input cluster encompassing the entire mesh and worst-case ACMR so that overdraw optimizer can sort *all* triangles
	// warning: this significantly deteriorates the vertex cache efficiency so it is not advised; look at optComplete for the recommended method
	const unsigned int clusters[] = {0};
	const float kThreshold = 3.f;

	optimizeOverdraw(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, sizeof(Vertex), mesh.vertices.size(), clusters, sizeof(clusters) / sizeof(clusters[0]), kCacheSize, kThreshold);
}

void optFetch(Mesh& mesh)
{
	optimizeVertexFetch(&mesh.vertices[0], &mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex));
}

void optComplete(Mesh& mesh)
{
	// vertex cache optimization should go first as it provides data for overdraw
	std::vector<unsigned int> clusters(mesh.indices.size() / 3);
	size_t cluster_count = 0;
	optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize, &clusters[0], &cluster_count);

	// reorder indices for overdraw, balancing overdraw and vertex cache efficiency
	const float kThreshold = 1.05f; // allow up to 5% worse ACMR to get more reordering opportunities for overdraw
	optimizeOverdraw(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, sizeof(Vertex), mesh.vertices.size(), &clusters[0], cluster_count, kCacheSize, kThreshold);

	// vertex fetch optimization should go last as it depends on the final index order
	optimizeVertexFetch(&mesh.vertices[0], &mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex));
}

void optimize(const Mesh& mesh, const char* name, void (*optf)(Mesh& mesh))
{
	Mesh copy = mesh;

	clock_t start = clock();
	optf(copy);
	clock_t end = clock();

	VertexCacheStatistics vcs = analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), kCacheSize);
	VertexFetchStatistics vfs = analyzeVertexFetch(&copy.indices[0], copy.indices.size(), copy.vertices.size(), sizeof(Vertex));
	OverdrawStatistics os = analyzeOverdraw(&copy.indices[0], copy.indices.size(), &copy.vertices[0].px, sizeof(Vertex), copy.vertices.size());

	printf("%-9s: ACMR %f ATVR %f Overfetch %f Overdraw %f in %f msec\n", name, vcs.acmr, vcs.atvr, vfs.overfetch, os.overdraw, double(end - start) / CLOCKS_PER_SEC * 1000);
}

void process(const char* path)
{
	Mesh mesh;

	if (path)
	{
		clock_t start = clock();
		mesh = parseObj(path);
		clock_t end = clock();

		if (mesh.vertices.empty())
		{
			printf("Mesh %s is empty, skipping\n", path);
			return;
		}

		printf("# %s: %d vertices, %d triangles; read in %f msec\n", path, int(mesh.vertices.size()), int(mesh.indices.size() / 3), double(end - start) / CLOCKS_PER_SEC * 1000);
	}
	else
	{
		mesh = generatePlane(1000);

		printf("# tessellated plane: %d vertices, %d triangles\n", int(mesh.vertices.size()), int(mesh.indices.size() / 3));
	}

	optimize(mesh, "Original", optNone);
	optimize(mesh, "Random", optRandomShuffle);
	optimize(mesh, "Cache", optCache);
	optimize(mesh, "Overdraw", optOverdraw);
	optimize(mesh, "Fetch", optFetch);
	optimize(mesh, "Complete", optComplete);
}

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: %s [.obj file]\n", argv[0]);
		process(0);
	}
	else
	{
		for (int i = 1; i < argc; ++i)
		{
			process(argv[i]);
		}
	}
}
