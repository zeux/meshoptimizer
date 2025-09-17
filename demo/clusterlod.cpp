// For reference, see the original Nanite paper:
// Brian Karis. Nanite: A Deep Dive. 2021
#include "clusterlod.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <vector>

#include "../src/meshoptimizer.h"

#ifndef TRACE
#define TRACE 0
#endif

struct Cluster
{
	std::vector<unsigned int> indices;

	int group;
	clodBounds bounds;
};

static clodBounds bounds(const clodMesh& mesh, const std::vector<unsigned int>& indices, float error)
{
	meshopt_Bounds bounds = meshopt_computeClusterBounds(&indices[0], indices.size(), mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride);

	clodBounds result;
	result.center[0] = bounds.center[0];
	result.center[1] = bounds.center[1];
	result.center[2] = bounds.center[2];
	result.radius = bounds.radius;
	result.error = error;
	return result;
}

static clodBounds boundsMerge(const std::vector<Cluster>& clusters, const std::vector<int>& group)
{
	std::vector<clodBounds> bounds(group.size());
	for (size_t j = 0; j < group.size(); ++j)
		bounds[j] = clusters[group[j]].bounds;

	meshopt_Bounds merged = meshopt_computeSphereBounds(&bounds[0].center[0], bounds.size(), sizeof(clodBounds), &bounds[0].radius, sizeof(clodBounds));

	clodBounds result = {};
	result.center[0] = merged.center[0];
	result.center[1] = merged.center[1];
	result.center[2] = merged.center[2];
	result.radius = merged.radius;

	// merged bounds error must be conservative wrt cluster errors
	result.error = 0.f;
	for (size_t j = 0; j < group.size(); ++j)
		result.error = std::max(result.error, clusters[group[j]].bounds.error);

	return result;
}

static std::vector<Cluster> clusterize(const clodConfig& config, const clodMesh& mesh, const unsigned int* indices, size_t index_count)
{
	size_t max_meshlets = meshopt_buildMeshletsBound(index_count, config.max_vertices, config.min_triangles);

	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(index_count);
	std::vector<unsigned char> meshlet_triangles(index_count);

	if (config.cluster_spatial)
		meshlets.resize(meshopt_buildMeshletsSpatial(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], indices, index_count,
		    mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride,
		    config.max_vertices, config.min_triangles, config.max_triangles, config.cluster_fill_weight));
	else
		meshlets.resize(meshopt_buildMeshletsFlex(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], indices, index_count,
		    mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride,
		    config.max_vertices, config.min_triangles, config.max_triangles, 0.f, config.cluster_split_factor));

	std::vector<Cluster> clusters(meshlets.size());

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& meshlet = meshlets[i];

		if (config.optimize_raster)
			meshopt_optimizeMeshlet(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset], meshlet.triangle_count, meshlet.vertex_count);

		// note: for now we discard meshlet-local indices; they are valuable for shader code so in the future we should bring them back
		clusters[i].indices.resize(meshlet.triangle_count * 3);
		for (size_t j = 0; j < meshlet.triangle_count * 3; ++j)
			clusters[i].indices[j] = meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + j]];

		clusters[i].group = -1;
		clusters[i].bounds = bounds(mesh, clusters[i].indices, 0.f);
	}

	return clusters;
}

static std::vector<std::vector<int> > partition(size_t partition_size, const clodMesh& mesh, const std::vector<Cluster>& clusters, const std::vector<int>& pending, const std::vector<unsigned int>& remap)
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
	size_t partition_count = meshopt_partitionClusters(&cluster_part[0], &cluster_indices[0], cluster_indices.size(), &cluster_counts[0], cluster_counts.size(), mesh.vertex_positions, remap.size(), mesh.vertex_positions_stride, partition_size);

	std::vector<std::vector<int> > partitions(partition_count);
	for (size_t i = 0; i < partition_count; ++i)
		partitions[i].reserve(partition_size + 4);

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

struct SloppyVertex
{
	float x, y, z;
	unsigned int id;
};

static void simplifyFallback(std::vector<unsigned int>& lod, const clodMesh& mesh, const std::vector<unsigned int>& indices, const std::vector<unsigned char>& locks, size_t target_count, float* error)
{
	std::vector<SloppyVertex> subset(indices.size());
	std::vector<unsigned char> subset_locks(indices.size());

	lod.resize(indices.size());

	size_t positions_stride = mesh.vertex_positions_stride / sizeof(float);

	// deindex the mesh subset to avoid calling simplifySloppy on the entire vertex buffer (which is prohibitively expensive without sparsity)
	for (size_t i = 0; i < indices.size(); ++i)
	{
		unsigned int v = indices[i];
		assert(v < mesh.vertex_count);

		subset[i].x = mesh.vertex_positions[v * positions_stride + 0];
		subset[i].y = mesh.vertex_positions[v * positions_stride + 1];
		subset[i].z = mesh.vertex_positions[v * positions_stride + 2];
		subset[i].id = v;

		subset_locks[i] = locks[v];
		lod[i] = unsigned(i);
	}

	lod.resize(meshopt_simplifySloppy(&lod[0], &lod[0], lod.size(), &subset[0].x, subset.size(), sizeof(SloppyVertex), subset_locks.data(), target_count, FLT_MAX, error));

	// convert error to absolute
	*error *= meshopt_simplifyScale(&subset[0].x, subset.size(), sizeof(SloppyVertex));

	// restore original vertex indices
	for (size_t i = 0; i < lod.size(); ++i)
		lod[i] = subset[lod[i]].id;
}

static std::vector<unsigned int> simplify(const clodConfig& config, const clodMesh& mesh, const std::vector<unsigned int>& indices, const std::vector<unsigned char>& locks, size_t target_count, float* error)
{
	if (target_count > indices.size())
		return indices;

	std::vector<unsigned int> lod(indices.size());

	unsigned int options = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute | (config.simplify_permissive ? meshopt_SimplifyPermissive : 0);

	lod.resize(meshopt_simplifyWithAttributes(&lod[0], &indices[0], indices.size(),
	    mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride,
	    mesh.vertex_attributes, mesh.vertex_attributes_stride, mesh.attribute_weights, mesh.attribute_count,
	    &locks[0], target_count, FLT_MAX, options, error));

	if (lod.size() > target_count && config.simplify_fallback_permissive && !config.simplify_permissive)
		lod.resize(meshopt_simplifyWithAttributes(&lod[0], &indices[0], indices.size(),
		    mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride,
		    mesh.vertex_attributes, mesh.vertex_attributes_stride, mesh.attribute_weights, mesh.attribute_count,
		    &locks[0], target_count, FLT_MAX, options | meshopt_SimplifyPermissive, error));

	// while it's possible to call simplifySloppy directly, it doesn't support sparsity or absolute error, so we need to do some extra work
	if (lod.size() > target_count && config.simplify_fallback_sloppy)
	{
		simplifyFallback(lod, mesh, indices, locks, target_count, error);
		*error *= config.simplify_sloppy_factor; // scale error up to account for appearance degradation
	}

	return lod;
}

#if TRACE
static void dumpMetrics(const clodConfig& config, int level, const std::vector<Cluster>& queue, const std::vector<std::vector<int> >& groups, const std::vector<unsigned int>& remap, const std::vector<unsigned char>& locks, const std::vector<int>& retry);
#endif

clodConfig clodDefaultConfig(size_t max_triangles)
{
	assert(max_triangles >= 4 && max_triangles <= 256);

	clodConfig config = {};
	config.max_vertices = max_triangles;
	config.min_triangles = max_triangles / 3;
	config.max_triangles = max_triangles;

	config.partition_size = 16;

	config.cluster_spatial = false;
	config.cluster_split_factor = 2.0f;

	config.optimize_raster = true;

	config.simplify_ratio = 0.5f;
	config.simplify_threshold = 0.85f;
	config.simplify_sloppy_factor = 2.0f;
	config.simplify_permissive = true;
	config.simplify_fallback_permissive = false; // note: by default we run in permissive mode, but it's also possible to disable that and use it only as a fallback
	config.simplify_fallback_sloppy = true;

	return config;
}

clodConfig clodDefaultConfigRT(size_t max_triangles)
{
	clodConfig config = clodDefaultConfig(max_triangles);

	config.max_vertices = std::max(size_t(256), max_triangles + max_triangles / 2);

	config.cluster_spatial = true;
	config.cluster_fill_weight = 0.5f;

	config.optimize_raster = false;

	return config;
}

size_t clodBuild(clodConfig config, clodMesh mesh, void* output_context, clodOutputCluster output_cluster, clodOutputGroup output_group)
{
	assert(mesh.vertex_attributes_stride % sizeof(float) == 0);
	assert(mesh.attribute_count * sizeof(float) <= mesh.vertex_attributes_stride);
	assert(mesh.attribute_protect_mask < (1u << (mesh.vertex_attributes_stride / sizeof(float))));

	std::vector<unsigned char> locks(mesh.vertex_count);

	// for cluster connectivity, we need a position-only remap that maps vertices with the same position to the same index
	std::vector<unsigned int> remap(mesh.vertex_count);
	meshopt_generatePositionRemap(&remap[0], mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride);

	// set up protect bits on UV seams for permissive mode
	if (mesh.attribute_protect_mask)
	{
		size_t max_attributes = mesh.vertex_attributes_stride / sizeof(float);

		for (size_t i = 0; i < mesh.vertex_count; ++i)
		{
			unsigned int r = remap[i]; // canonical vertex with the same position

			for (size_t j = 0; j < max_attributes; ++j)
				if (r != i && (mesh.attribute_protect_mask & (1u << j)) && mesh.vertex_attributes[i * max_attributes + j] != mesh.vertex_attributes[r * max_attributes + j])
					locks[i] |= meshopt_SimplifyVertex_Protect;
		}
	}

	// initial clusterization splits the original mesh
	std::vector<Cluster> clusters = clusterize(config, mesh, mesh.indices, mesh.index_count);

	if (output_cluster)
		for (size_t i = 0; i < clusters.size(); ++i)
			output_cluster(output_context, {0, -1, clusters[i].bounds, clusters[i].indices.data(), clusters[i].indices.size()});

	std::vector<int> pending(clusters.size());
	for (size_t i = 0; i < clusters.size(); ++i)
		pending[i] = int(i);

	int depth = 0;
	int next_group = 0;

	// merge and simplify clusters until we can't merge anymore
	while (pending.size() > 1)
	{
		std::vector<std::vector<int> > groups = partition(config.partition_size, mesh, clusters, pending, remap);

		lockBoundary(locks, groups, clusters, remap);

		pending.clear();

		std::vector<int> retry;

		size_t triangles = 0;
		size_t stuck_triangles = 0;

		// every group needs to be simplified now
		for (size_t i = 0; i < groups.size(); ++i)
		{
			std::vector<unsigned int> merged;
			merged.reserve(config.max_triangles * groups[i].size());
			for (size_t j = 0; j < groups[i].size(); ++j)
				merged.insert(merged.end(), clusters[groups[i][j]].indices.begin(), clusters[groups[i][j]].indices.end());

			size_t target_size = size_t((merged.size() / 3) * config.simplify_ratio) * 3;

			float error = 0.f;
			std::vector<unsigned int> simplified = simplify(config, mesh, merged, locks, target_size, &error);
			if (simplified.size() > merged.size() * config.simplify_threshold)
			{
				stuck_triangles += merged.size() / 3;
				for (size_t j = 0; j < groups[i].size(); ++j)
					retry.push_back(groups[i][j]);
				continue; // simplification is stuck; abandon the merge
			}

			// enforce bounds and error monotonicity
			// note: it is incorrect to use the precise bounds of the merged or simplified mesh, because this may violate monotonicity
			clodBounds groupb = boundsMerge(clusters, groups[i]);
			groupb.error += error; // this may overestimate the error, but we are starting from the simplified mesh so this is a little more correct

			int group_id = next_group++;

			if (output_group)
				output_group(output_context, {groupb, &groups[i][0], groups[i].size()});

			// update parent bounds and error for all clusters in the group
			// note that all clusters in the group need to switch simultaneously so they have the same bounds
			for (size_t j = 0; j < groups[i].size(); ++j)
			{
				assert(clusters[groups[i][j]].group == -1);
				clusters[groups[i][j]].group = group_id;
				clusters[groups[i][j]].bounds = groupb;
			}

			std::vector<Cluster> split = clusterize(config, mesh, simplified.data(), simplified.size());

			for (size_t j = 0; j < split.size(); ++j)
			{
				if (output_cluster)
					output_cluster(output_context, {depth + 1, group_id, split[j].bounds, &split[j].indices[0], split[j].indices.size()});

				// update cluster bounds to the group-merged bounds; this ensures that we compute the group bounds for whatever group this cluster will be part of conservatively
				split[j].bounds = groupb;

				triangles += split[j].indices.size() / 3;
				clusters.push_back(std::move(split[j]));
				pending.push_back(int(clusters.size()) - 1);
			}
		}

#if TRACE
		dumpMetrics(config, depth, clusters, groups, remap, locks, retry);
#endif

		depth++;

		if (config.retry_queue)
		{
			if (triangles < stuck_triangles / 3)
				break;

			pending.insert(pending.end(), retry.begin(), retry.end());
		}
	}

	size_t lowest_triangles = 0;
	for (size_t i = 0; i < clusters.size(); ++i)
		if (clusters[i].group == -1)
			lowest_triangles += clusters[i].indices.size() / 3;

#if TRACE
	printf("lod %d: (lowest) %d triangles\n", int(depth), int(lowest_triangles));
#endif

	return lowest_triangles;
}

#if TRACE
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

static void dumpMetrics(const clodConfig& config, int level, const std::vector<Cluster>& queue, const std::vector<std::vector<int> >& groups, const std::vector<unsigned int>& remap, const std::vector<unsigned char>& locks, const std::vector<int>& retry)
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
			full_clusters += cluster.indices.size() == config.max_triangles * 3;
			components += measureComponents(parents, cluster.indices, remap);
			xformed += measureUnique(parents, cluster.indices);
			boundary += measureUnique(parents, cluster.indices, &locks);
			radius += cluster.bounds.radius;
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
#endif
