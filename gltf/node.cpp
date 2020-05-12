// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <math.h>
#include <string.h>

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

void decomposeTransform(float translation[3], float rotation[4], float scale[3], const float* transform)
{
	float m[4][4] = {};
	memcpy(m, transform, 16 * sizeof(float));

	translation[0] = m[3][0];
	translation[1] = m[3][1];
	translation[2] = m[3][2];

	float det =
	m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
	m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
	m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

	float sign = (det < 0.f) ? -1.f : 1.f;

	scale[0] = sqrtf(m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0]) * sign;
	scale[1] = sqrtf(m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1]) * sign;
	scale[2] = sqrtf(m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2]) * sign;

	float rsx = (scale[0] == 0.f) ? 0.f : 1.f / scale[0];
	float rsy = (scale[1] == 0.f) ? 0.f : 1.f / scale[1];
	float rsz = (scale[2] == 0.f) ? 0.f : 1.f / scale[2];

	float q00 = m[0][0] * rsx, q10 = m[1][0] * rsx, q20 = m[2][0] * rsx;
	float q01 = m[0][1] * rsy, q11 = m[1][1] * rsy, q21 = m[2][1] * rsy;
	float q02 = m[0][2] * rsz, q12 = m[1][2] * rsz, q22 = m[2][2] * rsz;

	float qt = 1.f;

	if (q22 < 0)
	{
		if (q00 > q11)
		{
			rotation[0] = qt = 1.f + q00 - q11 - q22;
			rotation[1] = q01+q10;
			rotation[2] = q20+q02;
			rotation[3] = q12-q21;
		}
		else
		{
			rotation[0] = q01+q10;
			rotation[1] = qt = 1.f - q00 + q11 - q22;
			rotation[2] = q12+q21;
			rotation[3] = q20-q02;
		}
	}
	else
	{
		if (q00 < -q11)
		{
			rotation[0] = q20+q02;
			rotation[1] = q12+q21;
			rotation[2] = qt = 1.f - q00 - q11 + q22;
			rotation[3] = q01-q10;
		}
		else
		{
			rotation[0] = q12-q21;
			rotation[1] = q20-q02;
			rotation[2] = q01-q10;
			rotation[3] = qt = 1.f + q00 + q11 + q22;
		}
	}

	float qs = 0.5f / sqrtf(qt);

	rotation[0] *= qs;
	rotation[1] *= qs;
	rotation[2] *= qs;
	rotation[3] *= qs;
}
