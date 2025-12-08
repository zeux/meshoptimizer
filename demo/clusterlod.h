/**
 * clusterlod - a small "library"/example built on top of meshoptimizer to generate cluster LOD hierarchies
 * This is intended to either be used as is, or as a reference for implementing similar functionality in your engine.
 *
 * To use this code, you need to have one source file which includes meshoptimizer.h and defines CLUSTERLOD_IMPLEMENTATION
 * before including this file. Other source files in your project can just include this file and use the provided functions.
 *
 * Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * This code is distributed under the MIT License. See notice at the end of this file.
 */
#pragma once

#include <stddef.h>

struct clodConfig
{
	// configuration of each cluster; maps to meshopt_buildMeshlets* parameters
	size_t max_vertices;
	size_t min_triangles;
	size_t max_triangles;

	// partitioning setup; maps to meshopt_partitionClusters parameters (plus optional partition sorting)
	// note: partition size is the target size, not maximum; actual partitions may be up to 1/3 larger (e.g. target 24 results in maximum 32)
	bool partition_spatial;
	bool partition_sort;
	size_t partition_size;

	// clusterization setup; maps to meshopt_buildMeshletsSpatial / meshopt_buildMeshletsFlex
	bool cluster_spatial;
	float cluster_fill_weight;
	float cluster_split_factor;

	// every level aims to reduce the number of triangles by ratio, and considers clusters that don't reach the threshold stuck
	float simplify_ratio;
	float simplify_threshold;

	// to compute the error of simplified clusters, we use the formula that combines previous accumulated error as follows:
	// max(previous_error * simplify_error_merge_previous, current_error) + current_error * simplify_error_merge_additive
	float simplify_error_merge_previous;
	float simplify_error_merge_additive;

	// amplify the error of clusters that go through sloppy simplification to account for appearance degradation
	float simplify_error_factor_sloppy;

	// experimental: limit error by edge length, aiming to remove subpixel triangles even if the attribute error is high
	float simplify_error_edge_limit;

	// use permissive simplification instead of regular simplification (make sure to use attribute_protect_mask if this is set!)
	bool simplify_permissive;

	// use permissive or sloppy simplification but only if regular simplification gets stuck
	bool simplify_fallback_permissive;
	bool simplify_fallback_sloppy;

	// use regularization during simplification to make triangle density more uniform, at some cost to overall triangle count; recommended for deformable objects
	bool simplify_regularize;

	// should clodCluster::bounds be computed based on the geometry of each cluster
	bool optimize_bounds;

	// should clodCluster::indices be optimized for locality; helps with rasterization performance and ray tracing performance in fast-build modes
	bool optimize_clusters;
};

struct clodMesh
{
	// input triangle indices
	const unsigned int* indices;
	size_t index_count;

	// total vertex count
	size_t vertex_count;

	// input vertex positions; must be 3 floats per vertex
	const float* vertex_positions;
	size_t vertex_positions_stride;

	// input vertex attributes; used for attribute-aware simplification and permissive simplification
	const float* vertex_attributes;
	size_t vertex_attributes_stride;

	// input vertex locks; allows to preserve additional seams (when not using attribute_protect_mask) or lock vertices via meshopt_SimplifyVertex_* flags
	const unsigned char* vertex_lock;

	// attribute weights for attribute-aware simplification; maps to meshopt_simplifyWithAttributes parameters
	const float* attribute_weights;
	size_t attribute_count;

	// attribute mask to flag attribute discontinuities for permissive simplification; mask (1<<K) corresponds to attribute K
	unsigned int attribute_protect_mask;
};

// To compute approximate (perspective) projection error of a cluster in screen space (0..1; multiply by screen height to get pixels):
// - camera_proj is projection[1][1], or cot(fovy/2); camera_znear is *positive* near plane distance
// - for simplicity, we ignore perspective distortion and use rotationally invariant projection size estimation
// - return: bounds.error / max(distance(bounds.center, camera_position) - bounds.radius, camera_znear) * (camera_proj * 0.5f)
struct clodBounds
{
	// sphere bounds, in mesh coordinate space
	float center[3];
	float radius;

	// combined simplification error, in mesh coordinate space
	float error;
};

struct clodCluster
{
	// index of more refined group (with more triangles) that produced this cluster during simplification, or -1 for original geometry
	int refined;

	// cluster bounds; should only be used for culling, as bounds.error is not monotonic across DAG
	clodBounds bounds;

	// cluster indices; refer to the original mesh vertex buffer
	const unsigned int* indices;
	size_t index_count;

	// cluster vertex count; indices[] has vertex_count unique entries
	size_t vertex_count;
};

struct clodGroup
{
	// DAG level the group was generated at
	int depth;

	// simplified group bounds (reflects error for clusters with clodCluster::refined == group id; error is FLT_MAX for terminal groups)
	// cluster should be rendered if:
	// 1. clodGroup::simplified for the group it's in is over error threshold
	// 2. cluster.refined is -1 *or* clodGroup::simplified for groups[cluster.refined].simplified is at or under error threshold
	clodBounds simplified;
};

// gets called for each group in sequence
// returned value gets saved for clusters emitted from this group (clodCluster::refined)
typedef int (*clodOutput)(void* output_context, clodGroup group, const clodCluster* clusters, size_t cluster_count);

#ifdef __cplusplus
extern "C"
{
#endif

// default configuration optimized for rasterization / raytracing
clodConfig clodDefaultConfig(size_t max_triangles);
clodConfig clodDefaultConfigRT(size_t max_triangles);

// build cluster LOD hierarchy, calling output callbacks as new clusters and groups are generated
// returns the total number of clusters produced
size_t clodBuild(clodConfig config, clodMesh mesh, void* output_context, clodOutput output_callback);

// extract meshlet-local indices from cluster indices produced by clodBuild
// fills triangles[] and vertices[] such that vertices[triangles[i]] == indices[i]
// returns number of unique vertices (which will be equal to clodCluster::vertex_count)
size_t clodLocalIndices(unsigned int* vertices, unsigned char* triangles, const unsigned int* indices, size_t index_count);

#ifdef __cplusplus
} // extern "C"

template <typename Output>
size_t clodBuild(clodConfig config, clodMesh mesh, Output output)
{
	struct Call
	{
		static int output(void* output_context, clodGroup group, const clodCluster* clusters, size_t cluster_count)
		{
			return (*static_cast<Output*>(output_context))(group, clusters, cluster_count);
		}
	};

	return clodBuild(config, mesh, &output, &Call::output);
}
#endif

#ifdef CLUSTERLOD_IMPLEMENTATION
// For reference, see the original Nanite paper:
// Brian Karis. Nanite: A Deep Dive. 2021
#include <float.h>
#include <math.h>
#include <string.h>

#include <algorithm>
#include <vector>

namespace clod
{

struct Cluster
{
	size_t vertices;
	std::vector<unsigned int> indices;

	int group;
	int refined;

	clodBounds bounds;
};

static clodBounds boundsCompute(const clodMesh& mesh, const std::vector<unsigned int>& indices, float error)
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

#if MESHOPTIMIZER_VERSION < 1000
	std::vector<unsigned char> meshlet_triangles(index_count + max_meshlets * 3); // account for 4b alignment
#else
	std::vector<unsigned char> meshlet_triangles(index_count);
#endif

	if (config.cluster_spatial)
		meshlets.resize(meshopt_buildMeshletsSpatial(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices, index_count,
		    mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride,
		    config.max_vertices, config.min_triangles, config.max_triangles, config.cluster_fill_weight));
	else
		meshlets.resize(meshopt_buildMeshletsFlex(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices, index_count,
		    mesh.vertex_positions, mesh.vertex_count, mesh.vertex_positions_stride,
		    config.max_vertices, config.min_triangles, config.max_triangles, 0.f, config.cluster_split_factor));

	std::vector<Cluster> clusters(meshlets.size());

	for (size_t i = 0; i < meshlets.size(); ++i)
	{
		const meshopt_Meshlet& meshlet = meshlets[i];

		if (config.optimize_clusters)
			meshopt_optimizeMeshlet(&meshlet_vertices[meshlet.vertex_offset], &meshlet_triangles[meshlet.triangle_offset], meshlet.triangle_count, meshlet.vertex_count);

		clusters[i].vertices = meshlet.vertex_count;

		// note: we discard meshlet-local indices; they can be recovered by the caller using clodLocalIndices
		clusters[i].indices.resize(meshlet.triangle_count * 3);
		for (size_t j = 0; j < meshlet.triangle_count * 3; ++j)
			clusters[i].indices[j] = meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + j]];

		clusters[i].group = -1;
		clusters[i].refined = -1;
	}

	return clusters;
}

static std::vector<std::vector<int> > partition(const clodConfig& config, const clodMesh& mesh, const std::vector<Cluster>& clusters, const std::vector<int>& pending, const std::vector<unsigned int>& remap)
{
	if (pending.size() <= config.partition_size)
		return {pending};

	std::vector<unsigned int> cluster_indices;
	std::vector<unsigned int> cluster_counts(pending.size());

	// copy cluster index data into a flat array for partitioning
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

	// partition clusters into groups; the output is a partition id per cluster
	std::vector<unsigned int> cluster_part(pending.size());
	size_t partition_count = meshopt_partitionClusters(&cluster_part[0], &cluster_indices[0], cluster_indices.size(), &cluster_counts[0], cluster_counts.size(),
	    config.partition_spatial ? mesh.vertex_positions : NULL, remap.size(), mesh.vertex_positions_stride, config.partition_size);

	// preallocate partitions for worst case
	std::vector<std::vector<int> > partitions(partition_count);
	for (size_t i = 0; i < partition_count; ++i)
		partitions[i].reserve(config.partition_size + config.partition_size / 3);

	std::vector<unsigned int> partition_remap;

	if (config.partition_sort)
	{
		// compute partition points for sorting; any representative point will do, we use last cluster center for simplicity
		std::vector<float> partition_point(partition_count * 3);
		for (size_t i = 0; i < pending.size(); ++i)
			memcpy(&partition_point[cluster_part[i] * 3], clusters[pending[i]].bounds.center, sizeof(float) * 3);

		// sort partitions spatially; the output is a remap table from old index (partition id) to new index
		partition_remap.resize(partition_count);
		meshopt_spatialSortRemap(partition_remap.data(), partition_point.data(), partition_count, sizeof(float) * 3);
	}

	// distribute clusters into partitions, applying spatial order if requested
	for (size_t i = 0; i < pending.size(); ++i)
		partitions[partition_remap.empty() ? cluster_part[i] : partition_remap[cluster_part[i]]].push_back(pending[i]);

	return partitions;
}

static void lockBoundary(std::vector<unsigned char>& locks, const std::vector<std::vector<int> >& groups, const std::vector<Cluster>& clusters, const std::vector<unsigned int>& remap, const unsigned char* vertex_lock)
{
	// for each remapped vertex, use bit 7 as temporary storage to indicate that the vertex has been used by a different group previously
	for (size_t i = 0; i < locks.size(); ++i)
		locks[i] &= ~((1 << 0) | (1 << 7));

	for (size_t i = 0; i < groups.size(); ++i)
	{
		// mark all remapped vertices as locked if seen by a prior group
		for (size_t j = 0; j < groups[i].size(); ++j)
		{
			const Cluster& cluster = clusters[groups[i][j]];

			for (size_t k = 0; k < cluster.indices.size(); ++k)
			{
				unsigned int v = cluster.indices[k];
				unsigned int r = remap[v];

				locks[r] |= locks[r] >> 7;
			}
		}

		// mark all remapped vertices as seen
		for (size_t j = 0; j < groups[i].size(); ++j)
		{
			const Cluster& cluster = clusters[groups[i][j]];

			for (size_t k = 0; k < cluster.indices.size(); ++k)
			{
				unsigned int v = cluster.indices[k];
				unsigned int r = remap[v];

				locks[r] |= 1 << 7;
			}
		}
	}

	for (size_t i = 0; i < locks.size(); ++i)
	{
		unsigned int r = remap[i];

		// consistently lock all vertices with the same position; keep protect bit if set
		locks[i] = (locks[r] & 1) | (locks[i] & meshopt_SimplifyVertex_Protect);

		if (vertex_lock)
			locks[i] |= vertex_lock[i];
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

	unsigned int options = meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute | (config.simplify_permissive ? meshopt_SimplifyPermissive : 0) | (config.simplify_regularize ? meshopt_SimplifyRegularize : 0);

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
		*error *= config.simplify_error_factor_sloppy; // scale error up to account for appearance degradation
	}

	// optionally limit error by edge length, aiming to remove subpixel triangles even if the attribute error is high
	if (config.simplify_error_edge_limit > 0)
	{
		float max_edge_sq = 0;

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
			assert(a < mesh.vertex_count && b < mesh.vertex_count && c < mesh.vertex_count);

			const float* va = &mesh.vertex_positions[a * (mesh.vertex_positions_stride / sizeof(float))];
			const float* vb = &mesh.vertex_positions[b * (mesh.vertex_positions_stride / sizeof(float))];
			const float* vc = &mesh.vertex_positions[c * (mesh.vertex_positions_stride / sizeof(float))];

			// compute squared edge lengths
			float eab = (va[0] - vb[0]) * (va[0] - vb[0]) + (va[1] - vb[1]) * (va[1] - vb[1]) + (va[2] - vb[2]) * (va[2] - vb[2]);
			float eac = (va[0] - vc[0]) * (va[0] - vc[0]) + (va[1] - vc[1]) * (va[1] - vc[1]) + (va[2] - vc[2]) * (va[2] - vc[2]);
			float ebc = (vb[0] - vc[0]) * (vb[0] - vc[0]) + (vb[1] - vc[1]) * (vb[1] - vc[1]) + (vb[2] - vc[2]) * (vb[2] - vc[2]);

			float emax = std::max(std::max(eab, eac), ebc);
			float emin = std::min(std::min(eab, eac), ebc);

			// we prefer using min edge length to reduce the number of triangles <1px thick, but need some stopgap for thin and long triangles like wires
			max_edge_sq = std::max(max_edge_sq, std::max(emin, emax / 4));
		}

		// adjust the error to limit it for dense clusters based on edge lengths
		*error = std::min(*error, sqrtf(max_edge_sq) * config.simplify_error_edge_limit);
	}

	return lod;
}

static int outputGroup(const clodConfig& config, const clodMesh& mesh, const std::vector<Cluster>& clusters, const std::vector<int>& group, const clodBounds& simplified, int depth, void* output_context, clodOutput output_callback)
{
	std::vector<clodCluster> group_clusters(group.size());

	for (size_t i = 0; i < group.size(); ++i)
	{
		const Cluster& cluster = clusters[group[i]];
		clodCluster& result = group_clusters[i];

		result.refined = cluster.refined;
		result.bounds = (config.optimize_bounds && cluster.refined != -1) ? boundsCompute(mesh, cluster.indices, cluster.bounds.error) : cluster.bounds;
		result.indices = cluster.indices.data();
		result.index_count = cluster.indices.size();
		result.vertex_count = cluster.vertices;
	}

	return output_callback ? output_callback(output_context, {depth, simplified}, group_clusters.data(), group_clusters.size()) : -1;
}

} // namespace clod

clodConfig clodDefaultConfig(size_t max_triangles)
{
	assert(max_triangles >= 4 && max_triangles <= 256);

	clodConfig config = {};
	config.max_vertices = max_triangles;
	config.min_triangles = max_triangles / 3;
	config.max_triangles = max_triangles;

#if MESHOPTIMIZER_VERSION < 1000
	config.min_triangles &= ~3; // account for 4b alignment
#endif

	config.partition_spatial = true;
	config.partition_size = 16;

	config.cluster_spatial = false;
	config.cluster_split_factor = 2.0f;

	config.optimize_clusters = true;

	config.simplify_ratio = 0.5f;
	config.simplify_threshold = 0.85f;
	config.simplify_error_merge_previous = 1.0f;
	config.simplify_error_factor_sloppy = 2.0f;
	config.simplify_permissive = true;
	config.simplify_fallback_permissive = false; // note: by default we run in permissive mode, but it's also possible to disable that and use it only as a fallback
	config.simplify_fallback_sloppy = true;

	return config;
}

clodConfig clodDefaultConfigRT(size_t max_triangles)
{
	clodConfig config = clodDefaultConfig(max_triangles);

	// for ray tracing, we may want smaller clusters when that improves BVH quality further; for maximum ray tracing performance this could be reduced even further
	config.min_triangles = max_triangles / 4;

	// by default, we use larger max_vertices for RT; the vertex count is not important for ray tracing performance, and this helps improve cluster utilization
	config.max_vertices = std::min(size_t(256), max_triangles * 2);

	config.cluster_spatial = true;
	config.cluster_fill_weight = 0.5f;

	return config;
}

size_t clodBuild(clodConfig config, clodMesh mesh, void* output_context, clodOutput output_callback)
{
	using namespace clod;

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

	// compute initial precise bounds; subsequent bounds will be using group-merged bounds
	for (Cluster& cluster : clusters)
		cluster.bounds = boundsCompute(mesh, cluster.indices, 0.f);

	std::vector<int> pending(clusters.size());
	for (size_t i = 0; i < clusters.size(); ++i)
		pending[i] = int(i);

	int depth = 0;

	// merge and simplify clusters until we can't merge anymore
	while (pending.size() > 1)
	{
		std::vector<std::vector<int> > groups = partition(config, mesh, clusters, pending, remap);

		pending.clear();

		// mark boundaries between groups with a lock bit to avoid gaps in simplified result
		lockBoundary(locks, groups, clusters, remap, mesh.vertex_lock);

		// every group needs to be simplified now
		for (size_t i = 0; i < groups.size(); ++i)
		{
			std::vector<unsigned int> merged;
			merged.reserve(groups[i].size() * config.max_triangles * 3);
			for (size_t j = 0; j < groups[i].size(); ++j)
				merged.insert(merged.end(), clusters[groups[i][j]].indices.begin(), clusters[groups[i][j]].indices.end());

			size_t target_size = size_t((merged.size() / 3) * config.simplify_ratio) * 3;

			// enforce bounds and error monotonicity
			// note: it is incorrect to use the precise bounds of the merged or simplified mesh, because this may violate monotonicity
			clodBounds bounds = boundsMerge(clusters, groups[i]);

			float error = 0.f;
			std::vector<unsigned int> simplified = simplify(config, mesh, merged, locks, target_size, &error);
			if (simplified.size() > merged.size() * config.simplify_threshold)
			{
				bounds.error = FLT_MAX; // terminal group, won't simplify further
				outputGroup(config, mesh, clusters, groups[i], bounds, depth, output_context, output_callback);
				continue; // simplification is stuck; abandon the merge
			}

			// enforce error monotonicity (with an optional hierarchical factor to separate transitions more)
			bounds.error = std::max(bounds.error * config.simplify_error_merge_previous, error) + error * config.simplify_error_merge_additive;

			// output the new group with all clusters; the resulting id will be recorded in new clusters as clodCluster::refined
			int refined = outputGroup(config, mesh, clusters, groups[i], bounds, depth, output_context, output_callback);

			// discard clusters from the group - they won't be used anymore
			for (size_t j = 0; j < groups[i].size(); ++j)
				clusters[groups[i][j]].indices = std::vector<unsigned int>();

			std::vector<Cluster> split = clusterize(config, mesh, simplified.data(), simplified.size());

			for (Cluster& cluster : split)
			{
				cluster.refined = refined;

				// update cluster group bounds to the group-merged bounds; this ensures that we compute the group bounds for whatever group this cluster will be part of conservatively
				cluster.bounds = bounds;

				// enqueue new cluster for further processing
				clusters.push_back(std::move(cluster));
				pending.push_back(int(clusters.size()) - 1);
			}
		}

		depth++;
	}

	if (pending.size())
	{
		assert(pending.size() == 1);
		const Cluster& cluster = clusters[pending[0]];

		clodBounds bounds = cluster.bounds;
		bounds.error = FLT_MAX; // terminal group, won't simplify further

		outputGroup(config, mesh, clusters, pending, bounds, depth, output_context, output_callback);
	}

	return clusters.size();
}

size_t clodLocalIndices(unsigned int* vertices, unsigned char* triangles, const unsigned int* indices, size_t index_count)
{
	size_t unique = 0;

	// direct mapped cache for fast lookups based on low index bits; inspired by vk_lod_clusters from NVIDIA
	short cache[1024];
	memset(cache, -1, sizeof(cache));

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int v = indices[i];
		unsigned int key = v & (sizeof(cache) / sizeof(cache[0]) - 1);
		short c = cache[key];

		// fast path: vertex has been seen before
		if (c >= 0 && vertices[c] == v)
		{
			triangles[i] = (unsigned char)c;
			continue;
		}

		// fast path: vertex has never been seen before
		if (c < 0)
		{
			cache[key] = short(unique);
			triangles[i] = (unsigned char)unique;
			vertices[unique++] = v;
			continue;
		}

		// slow path: hash collision with a different vertex, so we need to look through all vertices
		int pos = -1;
		for (size_t j = 0; j < unique; ++j)
			if (vertices[j] == v)
			{
				pos = int(j);
				break;
			}

		if (pos < 0)
		{
			pos = int(unique);
			vertices[unique++] = v;
		}

		cache[key] = short(pos);
		triangles[i] = (unsigned char)pos;
	}

	assert(unique <= 256);
	return unique;
}
#endif

/**
 * Copyright (c) 2016-2025 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
