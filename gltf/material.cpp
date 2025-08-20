// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <string.h>

static bool areTexturesEqual(const cgltf_texture& lhs, const cgltf_texture& rhs)
{
	if (lhs.image != rhs.image)
		return false;

	if (lhs.sampler != rhs.sampler)
		return false;

	if (lhs.basisu_image != rhs.basisu_image)
		return false;

	if (lhs.webp_image != rhs.webp_image)
		return false;

	return true;
}

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

	if (lhs.texture != rhs.texture && (!lhs.texture || !rhs.texture || !areTexturesEqual(*lhs.texture, *rhs.texture)))
		return false;

	if (lhs.texcoord != rhs.texcoord)
		return false;

	if (lhs.scale != rhs.scale)
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

	if (!areTextureViewsEqual(lhs.specular_color_texture, rhs.specular_color_texture))
		return false;

	if (lhs.specular_factor != rhs.specular_factor)
		return false;

	if (memcmp(lhs.specular_color_factor, rhs.specular_color_factor, sizeof(cgltf_float) * 3) != 0)
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

static bool areMaterialComponentsEqual(const cgltf_volume& lhs, const cgltf_volume& rhs)
{
	if (!areTextureViewsEqual(lhs.thickness_texture, rhs.thickness_texture))
		return false;

	if (lhs.thickness_factor != rhs.thickness_factor)
		return false;

	if (memcmp(lhs.attenuation_color, rhs.attenuation_color, sizeof(cgltf_float) * 3) != 0)
		return false;

	if (lhs.attenuation_distance != rhs.attenuation_distance)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_emissive_strength& lhs, const cgltf_emissive_strength& rhs)
{
	if (lhs.emissive_strength != rhs.emissive_strength)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_iridescence& lhs, const cgltf_iridescence& rhs)
{
	if (lhs.iridescence_factor != rhs.iridescence_factor)
		return false;

	if (!areTextureViewsEqual(lhs.iridescence_texture, rhs.iridescence_texture))
		return false;

	if (lhs.iridescence_ior != rhs.iridescence_ior)
		return false;

	if (lhs.iridescence_thickness_min != rhs.iridescence_thickness_min)
		return false;

	if (lhs.iridescence_thickness_max != rhs.iridescence_thickness_max)
		return false;

	if (!areTextureViewsEqual(lhs.iridescence_thickness_texture, rhs.iridescence_thickness_texture))
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_anisotropy& lhs, const cgltf_anisotropy& rhs)
{
	if (lhs.anisotropy_strength != rhs.anisotropy_strength)
		return false;

	if (lhs.anisotropy_rotation != rhs.anisotropy_rotation)
		return false;

	if (!areTextureViewsEqual(lhs.anisotropy_texture, rhs.anisotropy_texture))
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_dispersion& lhs, const cgltf_dispersion& rhs)
{
	if (lhs.dispersion != rhs.dispersion)
		return false;

	return true;
}

static bool areMaterialComponentsEqual(const cgltf_diffuse_transmission& lhs, const cgltf_diffuse_transmission& rhs)
{
	if (lhs.diffuse_transmission_factor != rhs.diffuse_transmission_factor)
		return false;

	if (memcmp(lhs.diffuse_transmission_color_factor, rhs.diffuse_transmission_color_factor, sizeof(cgltf_float) * 3) != 0)
		return false;

	if (!areTextureViewsEqual(lhs.diffuse_transmission_texture, rhs.diffuse_transmission_texture))
		return false;

	if (!areTextureViewsEqual(lhs.diffuse_transmission_color_texture, rhs.diffuse_transmission_color_texture))
		return false;

	return true;
}

static bool areMaterialsEqual(const cgltf_material& lhs, const cgltf_material& rhs, const Settings& settings)
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

	if (lhs.has_volume != rhs.has_volume)
		return false;

	if (lhs.has_volume && !areMaterialComponentsEqual(lhs.volume, rhs.volume))
		return false;

	if (lhs.has_emissive_strength != rhs.has_emissive_strength)
		return false;

	if (lhs.has_emissive_strength && !areMaterialComponentsEqual(lhs.emissive_strength, rhs.emissive_strength))
		return false;

	if (lhs.has_iridescence != rhs.has_iridescence)
		return false;

	if (lhs.has_iridescence && !areMaterialComponentsEqual(lhs.iridescence, rhs.iridescence))
		return false;

	if (lhs.has_anisotropy != rhs.has_anisotropy)
		return false;

	if (lhs.has_anisotropy && !areMaterialComponentsEqual(lhs.anisotropy, rhs.anisotropy))
		return false;

	if (lhs.has_dispersion != rhs.has_dispersion)
		return false;

	if (lhs.has_dispersion && !areMaterialComponentsEqual(lhs.dispersion, rhs.dispersion))
		return false;

	if (lhs.has_diffuse_transmission != rhs.has_diffuse_transmission)
		return false;

	if (lhs.has_diffuse_transmission && !areMaterialComponentsEqual(lhs.diffuse_transmission, rhs.diffuse_transmission))
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

	if (settings.keep_extras && !areExtrasEqual(lhs.extras, rhs.extras))
		return false;

	return true;
}

void mergeMeshMaterials(cgltf_data* data, std::vector<Mesh>& meshes, const Settings& settings)
{
	std::vector<cgltf_material*> material_remap(data->materials_count);

	for (size_t i = 0; i < data->materials_count; ++i)
	{
		material_remap[i] = &data->materials[i];

		if (settings.keep_materials && data->materials[i].name && *data->materials[i].name)
			continue;

		assert(areMaterialsEqual(data->materials[i], data->materials[i], settings));

		for (size_t j = 0; j < i; ++j)
		{
			if (settings.keep_materials && data->materials[j].name && *data->materials[j].name)
				continue;

			if (areMaterialsEqual(data->materials[i], data->materials[j], settings))
			{
				material_remap[i] = &data->materials[j];
				break;
			}
		}
	}

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		if (mesh.material)
			mesh.material = material_remap[mesh.material - data->materials];

		for (size_t j = 0; j < mesh.variants.size(); ++j)
			mesh.variants[j].material = material_remap[mesh.variants[j].material - data->materials];
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

		for (size_t j = 0; j < mesh.variants.size(); ++j)
		{
			MaterialInfo& mi = materials[mesh.variants[j].material - data->materials];

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

bool hasValidTransform(const cgltf_texture_view& view)
{
	if (view.has_transform)
	{
		if (view.transform.offset[0] != 0.0f || view.transform.offset[1] != 0.0f ||
		    view.transform.scale[0] != 1.0f || view.transform.scale[1] != 1.0f ||
		    view.transform.rotation != 0.0f)
			return true;

		if (view.transform.has_texcoord && view.transform.texcoord != view.texcoord)
			return true;
	}

	return false;
}

static const cgltf_image* getTextureImage(const cgltf_texture* texture)
{
	if (texture && texture->image)
		return texture->image;

	if (texture && texture->basisu_image)
		return texture->basisu_image;

	if (texture && texture->webp_image)
		return texture->webp_image;

	return NULL;
}

static void analyzeMaterialTexture(const cgltf_texture_view& view, TextureKind kind, MaterialInfo& mi, cgltf_data* data, std::vector<TextureInfo>& textures, std::vector<ImageInfo>& images)
{
	mi.uses_texture_transform |= hasValidTransform(view);

	if (view.texture)
	{
		textures[view.texture - data->textures].keep = true;

		mi.texture_set_mask |= 1u << view.texcoord;
		mi.needs_tangents |= (kind == TextureKind_Normal);
	}

	if (const cgltf_image* image = getTextureImage(view.texture))
	{
		ImageInfo& info = images[image - data->images];

		if (info.kind == TextureKind_Generic)
			info.kind = kind;
		else if (info.kind > kind) // this is useful to keep color textures that have attrib data in alpha tagged as color
			info.kind = kind;

		info.normal_map |= (kind == TextureKind_Normal);
		info.srgb |= (kind == TextureKind_Color);
	}
}

static void analyzeMaterial(const cgltf_material& material, MaterialInfo& mi, cgltf_data* data, std::vector<TextureInfo>& textures, std::vector<ImageInfo>& images)
{
	if (material.has_pbr_metallic_roughness)
	{
		analyzeMaterialTexture(material.pbr_metallic_roughness.base_color_texture, TextureKind_Color, mi, data, textures, images);
		analyzeMaterialTexture(material.pbr_metallic_roughness.metallic_roughness_texture, TextureKind_Attrib, mi, data, textures, images);
	}

	if (material.has_pbr_specular_glossiness)
	{
		analyzeMaterialTexture(material.pbr_specular_glossiness.diffuse_texture, TextureKind_Color, mi, data, textures, images);
		analyzeMaterialTexture(material.pbr_specular_glossiness.specular_glossiness_texture, TextureKind_Attrib, mi, data, textures, images);
	}

	if (material.has_clearcoat)
	{
		analyzeMaterialTexture(material.clearcoat.clearcoat_texture, TextureKind_Attrib, mi, data, textures, images);
		analyzeMaterialTexture(material.clearcoat.clearcoat_roughness_texture, TextureKind_Attrib, mi, data, textures, images);
		analyzeMaterialTexture(material.clearcoat.clearcoat_normal_texture, TextureKind_Normal, mi, data, textures, images);
	}

	if (material.has_transmission)
	{
		analyzeMaterialTexture(material.transmission.transmission_texture, TextureKind_Attrib, mi, data, textures, images);
	}

	if (material.has_specular)
	{
		analyzeMaterialTexture(material.specular.specular_texture, TextureKind_Attrib, mi, data, textures, images);
		analyzeMaterialTexture(material.specular.specular_color_texture, TextureKind_Color, mi, data, textures, images);
	}

	if (material.has_sheen)
	{
		analyzeMaterialTexture(material.sheen.sheen_color_texture, TextureKind_Color, mi, data, textures, images);
		analyzeMaterialTexture(material.sheen.sheen_roughness_texture, TextureKind_Attrib, mi, data, textures, images);
	}

	if (material.has_volume)
	{
		analyzeMaterialTexture(material.volume.thickness_texture, TextureKind_Attrib, mi, data, textures, images);
	}

	if (material.has_iridescence)
	{
		analyzeMaterialTexture(material.iridescence.iridescence_texture, TextureKind_Attrib, mi, data, textures, images);
		analyzeMaterialTexture(material.iridescence.iridescence_thickness_texture, TextureKind_Attrib, mi, data, textures, images);
	}

	if (material.has_anisotropy)
	{
		analyzeMaterialTexture(material.anisotropy.anisotropy_texture, TextureKind_Normal, mi, data, textures, images);
	}

	if (material.has_diffuse_transmission)
	{
		analyzeMaterialTexture(material.diffuse_transmission.diffuse_transmission_texture, TextureKind_Attrib, mi, data, textures, images);
		analyzeMaterialTexture(material.diffuse_transmission.diffuse_transmission_color_texture, TextureKind_Color, mi, data, textures, images);
	}

	analyzeMaterialTexture(material.normal_texture, TextureKind_Normal, mi, data, textures, images);
	analyzeMaterialTexture(material.occlusion_texture, TextureKind_Attrib, mi, data, textures, images);
	analyzeMaterialTexture(material.emissive_texture, TextureKind_Color, mi, data, textures, images);

	if (material.unlit)
		mi.unlit = true;
}

void analyzeMaterials(cgltf_data* data, std::vector<MaterialInfo>& materials, std::vector<TextureInfo>& textures, std::vector<ImageInfo>& images)
{
	for (size_t i = 0; i < data->materials_count; ++i)
	{
		analyzeMaterial(data->materials[i], materials[i], data, textures, images);
	}
}

static int getChannels(const cgltf_image& image, ImageInfo& info, const char* input_path)
{
	if (info.channels)
		return info.channels;

	std::string img_data;
	std::string mime_type;
	if (readImage(image, input_path, img_data, mime_type))
		info.channels = hasAlpha(img_data, mime_type.c_str()) ? 4 : 3;
	else
		info.channels = -1;

	return info.channels;
}

static bool shouldKeepAlpha(const cgltf_texture_view& color, float alpha, cgltf_data* data, const char* input_path, std::vector<ImageInfo>& images)
{
	if (alpha != 1.f)
		return true;

	const cgltf_image* image = getTextureImage(color.texture);

	return image && getChannels(*image, images[image - data->images], input_path) == 4;
}

void optimizeMaterials(cgltf_data* data, std::vector<MaterialInfo>& materials, std::vector<ImageInfo>& images, const char* input_path)
{
	for (size_t i = 0; i < data->materials_count; ++i)
	{
		// remove BLEND/MASK from materials that don't have alpha information
		cgltf_material& material = data->materials[i];

		if (material.alpha_mode != cgltf_alpha_mode_opaque && !materials[i].mesh_alpha)
		{
			if (material.has_pbr_metallic_roughness && shouldKeepAlpha(material.pbr_metallic_roughness.base_color_texture, material.pbr_metallic_roughness.base_color_factor[3], data, input_path, images))
				continue;

			if (material.has_pbr_specular_glossiness && shouldKeepAlpha(material.pbr_specular_glossiness.diffuse_texture, material.pbr_specular_glossiness.diffuse_factor[3], data, input_path, images))
				continue;

			material.alpha_mode = cgltf_alpha_mode_opaque;
			material.alpha_cutoff = 0.5f; // reset to default to avoid writing it to output
		}
	}
}

void mergeTextures(cgltf_data* data, std::vector<TextureInfo>& textures)
{
	size_t offset = 0;

	for (size_t i = 0; i < textures.size(); ++i)
	{
		TextureInfo& info = textures[i];

		if (!info.keep)
			continue;

		for (size_t j = 0; j < i; ++j)
			if (textures[j].keep && areTexturesEqual(data->textures[i], data->textures[j]))
			{
				info.keep = false;
				info.remap = textures[j].remap;
				break;
			}

		if (info.keep)
		{
			info.remap = int(offset);
			offset++;
		}
	}
}
