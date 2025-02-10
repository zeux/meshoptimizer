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
struct Cluster
{
	const unsigned int* indices;
	size_t index_count;
};

// TODO: valence[] is only used as a seen[] buffer
static unsigned int countTotal(const std::vector<int>& group, const std::vector<Cluster>& clusters, std::vector<unsigned int>& valence)
{
	unsigned int total = 0;

	for (size_t i = 0; i < group.size(); ++i)
	{
		const Cluster& cluster = clusters[group[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
		{
			total += 1 - (valence[cluster.indices[j]] >> 31);
			valence[cluster.indices[j]] |= 1u << 31;
		}
	}

	for (size_t i = 0; i < group.size(); ++i)
	{
		const Cluster& cluster = clusters[group[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
			valence[cluster.indices[j]] &= ~(1u << 31);
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

static unsigned int countExternal(const std::vector<int>& group1, const std::vector<int>& group2, const std::vector<Cluster>& clusters, std::vector<unsigned int>& valence)
{
	unsigned int total = 0;

	for (size_t i = 0; i < group1.size(); ++i)
	{
		const Cluster& cluster = clusters[group1[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
			valence[cluster.indices[j]]--;
	}

	for (size_t i = 0; i < group2.size(); ++i)
	{
		const Cluster& cluster = clusters[group2[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
			valence[cluster.indices[j]]--;
	}

	for (size_t i = 0; i < group1.size(); ++i)
	{
		const Cluster& cluster = clusters[group1[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
			total += valence[cluster.indices[j]] != 0;
	}

	for (size_t i = 0; i < group2.size(); ++i)
	{
		const Cluster& cluster = clusters[group2[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
			total += valence[cluster.indices[j]] != 0;
	}

	for (size_t i = 0; i < group1.size(); ++i)
	{
		const Cluster& cluster = clusters[group1[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
			valence[cluster.indices[j]]++;
	}

	for (size_t i = 0; i < group2.size(); ++i)
	{
		const Cluster& cluster = clusters[group2[i]];

		for (size_t j = 0; j < cluster.index_count; ++j)
			valence[cluster.indices[j]]++;
	}

	return total;
}

static std::vector<std::vector<int> > partitionMerge(const std::vector<Cluster>& clusters, size_t vertex_count)
{
	const size_t GROUP_SIZE = 8;
	const size_t MAX_GROUP_SIZE = 12;

	std::vector<std::vector<int> > result;
	if (clusters.empty())
		return result;

	// Build index -> clusters mapping
	std::map<unsigned int, std::vector<size_t> > indexToClusters;
	for (size_t i = 0; i < clusters.size(); ++i)
	{
		const Cluster& cluster = clusters[i];
		for (size_t j = 0; j < cluster.index_count; ++j)
			indexToClusters[cluster.indices[j]].push_back(i);
	}

	// Build adjacency information
	std::vector<std::map<size_t, unsigned int> > adjacency(clusters.size());

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
	for (size_t i = 0; i < clusters.size(); ++i)
	{
		const Cluster& cluster = clusters[i];

		for (size_t j = 0; j < cluster.index_count; ++j)
			valence[cluster.indices[j]]++;
	}

	// Initially, create a singleton group for each cluster, stored as multimap sorted by external count
	std::multimap<unsigned int, std::vector<int> > groups;
	std::vector<std::multimap<unsigned int, std::vector<int> >::iterator> part(clusters.size(), groups.end());

	for (size_t i = 0; i < clusters.size(); ++i)
	{
		std::vector<int> group;
		group.push_back(i);
		unsigned int ext = kSortExternal ? countExternal(group, std::vector<int>(), clusters, valence) : countTotal(group, clusters, valence);
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

		if (group.size() >= GROUP_SIZE)
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

					if (group.size() + it->second.size() > MAX_GROUP_SIZE)
						continue;

					if (kMergeScoreSmallest && bestGroup != groups.end() && it->second.size() > bestGroup->second.size())
						continue;

					unsigned int score = kMergeScoreExternal ? ~countExternal(group, it->second, clusters, valence) : countShared(group, it->second, adjacency);

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

				unsigned int ext = kSortExternal ? countExternal(combined, std::vector<int>(), clusters, valence) : countTotal(combined, clusters, valence);

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

	(void)target_partition_size;

	meshopt_Allocator allocator;

	std::vector<Cluster> clusters(cluster_count);
	const unsigned int* cluster_offset = cluster_indices;

	for (size_t i = 0; i < cluster_count; ++i)
	{
		clusters[i].indices = cluster_offset;
		clusters[i].index_count = cluster_index_counts[i];
		cluster_offset += cluster_index_counts[i];
	}

	assert(cluster_offset == cluster_indices + total_index_count);
	(void)total_index_count;

	std::vector<std::vector<int> > groups = partitionMerge(clusters, vertex_count);

	for (size_t i = 0; i < groups.size(); ++i)
		for (size_t j = 0; j < groups[i].size(); ++j)
			destination[groups[i][j]] = unsigned(i);

	return groups.size();
}
