// Brian Karis. Nanite: A Deep Dive. 2021
#include "../src/meshoptimizer.h"

#include <float.h>
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

struct Cluster
{
	std::vector<unsigned int> indices;
};

const size_t kClusterSize = 128;

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

static std::vector<unsigned int> simplify(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, size_t target_count)
{
	std::vector<unsigned int> lod(indices.size());
	float error = 0.f;
	unsigned int options = meshopt_SimplifyLockBorder | meshopt_SimplifySparse;
	lod.resize(meshopt_simplify(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), target_count, FLT_MAX, options, &error));

#if TRACE > 1
	printf("cluster: %d => %d triangles\n", int(indices.size() / 3), int(lod.size() / 3));
#endif

	return lod;
}

void nanite(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	// initial clusterization splits the original mesh
	std::vector<Cluster> clusters = clusterize(vertices, indices);

	printf("lod 0: %d clusters, %d triangles\n", int(clusters.size()), int(indices.size() / 3));

	std::vector<int> pending(clusters.size());
	for (size_t i = 0; i < clusters.size(); ++i)
		pending[i] = int(i);

	int depth = 0;
	size_t stuck_triangles = 0;

	// merge and simplify clusters until we can't merge anymore
	while (pending.size() > 1)
	{
		std::vector<std::vector<int> > groups = partition(clusters, pending);
		pending.clear();

		size_t triangles = 0;
		int stuck_clusters = 0;
		int full_clusters = 0;

		// every group needs to be simplified now
		for (size_t i = 0; i < groups.size(); ++i)
		{
			std::vector<unsigned int> merged;
			for (size_t j = 0; j < groups[i].size(); ++j)
				merged.insert(merged.end(), clusters[groups[i][j]].indices.begin(), clusters[groups[i][j]].indices.end());

			if (merged.size() < kClusterSize * 3 * 3)
			{
#if TRACE
				printf("stuck cluster: merged triangles %d under threshold\n", int(merged.size() / 3));
#endif

				stuck_clusters++;
				stuck_triangles += merged.size() / 3;
				continue; // didn't merge enough
				// TODO: this is very suboptimal as it leaves some edges of other clusters permanently locked.
			}

			std::vector<unsigned int> simplified = simplify(vertices, merged, kClusterSize * 2 * 3);
			if (simplified.size() > kClusterSize * 3 * 3)
			{
#if TRACE
				printf("stuck cluster: simplified triangles %d over threshold\n", int(simplified.size() / 3));
#endif

				stuck_clusters++;
				stuck_triangles += merged.size() / 3;
				continue; // simplification is stuck; abandon the merge
			}

			std::vector<Cluster> split = clusterize(vertices, simplified);
			for (size_t j = 0; j < split.size(); ++j)
			{
				clusters.push_back(split[j]); // std::move
				pending.push_back(int(clusters.size()) - 1);
				triangles += split[j].indices.size() / 3;
				full_clusters += split[j].indices.size() == kClusterSize * 3;
			}
		}

		depth++;
		printf("lod %d: %d clusters, %d triangles (%d clusters stuck, %d full)\n", depth, int(pending.size()), int(triangles), stuck_clusters, full_clusters);
	}

	printf("lowest lod: %d triangles\n", int(stuck_triangles));
}
