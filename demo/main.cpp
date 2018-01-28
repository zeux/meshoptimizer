#include "../src/meshoptimizer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "miniz.h"
#include "objparser.h"

const size_t kCacheSize = 16;

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

struct Triangle
{
	Vertex v[3];

	bool operator<(const Triangle& other) const
	{
		return memcmp(v, other.v, sizeof(Triangle)) < 0;
	}
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

	size_t total_vertices = meshopt_generateVertexRemap(&remap[0], NULL, total_indices, &vertices[0], total_indices, sizeof(Vertex));

	result.indices.resize(total_indices);
	meshopt_remapIndexBuffer(&result.indices[0], NULL, total_indices, &remap[0]);

	result.vertices.resize(total_vertices);
	meshopt_remapVertexBuffer(&result.vertices[0], &vertices[0], total_indices, sizeof(Vertex), &remap[0]);

	return result;
}

bool isMeshValid(const Mesh& mesh)
{
	if (mesh.indices.size() % 3 != 0)
		return false;

	for (size_t i = 0; i < mesh.indices.size(); ++i)
		if (mesh.indices[i] >= mesh.vertices.size())
			return false;

	return true;
}

bool rotateTriangle(Triangle& t)
{
	int c01 = memcmp(&t.v[0], &t.v[1], sizeof(Vertex));
	int c02 = memcmp(&t.v[0], &t.v[2], sizeof(Vertex));
	int c12 = memcmp(&t.v[1], &t.v[2], sizeof(Vertex));

	if (c12 < 0 && c01 > 0)
	{
		// 1 is minimum, rotate 012 => 120
		Vertex tv = t.v[0];
		t.v[0] = t.v[1], t.v[1] = t.v[2], t.v[2] = tv;
	}
	else if (c02 > 0 && c12 > 0)
	{
		// 2 is minimum, rotate 012 => 201
		Vertex tv = t.v[2];
		t.v[2] = t.v[1], t.v[1] = t.v[0], t.v[0] = tv;
	}

	return c01 != 0 && c02 != 0 && c12 != 0;
}

void deindexMesh(std::vector<Triangle>& dest, const Mesh& mesh)
{
	size_t triangles = mesh.indices.size() / 3;

	dest.reserve(triangles);

	for (size_t i = 0; i < triangles; ++i)
	{
		Triangle t;

		for (int k = 0; k < 3; ++k)
			t.v[k] = mesh.vertices[mesh.indices[i * 3 + k]];

		// skip degenerate triangles since some algorithms don't preserve them
		if (rotateTriangle(t))
			dest.push_back(t);
	}
}

bool areMeshesEqual(const Mesh& lhs, const Mesh& rhs)
{
	std::vector<Triangle> lt, rt;
	deindexMesh(lt, lhs);
	deindexMesh(rt, rhs);

	std::sort(lt.begin(), lt.end());
	std::sort(rt.begin(), rt.end());

	return lt.size() == rt.size() && memcmp(&lt[0], &rt[0], lt.size() * sizeof(Triangle)) == 0;
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
	meshopt_optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size());
}

void optCacheFifo(Mesh& mesh)
{
	meshopt_optimizeVertexCacheFifo(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize);
}

void optOverdraw(Mesh& mesh)
{
	// use worst-case ACMR threshold so that overdraw optimizer can sort *all* triangles
	// warning: this significantly deteriorates the vertex cache efficiency so it is not advised; look at optComplete for the recommended method
	const float kThreshold = 3.f;
	meshopt_optimizeOverdraw(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), kThreshold);
}

void optFetch(Mesh& mesh)
{
	meshopt_optimizeVertexFetch(&mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex));
}

void optComplete(Mesh& mesh)
{
	// vertex cache optimization should go first as it provides starting order for overdraw
	meshopt_optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size());

	// reorder indices for overdraw, balancing overdraw and vertex cache efficiency
	const float kThreshold = 1.05f; // allow up to 5% worse ACMR to get more reordering opportunities for overdraw
	meshopt_optimizeOverdraw(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), kThreshold);

	// vertex fetch optimization should go last as it depends on the final index order
	meshopt_optimizeVertexFetch(&mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex));
}

void optCompleteSimplify(Mesh& mesh)
{
	static const size_t lod_count = 5;

	clock_t start = clock();

	// generate 4 LOD levels (1-4), with each subsequent LOD using 70% triangles
	// note that each LOD uses the same (shared) vertex buffer
	std::vector<unsigned int> lods[lod_count];

	lods[0] = mesh.indices;

	for (size_t i = 1; i < lod_count; ++i)
	{
		std::vector<unsigned int>& lod = lods[i];

		float threshold = powf(0.7f, float(i));
		size_t target_index_count = size_t(mesh.indices.size() * threshold) / 3 * 3;

		// we can simplify all the way from base level or from the last result
		// simplifying from the base level sometimes produces better results, but simplifying from last level is faster
		const std::vector<unsigned int>& source = lods[i - 1];

		lod.resize(source.size());
		lod.resize(meshopt_simplify(&lod[0], &source[0], source.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), std::min(source.size(), target_index_count)));
	}

	clock_t end = clock();

	// optimize each individual LOD for vertex cache & overdraw
	for (size_t i = 0; i < lod_count; ++i)
	{
		std::vector<unsigned int>& lod = lods[i];

		meshopt_optimizeVertexCache(&lod[0], &lod[0], lod.size(), mesh.vertices.size());
		meshopt_optimizeOverdraw(&lod[0], &lod[0], lod.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), 1.0f);
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
	meshopt_optimizeVertexFetch(&mesh.vertices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex));

	printf("%-9s:", "Simplify");

	for (size_t i = 0; i < sizeof(lods) / sizeof(lods[0]); ++i)
	{
		printf(" LOD%d %d", int(i), int(lods[i].size()) / 3);
	}

	printf(" in %.2f msec\n", double(end - start) / CLOCKS_PER_SEC * 1000);

	// for using LOD data at runtime, in addition to VB and IB you have to save lod_index_offsets/lod_index_counts.
	(void)lod_index_offsets;
	(void)lod_index_counts;
}

void optimize(const Mesh& mesh, const char* name, void (*optf)(Mesh& mesh), bool compare = true)
{
	Mesh copy = mesh;

	clock_t start = clock();
	optf(copy);
	clock_t end = clock();

	assert(isMeshValid(copy));
	assert(!compare || areMeshesEqual(mesh, copy));
	(void)compare;

	meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), kCacheSize, 0, 0);
	meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(&copy.indices[0], copy.indices.size(), copy.vertices.size(), sizeof(Vertex));
	meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(&copy.indices[0], copy.indices.size(), &copy.vertices[0].px, copy.vertices.size(), sizeof(Vertex));

	meshopt_VertexCacheStatistics vcs_nv = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 32, 32, 32);
	meshopt_VertexCacheStatistics vcs_amd = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 14, 64, 128);
	meshopt_VertexCacheStatistics vcs_intel = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 128, 0, 0);

	printf("%-9s: ACMR %f ATVR %f (NV %f AMD %f Intel %f) Overfetch %f Overdraw %f in %.2f msec\n", name, vcs.acmr, vcs.atvr, vcs_nv.atvr, vcs_amd.atvr, vcs_intel.atvr, vfs.overfetch, os.overdraw, double(end - start) / CLOCKS_PER_SEC * 1000);
}

size_t compress(const std::vector<unsigned char>& data)
{
	std::vector<unsigned char> cbuf(tdefl_compress_bound(data.size()));
	unsigned int flags = tdefl_create_comp_flags_from_zip_params(MZ_DEFAULT_LEVEL, 15, MZ_DEFAULT_STRATEGY);
	return tdefl_compress_mem_to_mem(&cbuf[0], cbuf.size(), &data[0], data.size(), flags);
}

void encodeIndex(const Mesh& mesh)
{
	clock_t start = clock();

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(mesh.indices.size(), mesh.vertices.size()));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), &mesh.indices[0], mesh.indices.size()));

	clock_t middle = clock();

	std::vector<unsigned int> result(mesh.indices.size());
	int res = meshopt_decodeIndexBuffer(&result[0], mesh.indices.size(), &buffer[0], buffer.size());
	assert(res == 0);
	(void)res;

	clock_t end = clock();

	size_t csize = compress(buffer);

	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		assert(
		    (result[i + 0] == mesh.indices[i + 0] && result[i + 1] == mesh.indices[i + 1] && result[i + 2] == mesh.indices[i + 2]) ||
		    (result[i + 1] == mesh.indices[i + 0] && result[i + 2] == mesh.indices[i + 1] && result[i + 0] == mesh.indices[i + 2]) ||
		    (result[i + 2] == mesh.indices[i + 0] && result[i + 0] == mesh.indices[i + 1] && result[i + 1] == mesh.indices[i + 2]));
	}

	printf("IdxCodec : %.1f bits/triangle (post-deflate %.1f bits/triangle); encode %.2f msec, decode %.2f msec (%.2f Mtri/s)\n",
	       double(buffer.size() * 8) / double(mesh.indices.size() / 3),
	       double(csize * 8) / double(mesh.indices.size() / 3),
	       double(middle - start) / CLOCKS_PER_SEC * 1000,
	       double(end - middle) / CLOCKS_PER_SEC * 1000,
	       (double(result.size() / 3) / 1e6) / (double(end - middle) / CLOCKS_PER_SEC));
}

struct PackedVertex
{
	unsigned short px, py, pz;
	unsigned short pw; // padding to 4b boundary
	unsigned char nx, ny, nz, nw;
	unsigned short tx, ty;
};

void encodeVertex(const Mesh& mesh)
{
	std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(mesh.indices.size(), mesh.vertices.size()));
	ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), &mesh.indices[0], mesh.indices.size()));

	std::vector<PackedVertex> pv(mesh.vertices.size());

	for (size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		pv[i].px = meshopt_quantizeHalf(mesh.vertices[i].px);
		pv[i].py = meshopt_quantizeHalf(mesh.vertices[i].py);
		pv[i].pz = meshopt_quantizeHalf(mesh.vertices[i].pz);
		pv[i].pw = 0;

		pv[i].nx = meshopt_quantizeSnorm(mesh.vertices[i].nx, 8);
		pv[i].ny = meshopt_quantizeSnorm(mesh.vertices[i].ny, 8);
		pv[i].nz = meshopt_quantizeSnorm(mesh.vertices[i].nz, 8);
		pv[i].nw = 0;

		pv[i].tx = meshopt_quantizeHalf(mesh.vertices[i].tx);
		pv[i].ty = meshopt_quantizeHalf(mesh.vertices[i].ty);
	}

	clock_t start = clock();

	std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(mesh.vertices.size(), sizeof(PackedVertex)));
	vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), &pv[0], mesh.vertices.size(), sizeof(PackedVertex), mesh.indices.size(), &ibuf[0], ibuf.size()));

	clock_t end = clock();

	size_t csize = compress(vbuf);

	printf("VtxCodec : %.1f bits/vertex (post-deflate %.1f bits/vertex); encode %.2f msec\n",
	       double(vbuf.size() * 8) / double(mesh.vertices.size()),
	       double(csize * 8) / double(mesh.vertices.size()),
	       double(end - start) / CLOCKS_PER_SEC * 1000);
}

void stripify(const Mesh& mesh)
{
	clock_t start = clock();
	std::vector<unsigned int> strip(mesh.indices.size() / 3 * 4);
	strip.resize(meshopt_stripify(&strip[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size()));
	clock_t end = clock();

	Mesh copy = mesh;
	copy.indices.resize(meshopt_unstripify(&copy.indices[0], &strip[0], strip.size()));

	assert(isMeshValid(copy));
	assert(areMeshesEqual(mesh, copy));

	meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), kCacheSize, 0, 0);
	meshopt_VertexCacheStatistics vcs_nv = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), 32, 32, 32);
	meshopt_VertexCacheStatistics vcs_amd = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), 14, 64, 128);
	meshopt_VertexCacheStatistics vcs_intel = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), 128, 0, 0);

	printf("Stripify : ACMR %f ATVR %f (NV %f AMD %f Intel %f); %d strip indices (%.1f%%) in %.2f msec\n",
	       vcs.acmr, vcs.atvr, vcs_nv.atvr, vcs_amd.atvr, vcs_intel.atvr,
	       int(strip.size()), double(strip.size()) / double(mesh.indices.size()) * 100,
	       double(end - start) / CLOCKS_PER_SEC * 1000);
}

bool loadMesh(Mesh& mesh, const char* path)
{
	if (path)
	{
		clock_t start = clock();
		clock_t middle;
		mesh = parseObj(path, middle);
		clock_t end = clock();

		if (mesh.vertices.empty())
		{
			printf("Mesh %s is empty, skipping\n", path);
			return false;
		}

		printf("# %s: %d vertices, %d triangles; read in %.2f msec; indexed in %.2f msec\n", path, int(mesh.vertices.size()), int(mesh.indices.size() / 3), double(middle - start) / CLOCKS_PER_SEC * 1000, double(end - middle) / CLOCKS_PER_SEC * 1000);
	}
	else
	{
		mesh = generatePlane(1000);

		printf("# tessellated plane: %d vertices, %d triangles\n", int(mesh.vertices.size()), int(mesh.indices.size() / 3));
	}

	return true;
}

void process(const char* path)
{
	Mesh mesh;
	if (!loadMesh(mesh, path))
		return;

	optimize(mesh, "Original", optNone);
	optimize(mesh, "Random", optRandomShuffle);
	optimize(mesh, "Cache", optCache);
	optimize(mesh, "CacheFifo", optCacheFifo);
	optimize(mesh, "Overdraw", optOverdraw);
	optimize(mesh, "Fetch", optFetch);
	optimize(mesh, "Complete", optComplete);

	// note: the ATVR/overdraw output from this pass is not necessarily correct since we analyze all LODs at once
	optimize(mesh, "CmpltLod", optCompleteSimplify, /* compare= */ false);

	Mesh copy = mesh;
	meshopt_optimizeVertexCache(&copy.indices[0], &copy.indices[0], copy.indices.size(), copy.vertices.size());
	meshopt_optimizeVertexFetch(&copy.vertices[0], &copy.indices[0], copy.indices.size(), &copy.vertices[0], copy.vertices.size(), sizeof(Vertex));

	stripify(copy);

	encodeIndex(copy);
}

void processDev(const char* path)
{
	Mesh mesh;
	if (!loadMesh(mesh, path))
		return;

	Mesh copy = mesh;
	meshopt_optimizeVertexCache(&copy.indices[0], &copy.indices[0], copy.indices.size(), copy.vertices.size());
	meshopt_optimizeVertexFetch(&copy.vertices[0], &copy.indices[0], copy.indices.size(), &copy.vertices[0], copy.vertices.size(), sizeof(Vertex));

	encodeVertex(copy);
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
		if (argc > 2 && strcmp(argv[1], "-d") == 0)
		{
			for (int i = 2; i < argc; ++i)
			{
				processDev(argv[i]);
			}
		}
		else
		{
			for (int i = 1; i < argc; ++i)
			{
				process(argv[i]);
			}
		}
	}
}
