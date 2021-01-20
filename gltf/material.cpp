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

static bool areExtrasEqual(cgltf_data* data, const cgltf_extras& lhs, const cgltf_extras& rhs)
{
	if (lhs.end_offset - lhs.start_offset != rhs.end_offset - rhs.start_offset)
		return false;

	if (memcmp(data->json + lhs.start_offset, data->json + rhs.start_offset, lhs.end_offset - lhs.start_offset) != 0)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_pbr_metallic_roughness& lhs, const cgltf_pbr_metallic_roughness& rhs)
{
	if (!areTextureViewsEqual(lhs.base_color_texture, rhs.base_color_texture))
		return false;

	if (!areTextureViewsEqual(lhs.metallic_roughness_texture, rhs.metallic_roughness_texture))
		return false;

	if (memcmp(lhs.base_color_factor, rhs.base_color_factor, sizeof(cgltf_float) * 4) != 0)
		return false;

	if (lhs.metallic_factor != rhs.metallic_factor)
		return false;

	if (lhs.roughness_factor != rhs.roughness_factor)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_pbr_specular_glossiness& lhs, const cgltf_pbr_specular_glossiness& rhs)
{
	if (!areTextureViewsEqual(lhs.diffuse_texture, rhs.diffuse_texture))
		return false;

	if (!areTextureViewsEqual(lhs.specular_glossiness_texture, rhs.specular_glossiness_texture))
		return false;

	if (memcmp(lhs.diffuse_factor, rhs.diffuse_factor, sizeof(cgltf_float) * 4) != 0)
		return false;

	if (memcmp(lhs.specular_factor, rhs.specular_factor, sizeof(cgltf_float) * 3) != 0)
		return false;

	if (lhs.glossiness_factor != rhs.glossiness_factor)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_clearcoat& lhs, const cgltf_clearcoat& rhs)
{
	if (!areTextureViewsEqual(lhs.clearcoat_texture, rhs.clearcoat_texture))
		return false;

	if (!areTextureViewsEqual(lhs.clearcoat_roughness_texture, rhs.clearcoat_roughness_texture))
		return false;

	if (!areTextureViewsEqual(lhs.clearcoat_normal_texture, rhs.clearcoat_normal_texture))
		return false;

	if (lhs.clearcoat_factor != rhs.clearcoat_factor)
		return false;

	if (lhs.clearcoat_roughness_factor != rhs.clearcoat_roughness_factor)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_transmission& lhs, const cgltf_transmission& rhs)
{
	if (!areTextureViewsEqual(lhs.transmission_texture, rhs.transmission_texture))
		return false;

	if (lhs.transmission_factor != rhs.transmission_factor)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_ior& lhs, const cgltf_ior& rhs)
{
	if (lhs.ior != rhs.ior)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_specular& lhs, const cgltf_specular& rhs)
{
	if (!areTextureViewsEqual(lhs.specular_texture, rhs.specular_texture))
		return false;

	if (memcmp(lhs.specular_color_factor, rhs.specular_color_factor, sizeof(cgltf_float) * 3) != 0)
		return false;

	if (lhs.specular_factor != rhs.specular_factor)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_sheen& lhs, const cgltf_sheen& rhs)
{
	if (!areTextureViewsEqual(lhs.sheen_color_texture, rhs.sheen_color_texture))
		return false;

	if (memcmp(lhs.sheen_color_factor, rhs.sheen_color_factor, sizeof(cgltf_float) * 3) != 0)
		return false;

	if (!areTextureViewsEqual(lhs.sheen_roughness_texture, rhs.sheen_roughness_texture))
		return false;

	if (lhs.sheen_roughness_factor != rhs.sheen_roughness_factor)
		return false;

	return true;
}

static bool areMaterialsEqual(cgltf_data* data, const cgltf_material& lhs, const cgltf_material& rhs, const Settings& settings)
{
	if (lhs.has_pbr_metallic_roughness != rhs.has_pbr_metallic_roughness)
		return false;

	if (lhs.has_pbr_metallic_roughness && !areMaterialComponentsEqual(lhs.pbr_metallic_roughness, rhs.pbr_metallic_roughness))
		return false;

	if (lhs.has_pbr_specular_glossiness != rhs.has_pbr_specular_glossiness)
		return false;

	if (lhs.has_pbr_specular_glossiness && !areMaterialComponentsEqual(lhs.pbr_specular_glossiness, rhs.pbr_specular_glossiness))
		return false;

	if (lhs.has_clearcoat != rhs.has_clearcoat)
		return false;

	if (lhs.has_clearcoat && !areMaterialComponentsEqual(lhs.clearcoat, rhs.clearcoat))
		return false;

	if (lhs.has_transmission != rhs.has_transmission)
		return false;

	if (lhs.has_transmission && !areMaterialComponentsEqual(lhs.transmission, rhs.transmission))
		return false;

	if (lhs.has_ior != rhs.has_ior)
		return false;

	if (lhs.has_ior && !areMaterialComponentsEqual(lhs.ior, rhs.ior))
		return false;

	if (lhs.has_specular != rhs.has_specular)
		return false;

	if (lhs.has_specular && !areMaterialComponentsEqual(lhs.specular, rhs.specular))
		return false;

	if (lhs.has_sheen != rhs.has_sheen)
		return false;

	if (lhs.has_sheen && !areMaterialComponentsEqual(lhs.sheen, rhs.sheen))
		return false;

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

	if (settings.keep_extras && !areExtrasEqual(data, lhs.extras, rhs.extras))
		return false;

	return true;
}

void mergeMeshMaterials(cgltf_data* data, std::vector<Mesh>& meshes, const Settings& settings)
{
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		if (!mesh.material)
			continue;

		if (settings.keep_materials && mesh.material->name && *mesh.material->name)
			continue;

		for (int j = 0; j < mesh.material - data->materials; ++j)
		{
			if (settings.keep_materials && data->materials[j].name && *data->materials[j].name)
				continue;

			if (areMaterialsEqual(data, *mesh.material, data->materials[j], settings))
			{
				mesh.material = &data->materials[j];
				break;
			}
		}
	}
}

void markNeededMaterials(cgltf_data* data, std::vector<MaterialInfo>& materials, const std::vector<Mesh>& meshes, const Settings& settings)
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

	// mark all named materials as kept if requested
	if (settings.keep_materials)
	{
		for (size_t i = 0; i < data->materials_count; ++i)
		{
			cgltf_material& material = data->materials[i];

			if (material.name && *material.name)
			{
				materials[i].keep = true;
			}
		}
	}
}

template <typename Pred>
static bool materialHasProperty(const cgltf_material& material, Pred pred)
{
	if (material.has_pbr_metallic_roughness)
	{
		if (pred(material.pbr_metallic_roughness.base_color_texture))
			return true;

		if (pred(material.pbr_metallic_roughness.metallic_roughness_texture))
			return true;
	}

	if (material.has_pbr_specular_glossiness)
	{
		if (pred(material.pbr_specular_glossiness.diffuse_texture))
			return true;

		if (pred(material.pbr_specular_glossiness.specular_glossiness_texture))
			return true;
	}

	if (material.has_clearcoat)
	{
		if (pred(material.clearcoat.clearcoat_texture))
			return true;

		if (pred(material.clearcoat.clearcoat_roughness_texture))
			return true;

		if (pred(material.clearcoat.clearcoat_normal_texture))
			return true;
	}

	if (material.has_transmission)
	{
		if (pred(material.transmission.transmission_texture))
			return true;
	}

	if (material.has_specular)
	{
		if (pred(material.specular.specular_texture))
			return true;
	}

	if (material.has_sheen)
	{
		if (pred(material.sheen.sheen_color_texture))
			return true;

		if (pred(material.sheen.sheen_roughness_texture))
			return true;
	}

	if (pred(material.normal_texture))
		return true;

	if (pred(material.occlusion_texture))
		return true;

	if (pred(material.emissive_texture))
		return true;

	return false;
}

struct UsesTextureSet
{
	int set;

	bool operator()(const cgltf_texture_view& view) const
	{
		return view.texture && view.texcoord == set;
	}
};

bool usesTextureSet(const cgltf_material& material, int set)
{
	UsesTextureSet pred = {set};

	return materialHasProperty(material, pred);
}

struct UsesTextureTransform
{
	bool operator()(const cgltf_texture_view& view) const
	{
		return view.has_transform;
	}
};

bool usesTextureTransform(const cgltf_material& material)
{
	return materialHasProperty(material, UsesTextureTransform());
}
