#include "../src/meshoptimizer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "objparser.h"

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
			result.indices.push_back((y + 0) * (N + 1) + (x + 0));
			result.indices.push_back((y + 0) * (N + 1) + (x + 1));
			result.indices.push_back((y + 1) * (N + 1) + (x + 0));

			result.indices.push_back((y + 1) * (N + 1) + (x + 0));
			result.indices.push_back((y + 0) * (N + 1) + (x + 1));
			result.indices.push_back((y + 1) * (N + 1) + (x + 1));
		}

	return result;
}

Mesh parseObj(const char* path, clock_t& reindex)
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

	reindex = clock();

	Mesh result;

	std::vector<unsigned int> remap(total_indices);

	size_t total_vertices = generateVertexRemap(&remap[0], (unsigned int*)0, total_indices, &vertices[0], total_indices, sizeof(Vertex));

	result.indices.resize(total_indices);
	remapIndexBuffer(&result.indices[0], (unsigned int*)0, total_indices, &remap[0]);

	result.vertices.resize(total_vertices);
	remapVertexBuffer(&result.vertices[0], &vertices[0], total_indices, sizeof(Vertex), &remap[0]);

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

void optCache0(Mesh& mesh)
{
	optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), 0);
}

void optOverdraw(Mesh& mesh)
{
	// use worst-case ACMR threshold so that overdraw optimizer can sort *all* triangles
	// warning: this significantly deteriorates the vertex cache efficiency so it is not advised; look at optComplete for the recommended method
	const float kThreshold = 3.f;
	optimizeOverdraw(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, sizeof(Vertex), mesh.vertices.size(), kCacheSize, kThreshold);
}

void optFetch(Mesh& mesh)
{
	optimizeVertexFetch(&mesh.vertices[0], &mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex));
}

void optComplete(Mesh& mesh)
{
	// vertex cache optimization should go first as it provides data for overdraw
	optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize);

	// reorder indices for overdraw, balancing overdraw and vertex cache efficiency
	const float kThreshold = 1.05f; // allow up to 5% worse ACMR to get more reordering opportunities for overdraw
	optimizeOverdraw(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, sizeof(Vertex), mesh.vertices.size(), kCacheSize, kThreshold);

	// vertex fetch optimization should go last as it depends on the final index order
	optimizeVertexFetch(&mesh.vertices[0], &mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex));
}

void optCompleteSimplify(Mesh& mesh)
{
	static const size_t lod_count = 5;

	// generate 4 LOD levels (1-4), with each subsequent LOD using 70% triangles
	// note that each LOD uses the same (shared) vertex buffer
	// vertex cache optimization should go first as it provides data for overdraw
	std::vector<unsigned int> lods[lod_count];

	lods[0] = mesh.indices;

	for (size_t i = 1; i < lod_count; ++i)
	{
		std::vector<unsigned int>& lod = lods[i];

		float threshold = powf(0.7f, float(i));
		size_t target_index_count = size_t(mesh.indices.size() * threshold);

		// we can simplify all the way from base level or from the last result
		// simplifying from the base level sometimes produces better results, but simplifying from last level is faster
		const std::vector<unsigned int>& source = lods[i - 1];

		lod.resize(source.size());
		lod.resize(simplify(&lod[0], &source[0], source.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex), target_index_count));
	}

	// optimize each individual LOD for vertex cache & overdraw
	for (size_t i = 0; i < lod_count; ++i)
	{
		std::vector<unsigned int>& lod = lods[i];

		optimizeVertexCache(&lod[0], &lod[0], lod.size(), mesh.vertices.size(), kCacheSize);
		optimizeOverdraw(&lod[0], &lod[0], lod.size(), &mesh.vertices[0].px, sizeof(Vertex), mesh.vertices.size(), kCacheSize, 1.0f);
	}

	// concatenate all LODs into one IB
	// note: the order of concatenation is important - since we optimize the entire IB for vertex fetch,
	// putting coarse LODs first makes sure that the vertex range referenced by them is as small as possible
	// some GPUs process the entire range referenced by the index buffer region so doing this optimizes the vertex transform
	// cost for coarse LODs
	size_t lod_index_offsets[lod_count] = {};
	size_t lod_index_counts[lod_count] = {};
	size_t total_index_count = 0;

	for (int i = lod_count - 1; i >= 0; --i)
	{
		lod_index_offsets[i] = total_index_count;
		lod_index_counts[i] = lods[i].size();

		total_index_count += lods[i].size();
	}

	mesh.indices.resize(total_index_count);

	for (size_t i = 0; i < lod_count; ++i)
	{
		memcpy(&mesh.indices[lod_index_offsets[i]], lods[i].data(), lods[i].size() * sizeof(lods[i][0]));
	}

	// vertex fetch optimization should go last as it depends on the final index order
	// note that the order of LODs above affects vertex fetch results
	optimizeVertexFetch(&mesh.vertices[0], &mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex));

	printf("%-9s:", "Simplify");

	for (size_t i = 0; i < sizeof(lods) / sizeof(lods[0]); ++i)
	{
		printf(" LOD%d %d", int(i), int(lods[i].size()) / 3);
	}

	printf("\n");

	// for using LOD data at runtime, in addition to VB and IB you have to save lod_index_offsets/lod_index_counts.
	(void)lod_index_offsets;
	(void)lod_index_counts;
}

void optimize(const Mesh& mesh, const char* name, void (*optf)(Mesh& mesh))
{
	Mesh copy = mesh;

	clock_t start = clock();
	optf(copy);
	clock_t end = clock();

	VertexCacheStatistics vcs = analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), kCacheSize);
	VertexCacheStatistics vc0s = analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 0);
	VertexFetchStatistics vfs = analyzeVertexFetch(&copy.indices[0], copy.indices.size(), copy.vertices.size(), sizeof(Vertex));
	OverdrawStatistics os = analyzeOverdraw(&copy.indices[0], copy.indices.size(), &copy.vertices[0].px, sizeof(Vertex), copy.vertices.size());

	printf("%-9s: ACMR %f ATVR %f (LRU %f) Overfetch %f Overdraw %f in %.2f msec\n", name, vcs.acmr, vcs.atvr, vc0s.atvr, vfs.overfetch, os.overdraw, double(end - start) / CLOCKS_PER_SEC * 1000);
}

void process(const char* path)
{
	Mesh mesh;

	if (path)
	{
		clock_t start = clock();
		clock_t middle;
		mesh = parseObj(path, middle);
		clock_t end = clock();

		if (mesh.vertices.empty())
		{
			printf("Mesh %s is empty, skipping\n", path);
			return;
		}

		printf("# %s: %d vertices, %d triangles; read in %.2f msec; indexed in %.2f msec\n", path, int(mesh.vertices.size()), int(mesh.indices.size() / 3), double(middle - start) / CLOCKS_PER_SEC * 1000, double(end - middle) / CLOCKS_PER_SEC * 1000);
	}
	else
	{
		mesh = generatePlane(1000);

		printf("# tessellated plane: %d vertices, %d triangles\n", int(mesh.vertices.size()), int(mesh.indices.size() / 3));
	}

	optimize(mesh, "Original", optNone);
	optimize(mesh, "Random", optRandomShuffle);
	optimize(mesh, "Cache", optCache);
	optimize(mesh, "Cache0", optCache0);
	optimize(mesh, "Overdraw", optOverdraw);
	optimize(mesh, "Fetch", optFetch);
	optimize(mesh, "Complete", optComplete);

	// note: the ATVR/overdraw output from this pass is not necessarily correct since we analyze all LODs at once
	optimize(mesh, "Simplify", optCompleteSimplify);
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
