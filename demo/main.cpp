#include "../src/meshoptimizer.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <vector>

#include "../extern/fast_obj.h"

#define SDEFL_IMPLEMENTATION
#include "../extern/sdefl.h"

// This file uses assert() to verify algorithm correctness
#undef NDEBUG
#include <assert.h>

#if defined(__linux__)
double timestamp()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
}
#elif defined(_WIN32)
struct LARGE_INTEGER
{
	__int64 QuadPart;
};
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

double timestamp()
{
	LARGE_INTEGER freq, counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	return double(counter.QuadPart) / double(freq.QuadPart);
}
#else
double timestamp()
{
	return double(clock()) / double(CLOCKS_PER_SEC);
}
#endif

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

union Triangle
{
	Vertex v[3];
	char data[sizeof(Vertex) * 3];
};

Mesh parseObj(const char* path, double& reindex)
{
	fastObjMesh* obj = fast_obj_read(path);
	if (!obj)
	{
		printf("Error loading %s: file not found\n", path);
		return Mesh();
	}

	size_t total_indices = 0;

	for (unsigned int i = 0; i < obj->face_count; ++i)
		if (obj->face_vertices[i] > 2)
			total_indices += 3 * (obj->face_vertices[i] - 2);

	std::vector<Vertex> vertices(total_indices);

	size_t vertex_offset = 0;
	size_t index_offset = 0;

	for (unsigned int i = 0; i < obj->face_count; ++i)
	{
		if (obj->face_vertices[i] <= 2)
			continue;

		for (unsigned int j = 0; j < obj->face_vertices[i]; ++j)
		{
			fastObjIndex gi = obj->indices[index_offset + j];

			Vertex v =
			    {
			        obj->positions[gi.p * 3 + 0],
			        obj->positions[gi.p * 3 + 1],
			        obj->positions[gi.p * 3 + 2],
			        obj->normals[gi.n * 3 + 0],
			        obj->normals[gi.n * 3 + 1],
			        obj->normals[gi.n * 3 + 2],
			        obj->texcoords[gi.t * 2 + 0],
			        obj->texcoords[gi.t * 2 + 1],
			    };

			// triangulate polygon on the fly; offset-3 is always the first polygon vertex
			if (j >= 3)
			{
				vertices[vertex_offset + 0] = vertices[vertex_offset - 3];
				vertices[vertex_offset + 1] = vertices[vertex_offset - 1];
				vertex_offset += 2;
			}

			vertices[vertex_offset] = v;
			vertex_offset++;
		}

		index_offset += obj->face_vertices[i];
	}

	fast_obj_destroy(obj);

	reindex = timestamp();

	Mesh result;

	// empty mesh
	if (total_indices == 0)
		return result;

	std::vector<unsigned int> remap(total_indices);

	size_t total_vertices = meshopt_generateVertexRemap(&remap[0], NULL, total_indices, &vertices[0], total_indices, sizeof(Vertex));

	result.indices.resize(total_indices);
	meshopt_remapIndexBuffer(&result.indices[0], NULL, total_indices, &remap[0]);

	result.vertices.resize(total_vertices);
	meshopt_remapVertexBuffer(&result.vertices[0], &vertices[0], total_indices, sizeof(Vertex), &remap[0]);

	return result;
}

void dumpObj(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, bool recomputeNormals = false)
{
	std::vector<float> normals;

	if (recomputeNormals)
	{
		normals.resize(vertices.size() * 3);

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			unsigned int a = indices[i], b = indices[i + 1], c = indices[i + 2];

			const Vertex& va = vertices[a];
			const Vertex& vb = vertices[b];
			const Vertex& vc = vertices[c];

			float nx = (vb.py - va.py) * (vc.pz - va.pz) - (vb.pz - va.pz) * (vc.py - va.py);
			float ny = (vb.pz - va.pz) * (vc.px - va.px) - (vb.px - va.px) * (vc.pz - va.pz);
			float nz = (vb.px - va.px) * (vc.py - va.py) - (vb.py - va.py) * (vc.px - va.px);

			for (int k = 0; k < 3; ++k)
			{
				unsigned int index = indices[i + k];

				normals[index * 3 + 0] += nx;
				normals[index * 3 + 1] += ny;
				normals[index * 3 + 2] += nz;
			}
		}
	}

	for (size_t i = 0; i < vertices.size(); ++i)
	{
		const Vertex& v = vertices[i];

		float nx = v.nx, ny = v.ny, nz = v.nz;

		if (recomputeNormals)
		{
			nx = normals[i * 3 + 0];
			ny = normals[i * 3 + 1];
			nz = normals[i * 3 + 2];

			float l = sqrtf(nx * nx + ny * ny + nz * nz);
			float s = l == 0.f ? 0.f : 1.f / l;

			nx *= s;
			ny *= s;
			nz *= s;
		}

		fprintf(stderr, "v %f %f %f\n", v.px, v.py, v.pz);
		fprintf(stderr, "vn %f %f %f\n", nx, ny, nz);
	}

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		unsigned int a = indices[i], b = indices[i + 1], c = indices[i + 2];

		fprintf(stderr, "f %d//%d %d//%d %d//%d\n", a + 1, a + 1, b + 1, b + 1, c + 1, c + 1);
	}
}

void dumpObj(const char* section, const std::vector<unsigned int>& indices)
{
	fprintf(stderr, "o %s\n", section);

	for (size_t j = 0; j < indices.size(); j += 3)
	{
		unsigned int a = indices[j], b = indices[j + 1], c = indices[j + 2];

		fprintf(stderr, "f %d//%d %d//%d %d//%d\n", a + 1, a + 1, b + 1, b + 1, c + 1, c + 1);
	}
}

struct PackedVertex
{
	unsigned short px, py, pz;
	unsigned short pw; // padding to 4b boundary
	signed char nx, ny, nz, nw;
	unsigned short tx, ty;
};

void packMesh(std::vector<PackedVertex>& pv, const std::vector<Vertex>& vertices)
{
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		const Vertex& vi = vertices[i];
		PackedVertex& pvi = pv[i];

		pvi.px = meshopt_quantizeHalf(vi.px);
		pvi.py = meshopt_quantizeHalf(vi.py);
		pvi.pz = meshopt_quantizeHalf(vi.pz);
		pvi.pw = 0;

		pvi.nx = char(meshopt_quantizeSnorm(vi.nx, 8));
		pvi.ny = char(meshopt_quantizeSnorm(vi.ny, 8));
		pvi.nz = char(meshopt_quantizeSnorm(vi.nz, 8));
		pvi.nw = 0;

		pvi.tx = meshopt_quantizeHalf(vi.tx);
		pvi.ty = meshopt_quantizeHalf(vi.ty);
	}
}

struct PackedVertexOct
{
	unsigned short px, py, pz;
	signed char nu, nv; // octahedron encoded normal, aliases .pw
	unsigned short tx, ty;
};

void packMesh(std::vector<PackedVertexOct>& pv, const std::vector<Vertex>& vertices)
{
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		const Vertex& vi = vertices[i];
		PackedVertexOct& pvi = pv[i];

		pvi.px = meshopt_quantizeHalf(vi.px);
		pvi.py = meshopt_quantizeHalf(vi.py);
		pvi.pz = meshopt_quantizeHalf(vi.pz);

		float nsum = fabsf(vi.nx) + fabsf(vi.ny) + fabsf(vi.nz);
		float nx = vi.nx / nsum;
		float ny = vi.ny / nsum;
		float nz = vi.nz;

		float nu = nz >= 0 ? nx : (1 - fabsf(ny)) * (nx >= 0 ? 1 : -1);
		float nv = nz >= 0 ? ny : (1 - fabsf(nx)) * (ny >= 0 ? 1 : -1);

		pvi.nu = char(meshopt_quantizeSnorm(nu, 8));
		pvi.nv = char(meshopt_quantizeSnorm(nv, 8));

		pvi.tx = meshopt_quantizeHalf(vi.tx);
		pvi.ty = meshopt_quantizeHalf(vi.ty);
	}
}

void simplify(const Mesh& mesh, float threshold = 0.2f, unsigned int options = 0)
{
	Mesh lod;

	double start = timestamp();

	size_t target_index_count = size_t(mesh.indices.size() * threshold);
	float target_error = 1e-2f;
	float result_error = 0;

	lod.indices.resize(mesh.indices.size()); // note: simplify needs space for index_count elements in the destination array, not target_index_count
	lod.indices.resize(meshopt_simplify(&lod.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), target_index_count, target_error, options, &result_error));

	lod.vertices.resize(lod.indices.size() < mesh.vertices.size() ? lod.indices.size() : mesh.vertices.size()); // note: this is just to reduce the cost of resize()
	lod.vertices.resize(meshopt_optimizeVertexFetch(&lod.vertices[0], &lod.indices[0], lod.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex)));

	double end = timestamp();

	printf("%-9s: %d triangles => %d triangles (%.2f%% deviation) in %.2f msec\n",
	    "Simplify",
	    int(mesh.indices.size() / 3), int(lod.indices.size() / 3),
	    result_error * 100,
	    (end - start) * 1000);
}

void simplifyAttr(const Mesh& mesh, float threshold = 0.2f, unsigned int options = 0)
{
	Mesh lod;

	double start = timestamp();

	size_t target_index_count = size_t(mesh.indices.size() * threshold);
	float target_error = 1e-2f;
	float result_error = 0;

	const float nrm_weight = 0.5f;
	const float attr_weights[3] = {nrm_weight, nrm_weight, nrm_weight};

	lod.indices.resize(mesh.indices.size()); // note: simplify needs space for index_count elements in the destination array, not target_index_count
	lod.indices.resize(meshopt_simplifyWithAttributes(&lod.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), &mesh.vertices[0].nx, sizeof(Vertex), attr_weights, 3, NULL, target_index_count, target_error, options, &result_error));

	lod.vertices.resize(lod.indices.size() < mesh.vertices.size() ? lod.indices.size() : mesh.vertices.size()); // note: this is just to reduce the cost of resize()
	lod.vertices.resize(meshopt_optimizeVertexFetch(&lod.vertices[0], &lod.indices[0], lod.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex)));

	double end = timestamp();

	printf("%-9s: %d triangles => %d triangles (%.2f%% deviation) in %.2f msec\n",
	    "SimplifyAttr",
	    int(mesh.indices.size() / 3), int(lod.indices.size() / 3),
	    result_error * 100,
	    (end - start) * 1000);
}

void simplifyUpdate(const Mesh& mesh, float threshold = 0.2f, unsigned int options = 0)
{
	Mesh lod;

	double start = timestamp();

	size_t target_index_count = size_t(mesh.indices.size() * threshold);
	float target_error = 1e-2f;
	float result_error = 0;

	const float nrm_weight = 0.5f;
	const float attr_weights[3] = {nrm_weight, nrm_weight, nrm_weight};

	lod = mesh; // start from the original mesh
	lod.indices.resize(meshopt_simplifyWithUpdate(&lod.indices[0], mesh.indices.size(), &lod.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), &lod.vertices[0].nx, sizeof(Vertex), attr_weights, 3, NULL, target_index_count, target_error, options, &result_error));

	lod.vertices.resize(meshopt_optimizeVertexFetch(&lod.vertices[0], &lod.indices[0], lod.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex)));

	for (size_t i = 0; i < lod.vertices.size(); ++i)
	{
		// update normals
		Vertex& v = lod.vertices[i];
		float nl = sqrtf(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
		if (nl > 0)
		{
			v.nx /= nl;
			v.ny /= nl;
			v.nz /= nl;
		}
	}

	double end = timestamp();

	printf("%-9s: %d triangles => %d triangles (%.2f%% deviation) in %.2f msec\n",
	    "SimplifyUpdt",
	    int(mesh.indices.size() / 3), int(lod.indices.size() / 3),
	    result_error * 100,
	    (end - start) * 1000);
}

void simplifySloppy(const Mesh& mesh, float threshold = 0.2f)
{
	Mesh lod;

	double start = timestamp();

	size_t target_index_count = size_t(mesh.indices.size() * threshold);
	float target_error = 1e-1f;
	float result_error = 0;

	lod.indices.resize(mesh.indices.size()); // note: simplify needs space for index_count elements in the destination array, not target_index_count
	lod.indices.resize(meshopt_simplifySloppy(&lod.indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), target_index_count, target_error, &result_error));

	lod.vertices.resize(lod.indices.size() < mesh.vertices.size() ? lod.indices.size() : mesh.vertices.size()); // note: this is just to reduce the cost of resize()
	lod.vertices.resize(meshopt_optimizeVertexFetch(&lod.vertices[0], &lod.indices[0], lod.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(Vertex)));

	double end = timestamp();

	printf("%-9s: %d triangles => %d triangles (%.2f%% deviation) in %.2f msec\n",
	    "SimplifyS",
	    int(mesh.indices.size() / 3), int(lod.indices.size() / 3),
	    result_error * 100,
	    (end - start) * 1000);
}

void simplifyPoints(const Mesh& mesh, float threshold = 0.2f)
{
	double start = timestamp();

	size_t target_vertex_count = size_t(mesh.vertices.size() * threshold);
	if (target_vertex_count == 0)
		return;

	std::vector<unsigned int> indices(target_vertex_count);
	indices.resize(meshopt_simplifyPoints(&indices[0], &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), NULL, 0, 0, target_vertex_count));

	double end = timestamp();

	printf("%-9s: %d points => %d points in %.2f msec\n",
	    "SimplifyP",
	    int(mesh.vertices.size()), int(indices.size()), (end - start) * 1000);
}

void simplifyComplete(const Mesh& mesh)
{
	static const size_t lod_count = 5;

	double start = timestamp();

	// generate 4 LOD levels (1-4), with each subsequent LOD using 70% triangles
	// note that each LOD uses the same (shared) vertex buffer
	std::vector<unsigned int> lods[lod_count];

	lods[0] = mesh.indices;

	for (size_t i = 1; i < lod_count; ++i)
	{
		std::vector<unsigned int>& lod = lods[i];

		float threshold = powf(0.7f, float(i));
		size_t target_index_count = size_t(mesh.indices.size() * threshold) / 3 * 3;
		float target_error = 1e-2f;

		// we can simplify all the way from base level or from the last result
		// simplifying from the base level sometimes produces better results, but simplifying from last level is faster
		const std::vector<unsigned int>& source = lods[i - 1];

		if (source.size() < target_index_count)
			target_index_count = source.size();

		lod.resize(source.size());
		lod.resize(meshopt_simplify(&lod[0], &source[0], source.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), target_index_count, target_error));
	}

	double middle = timestamp();

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
	// this order also produces much better vertex fetch cache coherency for coarse LODs (since they're essentially optimized first)
	// somewhat surprisingly, the vertex fetch cache coherency for fine LODs doesn't seem to suffer that much.
	size_t lod_index_offsets[lod_count] = {};
	size_t lod_index_counts[lod_count] = {};
	size_t total_index_count = 0;

	for (int i = lod_count - 1; i >= 0; --i)
	{
		lod_index_offsets[i] = total_index_count;
		lod_index_counts[i] = lods[i].size();

		total_index_count += lods[i].size();
	}

	std::vector<unsigned int> indices(total_index_count);

	for (size_t i = 0; i < lod_count; ++i)
	{
		memcpy(&indices[lod_index_offsets[i]], &lods[i][0], lods[i].size() * sizeof(lods[i][0]));
	}

	std::vector<Vertex> vertices = mesh.vertices;

	// vertex fetch optimization should go last as it depends on the final index order
	// note that the order of LODs above affects vertex fetch results
	meshopt_optimizeVertexFetch(&vertices[0], &indices[0], indices.size(), &vertices[0], vertices.size(), sizeof(Vertex));

	double end = timestamp();

	printf("%-9s: %d triangles => %d LOD levels down to %d triangles in %.2f msec, optimized in %.2f msec\n",
	    "SimplifyC",
	    int(lod_index_counts[0]) / 3, int(lod_count), int(lod_index_counts[lod_count - 1]) / 3,
	    (middle - start) * 1000, (end - middle) * 1000);

	// for using LOD data at runtime, in addition to vertices and indices you have to save lod_index_offsets/lod_index_counts.

	{
		meshopt_VertexCacheStatistics vcs0 = meshopt_analyzeVertexCache(&indices[lod_index_offsets[0]], lod_index_counts[0], vertices.size(), 16, 0, 0);
		meshopt_VertexFetchStatistics vfs0 = meshopt_analyzeVertexFetch(&indices[lod_index_offsets[0]], lod_index_counts[0], vertices.size(), sizeof(Vertex));
		meshopt_VertexCacheStatistics vcsN = meshopt_analyzeVertexCache(&indices[lod_index_offsets[lod_count - 1]], lod_index_counts[lod_count - 1], vertices.size(), 16, 0, 0);
		meshopt_VertexFetchStatistics vfsN = meshopt_analyzeVertexFetch(&indices[lod_index_offsets[lod_count - 1]], lod_index_counts[lod_count - 1], vertices.size(), sizeof(Vertex));

		typedef PackedVertexOct PV;

		std::vector<PV> pv(vertices.size());
		packMesh(pv, vertices);

		std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(vertices.size(), sizeof(PV)));
		vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), &pv[0], vertices.size(), sizeof(PV)));

		std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(indices.size(), vertices.size()));
		ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), &indices[0], indices.size()));

		printf("%-9s  ACMR %f...%f Overfetch %f..%f Codec VB %.1f bits/vertex IB %.1f bits/triangle\n",
		    "",
		    vcs0.acmr, vcsN.acmr, vfs0.overfetch, vfsN.overfetch,
		    double(vbuf.size()) / double(vertices.size()) * 8,
		    double(ibuf.size()) / double(indices.size() / 3) * 8);
	}
}

void simplifyClusters(const Mesh& mesh, float threshold = 0.2f)
{
	const size_t max_vertices = 64;
	const size_t max_triangles = 64;
	const size_t target_group_size = 8;

	double start = timestamp();

	// build clusters (meshlets) out of the mesh
	size_t max_meshlets = meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, max_triangles);
	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	meshlets.resize(meshopt_buildMeshlets(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), max_vertices, max_triangles, 0.f));

	double middle = timestamp();

	// generate position remap; we'll use that to partition clusters using position-only adjacency
	std::vector<unsigned int> remap(mesh.vertices.size());
	meshopt_generatePositionRemap(&remap[0], &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex));

	// partition clusters in groups; each group will be simplified separately and the boundaries between groups will be preserved
	std::vector<unsigned int> cluster_indices;
	cluster_indices.reserve(mesh.indices.size()); // slight underestimate, vector should realloc once
	std::vector<unsigned int> cluster_sizes(meshlets.size());

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& m = meshlets[i];

		for (size_t j = 0; j < m.triangle_count * 3; ++j)
		{
			unsigned int v = meshlet_vertices[m.vertex_offset + meshlet_triangles[m.triangle_offset + j]];

			// use the first vertex with equivalent position so that cluster adjacency ignores attribute seams
			cluster_indices.push_back(remap[v]);
		}

		cluster_sizes[i] = m.triangle_count * 3;
	}

	std::vector<unsigned int> partition(meshlets.size());
	size_t partition_count = meshopt_partitionClusters(&partition[0], &cluster_indices[0], cluster_indices.size(), &cluster_sizes[0], cluster_sizes.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), target_group_size);

	// convert partitions to linked lists to make it easier to iterate over (vectors of vectors would work too)
	std::vector<int> partnext(meshlets.size(), -1);
	std::vector<int> partlast(partition_count, -1);

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		unsigned int part = partition[i];

		if (partlast[part] >= 0)
			partnext[partlast[part]] = int(i);

		partlast[part] = int(i);
		partnext[i] = -1;
	}

	double parttime = timestamp();

	float scale = meshopt_simplifyScale(&mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex));

	std::vector<unsigned int> lod;
	lod.reserve(mesh.indices.size());

	float error = 0.f;

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		if (partlast[partition[i]] < 0)
			continue; // part of a group that was already processed

		// mark group as processed
		partlast[partition[i]] = -1;

		size_t group_offset = lod.size();

		for (int j = int(i); j >= 0; j = partnext[j])
		{
			const meshopt_Meshlet& m = meshlets[j];

			for (size_t k = 0; k < m.triangle_count * 3; ++k)
				lod.push_back(meshlet_vertices[m.vertex_offset + meshlet_triangles[m.triangle_offset + k]]);
		}

		size_t group_triangles = (lod.size() - group_offset) / 3;

		// simplify the group, preserving the border vertices
		// note: this technically also locks the exterior border; a full mesh analysis (see nanite.cpp / lockBoundary) would work better for some meshes
		unsigned int options = meshopt_SimplifyLockBorder | meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute;

		float group_target_error = 1e-2f * scale;
		size_t group_target = size_t(float(group_triangles) * threshold) * 3;
		float group_error = 0.f;
		size_t group_size = meshopt_simplify(&lod[group_offset], &lod[group_offset], group_triangles * 3, &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), group_target, group_target_error, options, &group_error);

		error = group_error > error ? group_error : error;

		// simplified group is available in lod[group_offset..group_offset + group_size]
		lod.resize(group_offset + group_size);
	}

	double end = timestamp();

	printf("%-9s: %d triangles => %d triangles (%.2f%% deviation) in %.2f msec, clusterized in %.2f msec, partitioned in %.2f msec (%d clusters in %d groups)\n",
	    "SimplifyN", // N for Nanite
	    int(mesh.indices.size() / 3), int(lod.size() / 3),
	    error / scale * 100,
	    (end - parttime) * 1000, (middle - start) * 1000, (parttime - middle) * 1000,
	    int(meshlets.size()), int(partition_count));
}

void optimize(const Mesh& mesh, bool fifo = false)
{
	Mesh copy = mesh;
	// note: we assume that the mesh is already optimally indexed (via parseObj); if that is not the case, you'd need to reindex first

	double start = timestamp();

	// vertex cache optimization should go first as it provides starting order for overdraw
	// note: fifo optimization is not recommended as a default, since it produces worse results, but it's faster to run so it can be useful for procedural meshes
	if (fifo)
		meshopt_optimizeVertexCacheFifo(&copy.indices[0], &copy.indices[0], copy.indices.size(), copy.vertices.size(), /* cache_size= */ 16);
	else
		meshopt_optimizeVertexCache(&copy.indices[0], &copy.indices[0], copy.indices.size(), copy.vertices.size());

	// reorder indices for overdraw, balancing overdraw and vertex cache efficiency
	const float kThreshold = 1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw
	meshopt_optimizeOverdraw(&copy.indices[0], &copy.indices[0], copy.indices.size(), &copy.vertices[0].px, copy.vertices.size(), sizeof(Vertex), kThreshold);

	// vertex fetch optimization should go last as it depends on the final index order
	meshopt_optimizeVertexFetch(&copy.vertices[0], &copy.indices[0], copy.indices.size(), &copy.vertices[0], copy.vertices.size(), sizeof(Vertex));

	double end = timestamp();

	meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 16, 0, 0);
	meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(&copy.indices[0], copy.indices.size(), copy.vertices.size(), sizeof(Vertex));
	meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(&copy.indices[0], copy.indices.size(), &copy.vertices[0].px, copy.vertices.size(), sizeof(Vertex));

	meshopt_VertexCacheStatistics vcs_nv = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 32, 32, 32);
	meshopt_VertexCacheStatistics vcs_amd = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 14, 64, 128);
	meshopt_VertexCacheStatistics vcs_intel = meshopt_analyzeVertexCache(&copy.indices[0], copy.indices.size(), copy.vertices.size(), 128, 0, 0);

	printf("Optimize%s: ACMR %f ATVR %f (NV %f AMD %f Intel %f) overfetch %f overdraw %f in %.2f msec\n",
	    fifo ? "F" : " ",
	    vcs.acmr, vcs.atvr, vcs_nv.atvr, vcs_amd.atvr, vcs_intel.atvr, vfs.overfetch, os.overdraw, (end - start) * 1000);
}

template <typename T>
size_t compress(const std::vector<T>& data, int level = SDEFL_LVL_DEF)
{
	std::vector<unsigned char> cbuf(sdefl_bound(int(data.size() * sizeof(T))));
	sdefl s = {};
	return sdeflate(&s, &cbuf[0], reinterpret_cast<const unsigned char*>(&data[0]), int(data.size() * sizeof(T)), level);
}

void encodeIndex(const std::vector<unsigned int>& indices, size_t vertex_count, char desc)
{
	// allocate result outside of the timing loop to exclude memset() from decode timing
	std::vector<unsigned int> result(indices.size());

	double start = timestamp();

	std::vector<unsigned char> buffer(meshopt_encodeIndexBufferBound(indices.size(), vertex_count));
	buffer.resize(meshopt_encodeIndexBuffer(&buffer[0], buffer.size(), &indices[0], indices.size()));

	double middle = timestamp();

	int res = meshopt_decodeIndexBuffer(&result[0], indices.size(), &buffer[0], buffer.size());
	assert(res == 0);
	(void)res;

	double end = timestamp();

	size_t csize = compress(buffer);

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		assert(
		    (result[i + 0] == indices[i + 0] && result[i + 1] == indices[i + 1] && result[i + 2] == indices[i + 2]) ||
		    (result[i + 1] == indices[i + 0] && result[i + 2] == indices[i + 1] && result[i + 0] == indices[i + 2]) ||
		    (result[i + 2] == indices[i + 0] && result[i + 0] == indices[i + 1] && result[i + 1] == indices[i + 2]));
	}

	printf("IdxCodec%c: %.1f bits/triangle (post-deflate %.1f bits/triangle); encode %.2f msec (%.3f GB/s), decode %.2f msec (%.2f GB/s)\n",
	    desc,
	    double(buffer.size() * 8) / double(indices.size() / 3),
	    double(csize * 8) / double(indices.size() / 3),
	    (middle - start) * 1000,
	    (double(result.size() * 4) / 1e9) / (middle - start),
	    (end - middle) * 1000,
	    (double(result.size() * 4) / 1e9) / (end - middle));
}

void encodeIndex(const Mesh& mesh, char desc)
{
	encodeIndex(mesh.indices, mesh.vertices.size(), desc);
}

void encodeIndexSequence(const std::vector<unsigned int>& data, size_t vertex_count, char desc)
{
	// allocate result outside of the timing loop to exclude memset() from decode timing
	std::vector<unsigned int> result(data.size());

	double start = timestamp();

	std::vector<unsigned char> buffer(meshopt_encodeIndexSequenceBound(data.size(), vertex_count));
	buffer.resize(meshopt_encodeIndexSequence(&buffer[0], buffer.size(), &data[0], data.size()));

	double middle = timestamp();

	int res = meshopt_decodeIndexSequence(&result[0], data.size(), &buffer[0], buffer.size());
	assert(res == 0);
	(void)res;

	double end = timestamp();

	size_t csize = compress(buffer);

	assert(memcmp(&data[0], &result[0], data.size() * sizeof(unsigned int)) == 0);

	printf("IdxCodec%c: %.1f bits/index (post-deflate %.1f bits/index); encode %.2f msec (%.3f GB/s), decode %.2f msec (%.2f GB/s)\n",
	    desc,
	    double(buffer.size() * 8) / double(data.size()),
	    double(csize * 8) / double(data.size()),
	    (middle - start) * 1000,
	    (double(result.size() * 4) / 1e9) / (middle - start),
	    (end - middle) * 1000,
	    (double(result.size() * 4) / 1e9) / (end - middle));
}

template <typename PV>
void packVertex(const Mesh& mesh, const char* pvn)
{
	std::vector<PV> pv(mesh.vertices.size());
	packMesh(pv, mesh.vertices);

	size_t csize = compress(pv);

	printf("VtxPack%s  : %.1f bits/vertex (post-deflate %.1f bits/vertex)\n", pvn,
	    double(pv.size() * sizeof(PV) * 8) / double(mesh.vertices.size()),
	    double(csize * 8) / double(mesh.vertices.size()));
}

template <typename PV>
void encodeVertex(const Mesh& mesh, const char* pvn, int level = 2)
{
	std::vector<PV> pv(mesh.vertices.size());
	packMesh(pv, mesh.vertices);

	// allocate result outside of the timing loop to exclude memset() from decode timing
	std::vector<PV> result(mesh.vertices.size());

	double start = timestamp();

	std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(mesh.vertices.size(), sizeof(PV)));
	vbuf.resize(meshopt_encodeVertexBufferLevel(&vbuf[0], vbuf.size(), &pv[0], mesh.vertices.size(), sizeof(PV), level, -1));

	double middle = timestamp();

	int res = meshopt_decodeVertexBuffer(&result[0], mesh.vertices.size(), sizeof(PV), &vbuf[0], vbuf.size());
	assert(res == 0);
	(void)res;

	double end = timestamp();

	assert(memcmp(&pv[0], &result[0], pv.size() * sizeof(PV)) == 0);

	size_t csize = compress(vbuf);

	printf("VtxCodec%1s: %.1f bits/vertex (post-deflate %.1f bits/vertex); encode %.2f msec (%.3f GB/s), decode %.2f msec (%.2f GB/s)\n", pvn,
	    double(vbuf.size() * 8) / double(mesh.vertices.size()),
	    double(csize * 8) / double(mesh.vertices.size()),
	    (middle - start) * 1000,
	    (double(result.size() * sizeof(PV)) / 1e9) / (middle - start),
	    (end - middle) * 1000,
	    (double(result.size() * sizeof(PV)) / 1e9) / (end - middle));
}

void stripify(const Mesh& mesh, bool use_restart, char desc)
{
	unsigned int restart_index = use_restart ? ~0u : 0;

	// note: input mesh is assumed to be optimized for vertex cache and vertex fetch
	double start = timestamp();
	std::vector<unsigned int> strip(meshopt_stripifyBound(mesh.indices.size()));
	strip.resize(meshopt_stripify(&strip[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), restart_index));
	double end = timestamp();

	size_t restarts = 0;
	for (size_t i = 0; i < strip.size(); ++i)
		restarts += use_restart && strip[i] == restart_index;

	Mesh copy = mesh;
	copy.indices.resize(meshopt_unstripify(&copy.indices[0], &strip[0], strip.size(), restart_index));
	assert(copy.indices.size() <= meshopt_unstripifyBound(strip.size()));

	meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), 16, 0, 0);
	meshopt_VertexCacheStatistics vcs_nv = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), 32, 32, 32);
	meshopt_VertexCacheStatistics vcs_amd = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), 14, 64, 128);
	meshopt_VertexCacheStatistics vcs_intel = meshopt_analyzeVertexCache(&copy.indices[0], mesh.indices.size(), mesh.vertices.size(), 128, 0, 0);

	printf("Stripify%c: ACMR %f ATVR %f (NV %f AMD %f Intel %f); %.1f run avg, %d strip indices (%.1f%%) in %.2f msec\n",
	    desc,
	    vcs.acmr, vcs.atvr, vcs_nv.atvr, vcs_amd.atvr, vcs_intel.atvr,
	    use_restart ? double(strip.size() - restarts) / double(restarts + 1) : 0,
	    int(strip.size()), double(strip.size()) / double(mesh.indices.size()) * 100,
	    (end - start) * 1000);
}

void shadow(const Mesh& mesh)
{
	// note: input mesh is assumed to be optimized for vertex cache and vertex fetch

	double start = timestamp();
	// this index buffer can be used for position-only rendering using the same vertex data that the original index buffer uses
	std::vector<unsigned int> shadow_indices(mesh.indices.size());
	meshopt_generateShadowIndexBuffer(&shadow_indices[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0], mesh.vertices.size(), sizeof(float) * 3, sizeof(Vertex));
	double end = timestamp();

	// while you can't optimize the vertex data after shadow IB was constructed, you can and should optimize the shadow IB for vertex cache
	// this is valuable even if the original indices array was optimized for vertex cache!
	meshopt_optimizeVertexCache(&shadow_indices[0], &shadow_indices[0], shadow_indices.size(), mesh.vertices.size());

	meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(&mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), 16, 0, 0);
	meshopt_VertexCacheStatistics vcss = meshopt_analyzeVertexCache(&shadow_indices[0], shadow_indices.size(), mesh.vertices.size(), 16, 0, 0);

	std::vector<char> shadow_flags(mesh.vertices.size());
	size_t shadow_vertices = 0;

	for (size_t i = 0; i < shadow_indices.size(); ++i)
	{
		unsigned int index = shadow_indices[i];
		shadow_vertices += 1 - shadow_flags[index];
		shadow_flags[index] = 1;
	}

	printf("ShadowIB : ACMR %f (%.2fx improvement); %d shadow vertices (%.2fx improvement) in %.2f msec\n",
	    vcss.acmr, double(vcs.vertices_transformed) / double(vcss.vertices_transformed),
	    int(shadow_vertices), double(mesh.vertices.size()) / double(shadow_vertices),
	    (end - start) * 1000);
}

static int follow(int* parents, int index)
{
	while (index != parents[index])
	{
		int parent = parents[index];
		parents[index] = parents[parent];
		index = parent;
	}

	return index;
}

void meshlets(const Mesh& mesh, bool scan = false, bool uniform = false, bool flex = false, bool spatial = false, bool dump = false)
{
	// NVidia-recommends 64/126; we round 126 down to a multiple of 4
	// alternatively we also test uniform configuration with 64/64 which is better for AMD
	const size_t max_vertices = 64;
	const size_t max_triangles = uniform ? 64 : 124;
	const size_t min_triangles = spatial ? 16 : (uniform ? 24 : 32); // only used in flex/spatial modes

	// note: should be set to 0 unless cone culling is used at runtime!
	const float cone_weight = flex ? -1.0f : 0.25f;
	const float split_factor = flex ? 2.0f : 0.0f;

	// note: input mesh is assumed to be optimized for vertex cache and vertex fetch
	double start = timestamp();
	size_t max_meshlets = meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, min_triangles);
	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	if (scan)
		meshlets.resize(meshopt_buildMeshletsScan(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size(), max_vertices, max_triangles));
	else if (flex)
		meshlets.resize(meshopt_buildMeshletsFlex(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), max_vertices, min_triangles, max_triangles, cone_weight, split_factor));
	else if (spatial)
		meshlets.resize(meshopt_buildMeshletsSpatial(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), max_vertices, min_triangles, max_triangles, 0.f));
	else // note: equivalent to the call of buildMeshletsFlex() with non-negative cone_weight and split_factor = 0
		meshlets.resize(meshopt_buildMeshlets(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), max_vertices, max_triangles, cone_weight));

	if (!dump)
		for (size_t i = 0; i < meshlets.size(); ++i)
			meshopt_optimizeMeshlet(&meshlet_vertices[meshlets[i].vertex_offset], &meshlet_triangles[meshlets[i].triangle_offset], meshlets[i].triangle_count, meshlets[i].vertex_count);

	if (meshlets.size())
	{
		const meshopt_Meshlet& last = meshlets.back();

		// this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
		meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
		meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
	}

	double end = timestamp();

	if (dump)
		dumpObj(mesh.vertices, std::vector<unsigned>());

	double avg_vertices = 0;
	double avg_triangles = 0;
	double avg_boundary = 0;
	double avg_connected = 0;
	size_t not_full = 0;

	std::vector<int> boundary(mesh.vertices.size());

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& m = meshlets[i];

		for (unsigned int j = 0; j < m.vertex_count; ++j)
			boundary[meshlet_vertices[m.vertex_offset + j]]++;
	}

	std::vector<unsigned int> cluster;

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& m = meshlets[i];

		if (dump)
		{
			cluster.clear();
			for (unsigned int j = 0; j < m.triangle_count * 3; ++j)
				cluster.push_back(meshlet_vertices[m.vertex_offset + meshlet_triangles[m.triangle_offset + j]]);

			char cname[32];
			snprintf(cname, sizeof(cname), "ml_%d\n", int(i));
			dumpObj(cname, cluster);
		}

		avg_vertices += m.vertex_count;
		avg_triangles += m.triangle_count;
		not_full += uniform ? m.triangle_count < max_triangles : m.vertex_count < max_vertices;

		for (unsigned int j = 0; j < m.vertex_count; ++j)
			if (boundary[meshlet_vertices[m.vertex_offset + j]] > 1)
				avg_boundary += 1;

		// union-find vertices to check if the meshlet is connected
		int parents[256];
		for (unsigned int j = 0; j < m.vertex_count; ++j)
			parents[j] = int(j);

		for (unsigned int j = 0; j < m.triangle_count * 3; ++j)
		{
			int v0 = meshlet_triangles[m.triangle_offset + j];
			int v1 = meshlet_triangles[m.triangle_offset + j + (j % 3 == 2 ? -2 : 1)];

			v0 = follow(parents, v0);
			v1 = follow(parents, v1);

			parents[v0] = v1;
		}

		int roots = 0;
		for (unsigned int j = 0; j < m.vertex_count; ++j)
			roots += follow(parents, j) == int(j);

		assert(roots != 0);
		avg_connected += roots;
	}

	avg_vertices /= double(meshlets.size());
	avg_triangles /= double(meshlets.size());
	avg_boundary /= double(meshlets.size());
	avg_connected /= double(meshlets.size());

	printf("Meshlets%c: %d meshlets (avg vertices %.1f, avg triangles %.1f, avg boundary %.1f, avg connected %.2f, not full %d) in %.2f msec\n",
	    scan ? 'S' : (flex ? 'F' : (spatial ? 'X' : (uniform ? 'U' : ' '))),
	    int(meshlets.size()), avg_vertices, avg_triangles, avg_boundary, avg_connected, int(not_full), (end - start) * 1000);

	float camera[3] = {100, 100, 100};

	size_t rejected = 0;
	size_t accepted = 0;
	double radius_mean = 0;
	double cone_mean = 0;

	std::vector<float> radii(meshlets.size());
	std::vector<float> cones(meshlets.size());

	double startc = timestamp();
	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& m = meshlets[i];

		meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[m.vertex_offset], &meshlet_triangles[m.triangle_offset], m.triangle_count, &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex));

		radii[i] = bounds.radius;
		cones[i] = 90.f - acosf(bounds.cone_cutoff) * (180.f / 3.1415926f);

		radius_mean += radii[i];
		cone_mean += cones[i];

		// trivial accept: we can't ever backface cull this meshlet
		accepted += (bounds.cone_cutoff >= 1);

		// perspective projection: dot(normalize(cone_apex - camera_position), cone_axis) > cone_cutoff
		// alternative formulation for perspective projection that doesn't use apex (and uses cluster bounding sphere instead):
		// dot(normalize(center - camera_position), cone_axis) > cone_cutoff + radius / length(center - camera_position)
		float cview[3] = {bounds.center[0] - camera[0], bounds.center[1] - camera[1], bounds.center[2] - camera[2]};
		float cviewlength = sqrtf(cview[0] * cview[0] + cview[1] * cview[1] + cview[2] * cview[2]);

		rejected += cview[0] * bounds.cone_axis[0] + cview[1] * bounds.cone_axis[1] + cview[2] * bounds.cone_axis[2] >= bounds.cone_cutoff * cviewlength + bounds.radius;
	}
	double endc = timestamp();

	radius_mean /= double(meshlets.size());
	cone_mean /= double(meshlets.size());

	double radius_variance = 0;

	for (size_t i = 0; i < meshlets.size(); ++i)
		radius_variance += (radii[i] - radius_mean) * (radii[i] - radius_mean);

	radius_variance /= double(meshlets.size() - 1);

	double radius_stddev = sqrt(radius_variance);

	size_t meshlets_std = 0;

	for (size_t i = 0; i < meshlets.size(); ++i)
		meshlets_std += radii[i] < radius_mean + radius_stddev;

	printf("Bounds   : radius mean %f stddev %f; %.1f%% meshlets under 1σ; cone angle %.1f°; cone reject %.1f%% trivial accept %.1f%% in %.2f msec\n",
	    radius_mean, radius_stddev,
	    double(meshlets_std) / double(meshlets.size()) * 100,
	    cone_mean, double(rejected) / double(meshlets.size()) * 100, double(accepted) / double(meshlets.size()) * 100,
	    (endc - startc) * 1000);
}

void spatialSort(const Mesh& mesh)
{
	typedef PackedVertexOct PV;

	std::vector<PV> pv(mesh.vertices.size());
	packMesh(pv, mesh.vertices);

	double start = timestamp();

	std::vector<unsigned int> remap(mesh.vertices.size());
	meshopt_spatialSortRemap(&remap[0], &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex));

	double end = timestamp();

	meshopt_remapVertexBuffer(&pv[0], &pv[0], mesh.vertices.size(), sizeof(PV), &remap[0]);

	std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(mesh.vertices.size(), sizeof(PV)));
	vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), &pv[0], mesh.vertices.size(), sizeof(PV)));

	size_t csize = compress(vbuf);

	printf("Spatial  : %.1f bits/vertex (post-deflate %.1f bits/vertex); sort %.2f msec\n",
	    double(vbuf.size() * 8) / double(mesh.vertices.size()),
	    double(csize * 8) / double(mesh.vertices.size()),
	    (end - start) * 1000);
}

void spatialSortTriangles(const Mesh& mesh)
{
	typedef PackedVertexOct PV;

	Mesh copy = mesh;

	double start = timestamp();

	meshopt_spatialSortTriangles(&copy.indices[0], &copy.indices[0], mesh.indices.size(), &copy.vertices[0].px, copy.vertices.size(), sizeof(Vertex));

	double end = timestamp();

	meshopt_optimizeVertexCache(&copy.indices[0], &copy.indices[0], copy.indices.size(), copy.vertices.size());
	meshopt_optimizeVertexFetch(&copy.vertices[0], &copy.indices[0], copy.indices.size(), &copy.vertices[0], copy.vertices.size(), sizeof(Vertex));

	std::vector<PV> pv(mesh.vertices.size());
	packMesh(pv, copy.vertices);

	std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(mesh.vertices.size(), sizeof(PV)));
	vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), &pv[0], mesh.vertices.size(), sizeof(PV)));

	std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(mesh.indices.size(), mesh.vertices.size()));
	ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), &copy.indices[0], mesh.indices.size()));

	size_t csizev = compress(vbuf);
	size_t csizei = compress(ibuf);

	printf("SpatialT : %.1f bits/vertex (post-deflate %.1f bits/vertex); %.1f bits/triangle (post-deflate %.1f bits/triangle); sort %.2f msec\n",
	    double(vbuf.size() * 8) / double(mesh.vertices.size()),
	    double(csizev * 8) / double(mesh.vertices.size()),
	    double(ibuf.size() * 8) / double(mesh.indices.size() / 3),
	    double(csizei * 8) / double(mesh.indices.size() / 3),
	    (end - start) * 1000);
}

void spatialClusterPoints(const Mesh& mesh, size_t cluster_size)
{
	typedef PackedVertexOct PV;

	double start = timestamp();

	std::vector<unsigned int> index(mesh.vertices.size());
	meshopt_spatialClusterPoints(&index[0], &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), cluster_size);

	double end = timestamp();

	std::vector<PV> pv(mesh.vertices.size());
	packMesh(pv, mesh.vertices);

	std::vector<PV> pvo(mesh.vertices.size());
	for (size_t i = 0; i < index.size(); ++i)
		pvo[i] = pv[index[i]];

	std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(mesh.vertices.size(), sizeof(PV)));
	vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), &pvo[0], mesh.vertices.size(), sizeof(PV)));

	size_t csize = compress(vbuf);

	printf("SpatialCP: %.1f bits/vertex (post-deflate %.1f bits/vertex); sort %.2f msec\n",
	    double(vbuf.size() * 8) / double(mesh.vertices.size()),
	    double(csize * 8) / double(mesh.vertices.size()),
	    (end - start) * 1000);
}

void tessellationAdjacency(const Mesh& mesh)
{
	double start = timestamp();

	// 12 indices per input triangle
	std::vector<unsigned int> tessib(mesh.indices.size() * 4);
	meshopt_generateTessellationIndexBuffer(&tessib[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex));

	double middle = timestamp();

	// 6 indices per input triangle
	std::vector<unsigned int> adjib(mesh.indices.size() * 2);
	meshopt_generateAdjacencyIndexBuffer(&adjib[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex));

	double end = timestamp();

	printf("Tesselltn: %d patches in %.2f msec\n", int(mesh.indices.size() / 3), (middle - start) * 1000);
	printf("Adjacency: %d patches in %.2f msec\n", int(mesh.indices.size() / 3), (end - middle) * 1000);
}

void provoking(const Mesh& mesh)
{
	double start = timestamp();

	// worst case number of vertices: vertex count + triangle count
	std::vector<unsigned int> pib(mesh.indices.size());
	std::vector<unsigned int> reorder(mesh.vertices.size() + mesh.indices.size() / 3);

	size_t pcount = meshopt_generateProvokingIndexBuffer(&pib[0], &reorder[0], &mesh.indices[0], mesh.indices.size(), mesh.vertices.size());
	reorder.resize(pcount);

	double end = timestamp();

	// validate invariant: pib[i] == i/3 for provoking vertices
	for (size_t i = 0; i < mesh.indices.size(); i += 3)
		assert(pib[i] == i / 3);

	// validate invariant: reorder[pib[x]] == ib[x] modulo triangle rotation
	// note: this is technically not promised by the interface (it may reorder triangles!), it just happens to hold right now
	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		unsigned int a = mesh.indices[i + 0], b = mesh.indices[i + 1], c = mesh.indices[i + 2];
		unsigned int ra = reorder[pib[i + 0]], rb = reorder[pib[i + 1]], rc = reorder[pib[i + 2]];

		assert((a == ra && b == rb && c == rc) || (a == rb && b == rc && c == ra) || (a == rc && b == ra && c == rb));
	}

	// best case number of vertices: max(vertex count, triangle count), assuming non-redundant indexing (all vertices are used)
	// note: this is a lower bound, and it's not theoretically possible on some meshes;
	// for example, a union of a flat shaded cube (12t 24v) and a smooth shaded icosahedron (20t 12v) will have 36 vertices and 32 triangles
	// however, the best case for that union is 44 vertices (24 cube vertices + 20 icosahedron vertices due to provoking invariant)
	size_t bestv = mesh.vertices.size() > mesh.indices.size() / 3 ? mesh.vertices.size() : mesh.indices.size() / 3;

	printf("Provoking: %d triangles / %d vertices (+%.1f%% extra) in %.2f msec\n",
	    int(mesh.indices.size() / 3), int(pcount), double(pcount) / double(bestv) * 100.0 - 100.0, (end - start) * 1000);
}

static int reindexCompare(void* context, unsigned int lhs, unsigned int rhs)
{
	const Vertex* vertices = static_cast<Vertex*>(context);
	const Vertex& lv = vertices[lhs];
	const Vertex& rv = vertices[rhs];

	float ln = lv.nx * lv.nx + lv.ny * lv.ny + lv.nz * lv.nz;
	float rn = rv.nx * rv.nx + rv.ny * rv.ny + rv.nz * rv.nz;

	// 1/1024px UV tolerance, 3 degree normal tolerance
	return fabsf(lv.tx - rv.tx) < 1e-3f &&
	       fabsf(lv.ty - rv.ty) < 1e-3f &&
	       (lv.nx * rv.nx + lv.ny * rv.ny + lv.nz * rv.nz >= 0.9986f * sqrtf(ln * rn));
}

void reindexFuzzy(const Mesh& mesh)
{
	std::vector<PackedVertex> pv(mesh.vertices.size());
	packMesh(pv, mesh.vertices);

	std::vector<unsigned int> remap(mesh.vertices.size());

	double start = timestamp();

	size_t up = meshopt_generateVertexRemap(&remap[0], &mesh.indices[0], mesh.indices.size(), &pv[0], mesh.vertices.size(), sizeof(PackedVertex));

	double middle = timestamp();

	size_t uf = meshopt_generateVertexRemapCustom(&remap[0], &mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), reindexCompare, const_cast<Vertex*>(&mesh.vertices[0]));

	double end = timestamp();

	printf("ReindexQ : %d vertices => %d unique vertices in %.2f msec\n",
	    int(mesh.vertices.size()), int(up), (middle - start) * 1000);
	printf("ReindexF : %d vertices => %d unique vertices in %.2f msec\n",
	    int(mesh.vertices.size()), int(uf), (end - middle) * 1000);
}

void coverage(const Mesh& mesh)
{
	double start = timestamp();

	meshopt_CoverageStatistics cs = meshopt_analyzeCoverage(&mesh.indices[0], mesh.indices.size(), &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex));

	double end = timestamp();

	printf("Coverage : X %.1f%% Y %.1f%% Z %.1f%% in %.2f msec\n",
	    cs.coverage[0] * 100, cs.coverage[1] * 100, cs.coverage[2] * 100, (end - start) * 1000);
}

void nanite(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices); // nanite.cpp

bool loadMesh(Mesh& mesh, const char* path)
{
	double start = timestamp();
	double middle;
	mesh = parseObj(path, middle);
	double end = timestamp();

	if (mesh.vertices.empty())
	{
		printf("Mesh %s is empty, skipping\n", path);
		return false;
	}

	printf("# %s: %d vertices, %d triangles; read in %.2f msec; indexed in %.2f msec\n", path, int(mesh.vertices.size()), int(mesh.indices.size() / 3), (middle - start) * 1000, (end - middle) * 1000);
	return true;
}

void processDeinterleaved(const char* path)
{
	// Most algorithms in the library work out of the box with deinterleaved geometry, but some require slightly special treatment;
	// this code runs a simplified version of complete opt. pipeline using deinterleaved geo. There's no compression performed but you
	// can trivially run it by quantizing all elements and running meshopt_encodeVertexBuffer once for each vertex stream.
	fastObjMesh* obj = fast_obj_read(path);
	if (!obj)
	{
		printf("Error loading %s: file not found\n", path);
		return;
	}

	size_t total_indices = 0;

	for (unsigned int i = 0; i < obj->face_count; ++i)
		total_indices += 3 * (obj->face_vertices[i] - 2);

	std::vector<float> unindexed_pos(total_indices * 3);
	std::vector<float> unindexed_nrm(total_indices * 3);
	std::vector<float> unindexed_uv(total_indices * 2);

	size_t vertex_offset = 0;
	size_t index_offset = 0;

	for (unsigned int i = 0; i < obj->face_count; ++i)
	{
		for (unsigned int j = 0; j < obj->face_vertices[i]; ++j)
		{
			fastObjIndex gi = obj->indices[index_offset + j];

			// triangulate polygon on the fly; offset-3 is always the first polygon vertex
			if (j >= 3)
			{
				memcpy(&unindexed_pos[(vertex_offset + 0) * 3], &unindexed_pos[(vertex_offset - 3) * 3], 3 * sizeof(float));
				memcpy(&unindexed_nrm[(vertex_offset + 0) * 3], &unindexed_nrm[(vertex_offset - 3) * 3], 3 * sizeof(float));
				memcpy(&unindexed_uv[(vertex_offset + 0) * 2], &unindexed_uv[(vertex_offset - 3) * 2], 2 * sizeof(float));
				memcpy(&unindexed_pos[(vertex_offset + 1) * 3], &unindexed_pos[(vertex_offset - 1) * 3], 3 * sizeof(float));
				memcpy(&unindexed_nrm[(vertex_offset + 1) * 3], &unindexed_nrm[(vertex_offset - 1) * 3], 3 * sizeof(float));
				memcpy(&unindexed_uv[(vertex_offset + 1) * 2], &unindexed_uv[(vertex_offset - 1) * 2], 2 * sizeof(float));
				vertex_offset += 2;
			}

			memcpy(&unindexed_pos[vertex_offset * 3], &obj->positions[gi.p * 3], 3 * sizeof(float));
			memcpy(&unindexed_nrm[vertex_offset * 3], &obj->normals[gi.n * 3], 3 * sizeof(float));
			memcpy(&unindexed_uv[vertex_offset * 2], &obj->texcoords[gi.t * 2], 2 * sizeof(float));
			vertex_offset++;
		}

		index_offset += obj->face_vertices[i];
	}

	fast_obj_destroy(obj);

	double start = timestamp();

	meshopt_Stream streams[] = {
	    {&unindexed_pos[0], sizeof(float) * 3, sizeof(float) * 3},
	    {&unindexed_nrm[0], sizeof(float) * 3, sizeof(float) * 3},
	    {&unindexed_uv[0], sizeof(float) * 2, sizeof(float) * 2},
	};

	std::vector<unsigned int> remap(total_indices);

	size_t total_vertices = meshopt_generateVertexRemapMulti(&remap[0], NULL, total_indices, total_indices, streams, sizeof(streams) / sizeof(streams[0]));

	std::vector<unsigned int> indices(total_indices);
	meshopt_remapIndexBuffer(&indices[0], NULL, total_indices, &remap[0]);

	std::vector<float> pos(total_vertices * 3);
	meshopt_remapVertexBuffer(&pos[0], &unindexed_pos[0], total_indices, sizeof(float) * 3, &remap[0]);

	std::vector<float> nrm(total_vertices * 3);
	meshopt_remapVertexBuffer(&nrm[0], &unindexed_nrm[0], total_indices, sizeof(float) * 3, &remap[0]);

	std::vector<float> uv(total_vertices * 2);
	meshopt_remapVertexBuffer(&uv[0], &unindexed_uv[0], total_indices, sizeof(float) * 2, &remap[0]);

	double reindex = timestamp();

	meshopt_optimizeVertexCache(&indices[0], &indices[0], total_indices, total_vertices);

	meshopt_optimizeVertexFetchRemap(&remap[0], &indices[0], total_indices, total_vertices);
	meshopt_remapVertexBuffer(&pos[0], &pos[0], total_vertices, sizeof(float) * 3, &remap[0]);
	meshopt_remapVertexBuffer(&nrm[0], &nrm[0], total_vertices, sizeof(float) * 3, &remap[0]);
	meshopt_remapVertexBuffer(&uv[0], &uv[0], total_vertices, sizeof(float) * 2, &remap[0]);

	double optimize = timestamp();

	// note: since shadow index buffer is computed based on regular vertex/index buffer, the stream points at the indexed data - not unindexed_pos
	meshopt_Stream shadow_stream = {&pos[0], sizeof(float) * 3, sizeof(float) * 3};

	std::vector<unsigned int> shadow_indices(total_indices);
	meshopt_generateShadowIndexBufferMulti(&shadow_indices[0], &indices[0], total_indices, total_vertices, &shadow_stream, 1);

	meshopt_optimizeVertexCache(&shadow_indices[0], &shadow_indices[0], total_indices, total_vertices);

	double shadow = timestamp();

	printf("Deintrlvd: %d vertices, reindexed in %.2f msec, optimized in %.2f msec, generated & optimized shadow indices in %.2f msec\n",
	    int(total_vertices), (reindex - start) * 1000, (optimize - reindex) * 1000, (shadow - optimize) * 1000);
}

void process(const char* path)
{
	Mesh mesh;
	if (!loadMesh(mesh, path))
		return;

	optimize(mesh);
	optimize(mesh, /* fifo= */ true);

	Mesh copy = mesh;
	meshopt_optimizeVertexCache(&copy.indices[0], &copy.indices[0], copy.indices.size(), copy.vertices.size());
	meshopt_optimizeVertexFetch(&copy.vertices[0], &copy.indices[0], copy.indices.size(), &copy.vertices[0], copy.vertices.size(), sizeof(Vertex));

	Mesh copystrip = mesh;
	meshopt_optimizeVertexCacheStrip(&copystrip.indices[0], &copystrip.indices[0], copystrip.indices.size(), copystrip.vertices.size());
	meshopt_optimizeVertexFetch(&copystrip.vertices[0], &copystrip.indices[0], copystrip.indices.size(), &copystrip.vertices[0], copystrip.vertices.size(), sizeof(Vertex));

	stripify(copy, false, ' ');
	stripify(copy, /* use_restart= */ true, 'R');
	stripify(copystrip, /* use_restart= */ true, 'S');

	meshlets(copy, /* scan= */ true);
	meshlets(copy, /* scan= */ false);
	meshlets(copy, /* scan= */ false, /* uniform= */ true);
	meshlets(copy, /* scan= */ false, /* uniform= */ false, /* flex= */ true);
	meshlets(copy, /* scan= */ false, /* uniform= */ true, /* flex= */ false, /* spatial= */ true);

	shadow(copy);
	tessellationAdjacency(copy);
	provoking(copy);

	encodeIndex(copy, ' ');
	encodeIndex(copystrip, 'S');

	std::vector<unsigned int> strip(meshopt_stripifyBound(copystrip.indices.size()));
	strip.resize(meshopt_stripify(&strip[0], &copystrip.indices[0], copystrip.indices.size(), copystrip.vertices.size(), 0));

	encodeIndexSequence(strip, copystrip.vertices.size(), 'D');

	packVertex<PackedVertex>(copy, "");
	encodeVertex<PackedVertex>(copy, "");
	encodeVertex<PackedVertexOct>(copy, "O");

	simplify(mesh);
	simplify(mesh, 0.1f, meshopt_SimplifyPrune);
	simplifyAttr(mesh);
	simplifyAttr(mesh, 0.1f, meshopt_SimplifyPermissive);
	simplifyUpdate(mesh);
	simplifySloppy(mesh);
	simplifyComplete(mesh);
	simplifyPoints(mesh);
	simplifyClusters(mesh);

	spatialSort(mesh);
	spatialSortTriangles(mesh);
	spatialClusterPoints(mesh, 64);

	reindexFuzzy(mesh);
	coverage(mesh);

	if (path)
		processDeinterleaved(path);
}

void processDev(const char* path)
{
	Mesh mesh;
	if (!loadMesh(mesh, path))
		return;

	simplifyUpdate(mesh, 0.1f, meshopt_SimplifyPrune | meshopt_SimplifyPermissive);
}

void processNanite(const char* path)
{
	Mesh mesh;
	if (!loadMesh(mesh, path))
		return;

	nanite(mesh.vertices, mesh.indices);
}

int main(int argc, char** argv)
{
	void runTests();

	meshopt_encodeVertexVersion(1);
	meshopt_encodeIndexVersion(1);

	if (argc == 1)
	{
		runTests();
	}
	else
	{
		if (strcmp(argv[1], "-d") == 0)
		{
			for (int i = 2; i < argc; ++i)
				processDev(argv[i]);
		}
		else if (strcmp(argv[1], "-n") == 0)
		{
			for (int i = 2; i < argc; ++i)
				processNanite(argv[i]);
		}
		else
		{
			for (int i = 1; i < argc; ++i)
				process(argv[i]);

			runTests();
		}
	}
}
