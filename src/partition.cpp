// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

// TOOD part of prototype code, to be removed
#include <map>
#include <vector>

namespace meshopt
{

static const bool kMergeScoreExternal = false;
static const bool kMergeScoreSmallest = false;
static const bool kSortExternal = false;

struct ClusterAdjacency
{
	unsigned int* offsets;
	unsigned int* clusters;
	unsigned int* shared;
};

static void buildClusterAdjacency(ClusterAdjacency& adjacency, const unsigned int* cluster_indices, const unsigned int* cluster_offsets, size_t cluster_count, unsigned char* used, size_t vertex_count, meshopt_Allocator& allocator)
{
	unsigned int* ref_offsets = allocator.allocate<unsigned int>(vertex_count + 1);

	// compute number of clusters referenced by each vertex
	memset(ref_offsets, 0, vertex_count * sizeof(unsigned int));

	for (size_t i = 0; i < cluster_count; ++i)
	{
		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
		{
			unsigned int v = cluster_indices[j];
			assert(v < vertex_count);

			ref_offsets[v] += 1 - used[v];
			used[v] = 1;
		}

		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
			used[cluster_indices[j]] = 0;
	}

	// compute (worst-case) number of adjacent clusters for each cluster
	size_t total_adjacency = 0;

	for (size_t i = 0; i < cluster_count; ++i)
	{
		size_t count = 0;
		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
		{
			unsigned int v = cluster_indices[j];
			assert(v < vertex_count);

			// worst case is every vertex has a disjoint cluster list
			count += used[v] ? 0 : ref_offsets[v] - 1;
			used[v] = 1;
		}

		// ... but only every other cluster can be adjacent in the end
		total_adjacency += count < cluster_count - 1 ? count : cluster_count - 1;

		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
			used[cluster_indices[j]] = 0;
	}

	// convert ref counts to offsets
	size_t total_refs = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		size_t count = ref_offsets[i];
		ref_offsets[i] = unsigned(total_refs);
		total_refs += count;
	}

	unsigned int* ref_data = allocator.allocate<unsigned int>(total_refs);

	// fill cluster refs for each vertex
	for (size_t i = 0; i < cluster_count; ++i)
	{
		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
		{
			unsigned int v = cluster_indices[j];
			assert(v < vertex_count);

			if (used[v])
				continue;

			ref_data[ref_offsets[v]++] = unsigned(i);
			used[v] = 1;
		}

		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
			used[cluster_indices[j]] = 0;
	}

	// after the previous pass, ref_offsets contain the end of the data for each vertex; shift it forward to get the start
	memmove(ref_offsets + 1, ref_offsets, vertex_count * sizeof(unsigned int));
	ref_offsets[0] = 0;

	// we can now allocate adjacency buffers
	adjacency.offsets = allocator.allocate<unsigned int>(cluster_count + 1);
	adjacency.clusters = allocator.allocate<unsigned int>(total_adjacency);
	adjacency.shared = allocator.allocate<unsigned int>(total_adjacency);

	// fill cluster adjacency for each cluster...
	adjacency.offsets[0] = 0;

	for (size_t i = 0; i < cluster_count; ++i)
	{
		unsigned int* adj = adjacency.clusters + adjacency.offsets[i];
		unsigned int* shd = adjacency.shared + adjacency.offsets[i];
		size_t count = 0;

		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
		{
			unsigned int v = cluster_indices[j];
			assert(v < vertex_count);

			if (used[v])
				continue;

			// merge the entire cluster list of each vertex into current list
			for (size_t k = ref_offsets[v]; k < ref_offsets[v + 1]; ++k)
			{
				unsigned int c = ref_data[k];
				assert(c < cluster_count);

				if (c == unsigned(i))
					continue;

				// if the cluster is already in the list, increment the shared count
				bool found = false;
				for (size_t l = 0; l < count; ++l)
					if (adj[l] == c)
					{
						found = true;
						shd[l]++;
						break;
					}

				// .. or append a new cluster
				if (!found)
				{
					adj[count] = c;
					shd[count] = 1;
					count++;
				}
			}

			used[v] = 1;
		}

		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
			used[cluster_indices[j]] = 0;

		// mark the end of the adjacency list; the next cluster will start there as well
		adjacency.offsets[i + 1] = adjacency.offsets[i] + unsigned(count);
	}
}

// TOOD part of prototype code, to be removed
// TODO: valence[] is only used as a seen[] buffer
static unsigned int countTotal(const std::vector<int>& group, const unsigned int* cluster_indices, const unsigned int* cluster_offsets, std::vector<unsigned int>& valence)
{
	unsigned int total = 0;

	for (size_t i = 0; i < group.size(); ++i)
	{
		for (size_t j = cluster_offsets[group[i]]; j < cluster_offsets[group[i] + 1]; ++j)
		{
			total += 1 - (valence[cluster_indices[j]] >> 31);
			valence[cluster_indices[j]] |= 1u << 31;
		}
	}

	for (size_t i = 0; i < group.size(); ++i)
	{
		for (size_t j = cluster_offsets[group[i]]; j < cluster_offsets[group[i] + 1]; ++j)
			valence[cluster_indices[j]] &= ~(1u << 31);
	}

	return total;
}

static unsigned int countShared(const std::vector<int>& group1, const std::vector<int>& group2, const ClusterAdjacency& adjacency)
{
	unsigned int total = 0;

	for (size_t i1 = 0; i1 < group1.size(); ++i1)
	{
		unsigned int c1 = group1[i1];
		for (size_t i2 = 0; i2 < group2.size(); ++i2)
		{
			unsigned int c2 = group2[i2];

			for (unsigned int adj = adjacency.offsets[c1]; adj < adjacency.offsets[c1 + 1]; ++adj)
			{
				if (adjacency.clusters[adj] == c2)
				{
					total += adjacency.shared[adj];
					break;
				}
			}
		}
	}

	return total;
}

static unsigned int countExternal(const std::vector<int>& group1, const std::vector<int>& group2, const unsigned int* cluster_indices, const unsigned int* cluster_offsets, std::vector<unsigned int>& valence)
{
	unsigned int total = 0;

	for (size_t i = 0; i < group1.size(); ++i)
	{
		for (size_t j = cluster_offsets[group1[i]]; j < cluster_offsets[group1[i] + 1]; ++j)
			valence[cluster_indices[j]]--;
	}

	for (size_t i = 0; i < group2.size(); ++i)
	{
		for (size_t j = cluster_offsets[group2[i]]; j < cluster_offsets[group2[i] + 1]; ++j)
			valence[cluster_indices[j]]--;
	}

	for (size_t i = 0; i < group1.size(); ++i)
	{
		for (size_t j = cluster_offsets[group1[i]]; j < cluster_offsets[group1[i] + 1]; ++j)
			total += valence[cluster_indices[j]] != 0;
	}

	for (size_t i = 0; i < group2.size(); ++i)
	{
		for (size_t j = cluster_offsets[group2[i]]; j < cluster_offsets[group2[i] + 1]; ++j)
			total += valence[cluster_indices[j]] != 0;
	}

	for (size_t i = 0; i < group1.size(); ++i)
	{
		for (size_t j = cluster_offsets[group1[i]]; j < cluster_offsets[group1[i] + 1]; ++j)
			valence[cluster_indices[j]]++;
	}

	for (size_t i = 0; i < group2.size(); ++i)
	{
		for (size_t j = cluster_offsets[group2[i]]; j < cluster_offsets[group2[i] + 1]; ++j)
			valence[cluster_indices[j]]++;
	}

	return total;
}

static std::vector<std::vector<int> > partitionMerge(const ClusterAdjacency& adjacency, const unsigned int* cluster_indices, const unsigned int* cluster_offsets, size_t cluster_count, size_t vertex_count, size_t target_group_size, size_t max_group_size)
{
	std::vector<std::vector<int> > result;

	std::vector<unsigned int> valence(vertex_count);
	for (size_t i = 0; i < cluster_count; ++i)
	{
		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
			valence[cluster_indices[j]]++;
	}

	// Initially, create a singleton group for each cluster, stored as multimap sorted by external count
	std::multimap<unsigned int, std::vector<int> > groups;
	std::vector<std::multimap<unsigned int, std::vector<int> >::iterator> part(cluster_count, groups.end());

	for (size_t i = 0; i < cluster_count; ++i)
	{
		std::vector<int> group;
		group.push_back(i);
		unsigned int ext = kSortExternal ? countExternal(group, std::vector<int>(), cluster_indices, cluster_offsets, valence) : countTotal(group, cluster_indices, cluster_offsets, valence);
		std::multimap<unsigned int, std::vector<int> >::iterator it = groups.insert(std::make_pair(ext, group));
		part[i] = it;
	}

	// Iteratively take group with smallest number of external vertices, merge it with another group that minimizes external vertices
	while (!groups.empty())
	{
		std::vector<int> group = groups.begin()->second;
		groups.erase(groups.begin());

		for (size_t i = 0; i < group.size(); ++i)
			part[group[i]] = groups.end();

		if (group.size() >= target_group_size)
		{
			result.push_back(group);
		}
		else
		{
			std::multimap<unsigned int, std::vector<int> >::iterator bestGroup = groups.end();
			unsigned int bestScore = 0;

			for (size_t ci = 0; ci < group.size(); ++ci)
			{
				for (unsigned int adj = adjacency.offsets[group[ci]]; adj != adjacency.offsets[group[ci] + 1]; ++adj)
				{
					std::multimap<unsigned int, std::vector<int> >::iterator it = part[adjacency.clusters[adj]];
					if (it == groups.end())
						continue;

					if (group.size() + it->second.size() > max_group_size)
						continue;

					if (kMergeScoreSmallest && bestGroup != groups.end() && it->second.size() > bestGroup->second.size())
						continue;

					unsigned int score = kMergeScoreExternal ? ~countExternal(group, it->second, cluster_indices, cluster_offsets, valence) : countShared(group, it->second, adjacency);

					if (score > bestScore)
					{
						bestGroup = it;
						bestScore = score;
					}
				}
			}

			if (bestGroup == groups.end())
			{
				// we're stuck, emit as is
				result.push_back(group);
			}
			else
			{
				// combine and reinsert
				std::vector<int> combined;
				combined.reserve(combined.size() + bestGroup->second.size());
				combined.insert(combined.end(), group.begin(), group.end());
				combined.insert(combined.end(), bestGroup->second.begin(), bestGroup->second.end());

				unsigned int ext = kSortExternal ? countExternal(combined, std::vector<int>(), cluster_indices, cluster_offsets, valence) : countTotal(combined, cluster_indices, cluster_offsets, valence);

				for (size_t i = 0; i < bestGroup->second.size(); ++i)
					part[bestGroup->second[i]] = groups.end();

				groups.erase(bestGroup);
				std::multimap<unsigned int, std::vector<int> >::iterator it = groups.insert(std::make_pair(ext, combined));

				for (size_t i = 0; i < combined.size(); ++i)
					part[combined[i]] = it;
			}
		}
	}

	return result;
}

} // namespace meshopt

size_t meshopt_partitionClusters(unsigned int* destination, const unsigned int* cluster_indices, size_t total_index_count, const unsigned int* cluster_index_counts, size_t cluster_count, size_t vertex_count, size_t target_partition_size)
{
	using namespace meshopt;

	assert(target_partition_size > 0);

	meshopt_Allocator allocator;

	unsigned char* used = allocator.allocate<unsigned char>(vertex_count);
	memset(used, 0, vertex_count);

	// build cluster index offsets as a prefix sum
	unsigned int* cluster_offsets = allocator.allocate<unsigned int>(cluster_count + 1);
	unsigned int cluster_nextoffset = 0;

	for (size_t i = 0; i < cluster_count; ++i)
	{
		cluster_offsets[i] = cluster_nextoffset;
		cluster_nextoffset += cluster_index_counts[i];
	}

	assert(cluster_nextoffset == total_index_count);
	cluster_offsets[cluster_count] = total_index_count;

	// build cluster adjacency along with edge weights (shared vertex count)
	ClusterAdjacency adjacency = {};
	buildClusterAdjacency(adjacency, cluster_indices, cluster_offsets, cluster_count, used, vertex_count, allocator);

	std::vector<std::vector<int> > groups = partitionMerge(adjacency, cluster_indices, cluster_offsets, cluster_count, vertex_count, target_partition_size, target_partition_size + target_partition_size / 2);

	for (size_t i = 0; i < groups.size(); ++i)
		for (size_t j = 0; j < groups[i].size(); ++j)
			destination[groups[i][j]] = unsigned(i);

	return groups.size();
}
