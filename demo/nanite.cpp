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

double timestamp();

static std::vector<Cluster> clusterize(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	const size_t max_vertices = 255;
	const size_t max_triangles = 128;

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

#if TRACE
		printf("cluster %d: %d triangles\n", int(i), int(meshlet.triangle_count));
#endif
	}

	return clusters;
}

static Cluster simplify(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, size_t target_count)
{
	std::vector<unsigned int> lod(indices.size());
	float error = 0.f;
	unsigned int options = meshopt_SimplifyLockBorder | meshopt_SimplifySparse;
	lod.resize(meshopt_simplify(&lod[0], &indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex), target_count, FLT_MAX, options, &error));

#if TRACE
	printf("cluster: %d => %d triangles\n", int(indices.size() / 3), int(lod.size() / 3));
#endif

	Cluster result;
	result.indices = lod;
	return result;
}

void nanite(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	std::vector<Cluster> clusters;

	// initial clusterization splits the original mesh
	std::vector<Cluster> pending = clusterize(vertices, indices);
	printf("lod 0: %d clusters\n", int(pending.size()));

	int depth = 0;

	// merge and simplify clusters until we can't merge anymore
	while (!pending.empty())
	{
		size_t start = clusters.size();

		for (size_t i = 0; i < pending.size(); ++i)
			clusters.push_back(pending[i]); // std::move
		pending.clear();

		std::vector<Cluster> merged;

		// rough merge; while clusters are approximately spatially ordered, this should use a proper partitioning algorithm
		for (size_t i = start; i < clusters.size(); ++i)
		{
			if (merged.empty() || merged.back().indices.size() + clusters[i].indices.size() > 128 * 4 * 3)
				merged.push_back(Cluster());

			merged.back().indices.insert(merged.back().indices.end(), clusters[i].indices.begin(), clusters[i].indices.end());
		}

		// every merged cluster needs to be simplified now
		for (size_t i = 0; i < merged.size(); ++i)
		{
			if (merged[i].indices.size() < 128 * 3 * 3)
				continue; // didn't merge enough

			Cluster simplified = simplify(vertices, merged[i].indices, 128 * 2 * 3);
			if (simplified.indices.size() > 128 * 3 * 3)
				continue; // simplification is stuck; abandon the merge

			std::vector<Cluster> split = clusterize(vertices, simplified.indices);
			for (size_t j = 0; j < split.size(); ++j)
				pending.push_back(split[j]); // std::move
		}

		depth++;
		printf("lod %d: %d clusters\n", depth, int(pending.size()));
	}
}
