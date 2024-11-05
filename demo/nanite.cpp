// This is a playground for experimenting with algorithms necessary for Nanite like hierarchical clustering
// The code is not optimized, not robust, and not intended for production use.
// It optionally supports METIS for clustering and partitioning, with an eventual goal of removing this code
// in favor of meshopt algorithms.

// For reference, see the original Nanite paper:
// Brian Karis. Nanite: A Deep Dive. 2021

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "../src/meshoptimizer.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <map>
#include <vector>

#ifdef METIS
#include <metis.h>
#endif

#ifndef TRACE
#define TRACE 0
#endif

struct Vertex
{
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;
};

struct LODBounds
{
	float center[3];
	float radius;
	float error;
};

struct Cluster
{
	std::vector<unsigned int> indices;

	LODBounds self;
	LODBounds parent;
};

const size_t kClusterSize = 128;
const size_t kGroupSize = 8;
const bool kUseLocks = true;
const bool kUseNormals = true;

static LODBounds bounds(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, float error)
{
	meshopt_Bounds bounds = meshopt_computeClusterBounds(&indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex));

	LODBounds result;
	result.center[0] = bounds.center[0];
	result.center[1] = bounds.center[1];
	result.center[2] = bounds.center[2];
	result.radius = bounds.radius;
	result.error = error;
	return result;
}

static LODBounds boundsMerge(const std::vector<Cluster>& clusters, const std::vector<int>& group)
{
	LODBounds result = {};

	// we approximate merged bounds center as weighted average of cluster centers
	// (could also use bounds() center, but we can't use bounds() radius so might as well just merge manually)
	float weight = 0.f;
	for (size_t j = 0; j < group.size(); ++j)
	{
		result.center[0] += clusters[group[j]].self.center[0] * clusters[group[j]].self.radius;
		result.center[1] += clusters[group[j]].self.center[1] * clusters[group[j]].self.radius;
		result.center[2] += clusters[group[j]].self.center[2] * clusters[group[j]].self.radius;
		weight += clusters[group[j]].self.radius;
	}

	if (weight > 0)
	{
		result.center[0] /= weight;
		result.center[1] /= weight;
		result.center[2] /= weight;
	}

	// merged bounds must strictly contain all cluster bounds
	result.radius = 0.f;
	for (size_t j = 0; j < group.size(); ++j)
	{
		float dx = clusters[group[j]].self.center[0] - result.center[0];
		float dy = clusters[group[j]].self.center[1] - result.center[1];
		float dz = clusters[group[j]].self.center[2] - result.center[2];
		result.radius = std::max(result.radius, clusters[group[j]].self.radius + sqrtf(dx * dx + dy * dy + dz * dz));
	}

	// merged bounds error must be conservative wrt cluster errors
	result.error = 0.f;
	for (size_t j = 0; j < group.size(); ++j)
		result.error = std::max(result.error, clusters[group[j]].self.error);

	return result;
}

// computes approximate (perspective) projection error of a cluster in screen space (0..1; multiply by screen height to get pixels)
// camera_proj is projection[1][1], or cot(fovy/2); camera_znear is *positive* near plane distance
// for DAG cut to be valid, boundsError must be monotonic: it must return a larger error for parent cluster
// for simplicity, we ignore perspective distortion and use rotationally invariant projection size estimation
static float boundsError(const LODBounds& bounds, float camera_x, float camera_y, float camera_z, float camera_proj, float camera_znear)
{
	float dx = bounds.center[0] - camera_x, dy = bounds.center[1] - camera_y, dz = bounds.center[2] - camera_z;
	float d = sqrtf(dx * dx + dy * dy + dz * dz) - bounds.radius;
	return bounds.error / (d > camera_znear ? d : camera_znear) * (camera_proj * 0.5f);
}

#ifdef METIS
static void clusterizeMetisRec(std::vector<Cluster>& result, const std::vector<unsigned int>& indices, const std::vector<int>& triidx, const std::vector<int>& triadj)
{
	assert(triadj.size() == triidx.size() * 3);

	if (triidx.size() <= kClusterSize)
	{
		Cluster cluster;
		for (size_t i = 0; i < triidx.size(); ++i)
		{
			cluster.indices.push_back(indices[triidx[i] * 3 + 0]);
			cluster.indices.push_back(indices[triidx[i] * 3 + 1]);
			cluster.indices.push_back(indices[triidx[i] * 3 + 2]);
		}

		cluster.parent.error = FLT_MAX;
		result.push_back(cluster);
		return;
	}

	std::vector<int> xadj(triidx.size() + 1);
	std::vector<int> adjncy;
	std::vector<int> part(triidx.size());

	for (size_t i = 0; i < triidx.size(); ++i)
	{
		for (int j = 0; j < 3; ++j)
			if (triadj[i * 3 + j] != -1)
				adjncy.push_back(triadj[i * 3 + j]);

		xadj[i + 1] = adjncy.size();
	}

	int options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);
	options[METIS_OPTION_SEED] = 42;
	options[METIS_OPTION_UFACTOR] = triidx.size() > 8 * kClusterSize ? 100 : 1;

	int nvtxs = int(triidx.size());
	int ncon = 1;
	int nparts = 2;
	int edgecut = 0;

	int nvtxspad = (nvtxs + kClusterSize - 1) / kClusterSize * kClusterSize;
	int idealcut = ((nvtxspad / kClusterSize) / 2) * kClusterSize;
	float partw[2] = {idealcut / float(nvtxspad), 1.f - idealcut / float(nvtxspad)};

	int r = METIS_PartGraphRecursive(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL, NULL, &nparts, partw, NULL, options, &edgecut, &part[0]);
	assert(r == METIS_OK);
	(void)r;

	int partsize[2] = {};
	std::vector<int> partoff(part.size());
	for (size_t i = 0; i < part.size(); ++i)
		partoff[i] = partsize[part[i]]++;

	for (int p = 0; p < 2; ++p)
	{
		std::vector<int> partidx, partadj;
		partidx.reserve(partsize[p]);
		partadj.reserve(partsize[p] * 3);

		for (size_t i = 0; i < triidx.size(); ++i)
		{
			if (part[i] != p)
				continue;

			partidx.push_back(triidx[i]);

			for (int j = 0; j < 3; ++j)
			{
				if (triadj[i * 3 + j] >= 0 && part[triadj[i * 3 + j]] == p)
					partadj.push_back(partoff[triadj[i * 3 + j]]);
				else
					partadj.push_back(-1);
			}
		}

		clusterizeMetisRec(result, indices, partidx, partadj);
	}
}

static std::vector<Cluster> clusterizeMetis(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	std::vector<unsigned int> shadowib(indices.size());
	meshopt_generateShadowIndexBuffer(&shadowib[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(float) * 3, sizeof(Vertex));

	std::map<std::pair<unsigned int, unsigned int>, unsigned int> edges;

	for (size_t i = 0; i < indices.size(); ++i)
	{
		unsigned int v0 = shadowib[i + 0];
		unsigned int v1 = shadowib[i + (i % 3 == 2 ? -2 : 1)];

		// we don't track adjacency fully on non-manifold edges for now
		edges[std::make_pair(v0, v1)] = unsigned(i / 3);
	}

	std::vector<int> triadj(indices.size(), -1);

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		unsigned int v0 = shadowib[i + 0], v1 = shadowib[i + 1], v2 = shadowib[i + 2];

		std::map<std::pair<unsigned int, unsigned int>, unsigned int>::iterator oab = edges.find(std::make_pair(v1, v0));
		std::map<std::pair<unsigned int, unsigned int>, unsigned int>::iterator obc = edges.find(std::make_pair(v2, v1));
		std::map<std::pair<unsigned int, unsigned int>, unsigned int>::iterator oca = edges.find(std::make_pair(v0, v2));

		triadj[i + 0] = oab != edges.end() ? int(oab->second) : -1;
		triadj[i + 1] = obc != edges.end() ? int(obc->second) : -1;
		triadj[i + 2] = oca != edges.end() ? int(oca->second) : -1;
	}

	std::vector<int> triidx(indices.size() / 3);
	for (size_t i = 0; i < indices.size(); i += 3)
		triidx[i / 3] = int(i / 3);

	std::vector<Cluster> result;
	clusterizeMetisRec(result, indices, triidx, triadj);

	return result;
}
#endif

static std::vector<Cluster> clusterize(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
#ifdef METIS
	static const char* metis = getenv("METIS");
	if (metis && atoi(metis) >= 2)
		return clusterizeMetis(vertices, indices);
#endif

	const size_t max_vertices = 192; // TODO: depends on kClusterSize, also may want to dial down for mesh shaders
	const size_t max_triangles = kClusterSize;

	size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	meshlets.resize(meshopt_buildMeshlets(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), max_vertices, max_triangles, 0.f));

	std::vector<Cluster> clusters(meshlets.size());

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& meshlet = meshlets[i];

		meshopt_optimizeMeshlet(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset], meshlet.triangle_count, meshlet.vertex_count);

		// note: for now we discard meshlet-local indices; they are valuable for shader code so in the future we should bring them back
		clusters[i].indices.resize(meshlet.triangle_count * 3);
		for (size_t j = 0; j < meshlet.triangle_count * 3; ++j)
			clusters[i].indices[j] = meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + j]];

		clusters[i].parent.error = FLT_MAX;
	}

	return clusters;
}

#ifdef METIS
static std::vector<std::vector<int> > partitionMetis(const std::vector<Cluster>& clusters, const std::vector<int>& pending, const std::vector<unsigned int>& remap)
{
	std::vector<std::vector<int> > result;
	std::vector<std::vector<int> > vertices(remap.size());

	for (size_t i = 0; i < pending.size(); ++i)
	{
		const Cluster& cluster = clusters[pending[i]];

		for (size_t j = 0; j < cluster.indices.size(); ++j)
		{
			int v = remap[cluster.indices[j]];

			std::vector<int>& list = vertices[v];
			if (list.empty() || list.back() != int(i))
				list.push_back(int(i));
		}
	}

	std::map<std::pair<int, int>, int> adjacency;

	for (size_t v = 0; v < vertices.size(); ++v)
	{
		const std::vector<int>& list = vertices[v];

		for (size_t i = 0; i < list.size(); ++i)
			for (size_t j = i + 1; j < list.size(); ++j)
				adjacency[std::make_pair(std::min(list[i], list[j]), std::max(list[i], list[j]))]++;
	}

	std::vector<int> xadj(pending.size() + 1);
	std::vector<int> adjncy;
	std::vector<int> adjwgt;
	std::vector<int> part(pending.size());

	for (size_t i = 0; i < pending.size(); ++i)
	{
		for (std::map<std::pair<int, int>, int>::iterator it = adjacency.begin(); it != adjacency.end(); ++it)
			if (it->first.first == int(i))
			{
				adjncy.push_back(it->first.second);
				adjwgt.push_back(it->second);
			}
			else if (it->first.second == int(i))
			{
				adjncy.push_back(it->first.first);
				adjwgt.push_back(it->second);
			}

		xadj[i + 1] = adjncy.size();
	}

	int options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);
	options[METIS_OPTION_SEED] = 42;
	options[METIS_OPTION_UFACTOR] = 100;

	int nvtxs = int(pending.size());
	int ncon = 1;
	int nparts = int(pending.size() + kGroupSize - 1) / kGroupSize;
	int edgecut = 0;

	if (nparts <= 1)
	{
		// not sure why this is a special case that we need to handle but okay metis
		result.push_back(pending);
	}
	else
	{
		int r = METIS_PartGraphKway(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL, &adjwgt[0], &nparts, NULL, NULL, options, &edgecut, &part[0]);
		assert(r == METIS_OK);
		(void)r;

		result.resize(nparts);
		for (size_t i = 0; i < part.size(); ++i)
			result[part[i]].push_back(pending[i]);
	}

	return result;
}
#endif

static std::vector<std::vector<int> > partition(const std::vector<Cluster>& clusters, const std::vector<int>& pending, const std::vector<unsigned int>& remap)
{
#ifdef METIS
	static const char* metis = getenv("METIS");
	if (metis && atoi(metis) >= 1)
		return partitionMetis(clusters, pending, remap);
#endif

	(void)remap;

	std::vector<std::vector<int> > result;

	size_t last_indices = 0;

	// rough merge; while clusters are approximately spatially ordered, this should use a proper partitioning algorithm
	for (size_t i = 0; i < pending.size(); ++i)
	{
		if (result.empty() || last_indices + clusters[pending[i]].indices.size() > kClusterSize * kGroupSize * 3)
		{
			result.push_back(std::vector<int>());
			last_indices = 0;
		}

		result.back().push_back(pending[i]);
		last_indices += clusters[pending[i]].indices.size();
	}

	return result;
}

static void lockBoundary(std::vector<unsigned char>& locks, const std::vector<std::vector<int> >& groups, const std::vector<Cluster>& clusters, const std::vector<unsigned int>& remap)
{
	std::vector<int> groupmap(locks.size(), -1);

	for (size_t i = 0; i < groups.size(); ++i)
		for (size_t j = 0; j < groups[i].size(); ++j)
		{
			const Cluster& cluster = clusters[groups[i][j]];

			for (size_t k = 0; k < cluster.indices.size(); ++k)
			{
				unsigned int v = cluster.indices[k];
				unsigned int r = remap[v];

				if (groupmap[r] == -1 || groupmap[r] == int(i))
					groupmap[r] = int(i);
				else
					groupmap[r] = -2;
			}
		}

	// note: we need to consistently lock all vertices with the same position to avoid holes
	for (size_t i = 0; i < locks.size(); ++i)
	{
		unsigned int r = remap[i];

		locks[i] = (groupmap[r] == -2);
	}
}

static std::vector<unsigned int> simplify(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<unsigned char>* locks, size_t target_count, float* error = NULL)
{
	if (target_count > indices.size())
		return indices;

	std::vector<unsigned int> lod(indices.size());
	unsigned int options = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute;
	float normal_weights[3] = {0.5f, 0.5f, 0.5f};
	if (kUseNormals)
		lod.resize(meshopt_simplifyWithAttributes(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), &vertices[0].nx, sizeof(Vertex), normal_weights, 3, locks ? &(*locks)[0] : NULL, target_count, FLT_MAX, options, error));
	else if (locks)
		lod.resize(meshopt_simplifyWithAttributes(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), NULL, 0, NULL, 0, &(*locks)[0], target_count, FLT_MAX, options, error));
	else
		lod.resize(meshopt_simplify(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), target_count, FLT_MAX, options | meshopt_SimplifyLockBorder, error));
	return lod;
}

void dumpObj(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, bool recomputeNormals = false);
void dumpObj(const char* section, const std::vector<unsigned int>& indices);

void nanite(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	static const char* metis = getenv("METIS");
	if (metis && atoi(metis))
	{
#ifdef METIS
		printf("using metis for %s\n", atoi(metis) >= 2 ? "both clustering and partition" : "partition only");
#else
		printf("ERROR: build does not have metis available\n");
#endif
	}

	static const char* dump = getenv("DUMP");

	// initial clusterization splits the original mesh
	std::vector<Cluster> clusters = clusterize(vertices, indices);
	for (size_t i = 0; i < clusters.size(); ++i)
		clusters[i].self = bounds(vertices, clusters[i].indices, 0.f);

	printf("lod 0: %d clusters, %d triangles\n", int(clusters.size()), int(indices.size() / 3));

	std::vector<int> pending(clusters.size());
	for (size_t i = 0; i < clusters.size(); ++i)
		pending[i] = int(i);

#ifndef NDEBUG
	std::vector<std::pair<int, int> > dag_debug;
#endif

	int depth = 0;
	std::vector<unsigned char> locks(vertices.size());

	// for cluster connectivity, we need a position-only remap that maps vertices with the same position to the same index
	// it's more efficient to build it once; unfortunately, meshopt_generateVertexRemap doesn't support stride so we need to use *Multi version
	std::vector<unsigned int> remap(vertices.size());
	meshopt_Stream position = {&vertices[0].px, sizeof(float) * 3, sizeof(Vertex)};
	meshopt_generateVertexRemapMulti(&remap[0], &indices[0], indices.size(), vertices.size(), &position, 1);

	// merge and simplify clusters until we can't merge anymore
	while (pending.size() > 1)
	{
		std::vector<std::vector<int> > groups = partition(clusters, pending, remap);
		pending.clear();

		std::vector<int> retry;

		size_t triangles = 0;
		size_t stuck_triangles = 0;
		int single_clusters = 0;
		int stuck_clusters = 0;
		int full_clusters = 0;

		if (dump && depth == atoi(dump))
			dumpObj(vertices, std::vector<unsigned int>());

		if (kUseLocks)
			lockBoundary(locks, groups, clusters, remap);

		// every group needs to be simplified now
		for (size_t i = 0; i < groups.size(); ++i)
		{
			if (groups[i].empty())
				continue; // metis shortcut

			if (groups[i].size() == 1)
			{
#if TRACE
				printf("stuck cluster: singleton with %d triangles\n", int(clusters[groups[i][0]].indices.size() / 3));
#endif

				if (dump && depth == atoi(dump))
					dumpObj("cluster", clusters[groups[i][0]].indices);

				single_clusters++;
				stuck_clusters++;
				stuck_triangles += clusters[groups[i][0]].indices.size() / 3;
				retry.push_back(groups[i][0]);
				continue;
			}

			std::vector<unsigned int> merged;
			for (size_t j = 0; j < groups[i].size(); ++j)
				merged.insert(merged.end(), clusters[groups[i][j]].indices.begin(), clusters[groups[i][j]].indices.end());

			if (dump && depth == atoi(dump))
			{
				for (size_t j = 0; j < groups[i].size(); ++j)
					dumpObj("cluster", clusters[groups[i][j]].indices);

				dumpObj("group", merged);
			}

			size_t target_size = ((groups[i].size() + 1) / 2) * kClusterSize * 3;
			float error = 0.f;
			std::vector<unsigned int> simplified = simplify(vertices, merged, kUseLocks ? &locks : NULL, target_size, &error);
			if (simplified.size() > merged.size() * 0.85f || simplified.size() / (kClusterSize * 3) >= merged.size() / (kClusterSize * 3))
			{
#if TRACE
				printf("stuck cluster: simplified %d => %d over threshold\n", int(merged.size() / 3), int(simplified.size() / 3));
#endif
				stuck_clusters++;
				stuck_triangles += merged.size() / 3;
				for (size_t j = 0; j < groups[i].size(); ++j)
					retry.push_back(groups[i][j]);
				continue; // simplification is stuck; abandon the merge
			}

			// enforce bounds and error monotonicity
			// note: it is incorrect to use the precise bounds of the merged or simplified mesh, because this may violate monotonicity
			LODBounds groupb = boundsMerge(clusters, groups[i]);
			groupb.error += error; // this may overestimate the error, but we are starting from the simplified mesh so this is a little more correct

			std::vector<Cluster> split = clusterize(vertices, simplified);

			// update parent bounds and error for all clusters in the group
			// note that all clusters in the group need to switch simultaneously so they have the same bounds
			for (size_t j = 0; j < groups[i].size(); ++j)
			{
				assert(clusters[groups[i][j]].parent.error == FLT_MAX);
				clusters[groups[i][j]].parent = groupb;
			}

#ifndef NDEBUG
			// record DAG edges for validation during the cut
			for (size_t j = 0; j < groups[i].size(); ++j)
				for (size_t k = 0; k < split.size(); ++k)
					dag_debug.push_back(std::make_pair(groups[i][j], int(clusters.size()) + int(k)));
#endif

			for (size_t j = 0; j < split.size(); ++j)
			{
				split[j].self = groupb;

				clusters.push_back(split[j]); // std::move
				pending.push_back(int(clusters.size()) - 1);

				triangles += split[j].indices.size() / 3;
				full_clusters += split[j].indices.size() == kClusterSize * 3;
			}
		}

		depth++;
		printf("lod %d: simplified %d clusters (%d full, %.1f tri/cl), %d triangles; stuck %d clusters (%d single), %d triangles\n", depth,
		    int(pending.size()), full_clusters, pending.empty() ? 0 : double(triangles) / double(pending.size()), int(triangles), stuck_clusters, single_clusters, int(stuck_triangles));

		if (triangles < stuck_triangles / 3)
			break;

		pending.insert(pending.end(), retry.begin(), retry.end());
	}

	size_t total_triangles = 0;
	size_t lowest_triangles = 0;
	for (size_t i = 0; i < clusters.size(); ++i)
	{
		total_triangles += clusters[i].indices.size() / 3;
		if (clusters[i].parent.error == FLT_MAX)
			lowest_triangles += clusters[i].indices.size() / 3;
	}

	printf("total: %d triangles in %d clusters\n", int(total_triangles), int(clusters.size()));
	printf("lowest lod: %d triangles\n", int(lowest_triangles));

	// for testing purposes, we can compute a DAG cut from a given viewpoint and dump it as an OBJ
	float maxx = 0.f, maxy = 0.f, maxz = 0.f;
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		maxx = std::max(maxx, vertices[i].px * 2);
		maxy = std::max(maxy, vertices[i].py * 2);
		maxz = std::max(maxz, vertices[i].pz * 2);
	}

	float threshold = 2e-3f; // 2 pixels at 1080p
	float fovy = 60.f;
	float znear = 1e-2f;
	float proj = 1.f / tanf(fovy * 3.1415926f / 180.f * 0.5f);

	std::vector<unsigned int> cut;
	for (size_t i = 0; i < clusters.size(); ++i)
		if (boundsError(clusters[i].self, maxx, maxy, maxz, proj, znear) <= threshold && boundsError(clusters[i].parent, maxx, maxy, maxz, proj, znear) > threshold)
			cut.insert(cut.end(), clusters[i].indices.begin(), clusters[i].indices.end());

#ifndef NDEBUG
	for (size_t i = 0; i < dag_debug.size(); ++i)
	{
		int j = dag_debug[i].first, k = dag_debug[i].second;
		float ej = boundsError(clusters[j].self, maxx, maxy, maxz, proj, znear);
		float ejp = boundsError(clusters[j].parent, maxx, maxy, maxz, proj, znear);
		float ek = boundsError(clusters[k].self, maxx, maxy, maxz, proj, znear);

		assert(ej <= ek);
		assert(ejp >= ej);
	}
#endif

	printf("cut (%.3f): %d triangles\n", threshold, int(cut.size() / 3));

	if (dump && -1 == atoi(dump))
	{
		dumpObj(vertices, cut);

		for (size_t i = 0; i < clusters.size(); ++i)
			if (boundsError(clusters[i].self, maxx, maxy, maxz, proj, znear) <= threshold && boundsError(clusters[i].parent, maxx, maxy, maxz, proj, znear) > threshold)
				dumpObj("cluster", clusters[i].indices);
	}
}
