// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <cassert>
#include <cmath>

#include <algorithm>
#include <vector>

// This work is based on:
// Pedro Sander, Diego Nehab and Joshua Barczak. Fast Triangle Reordering for Vertex Locality and Reduced Overdraw. 2007
namespace meshopt
{

struct Vector3
{
	float x, y, z;
};

struct ClusterSortData
{
	unsigned int cluster;
	float dot_product;

	bool operator<(const ClusterSortData& other) const
	{
		// high product = possible occluder, render early
		return dot_product > other.dot_product;
	}
};

static void calculateSortData(std::vector<ClusterSortData>& sort_data, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, const std::vector<unsigned int>& clusters)
{
	assert(sort_data.size() == clusters.size());

	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	Vector3 mesh_centroid = Vector3();

	for (size_t i = 0; i < index_count; ++i)
	{
		const Vector3& p = *reinterpret_cast<const Vector3*>(vertex_positions + vertex_stride_float * indices[i]);

		mesh_centroid.x += p.x;
		mesh_centroid.y += p.y;
		mesh_centroid.z += p.z;
	}

	mesh_centroid.x /= index_count;
	mesh_centroid.y /= index_count;
	mesh_centroid.z /= index_count;

	for (size_t cluster = 0; cluster < clusters.size(); ++cluster)
	{
		size_t cluster_begin = clusters[cluster] * 3;
		size_t cluster_end = (clusters.size() > cluster + 1) ? clusters[cluster + 1] * 3 : index_count;
		assert(cluster_begin < cluster_end);

		float cluster_area = 0;
		Vector3 cluster_centroid = Vector3();
		Vector3 cluster_normal = Vector3();

		for (size_t i = cluster_begin; i < cluster_end; i += 3)
		{
			const Vector3& p0 = *reinterpret_cast<const Vector3*>(vertex_positions + vertex_stride_float * indices[i + 0]);
			const Vector3& p1 = *reinterpret_cast<const Vector3*>(vertex_positions + vertex_stride_float * indices[i + 1]);
			const Vector3& p2 = *reinterpret_cast<const Vector3*>(vertex_positions + vertex_stride_float * indices[i + 2]);

			Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
			Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};
			Vector3 normal = {p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x};

			float area = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
			Vector3 centroid_area = {(p0.x + p1.x + p2.x) * (area / 3), (p0.y + p1.y + p2.y) * (area / 3), (p0.z + p1.z + p2.z) * (area / 3)};

			cluster_centroid.x += centroid_area.x;
			cluster_centroid.y += centroid_area.y;
			cluster_centroid.z += centroid_area.z;
			cluster_normal.x += normal.x;
			cluster_normal.y += normal.y;
			cluster_normal.z += normal.z;
			cluster_area += area;
		}

		float inv_cluster_area = cluster_area == 0 ? 0 : 1 / cluster_area;

		cluster_centroid.x *= inv_cluster_area;
		cluster_centroid.y *= inv_cluster_area;
		cluster_centroid.z *= inv_cluster_area;

		float cluster_normal_length = sqrtf(cluster_normal.x * cluster_normal.x + cluster_normal.y * cluster_normal.y + cluster_normal.z * cluster_normal.z);
		float inv_cluster_normal_length = cluster_normal_length == 0 ? 0 : 1 / cluster_normal_length;

		cluster_normal.x *= inv_cluster_normal_length;
		cluster_normal.y *= inv_cluster_normal_length;
		cluster_normal.z *= inv_cluster_normal_length;

		Vector3 centroid_vector = {cluster_centroid.x - mesh_centroid.x, cluster_centroid.y - mesh_centroid.y, cluster_centroid.z - mesh_centroid.z};

		sort_data[cluster].cluster = unsigned(cluster);
		sort_data[cluster].dot_product = centroid_vector.x * cluster_normal.x + centroid_vector.y * cluster_normal.y + centroid_vector.z * cluster_normal.z;
	}
}

static unsigned int updateCache(unsigned int a, unsigned int b, unsigned int c, unsigned int cache_size, std::vector<unsigned int>& cache_timestamps, unsigned int& timestamp)
{
	assert(a < cache_timestamps.size());
	assert(b < cache_timestamps.size());
	assert(c < cache_timestamps.size());

	unsigned int cache_misses = 0;

	// if vertex is not in cache, put it in cache
	if (timestamp - cache_timestamps[a] > cache_size)
	{
		cache_timestamps[a] = timestamp++;
		cache_misses++;
	}

	if (timestamp - cache_timestamps[b] > cache_size)
	{
		cache_timestamps[b] = timestamp++;
		cache_misses++;
	}

	if (timestamp - cache_timestamps[c] > cache_size)
	{
		cache_timestamps[c] = timestamp++;
		cache_misses++;
	}

	return cache_misses;
}

static void generateHardBoundaries(std::vector<unsigned int>& destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	std::vector<unsigned int> cache_timestamps(vertex_count, 0);
	unsigned int timestamp = cache_size + 1;

	size_t face_count = index_count / 3;

	for (size_t i = 0; i < face_count; ++i)
	{
		unsigned int m = updateCache(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2], cache_size, cache_timestamps, timestamp);

		// when all three vertices are not in the cache it's usually relatively safe to assume that this is a new patch in the mesh
		// that is disjoint from previous vertices; sometimes it might come back to reference existing vertices but that frequently
		// suggests an inefficiency in the vertex cache optimization algorithm
		// usually the first triangle has 3 misses unless it's degenerate - thus we make sure the first cluster always starts with 0
		if (i == 0 || m == 3)
		{
			destination.push_back(unsigned(i));
		}
	}
}

static void generateSoftBoundaries(std::vector<unsigned int>& destination, const unsigned int* indices, size_t index_count, size_t vertex_count, const std::vector<unsigned int>& clusters, unsigned int cache_size, float threshold)
{
	std::vector<unsigned int> cache_timestamps(vertex_count, 0);
	unsigned int timestamp = 0;

	for (size_t it = 0; it < clusters.size(); ++it)
	{
		size_t start = clusters[it];
		size_t end = (it + 1 < clusters.size()) ? clusters[it + 1] : index_count / 3;
		assert(start < end);

		// reset cache
		timestamp += cache_size + 1;

		// measure cluster ACMR
		unsigned int cluster_misses = 0;

		for (size_t i = start; i < end; ++i)
		{
			unsigned int m = updateCache(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2], cache_size, cache_timestamps, timestamp);

			cluster_misses += m;
		}

		float cluster_threshold = threshold * (float(cluster_misses) / float(end - start));

		// first cluster always starts from the hard cluster boundary
		destination.push_back(unsigned(start));

		// reset cache
		timestamp += cache_size + 1;

		unsigned int running_misses = 0;
		unsigned int running_faces = 0;

		for (size_t i = start; i < end; ++i)
		{
			unsigned int m = updateCache(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2], cache_size, cache_timestamps, timestamp);

			running_misses += m;
			running_faces += 1;

			if (float(running_misses) / float(running_faces) <= cluster_threshold)
			{
				// we have reached the target ACMR with the current triangle so we need to start a new cluster on the next one
				// note that this may mean that we add 'end` to destination for the last triangle, which will imply that the last
				// cluster is empty; however, the 'pop_back' after the loop will clean it up
				destination.push_back(unsigned(i + 1));

				// reset cache
				timestamp += cache_size + 1;

				running_misses = 0;
				running_faces = 0;
			}
		}

		// each time we reach the target ACMR we flush the cluster
		// this means that the last cluster is by definition not very good - there are frequent cases where we are left with a few triangles
		// in the last cluster, producing a very bad ACMR and significantly penalizing the overall results
		// thus we remove the last cluster boundary, merging the last complete cluster with the last incomplete one
		// there are sometimes cases when the last cluster is actually good enough - in which case the code above would have added 'end'
		// to the cluster boundary array which we need to remove anyway - this code will do that automatically
		if (destination.back() != start)
		{
			destination.pop_back();
		}
	}

	assert(destination.size() >= clusters.size());
	assert(destination.size() <= index_count / 3);
}

} // namespace

void meshopt_optimizeOverdraw(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, unsigned int cache_size, float threshold)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride > 0 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(cache_size >= 3);

	// guard for empty meshes
	if (index_count == 0 || vertex_count == 0)
	{
		return;
	}

	// support in-place optimization
	std::vector<unsigned int> indices_copy;

	if (destination == indices)
	{
		indices_copy.assign(indices, indices + index_count);
		indices = &indices_copy[0];
	}

	// generate hard boundaries from full-triangle cache misses
	std::vector<unsigned int> hard_clusters;
	generateHardBoundaries(hard_clusters, indices, index_count, vertex_count, cache_size);

	// generate soft boundaries
	std::vector<unsigned int> soft_clusters;
	generateSoftBoundaries(soft_clusters, indices, index_count, vertex_count, hard_clusters, cache_size, threshold);

	const std::vector<unsigned int>& clusters = soft_clusters;

	// fill sort data
	std::vector<ClusterSortData> sort_data(clusters.size());
	calculateSortData(sort_data, indices, index_count, vertex_positions, vertex_positions_stride, clusters);

	// high product = possible occluder, render early
	std::sort(sort_data.begin(), sort_data.end());

	// fill output buffer
	size_t offset = 0;

	for (size_t i = 0; i < clusters.size(); ++i)
	{
		unsigned int cluster = sort_data[i].cluster;
		assert(cluster < clusters.size());

		size_t cluster_begin = clusters[cluster] * 3;
		size_t cluster_end = (cluster + 1 < clusters.size()) ? clusters[cluster + 1] * 3 : index_count;
		assert(cluster_begin < cluster_end);

		for (size_t j = cluster_begin; j < cluster_end; ++j)
			destination[offset++] = indices[j];
	}

	assert(offset == index_count);
}
