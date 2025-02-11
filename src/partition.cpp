// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>

// TOOD part of prototype code, to be removed
#include <map>
#include <vector>

namespace meshopt
{

static const bool kMergeScoreExternal = false;
static const bool kMergeScoreSmallest = false;
static const bool kSortExternal = false;

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

static unsigned int countShared(const std::vector<int>& group1, const std::vector<int>& group2, const std::vector<std::map<size_t, unsigned int> >& adjacency)
{
	unsigned int total = 0;

	for (size_t i1 = 0; i1 < group1.size(); ++i1)
	{
		const std::map<size_t, unsigned int>& adj = adjacency[group1[i1]];

		for (size_t i2 = 0; i2 < group2.size(); ++i2)
		{
			std::map<size_t, unsigned int>::const_iterator it = adj.find(group2[i2]);
			if (it != adj.end())
				total += it->second;
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

static std::vector<std::vector<int> > partitionMerge(const unsigned int* cluster_indices, const unsigned int* cluster_offsets, size_t cluster_count, size_t vertex_count, size_t target_group_size, size_t max_group_size)
{
	std::vector<std::vector<int> > result;

	// Build index -> clusters mapping
	std::map<unsigned int, std::vector<size_t> > indexToClusters;
	for (size_t i = 0; i < cluster_count; ++i)
	{
		for (size_t j = cluster_offsets[i]; j < cluster_offsets[i + 1]; ++j)
		{
			std::vector<size_t>& list = indexToClusters[cluster_indices[j]];
			if (list.empty() || list.back() != i)
				list.push_back(i);
		}
	}

	// Build adjacency information
	std::vector<std::map<size_t, unsigned int> > adjacency(cluster_count);

	// For each remapped index, increment shared count for each pair of clusters that contains it
	for (std::map<unsigned int, std::vector<size_t> >::const_iterator it = indexToClusters.begin(); it != indexToClusters.end(); ++it)
	{
		const std::vector<size_t>& clusterList = it->second;

		for (size_t i = 0; i < clusterList.size(); ++i)
		{
			for (size_t j = i + 1; j < clusterList.size(); ++j)
			{
				size_t c1 = clusterList[i];
				size_t c2 = clusterList[j];
				adjacency[c1][c2]++;
				adjacency[c2][c1]++;
			}
		}
	}

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
				for (std::map<size_t, unsigned int>::iterator adj = adjacency[group[ci]].begin(); adj != adjacency[group[ci]].end(); ++adj)
				{
					std::multimap<unsigned int, std::vector<int> >::iterator it = part[adj->first];
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

	std::vector<std::vector<int> > groups = partitionMerge(cluster_indices, cluster_offsets, cluster_count, vertex_count, target_partition_size, target_partition_size + target_partition_size / 2);

	for (size_t i = 0; i < groups.size(); ++i)
		for (size_t j = 0; j < groups[i].size(); ++j)
			destination[groups[i][j]] = unsigned(i);

	return groups.size();
}
