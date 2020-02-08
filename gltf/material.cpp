// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <string.h>

static bool areTextureViewsEqual(const cgltf_texture_view& lhs, const cgltf_texture_view& rhs)
{
	if (lhs.has_transform != rhs.has_transform)
		return false;

	if (lhs.has_transform)
	{
		const cgltf_texture_transform& lt = lhs.transform;
		const cgltf_texture_transform& rt = rhs.transform;

		if (memcmp(lt.offset, rt.offset, sizeof(cgltf_float) * 2) != 0)
			return false;

		if (lt.rotation != rt.rotation)
			return false;

		if (memcmp(lt.scale, rt.scale, sizeof(cgltf_float) * 2) != 0)
			return false;

		if (lt.texcoord != rt.texcoord)
			return false;
	}

	if (lhs.texture != rhs.texture)
		return false;

	if (lhs.texcoord != rhs.texcoord)
		return false;

	if (lhs.scale != rhs.scale)
		return false;

	return true;
}

static bool areMaterialsEqual(const cgltf_material& lhs, const cgltf_material& rhs)
{
	if (lhs.has_pbr_metallic_roughness != rhs.has_pbr_metallic_roughness)
		return false;

	if (lhs.has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness& lpbr = lhs.pbr_metallic_roughness;
		const cgltf_pbr_metallic_roughness& rpbr = rhs.pbr_metallic_roughness;

		if (!areTextureViewsEqual(lpbr.base_color_texture, rpbr.base_color_texture))
			return false;

		if (!areTextureViewsEqual(lpbr.metallic_roughness_texture, rpbr.metallic_roughness_texture))
			return false;

		if (memcmp(lpbr.base_color_factor, rpbr.base_color_factor, sizeof(cgltf_float) * 4) != 0)
			return false;

		if (lpbr.metallic_factor != rpbr.metallic_factor)
			return false;

		if (lpbr.roughness_factor != rpbr.roughness_factor)
			return false;
	}

	if (lhs.has_pbr_specular_glossiness != rhs.has_pbr_specular_glossiness)
		return false;

	if (lhs.has_pbr_specular_glossiness)
	{
		const cgltf_pbr_specular_glossiness& lpbr = lhs.pbr_specular_glossiness;
		const cgltf_pbr_specular_glossiness& rpbr = rhs.pbr_specular_glossiness;

		if (!areTextureViewsEqual(lpbr.diffuse_texture, rpbr.diffuse_texture))
			return false;

		if (!areTextureViewsEqual(lpbr.specular_glossiness_texture, rpbr.specular_glossiness_texture))
			return false;

		if (memcmp(lpbr.diffuse_factor, rpbr.diffuse_factor, sizeof(cgltf_float) * 4) != 0)
			return false;

		if (memcmp(lpbr.specular_factor, rpbr.specular_factor, sizeof(cgltf_float) * 3) != 0)
			return false;

		if (lpbr.glossiness_factor != rpbr.glossiness_factor)
			return false;
	}

	if (!areTextureViewsEqual(lhs.normal_texture, rhs.normal_texture))
		return false;

	if (!areTextureViewsEqual(lhs.occlusion_texture, rhs.occlusion_texture))
		return false;

	if (!areTextureViewsEqual(lhs.emissive_texture, rhs.emissive_texture))
		return false;

	if (memcmp(lhs.emissive_factor, rhs.emissive_factor, sizeof(cgltf_float) * 3) != 0)
		return false;

	if (lhs.alpha_mode != rhs.alpha_mode)
		return false;

	if (lhs.alpha_cutoff != rhs.alpha_cutoff)
		return false;

	if (lhs.double_sided != rhs.double_sided)
		return false;

	if (lhs.unlit != rhs.unlit)
		return false;

	return true;
}

void mergeMeshMaterials(cgltf_data* data, std::vector<Mesh>& meshes)
{
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		if (!mesh.material)
			continue;

		for (int j = 0; j < mesh.material - data->materials; ++j)
		{
			if (areMaterialsEqual(*mesh.material, data->materials[j]))
			{
				mesh.material = &data->materials[j];
				break;
			}
		}
	}
}

void markNeededMaterials(cgltf_data* data, std::vector<MaterialInfo>& materials, const std::vector<Mesh>& meshes)
{
	// mark all used materials as kept
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		if (mesh.material)
		{
			MaterialInfo& mi = materials[mesh.material - data->materials];

			mi.keep = true;
		}
	}
}

bool usesTextureSet(const cgltf_material& material, int set)
{
	if (material.has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;

		if (pbr.base_color_texture.texture && pbr.base_color_texture.texcoord == set)
			return true;

		if (pbr.metallic_roughness_texture.texture && pbr.metallic_roughness_texture.texcoord == set)
			return true;
	}

	if (material.has_pbr_specular_glossiness)
	{
		const cgltf_pbr_specular_glossiness& pbr = material.pbr_specular_glossiness;

		if (pbr.diffuse_texture.texture && pbr.diffuse_texture.texcoord == set)
			return true;

		if (pbr.specular_glossiness_texture.texture && pbr.specular_glossiness_texture.texcoord == set)
			return true;
	}

	if (material.normal_texture.texture && material.normal_texture.texcoord == set)
		return true;

	if (material.occlusion_texture.texture && material.occlusion_texture.texcoord == set)
		return true;

	if (material.emissive_texture.texture && material.emissive_texture.texcoord == set)
		return true;

	return false;
}


