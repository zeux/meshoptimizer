// This file is part of meshoptimizer library; see meshoptimizer.hpp for version/license details
#include "meshoptimizer.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <vector>

// This work is based on:
// Pedro Sander, Diego Nehab and Joshua Barcz. Fast Triangle Reordering for Vertex Locality and Reduced Overdraw. 2007
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

	bool operator>(const ClusterSortData& other) const
	{
		return (dot_product > other.dot_product);
	}
};

template <typename T>
static void calculateSortData(std::vector<ClusterSortData>& sort_data, const T* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, const std::vector<unsigned int>& clusters)
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

		sort_data[cluster].cluster = static_cast<unsigned int>(cluster);
		sort_data[cluster].dot_product = centroid_vector.x * cluster_normal.x + centroid_vector.y * cluster_normal.y + centroid_vector.z * cluster_normal.z;
	}
}

template <typename T>
static std::pair<float, size_t> calculateACMR(const T* indices, size_t index_count, unsigned int cache_size, float threshold, std::vector<unsigned int>& cache_time_stamps, unsigned int& time_stamp)
{
	// Ensures that all vertices are not in cache
	assert(time_stamp <= static_cast<unsigned int>(-1) - (cache_size + 1));
	time_stamp += cache_size + 1;

	float acmr = 0;
	unsigned int cache_misses = 0;

	size_t face_count = index_count / 3;

	for (size_t face = 0; face < face_count; ++face)
	{
		unsigned int a = indices[face * 3 + 0], b = indices[face * 3 + 1], c = indices[face * 3 + 2];

		// if vertex is not in cache, put it in cache
		if (time_stamp - cache_time_stamps[a] > cache_size)
		{
			cache_time_stamps[a] = time_stamp++;
			cache_misses++;
		}

		if (time_stamp - cache_time_stamps[b] > cache_size)
		{
			cache_time_stamps[b] = time_stamp++;
			cache_misses++;
		}

		if (time_stamp - cache_time_stamps[c] > cache_size)
		{
			cache_time_stamps[c] = time_stamp++;
			cache_misses++;
		}

		// update ACMR & check for threshold
		acmr = float(cache_misses) / float(face + 1);

		if (acmr <= threshold)
		{
			return std::make_pair(acmr, face + 1);
		}
	}

	return std::make_pair(acmr, face_count);
}

template <typename T>
static void generateSoftBoundaries(std::vector<unsigned int>& destination, const T* indices, size_t index_count, size_t vertex_count, const unsigned int* clusters, size_t cluster_count, unsigned int cache_size, float threshold)
{
	if (threshold <= 0)
	{
		// hard boundaries only
		destination.assign(clusters, clusters + cluster_count);
		return;
	}

	std::vector<unsigned int> cache_time_stamps(vertex_count, 0);
	unsigned int time_stamp = 0;

	std::pair<float, size_t> p = calculateACMR(indices, index_count, cache_size, 0, cache_time_stamps, time_stamp);
	assert(p.second == index_count / 3);

	float acmr_threshold = p.first * threshold;

	for (size_t it = 0; it < cluster_count; ++it)
	{
		size_t start = clusters[it];
		size_t end = (it + 1 < cluster_count) ? clusters[it + 1] : index_count / 3;
		assert(start <= end);

		while (start != end)
		{
			std::pair<float, size_t> cp = calculateACMR(indices + start * 3, (end - start) * 3, cache_size, acmr_threshold, cache_time_stamps, time_stamp);

			destination.push_back(static_cast<unsigned int>(start));
			start += cp.second;
		}
	}
}

template <typename T>
static void optimizeOverdrawTipsify(T* destination, const T* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const unsigned int* hard_clusters, size_t hard_cluster_count, unsigned int cache_size, float threshold)
{
	assert(vertex_positions_stride > 0);
	assert(vertex_positions_stride % sizeof(float) == 0);

	// guard for empty meshes
	if (index_count == 0 || vertex_count == 0)
	{
		return;
	}

	// support in-place optimization
	std::vector<T> indices_copy;

	if (destination == indices)
	{
		indices_copy.assign(indices, indices + index_count);
		indices = &indices_copy[0];
	}

	// we're expecting at least one cluster as an input
	assert(hard_clusters && hard_cluster_count > 0);

	// generate soft boundaries
	std::vector<unsigned int> clusters;
	generateSoftBoundaries(clusters, indices, index_count, vertex_count, hard_clusters, hard_cluster_count, cache_size, threshold);

	// fill sort data
	std::vector<ClusterSortData> sort_data(clusters.size());
	calculateSortData(sort_data, indices, index_count, vertex_positions, vertex_positions_stride, clusters);

	// high product = possible occluder, render early
	std::sort(sort_data.begin(), sort_data.end(), std::greater<ClusterSortData>());

	// fill output buffer
	T* dest_it = destination;

	for (size_t i = 0; i < clusters.size(); ++i)
	{
		unsigned int cluster = sort_data[i].cluster;

		size_t cluster_begin = clusters[cluster] * 3;
		size_t cluster_end = (clusters.size() > cluster + 1) ? clusters[cluster + 1] * 3 : index_count;

		for (size_t j = cluster_begin; j < cluster_end; ++j)
			*dest_it++ = indices[j];
	}

	assert(dest_it == destination + index_count);
}

void optimizeOverdraw(unsigned short* destination, const unsigned short* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const unsigned int* clusters, size_t cluster_count, unsigned int cache_size, float threshold)
{
	optimizeOverdrawTipsify(destination, indices, index_count, vertex_positions, vertex_positions_stride, vertex_count, clusters, cluster_count, cache_size, threshold);
}

void optimizeOverdraw(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const unsigned int* clusters, size_t cluster_count, unsigned int cache_size, float threshold)
{
	optimizeOverdrawTipsify(destination, indices, index_count, vertex_positions, vertex_positions_stride, vertex_count, clusters, cluster_count, cache_size, threshold);
}

} // namespace meshopt
