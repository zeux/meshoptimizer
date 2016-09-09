#include "overdrawoptimizertipsify.hpp"

#include "math.hpp"

#include <cassert>
#include <algorithm>

namespace
{
	struct MeshInfo
	{
		std::vector<vec3> cluster_centroids;
		std::vector<vec3> cluster_normals;
		vec3 mesh_centroid;
	};

	struct ClusterSortData
	{
		unsigned int cluster;
		float dot_product;

		bool operator<(const ClusterSortData& other) const
		{
			return (dot_product < other.dot_product);
		}
		
		bool operator>(const ClusterSortData& other) const
		{
			return (dot_product > other.dot_product);
		}
	};

	template <typename T> void calculateMeshInfo(MeshInfo& info, const T* indices, size_t index_count, const void* vertex_positions, size_t vertex_positions_stride, const std::vector<unsigned int>& clusters)
	{
		info.cluster_centroids.resize(clusters.size(), vec3(0, 0, 0));
		info.cluster_normals.resize(clusters.size(), vec3(0, 0, 0));
		info.mesh_centroid = vec3(0, 0, 0);

		size_t face_count = index_count / 3;

		unsigned int current_cluster = 0;
		unsigned int next_cluster_face = (clusters.size() > 1) ? clusters[1] : face_count;

		float cluster_area = 0;
		float mesh_area = 0;

		for (size_t i = 0; i <= face_count; ++i)
		{
			if (i == next_cluster_face)
			{
				// finalize cluster information
				info.cluster_centroids[current_cluster] /= cluster_area * 3;
				info.cluster_normals[current_cluster] = normalize(info.cluster_normals[current_cluster]);

				// move to next cluster
				++current_cluster;

				if (current_cluster == clusters.size())
				{
					assert(i == face_count);
					break;
				}

				next_cluster_face = (clusters.size() > current_cluster + 1) ? clusters[current_cluster + 1] : face_count;
				
				cluster_area = 0;
			}

			vec3 positions[] =
			{
				vec3(reinterpret_cast<const float*>(static_cast<const char*>(vertex_positions) + indices[i * 3 + 0] * vertex_positions_stride)),
				vec3(reinterpret_cast<const float*>(static_cast<const char*>(vertex_positions) + indices[i * 3 + 1] * vertex_positions_stride)),
				vec3(reinterpret_cast<const float*>(static_cast<const char*>(vertex_positions) + indices[i * 3 + 2] * vertex_positions_stride))
			};
			
			vec3 normal = cross(positions[1] - positions[0], positions[2] - positions[0]);

			float area = length(normal);

			info.mesh_centroid += (positions[0] + positions[1] + positions[2]) * area;
			info.cluster_centroids[current_cluster] += (positions[0] + positions[1] + positions[2]) * area;
			info.cluster_normals[current_cluster] += normal;
			
			cluster_area += area;
			mesh_area += area;
		}
	
		info.mesh_centroid /= mesh_area * 3;
	}

	template <typename T> void optimizeOverdrawOrder(T* destination, const T* indices, size_t index_count, const void* vertex_positions, size_t vertex_positions_stride, const std::vector<unsigned int>& clusters)
	{
		// mesh information
		MeshInfo info;
		calculateMeshInfo(info, indices, index_count, vertex_positions, vertex_positions_stride, clusters);

		// fill sort data
		std::vector<ClusterSortData> sort_data(clusters.size());

		for (size_t i = 0; i < clusters.size(); ++i)
		{
			sort_data[i].cluster = static_cast<unsigned int>(i);
			sort_data[i].dot_product = dot(info.cluster_centroids[i] - info.mesh_centroid, info.cluster_normals[i]);
		}

		// high product = possible occluder, render early
		std::sort(sort_data.begin(), sort_data.end(), std::greater<ClusterSortData>());

		// fill output buffer
		T* dest_it = destination;

		for (size_t i = 0; i < clusters.size(); ++i)
		{
			unsigned int cluster = sort_data[i].cluster;

			unsigned int next_cluster_face = (clusters.size() > cluster + 1) ? clusters[cluster + 1] : index_count / 3;

			for (size_t j = clusters[cluster] * 3; j < next_cluster_face * 3; ++j)
				*dest_it++ = indices[j];
		}

		assert(dest_it == destination);
	}
	
	template <typename T> std::pair<float, unsigned int> calculateACMR(const T* indices, size_t index_count, unsigned int cache_size, float threshold, std::vector<unsigned int>& cache_time_stamps, unsigned int& time_stamp)
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
			acmr = static_cast<float>(cache_misses) / (face + 1);
			
			if (acmr <= threshold)
			{
				return std::make_pair(acmr, face + 1);
			}
		}
		
		return std::make_pair(acmr, face_count);
	}
	
	template <typename T> void generateSoftBoundaries(std::vector<unsigned int>& destination, const T* indices, size_t index_count, size_t vertex_count, const std::vector<unsigned int>& clusters, unsigned int cache_size, float threshold)
	{
		if (threshold <= 0)
		{
			// hard boundaries only
			destination = clusters;
			return;
		}
		
		std::vector<unsigned int> cache_time_stamps(vertex_count, 0);
		unsigned int time_stamp = 0;
		
		std::pair<float, unsigned int> p = calculateACMR(indices, index_count, cache_size, 0, cache_time_stamps, time_stamp);
		
		assert(p.second == index_count / 3);
		
		float acmr_threshold = p.first * threshold;
		
		for (size_t it = 0; it < clusters.size(); ++it)
		{
			unsigned int start = clusters[it]; 
			unsigned int end = (it + 1 < clusters.size()) ? clusters[it + 1] : index_count / 3;
			
			while (start != end)
			{
				std::pair<float, unsigned int> cp = calculateACMR(indices + start * 3, (end - start) * 3, cache_size, acmr_threshold, cache_time_stamps, time_stamp);
				
				destination.push_back(start);
				start += cp.second;
			}
		}
	}

	template <typename T> void optimizeOverdrawTipsifyImpl(T* destination, const T* indices, size_t index_count, const void* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const std::vector<unsigned int>& hard_clusters, unsigned int cache_size, float threshold)
	{
		assert(destination != indices);

		// guard for empty meshes
		if (index_count == 0 || vertex_count == 0)
		{
			return;
		}

		assert(!hard_clusters.empty());

		// generate soft boundaries
		std::vector<unsigned int> clusters;
		generateSoftBoundaries(clusters, indices, index_count, vertex_count, hard_clusters, cache_size, threshold);

		// optimize overdraw order
		optimizeOverdrawOrder(destination, indices, index_count, vertex_positions, vertex_positions_stride, clusters);
	}
}

void optimizeOverdrawTipsify(unsigned short* destination, const unsigned short* indices, size_t index_count, const void* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const std::vector<unsigned int>& clusters, unsigned int cache_size, float threshold)
{
	optimizeOverdrawTipsifyImpl(destination, indices, index_count, vertex_positions, vertex_positions_stride, vertex_count, clusters, cache_size, threshold);
}

void optimizeOverdrawTipsify(unsigned int* destination, const unsigned int* indices, size_t index_count, const void* vertex_positions, size_t vertex_positions_stride, size_t vertex_count, const std::vector<unsigned int>& clusters, unsigned int cache_size, float threshold)
{
	optimizeOverdrawTipsifyImpl(destination, indices, index_count, vertex_positions, vertex_positions_stride, vertex_count, clusters, cache_size, threshold);
}
