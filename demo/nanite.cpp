// Brian Karis. Nanite: A Deep Dive. 2021
#include "../src/meshoptimizer.h"

#include <float.h>
#include <math.h>
#include <stdio.h>

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

static float boundsError(const LODBounds& bounds, float x, float y, float z)
{
	float dx = bounds.center[0] - x, dy = bounds.center[1] - y, dz = bounds.center[2] - z;
	float d = sqrtf(dx * dx + dy * dy + dz * dz) - bounds.radius;
	return d <= 0 ? FLT_MAX : bounds.error / d;
}

static std::vector<Cluster> clusterize(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
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

#if TRACE > 1
		printf("cluster %d: %d triangles\n", int(i), int(meshlet.triangle_count));
#endif
	}

	return clusters;
}

static std::vector<std::vector<int> > partition(const std::vector<Cluster>& clusters, const std::vector<int>& pending)
{
	std::vector<std::vector<int> > result;
	size_t last_indices = 0;

	// rough merge; while clusters are approximately spatially ordered, this should use a proper partitioning algorithm
	for (size_t i = 0; i < pending.size(); ++i)
	{
		if (result.empty() || last_indices + clusters[pending[i]].indices.size() > kClusterSize * 4 * 3)
		{
			result.push_back(std::vector<int>());
			last_indices = 0;
		}

		result.back().push_back(pending[i]);
		last_indices += clusters[pending[i]].indices.size();
	}

	return result;
}

static std::vector<unsigned int> simplify(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, size_t target_count, float* error = NULL)
{
	std::vector<unsigned int> lod(indices.size());
	unsigned int options = meshopt_SimplifyLockBorder | meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute;
	lod.resize(meshopt_simplify(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), target_count, FLT_MAX, options, error));

#if TRACE > 1
	printf("cluster: %d => %d triangles\n", int(indices.size() / 3), int(lod.size() / 3));
#endif

	return lod;
}

void dumpObj(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, bool recomputeNormals = false);

void nanite(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
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

	// merge and simplify clusters until we can't merge anymore
	while (pending.size() > 1)
	{
		std::vector<std::vector<int> > groups = partition(clusters, pending);
		pending.clear();

		size_t triangles = 0;
		size_t stuck_triangles = 0;
		int stuck_clusters = 0;
		int full_clusters = 0;

		// every group needs to be simplified now
		for (size_t i = 0; i < groups.size(); ++i)
		{
			std::vector<unsigned int> merged;
			for (size_t j = 0; j < groups[i].size(); ++j)
				merged.insert(merged.end(), clusters[groups[i][j]].indices.begin(), clusters[groups[i][j]].indices.end());

			if (merged.size() <= kClusterSize * 3 * 3)
			{
#if TRACE
				printf("stuck cluster: merged triangles %d under threshold\n", int(merged.size() / 3));
#endif

				stuck_clusters++;
				stuck_triangles += merged.size() / 3;
				continue; // didn't merge enough
				          // TODO: this is very suboptimal as it leaves some edges of other clusters permanently locked.
			}

			float error = 0.f;
			std::vector<unsigned int> simplified = simplify(vertices, merged, kClusterSize * 2 * 3, &error);
			if (simplified.size() >= kClusterSize * 3 * 3)
			{
#if TRACE
				printf("stuck cluster: simplified triangles %d over threshold\n", int(simplified.size() / 3));
#endif
				stuck_clusters++;
				stuck_triangles += merged.size() / 3;
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
		printf("lod %d: %d clusters (%d full), %d triangles (%d triangles stuck in %d clusters)\n", depth,
		    int(pending.size()), full_clusters, int(triangles), int(stuck_triangles), stuck_clusters);
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
		maxx = std::max(maxx, vertices[i].px);
		maxy = std::max(maxy, vertices[i].py);
		maxz = std::max(maxz, vertices[i].pz);
	}
	float threshold = 7e-3f;

	std::vector<unsigned int> cut;
	for (size_t i = 0; i < clusters.size(); ++i)
		if (boundsError(clusters[i].self, maxx, maxy, maxz) <= threshold && boundsError(clusters[i].parent, maxx, maxy, maxz) > threshold)
			cut.insert(cut.end(), clusters[i].indices.begin(), clusters[i].indices.end());

#ifndef NDEBUG
	for (size_t i = 0; i < dag_debug.size(); ++i)
	{
		int j = dag_debug[i].first, k = dag_debug[i].second;
		float ej = boundsError(clusters[j].self, maxx, maxy, maxz);
		float ejp = boundsError(clusters[j].parent, maxx, maxy, maxz);
		float ek = boundsError(clusters[k].self, maxx, maxy, maxz);

		assert(ej <= ek);
		assert(ejp >= ej);
	}
#endif

	printf("cut (%.3f): %d triangles\n", threshold, int(cut.size() / 3));
	// dumpObj(vertices, cut);
}
