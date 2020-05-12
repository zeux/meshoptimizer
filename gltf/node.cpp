// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

void markAnimated(cgltf_data* data, std::vector<NodeInfo>& nodes, const std::vector<Animation>& animations)
{
	for (size_t i = 0; i < animations.size(); ++i)
	{
		const Animation& animation = animations[i];

		for (size_t j = 0; j < animation.tracks.size(); ++j)
		{
			const Track& track = animation.tracks[j];

			// mark nodes that have animation tracks that change their base transform as animated
			if (!track.dummy)
			{
				NodeInfo& ni = nodes[track.node - data->nodes];

				ni.animated_paths |= (1 << track.path);
			}
		}
	}

	for (size_t i = 0; i < data->nodes_count; ++i)
	{
		NodeInfo& ni = nodes[i];

		for (cgltf_node* node = &data->nodes[i]; node; node = node->parent)
			ni.animated |= nodes[node - data->nodes].animated_paths != 0;
	}
}

void markNeededNodes(cgltf_data* data, std::vector<NodeInfo>& nodes, const std::vector<Mesh>& meshes, const std::vector<Animation>& animations, const Settings& settings)
{
	// mark all joints as kept
	for (size_t i = 0; i < data->skins_count; ++i)
	{
		const cgltf_skin& skin = data->skins[i];

		// for now we keep all joints directly referenced by the skin and the entire ancestry tree; we keep names for joints as well
		for (size_t j = 0; j < skin.joints_count; ++j)
		{
			NodeInfo& ni = nodes[skin.joints[j] - data->nodes];

			ni.keep = true;
		}
	}

	// mark all animated nodes as kept
	for (size_t i = 0; i < animations.size(); ++i)
	{
		const Animation& animation = animations[i];

		for (size_t j = 0; j < animation.tracks.size(); ++j)
		{
			const Track& track = animation.tracks[j];

			if (settings.anim_const || !track.dummy)
			{
				NodeInfo& ni = nodes[track.node - data->nodes];

				ni.keep = true;
			}
		}
	}

	// mark all mesh nodes as kept
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		for (size_t j = 0; j < mesh.nodes.size(); ++j)
		{
			NodeInfo& ni = nodes[mesh.nodes[j] - data->nodes];

			ni.keep = true;
		}
	}

	// mark all light/camera nodes as kept
	for (size_t i = 0; i < data->nodes_count; ++i)
	{
		const cgltf_node& node = data->nodes[i];

		if (node.light || node.camera)
		{
			nodes[i].keep = true;
		}
	}

	// mark all named nodes as needed (if -kn is specified)
	if (settings.keep_named)
	{
		for (size_t i = 0; i < data->nodes_count; ++i)
		{
			const cgltf_node& node = data->nodes[i];

			if (node.name && *node.name)
			{
				nodes[i].keep = true;
			}
		}
	}
}

void remapNodes(cgltf_data* data, std::vector<NodeInfo>& nodes, size_t& node_offset)
{
	// to keep a node, we currently need to keep the entire ancestry chain
	for (size_t i = 0; i < data->nodes_count; ++i)
	{
		if (!nodes[i].keep)
			continue;

		for (cgltf_node* node = &data->nodes[i]; node; node = node->parent)
			nodes[node - data->nodes].keep = true;
	}

	// generate sequential indices for all nodes; they aren't sorted topologically
	for (size_t i = 0; i < data->nodes_count; ++i)
	{
		NodeInfo& ni = nodes[i];

		if (ni.keep)
		{
			ni.remap = int(node_offset);

			node_offset++;
		}
	}
}
