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

#include <algorithm>
#include <map> // only for METIS
#include <vector>

#ifndef _WIN32
#include <dlfcn.h>
#endif

#define METIS_OK 1
#define METIS_OPTION_SEED 8
#define METIS_OPTION_UFACTOR 16
#define METIS_NOPTIONS 40

static int METIS = 0;
static int (*METIS_SetDefaultOptions)(int* options);
static int (*METIS_PartGraphRecursive)(int* nvtxs, int* ncon, int* xadj,
    int* adjncy, int* vwgt, int* vsize, int* adjwgt,
    int* nparts, float* tpwgts, float* ubvec, int* options,
    int* edgecut, int* part);

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
const bool kUseRetry = true;
const bool kUseSpatial = false;
const bool kUsePermissiveFallback = true;
const bool kUseSloppyFallback = false;
const int kMetisSlop = 2;
const float kSimplifyThreshold = 0.85f;

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
	std::vector<LODBounds> bounds(group.size());
	for (size_t j = 0; j < group.size(); ++j)
		bounds[j] = clusters[group[j]].self;

	meshopt_Bounds merged = meshopt_computeSphereBounds(&bounds[0].center[0], bounds.size(), sizeof(LODBounds), &bounds[0].radius, sizeof(LODBounds));

	LODBounds result = {};
	result.center[0] = merged.center[0];
	result.center[1] = merged.center[1];
	result.center[2] = merged.center[2];
	result.radius = merged.radius;

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

static std::vector<Cluster> clusterizeMetis(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
static std::vector<std::vector<int> > partitionMetis(const std::vector<Cluster>& clusters, const std::vector<int>& pending, const std::vector<unsigned int>& remap);

static std::vector<Cluster> clusterize(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	if (METIS & 2)
		return clusterizeMetis(vertices, indices);

	const size_t max_vertices = 192; // TODO: depends on kClusterSize, also may want to dial down for mesh shaders
	const size_t max_triangles = kClusterSize;
	const size_t min_triangles = (kClusterSize / 3) & ~3;
	const float split_factor = 2.0f;
	const float fill_weight = 0.75f;

	size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, min_triangles);

	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	if (kUseSpatial)
		meshlets.resize(meshopt_buildMeshletsSpatial(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), max_vertices, min_triangles, max_triangles, fill_weight));
	else
		meshlets.resize(meshopt_buildMeshletsFlex(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), max_vertices, min_triangles, max_triangles, 0.f, split_factor));

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

static std::vector<std::vector<int> > partition(const std::vector<Cluster>& clusters, const std::vector<int>& pending, const std::vector<unsigned int>& remap, const std::vector<Vertex>& vertices)
{
	if (METIS & 1)
		return partitionMetis(clusters, pending, remap);

	std::vector<unsigned int> cluster_indices;
	std::vector<unsigned int> cluster_counts(pending.size());

	size_t total_index_count = 0;
	for (size_t i = 0; i < pending.size(); ++i)
		total_index_count += clusters[pending[i]].indices.size();

	cluster_indices.reserve(total_index_count);

	for (size_t i = 0; i < pending.size(); ++i)
	{
		const Cluster& cluster = clusters[pending[i]];

		cluster_counts[i] = unsigned(cluster.indices.size());

		for (size_t j = 0; j < cluster.indices.size(); ++j)
			cluster_indices.push_back(remap[cluster.indices[j]]);
	}

	std::vector<unsigned int> cluster_part(pending.size());
	size_t partition_count = meshopt_partitionClusters(&cluster_part[0], &cluster_indices[0], cluster_indices.size(), &cluster_counts[0], cluster_counts.size(), &vertices[0].px, remap.size(), sizeof(Vertex), kGroupSize);

	std::vector<std::vector<int> > partitions(partition_count);
	for (size_t i = 0; i < partition_count; ++i)
		partitions[i].reserve(kGroupSize + 4);

	for (size_t i = 0; i < pending.size(); ++i)
		partitions[cluster_part[i]].push_back(pending[i]);

	return partitions;
}

static void lockBoundary(std::vector<unsigned char>& locks, const std::vector<std::vector<int> >& groups, const std::vector<Cluster>& clusters, const std::vector<unsigned int>& remap)
{
	// for each remapped vertex, keep track of index of the group it's in (or -2 if it's in multiple groups)
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

	for (size_t i = 0; i < locks.size(); ++i)
	{
		unsigned int r = remap[i];

		// keep protect bit if set
		locks[i] = (groupmap[r] == -2) | (locks[i] & 2);
	}
}

static std::vector<unsigned int> simplify(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<unsigned char>* locks, size_t target_count, float* error = NULL)
{
	if (target_count > indices.size())
		return indices;

	std::vector<unsigned int> lod(indices.size());

	unsigned int options = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute | (locks ? 0 : meshopt_SimplifyLockBorder);

	float normal_weight = kUseNormals ? 0.5f : 0.0f;
	float normal_weights[3] = {normal_weight, normal_weight, normal_weight};

	lod.resize(meshopt_simplifyWithAttributes(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), &vertices[0].nx, sizeof(Vertex), normal_weights, 3, locks ? &(*locks)[0] : NULL, target_count, FLT_MAX, options, error));

	if (lod.size() > target_count && kUsePermissiveFallback)
		lod.resize(meshopt_simplifyWithAttributes(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), &vertices[0].nx, sizeof(Vertex), normal_weights, 3, locks ? &(*locks)[0] : NULL, target_count, FLT_MAX, options | meshopt_SimplifyPermissive, error));

	if (lod.size() > target_count && kUseSloppyFallback)
		lod.resize(meshopt_simplifySloppy(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), locks ? &(*locks)[0] : NULL, target_count, FLT_MAX, error));

	return lod;
}

static void dumpMetrics(int level, const std::vector<Cluster>& queue, const std::vector<std::vector<int> >& groups, const std::vector<unsigned int>& remap, const std::vector<unsigned char>& locks, const std::vector<int>& retry);

static bool loadMetis();

void dumpObj(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, bool recomputeNormals = false);
void dumpObj(const char* section, const std::vector<unsigned int>& indices);

void clrt(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

void nanite(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	static const char* clrt = getenv("CLRT");

	if (clrt && atoi(clrt))
		return ::clrt(vertices, indices);

	static const char* metis = getenv("METIS");
	METIS = metis ? atoi(metis) : 0;

	if (METIS)
	{
		if (loadMetis())
			printf("using metis for %s\n", (METIS & 3) == 3 ? "clustering and partition" : ((METIS & 1) ? "partition only" : "clustering only"));
		else
			printf("metis library is not available\n"), METIS = 0;
	}

	static const char* dump = getenv("DUMP");

	int depth = 0;
	std::vector<unsigned char> locks(vertices.size());

	// for cluster connectivity, we need a position-only remap that maps vertices with the same position to the same index
	std::vector<unsigned int> remap(vertices.size());
	meshopt_generatePositionRemap(&remap[0], &vertices[0].px, vertices.size(), sizeof(Vertex));

	// set up protect bits on UV seams for permissive mode
	if (kUsePermissiveFallback)
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			unsigned int r = remap[i]; // canonical vertex with the same position

			if (r != i && (vertices[r].tx != vertices[i].tx || vertices[r].ty != vertices[i].ty))
				locks[i] |= 2;
		}

	// initial clusterization splits the original mesh
	std::vector<Cluster> clusters = clusterize(vertices, indices);
	for (size_t i = 0; i < clusters.size(); ++i)
		clusters[i].self = bounds(vertices, clusters[i].indices, 0.f);

	printf("ideal lod chain: %.1f levels\n", log2(double(indices.size() / 3) / double(kClusterSize)));

	std::vector<int> pending(clusters.size());
	for (size_t i = 0; i < clusters.size(); ++i)
		pending[i] = int(i);

	// merge and simplify clusters until we can't merge anymore
	while (pending.size() > 1)
	{
		std::vector<std::vector<int> > groups = partition(clusters, pending, remap, vertices);

		if (kUseLocks)
			lockBoundary(locks, groups, clusters, remap);

		pending.clear();

		std::vector<int> retry;

		size_t triangles = 0;
		size_t stuck_triangles = 0;

		if (dump && depth == atoi(dump))
			dumpObj(vertices, std::vector<unsigned int>());

		// every group needs to be simplified now
		for (size_t i = 0; i < groups.size(); ++i)
		{
			if (groups[i].empty())
				continue; // metis shortcut

			std::vector<unsigned int> merged;
			for (size_t j = 0; j < groups[i].size(); ++j)
				merged.insert(merged.end(), clusters[groups[i][j]].indices.begin(), clusters[groups[i][j]].indices.end());

			if (dump && depth == atoi(dump))
			{
				for (size_t j = 0; j < groups[i].size(); ++j)
					dumpObj("cluster", clusters[groups[i][j]].indices);

				dumpObj("group", merged);
			}

			// aim to reduce group size in half
			size_t target_size = (merged.size() / 3) / 2 * 3;

			float error = 0.f;
			std::vector<unsigned int> simplified = simplify(vertices, merged, kUseLocks ? &locks : NULL, target_size, &error);
			if (simplified.size() > merged.size() * kSimplifyThreshold)
			{
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

			for (size_t j = 0; j < split.size(); ++j)
			{
				split[j].self = groupb;

				clusters.push_back(split[j]); // std::move
				pending.push_back(int(clusters.size()) - 1);

				triangles += split[j].indices.size() / 3;
			}
		}

		dumpMetrics(depth, clusters, groups, remap, locks, retry);
		depth++;

		if (kUseRetry)
		{
			if (triangles < stuck_triangles / 3)
				break;

			pending.insert(pending.end(), retry.begin(), retry.end());
		}
	}

	size_t lowest_triangles = 0;
	for (size_t i = 0; i < clusters.size(); ++i)
		if (clusters[i].parent.error == FLT_MAX)
			lowest_triangles += clusters[i].indices.size() / 3;

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

	printf("cut (%.3f): %d triangles\n", threshold, int(cut.size() / 3));

	if (dump && -1 == atoi(dump))
	{
		dumpObj(vertices, cut);

		for (size_t i = 0; i < clusters.size(); ++i)
			if (boundsError(clusters[i].self, maxx, maxy, maxz, proj, znear) <= threshold && boundsError(clusters[i].parent, maxx, maxy, maxz, proj, znear) > threshold)
				dumpObj("cluster", clusters[i].indices);
	}
}

// What follows is code that optionally uses METIS library to perform partitioning and/or clustering.
// The focus of this example is on combining meshopt_ algorithms, but METIS fallbacks are provided for now.

static bool loadMetis()
{
#ifdef _WIN32
	return false;
#else
	void* handle = dlopen("libmetis.so", RTLD_NOW | RTLD_LOCAL);
	if (!handle)
		return false;

	METIS_SetDefaultOptions = (int (*)(int*))dlsym(handle, "METIS_SetDefaultOptions");
	METIS_PartGraphRecursive = (int (*)(int*, int*, int*, int*, int*, int*, int*, int*, float*, float*, int*, int*, int*))dlsym(handle, "METIS_PartGraphRecursive");

	return METIS_SetDefaultOptions && METIS_PartGraphRecursive;
#endif
}

static std::vector<Cluster> clusterizeMetis(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	std::vector<unsigned int> shadowib(indices.size());
	meshopt_generateShadowIndexBuffer(&shadowib[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(float) * 3, sizeof(Vertex));

	std::vector<std::vector<int> > trilist(vertices.size());

	for (size_t i = 0; i < indices.size(); ++i)
		trilist[shadowib[i]].push_back(int(i / 3));

	std::vector<int> xadj(indices.size() / 3 + 1);
	std::vector<int> adjncy;
	std::vector<int> adjwgt;
	std::vector<int> part(indices.size() / 3);

	std::vector<int> scratch;

	for (size_t i = 0; i < indices.size() / 3; ++i)
	{
		unsigned int a = shadowib[i * 3 + 0], b = shadowib[i * 3 + 1], c = shadowib[i * 3 + 2];

		scratch.clear();
		scratch.insert(scratch.end(), trilist[a].begin(), trilist[a].end());
		scratch.insert(scratch.end(), trilist[b].begin(), trilist[b].end());
		scratch.insert(scratch.end(), trilist[c].begin(), trilist[c].end());
		std::sort(scratch.begin(), scratch.end());

		for (size_t j = 0; j < scratch.size(); ++j)
		{
			if (scratch[j] == int(i))
				continue;

			if (j == 0 || scratch[j] != scratch[j - 1])
			{
				adjncy.push_back(scratch[j]);
				adjwgt.push_back(1);
			}
			else if (j != 0)
			{
				assert(scratch[j] == scratch[j - 1]);
				adjwgt.back()++;
			}
		}

		xadj[i + 1] = int(adjncy.size());
	}

	int options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);
	options[METIS_OPTION_SEED] = 42;
	options[METIS_OPTION_UFACTOR] = 1; // minimize partition imbalance

	// since Metis can't enforce partition sizes, add a little slop to reduce the change we need to split results further
	int nvtxs = int(indices.size() / 3);
	int ncon = 1;
	int nparts = int(indices.size() / 3 + (kClusterSize - kMetisSlop) - 1) / (kClusterSize - kMetisSlop);
	int edgecut = 0;

	// not sure why this is a special case that we need to handle but okay metis
	if (nparts > 1)
	{
		int r = METIS_PartGraphRecursive(&nvtxs, &ncon, xadj.data(), adjncy.data(), NULL, NULL, adjwgt.data(), &nparts, NULL, NULL, options, &edgecut, part.data());
		assert(r == METIS_OK);
		(void)r;
	}

	std::vector<Cluster> result(nparts);

	for (size_t i = 0; i < indices.size() / 3; ++i)
	{
		result[part[i]].indices.push_back(indices[i * 3 + 0]);
		result[part[i]].indices.push_back(indices[i * 3 + 1]);
		result[part[i]].indices.push_back(indices[i * 3 + 2]);
	}

	for (int i = 0; i < nparts; ++i)
	{
		result[i].parent.error = FLT_MAX;

		// need to split the cluster further...
		// this could use meshopt but we're trying to get a complete baseline from metis
		if (result[i].indices.size() > kClusterSize * 3)
		{
			std::vector<Cluster> splits = clusterizeMetis(vertices, result[i].indices);
			assert(splits.size() > 1);

			result[i] = splits[0];
			for (size_t j = 1; j < splits.size(); ++j)
				result.push_back(splits[j]);
		}
	}

	return result;
}

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

	std::vector<std::vector<std::pair<int, int> > > neighbors(pending.size());

	for (std::map<std::pair<int, int>, int>::iterator it = adjacency.begin(); it != adjacency.end(); ++it)
	{
		neighbors[it->first.first].push_back(std::make_pair(it->first.second, it->second));
		neighbors[it->first.second].push_back(std::make_pair(it->first.first, it->second));
	}

	std::vector<int> xadj(pending.size() + 1);
	std::vector<int> adjncy;
	std::vector<int> adjwgt;
	std::vector<int> part(pending.size());

	for (size_t i = 0; i < pending.size(); ++i)
	{
		for (size_t j = 0; j < neighbors[i].size(); ++j)
		{
			adjncy.push_back(neighbors[i][j].first);
			adjwgt.push_back(neighbors[i][j].second);
		}

		xadj[i + 1] = int(adjncy.size());
	}

	int options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);
	options[METIS_OPTION_SEED] = 42;
	options[METIS_OPTION_UFACTOR] = 100;

	int nvtxs = int(pending.size());
	int ncon = 1;
	int nparts = int(pending.size() + kGroupSize - 1) / kGroupSize;
	int edgecut = 0;

	// not sure why this is a special case that we need to handle but okay metis
	if (nparts > 1)
	{
		int r = METIS_PartGraphRecursive(&nvtxs, &ncon, &xadj[0], &adjncy[0], NULL, NULL, &adjwgt[0], &nparts, NULL, NULL, options, &edgecut, &part[0]);
		assert(r == METIS_OK);
		(void)r;
	}

	result.resize(nparts);
	for (size_t i = 0; i < part.size(); ++i)
		result[part[i]].push_back(pending[i]);

	return result;
}

// What follows is code that is helpful for collecting metrics, visualizing cuts, etc.
// This code is not used in the actual clustering implementation and can be ignored.

static int follow(std::vector<int>& parents, int index)
{
	while (index != parents[index])
	{
		int parent = parents[index];
		parents[index] = parents[parent];
		index = parent;
	}

	return index;
}

static int measureComponents(std::vector<int>& parents, const std::vector<unsigned int>& indices, const std::vector<unsigned int>& remap)
{
	assert(parents.size() == remap.size());

	for (size_t i = 0; i < indices.size(); ++i)
	{
		unsigned int v = remap[indices[i]];
		parents[v] = v;
	}

	for (size_t i = 0; i < indices.size(); ++i)
	{
		int v0 = remap[indices[i]];
		int v1 = remap[indices[i + (i % 3 == 2 ? -2 : 1)]];

		v0 = follow(parents, v0);
		v1 = follow(parents, v1);

		parents[v0] = v1;
	}

	for (size_t i = 0; i < indices.size(); ++i)
	{
		unsigned int v = remap[indices[i]];
		parents[v] = follow(parents, v);
	}

	int roots = 0;
	for (size_t i = 0; i < indices.size(); ++i)
	{
		unsigned int v = remap[indices[i]];
		roots += parents[v] == int(v);
		parents[v] = -1; // make sure we only count each root once
	}

	return roots;
}

static int measureUnique(std::vector<int>& used, const std::vector<unsigned int>& indices, const std::vector<unsigned char>* locks = NULL)
{
	for (size_t i = 0; i < indices.size(); ++i)
	{
		unsigned int v = indices[i];
		used[v] = 1;
	}

	size_t vertices = 0;

	for (size_t i = 0; i < indices.size(); ++i)
	{
		unsigned int v = indices[i];
		vertices += used[v] && (!locks || (*locks)[v]);
		used[v] = 0;
	}

	return int(vertices);
}

static void dumpMetrics(int level, const std::vector<Cluster>& queue, const std::vector<std::vector<int> >& groups, const std::vector<unsigned int>& remap, const std::vector<unsigned char>& locks, const std::vector<int>& retry)
{
	std::vector<int> parents(remap.size());

	int clusters = 0;
	int triangles = 0;
	int full_clusters = 0;
	int components = 0;
	int xformed = 0;
	int boundary = 0;
	float radius = 0;

	for (size_t i = 0; i < groups.size(); ++i)
	{
		for (size_t j = 0; j < groups[i].size(); ++j)
		{
			const Cluster& cluster = queue[groups[i][j]];

			clusters++;
			triangles += int(cluster.indices.size() / 3);
			full_clusters += cluster.indices.size() == kClusterSize * 3;
			components += measureComponents(parents, cluster.indices, remap);
			xformed += measureUnique(parents, cluster.indices);
			boundary += kUseLocks ? measureUnique(parents, cluster.indices, &locks) : 0;
			radius += cluster.self.radius;
		}
	}

	int stuck_clusters = 0;
	int stuck_triangles = 0;

	for (size_t i = 0; i < retry.size(); ++i)
	{
		const Cluster& cluster = queue[retry[i]];

		stuck_clusters++;
		stuck_triangles += int(cluster.indices.size() / 3);
	}

	double avg_group = double(clusters) / double(groups.size());
	double inv_clusters = 1.0 / double(clusters);

	printf("lod %d: %d clusters (%.1f%% full, %.1f tri/cl, %.1f vtx/cl, %.2f connected, %.1f boundary, %.1f partition, %f radius), %d triangles",
	    level, clusters,
	    double(full_clusters) * inv_clusters * 100, double(triangles) * inv_clusters, double(xformed) * inv_clusters, double(components) * inv_clusters, double(boundary) * inv_clusters, avg_group, radius * inv_clusters,
	    int(triangles));
	if (stuck_clusters)
		printf("; stuck %d clusters (%d triangles)", stuck_clusters, stuck_triangles);
	printf("\n");
}

// What follows is code for metrics collection of RT impact of clustering on performance; for now this is not integrated with the rest of Nanite example
struct Box
{
	float min[3];
	float max[3];
	float pos[3];
};

struct BoxSort
{
	const Box* boxes;
	int axis;

	bool operator()(unsigned int lhs, unsigned int rhs) const
	{
		return boxes[lhs].pos[axis] < boxes[rhs].pos[axis];
	}
};

static void mergeBox(Box& box, const Box& other)
{
	for (int k = 0; k < 3; ++k)
	{
		box.min[k] = std::min(box.min[k], other.min[k]);
		box.max[k] = std::max(box.max[k], other.max[k]);
	}
}

inline float surface(const Box& box)
{
	float sx = box.max[0] - box.min[0], sy = box.max[1] - box.min[1], sz = box.max[2] - box.min[2];
	return sx * sy + sx * sz + sy * sz;
}

static float sahCost(const Box* boxes, unsigned int* orderx, unsigned int* ordery, unsigned int* orderz, float* scratch, unsigned char* sides, size_t count, int depth)
{
	assert(count > 0);

	if (count == 1)
		return surface(boxes[orderx[0]]);

	// for each axis, accumulated SAH cost in forward and backward directions
	float* costs = scratch;
	Box accum[6] = {boxes[orderx[0]], boxes[orderx[count - 1]], boxes[ordery[0]], boxes[ordery[count - 1]], boxes[orderz[0]], boxes[orderz[count - 1]]};
	unsigned int* axes[3] = {orderx, ordery, orderz};

	for (size_t i = 0; i < count; ++i)
	{
		for (int k = 0; k < 3; ++k)
		{
			mergeBox(accum[2 * k + 0], boxes[axes[k][i]]);
			mergeBox(accum[2 * k + 1], boxes[axes[k][count - 1 - i]]);
		}

		for (int k = 0; k < 3; ++k)
		{
			costs[i + (2 * k + 0) * count] = surface(accum[2 * k + 0]);
			costs[i + (2 * k + 1) * count] = surface(accum[2 * k + 1]);
		}
	}

	// find best split that minimizes SAH
	int bestk = -1;
	size_t bestsplit = 0;
	float bestcost = FLT_MAX;

	for (size_t i = 0; i < count - 1; ++i)
		for (int k = 0; k < 3; ++k)
		{
			// costs[x] = inclusive cost of boxes[0..x]
			float costl = costs[i + (2 * k + 0) * count] * (i + 1);
			// costs[count-1-x] = inclusive cost of boxes[x..count-1]
			float costr = costs[(count - 1 - (i + 1)) + (2 * k + 1) * count] * (count - (i + 1));
			float cost = costl + costr;

			if (cost < bestcost)
			{
				bestcost = cost;
				bestk = k;
				bestsplit = i;
			}
		}

	float total = costs[count - 1];

	// mark sides of split
	for (size_t i = 0; i < bestsplit + 1; ++i)
		sides[axes[bestk][i]] = 0;

	for (size_t i = bestsplit + 1; i < count; ++i)
		sides[axes[bestk][i]] = 1;

	// partition all axes into two sides, maintaining order
	// note: we reuse scratch[], invalidating costs[]
	for (int k = 0; k < 3; ++k)
	{
		if (k == bestk)
			continue;

		unsigned int* temp = reinterpret_cast<unsigned int*>(scratch);
		memcpy(temp, axes[k], sizeof(unsigned int) * count);

		unsigned int* ptr[2] = {axes[k], axes[k] + bestsplit + 1};

		for (size_t i = 0; i < count; ++i)
		{
			unsigned char side = sides[temp[i]];
			*ptr[side] = temp[i];
			ptr[side]++;
		}
	}

	float sahl = sahCost(boxes, orderx, ordery, orderz, scratch, sides, bestsplit + 1, depth + 1);
	float sahr = sahCost(boxes, orderx + bestsplit + 1, ordery + bestsplit + 1, orderz + bestsplit + 1, scratch, sides, count - bestsplit - 1, depth + 1);

	return total + sahl + sahr;
}

static float sahCost(const Box* boxes, size_t count)
{
	std::vector<unsigned int> axes(count * 3);

	for (int k = 0; k < 3; ++k)
	{
		for (size_t i = 0; i < count; ++i)
			axes[i + k * count] = unsigned(i);

		BoxSort sort = {boxes, k};
		std::sort(&axes[k * count], &axes[k * count] + count, sort);
	}

	std::vector<float> scratch(count * 6);
	std::vector<unsigned char> sides(count);

	return sahCost(boxes, &axes[0], &axes[count], &axes[count * 2], &scratch[0], &sides[0], count, 0);
}

static void expandBox(Box& box, float x, float y, float z)
{
	box.min[0] = std::min(box.min[0], x);
	box.min[1] = std::min(box.min[1], y);
	box.min[2] = std::min(box.min[2], z);

	box.max[0] = std::max(box.max[0], x);
	box.max[1] = std::max(box.max[1], y);
	box.max[2] = std::max(box.max[2], z);

	box.pos[0] += x;
	box.pos[1] += y;
	box.pos[2] += z;
}

double timestamp();

void clrt(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	std::vector<Box> triangles(indices.size() / 3);

	for (size_t i = 0; i < indices.size() / 3; ++i)
	{
		Box& box = triangles[i];

		box.min[0] = box.min[1] = box.min[2] = FLT_MAX;
		box.max[0] = box.max[1] = box.max[2] = -FLT_MAX;
		box.pos[0] = box.pos[1] = box.pos[2] = 0.f;

		for (int j = 0; j < 3; ++j)
		{
			const Vertex& vertex = vertices[indices[i * 3 + j]];

			expandBox(box, vertex.px, vertex.py, vertex.pz);
		}

		for (int k = 0; k < 3; ++k)
			box.pos[k] /= 3.f;
	}

	Box all = triangles[0];
	for (size_t i = 1; i < triangles.size(); ++i)
		mergeBox(all, triangles[i]);

	double start = timestamp();

	float sahr = surface(all);
	float saht = sahCost(&triangles[0], triangles.size());

	double middle = timestamp();

	const bool use_spatial = true;
	const size_t max_vertices = 64;
	const size_t min_triangles = 16;
	const size_t max_triangles = 64;
	const float cone_weight = -0.25f;
	const float split_factor = 2.0f;
	const float fill_weight = 0.5f;

	size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, min_triangles);

	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	if (use_spatial)
		meshlets.resize(meshopt_buildMeshletsSpatial(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), max_vertices, min_triangles, max_triangles, fill_weight));
	else
		meshlets.resize(meshopt_buildMeshletsFlex(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), max_vertices, min_triangles, max_triangles, cone_weight, split_factor));

	double end = timestamp();

	std::vector<Box> meshlet_boxes(meshlets.size());
	std::vector<Box> cluster_tris(max_triangles);

	float sahc = 0.f;
	size_t xformed = 0;

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& meshlet = meshlets[i];

		{
			Box& box = meshlet_boxes[i];

			box.min[0] = box.min[1] = box.min[2] = FLT_MAX;
			box.max[0] = box.max[1] = box.max[2] = -FLT_MAX;
			box.pos[0] = box.pos[1] = box.pos[2] = 0.f;

			for (size_t j = 0; j < meshlet.vertex_count; ++j)
			{
				const Vertex& vertex = vertices[meshlet_vertices[meshlet.vertex_offset + j]];

				expandBox(box, vertex.px, vertex.py, vertex.pz);
			}

			for (int k = 0; k < 3; ++k)
				box.pos[k] = (box.max[k] + box.min[k]) * 0.5f;
		}

		for (size_t j = 0; j < meshlet.triangle_count; ++j)
		{
			Box& box = cluster_tris[j];

			box.min[0] = box.min[1] = box.min[2] = FLT_MAX;
			box.max[0] = box.max[1] = box.max[2] = -FLT_MAX;
			box.pos[0] = box.pos[1] = box.pos[2] = 0.f;

			for (int k = 0; k < 3; ++k)
			{
				const Vertex& vertex = vertices[meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + j * 3 + k]]];

				expandBox(box, vertex.px, vertex.py, vertex.pz);
			}

			for (int k = 0; k < 3; ++k)
				box.pos[k] /= 3.f;
		}

		sahc += sahCost(&cluster_tris[0], meshlet.triangle_count);
		sahc -= surface(meshlet_boxes[i]); // box will be accounted for in tlas

		xformed += meshlet.vertex_count;
	}

	sahc += sahCost(&meshlet_boxes[0], meshlet_boxes.size());

	printf("BLAS SAH %f\n", saht / sahr);
	printf("CLAS SAH %f\n", sahc / sahr);
	printf("%d clusters, %.1f tri/cl, %.1f vtx/cl, SAH overhead %f\n", int(meshlet_boxes.size()), double(indices.size() / 3) / double(meshlet_boxes.size()), double(xformed) / double(meshlet_boxes.size()), sahc / saht);
	printf("BVH build %.1f ms, clusterize %.1f ms\n", (middle - start) * 1000.0, (end - middle) * 1000.0);
}
