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
#include <vector>

struct Vertex
{
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;
};

// To compute approximate (perspective) projection error of a cluster in screen space (0..1; multiply by screen height to get pixels):
// - camera_proj is projection[1][1], or cot(fovy/2); camera_znear is *positive* near plane distance
// - for simplicity, we ignore perspective distortion and use rotationally invariant projection size estimation
// - return: bounds.error / max(distance(bounds.center, camera_position) - bounds.radius, camera_znear) * (camera_proj * 0.5f)
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

static std::vector<Cluster> clusterize(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	const size_t max_vertices = 192; // TODO: depends on kClusterSize, also may want to dial down for mesh shaders
	const size_t max_triangles = kClusterSize;
	const size_t min_triangles = kClusterSize / 3;
	const float split_factor = 2.0f;
	const float fill_weight = 0.75f;

	size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, min_triangles);

	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(indices.size());
	std::vector<unsigned char> meshlet_triangles(indices.size());

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
		locks[i] = (groupmap[r] == -2) | (locks[i] & meshopt_SimplifyVertex_Protect);
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

void dumpObj(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, bool recomputeNormals = false);
void dumpObj(const char* section, const std::vector<unsigned int>& indices);

void clod(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
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
				locks[i] |= meshopt_SimplifyVertex_Protect;
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
