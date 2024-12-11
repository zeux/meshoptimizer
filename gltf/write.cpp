// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* componentType(cgltf_component_type type)
{
	switch (type)
	{
	case cgltf_component_type_r_8:
		return "5120";
	case cgltf_component_type_r_8u:
		return "5121";
	case cgltf_component_type_r_16:
		return "5122";
	case cgltf_component_type_r_16u:
		return "5123";
	case cgltf_component_type_r_32u:
		return "5125";
	case cgltf_component_type_r_32f:
		return "5126";
	default:
		return "0";
	}
}

static const char* shapeType(cgltf_type type)
{
	switch (type)
	{
	case cgltf_type_scalar:
		return "SCALAR";
	case cgltf_type_vec2:
		return "VEC2";
	case cgltf_type_vec3:
		return "VEC3";
	case cgltf_type_vec4:
		return "VEC4";
	case cgltf_type_mat2:
		return "MAT2";
	case cgltf_type_mat3:
		return "MAT3";
	case cgltf_type_mat4:
		return "MAT4";
	default:
		return "";
	}
}

const char* attributeType(cgltf_attribute_type type)
{
	switch (type)
	{
	case cgltf_attribute_type_position:
		return "POSITION";
	case cgltf_attribute_type_normal:
		return "NORMAL";
	case cgltf_attribute_type_tangent:
		return "TANGENT";
	case cgltf_attribute_type_texcoord:
		return "TEXCOORD";
	case cgltf_attribute_type_color:
		return "COLOR";
	case cgltf_attribute_type_joints:
		return "JOINTS";
	case cgltf_attribute_type_weights:
		return "WEIGHTS";
	case cgltf_attribute_type_custom:
		return "CUSTOM";
	default:
		return "ATTRIBUTE";
	}
}

const char* animationPath(cgltf_animation_path_type type)
{
	switch (type)
	{
	case cgltf_animation_path_type_translation:
		return "translation";
	case cgltf_animation_path_type_rotation:
		return "rotation";
	case cgltf_animation_path_type_scale:
		return "scale";
	case cgltf_animation_path_type_weights:
		return "weights";
	default:
		return "";
	}
}

static const char* lightType(cgltf_light_type type)
{
	switch (type)
	{
	case cgltf_light_type_directional:
		return "directional";
	case cgltf_light_type_point:
		return "point";
	case cgltf_light_type_spot:
		return "spot";
	default:
		return "";
	}
}

static const char* alphaMode(cgltf_alpha_mode mode)
{
	switch (mode)
	{
	case cgltf_alpha_mode_opaque:
		return "OPAQUE";
	case cgltf_alpha_mode_mask:
		return "MASK";
	case cgltf_alpha_mode_blend:
		return "BLEND";
	default:
		return "";
	}
}

static const char* interpolationType(cgltf_interpolation_type type)
{
	switch (type)
	{
	case cgltf_interpolation_type_linear:
		return "LINEAR";
	case cgltf_interpolation_type_step:
		return "STEP";
	case cgltf_interpolation_type_cubic_spline:
		return "CUBICSPLINE";
	default:
		return "";
	}
}

static const char* compressionMode(BufferView::Compression mode)
{
	switch (mode)
	{
	case BufferView::Compression_Attribute:
		return "ATTRIBUTES";

	case BufferView::Compression_Index:
		return "TRIANGLES";

	case BufferView::Compression_IndexSequence:
		return "INDICES";

	default:
		return "";
	}
}

static const char* compressionFilter(StreamFormat::Filter filter)
{
	switch (filter)
	{
	case StreamFormat::Filter_None:
		return "NONE";

	case StreamFormat::Filter_Oct:
		return "OCTAHEDRAL";

	case StreamFormat::Filter_Quat:
		return "QUATERNION";

	case StreamFormat::Filter_Exp:
		return "EXPONENTIAL";

	default:
		return "";
	}
}

static void writeTextureInfo(std::string& json, const cgltf_data* data, const cgltf_texture_view& view, const QuantizationTexture* qt, std::vector<TextureInfo>& textures, const char* scale = NULL)
{
	assert(view.texture);

	bool has_transform = false;
	cgltf_texture_transform transform = {};
	transform.scale[0] = transform.scale[1] = 1.f;

	if (hasValidTransform(view))
	{
		transform = view.transform;
		has_transform = true;
	}

	if (qt)
	{
		transform.offset[0] += qt->offset[0];
		transform.offset[1] += qt->offset[1];
		transform.scale[0] *= qt->scale[0] / float((1 << qt->bits) - 1) * (qt->normalized ? 65535.f : 1.f);
		transform.scale[1] *= qt->scale[1] / float((1 << qt->bits) - 1) * (qt->normalized ? 65535.f : 1.f);
		has_transform = true;
	}

	append(json, "{\"index\":");
	append(json, size_t(textures[view.texture - data->textures].remap));
	if (view.texcoord != 0)
	{
		append(json, ",\"texCoord\":");
		append(json, size_t(view.texcoord));
	}
	if (scale && view.scale != 1)
	{
		append(json, ",\"");
		append(json, scale);
		append(json, "\":");
		append(json, view.scale);
	}
	if (has_transform)
	{
		append(json, ",\"extensions\":{\"KHR_texture_transform\":{");
		append(json, "\"offset\":");
		append(json, transform.offset, 2);
		append(json, ",\"scale\":");
		append(json, transform.scale, 2);
		if (transform.rotation != 0.f)
		{
			append(json, ",\"rotation\":");
			append(json, transform.rotation);
		}
		append(json, "}}");
	}
	append(json, "}");
}

static const float white[4] = {1, 1, 1, 1};
static const float black[4] = {0, 0, 0, 0};

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_pbr_metallic_roughness& pbr, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"pbrMetallicRoughness\":{");
	if (memcmp(pbr.base_color_factor, white, 16) != 0)
	{
		comma(json);
		append(json, "\"baseColorFactor\":");
		append(json, pbr.base_color_factor, 4);
	}
	if (pbr.base_color_texture.texture)
	{
		comma(json);
		append(json, "\"baseColorTexture\":");
		writeTextureInfo(json, data, pbr.base_color_texture, qt, textures);
	}
	if (pbr.metallic_factor != 1)
	{
		comma(json);
		append(json, "\"metallicFactor\":");
		append(json, pbr.metallic_factor);
	}
	if (pbr.roughness_factor != 1)
	{
		comma(json);
		append(json, "\"roughnessFactor\":");
		append(json, pbr.roughness_factor);
	}
	if (pbr.metallic_roughness_texture.texture)
	{
		comma(json);
		append(json, "\"metallicRoughnessTexture\":");
		writeTextureInfo(json, data, pbr.metallic_roughness_texture, qt, textures);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_pbr_specular_glossiness& pbr, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_pbrSpecularGlossiness\":{");
	if (pbr.diffuse_texture.texture)
	{
		comma(json);
		append(json, "\"diffuseTexture\":");
		writeTextureInfo(json, data, pbr.diffuse_texture, qt, textures);
	}
	if (pbr.specular_glossiness_texture.texture)
	{
		comma(json);
		append(json, "\"specularGlossinessTexture\":");
		writeTextureInfo(json, data, pbr.specular_glossiness_texture, qt, textures);
	}
	if (memcmp(pbr.diffuse_factor, white, 16) != 0)
	{
		comma(json);
		append(json, "\"diffuseFactor\":");
		append(json, pbr.diffuse_factor, 4);
	}
	if (memcmp(pbr.specular_factor, white, 12) != 0)
	{
		comma(json);
		append(json, "\"specularFactor\":");
		append(json, pbr.specular_factor, 3);
	}
	if (pbr.glossiness_factor != 1)
	{
		comma(json);
		append(json, "\"glossinessFactor\":");
		append(json, pbr.glossiness_factor);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_clearcoat& cc, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_clearcoat\":{");
	if (cc.clearcoat_texture.texture)
	{
		comma(json);
		append(json, "\"clearcoatTexture\":");
		writeTextureInfo(json, data, cc.clearcoat_texture, qt, textures);
	}
	if (cc.clearcoat_roughness_texture.texture)
	{
		comma(json);
		append(json, "\"clearcoatRoughnessTexture\":");
		writeTextureInfo(json, data, cc.clearcoat_roughness_texture, qt, textures);
	}
	if (cc.clearcoat_normal_texture.texture)
	{
		comma(json);
		append(json, "\"clearcoatNormalTexture\":");
		writeTextureInfo(json, data, cc.clearcoat_normal_texture, qt, textures, "scale");
	}
	if (cc.clearcoat_factor != 0)
	{
		comma(json);
		append(json, "\"clearcoatFactor\":");
		append(json, cc.clearcoat_factor);
	}
	if (cc.clearcoat_factor != 0)
	{
		comma(json);
		append(json, "\"clearcoatRoughnessFactor\":");
		append(json, cc.clearcoat_roughness_factor);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_transmission& tm, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_transmission\":{");
	if (tm.transmission_texture.texture)
	{
		comma(json);
		append(json, "\"transmissionTexture\":");
		writeTextureInfo(json, data, tm.transmission_texture, qt, textures);
	}
	if (tm.transmission_factor != 0)
	{
		comma(json);
		append(json, "\"transmissionFactor\":");
		append(json, tm.transmission_factor);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_ior& tm)
{
	(void)data;

	comma(json);
	append(json, "\"KHR_materials_ior\":{");
	append(json, "\"ior\":");
	append(json, tm.ior);
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_specular& tm, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_specular\":{");
	if (tm.specular_texture.texture)
	{
		comma(json);
		append(json, "\"specularTexture\":");
		writeTextureInfo(json, data, tm.specular_texture, qt, textures);
	}
	if (tm.specular_color_texture.texture)
	{
		comma(json);
		append(json, "\"specularColorTexture\":");
		writeTextureInfo(json, data, tm.specular_color_texture, qt, textures);
	}
	if (tm.specular_factor != 1)
	{
		comma(json);
		append(json, "\"specularFactor\":");
		append(json, tm.specular_factor);
	}
	if (memcmp(tm.specular_color_factor, white, 12) != 0)
	{
		comma(json);
		append(json, "\"specularColorFactor\":");
		append(json, tm.specular_color_factor, 3);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_sheen& tm, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_sheen\":{");
	if (tm.sheen_color_texture.texture)
	{
		comma(json);
		append(json, "\"sheenColorTexture\":");
		writeTextureInfo(json, data, tm.sheen_color_texture, qt, textures);
	}
	if (tm.sheen_roughness_texture.texture)
	{
		comma(json);
		append(json, "\"sheenRoughnessTexture\":");
		writeTextureInfo(json, data, tm.sheen_roughness_texture, qt, textures);
	}
	if (memcmp(tm.sheen_color_factor, black, 12) != 0)
	{
		comma(json);
		append(json, "\"sheenColorFactor\":");
		append(json, tm.sheen_color_factor, 3);
	}
	if (tm.sheen_roughness_factor != 0)
	{
		comma(json);
		append(json, "\"sheenRoughnessFactor\":");
		append(json, tm.sheen_roughness_factor);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_volume& tm, const QuantizationPosition* qp, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_volume\":{");
	if (tm.thickness_texture.texture)
	{
		comma(json);
		append(json, "\"thicknessTexture\":");
		writeTextureInfo(json, data, tm.thickness_texture, qt, textures);
	}
	if (tm.thickness_factor != 0)
	{
		// thickness is in mesh coordinate space which is rescaled by quantization
		comma(json);
		append(json, "\"thicknessFactor\":");
		append(json, tm.thickness_factor / (qp ? qp->node_scale : 1.f));
	}
	if (memcmp(tm.attenuation_color, white, 12) != 0)
	{
		comma(json);
		append(json, "\"attenuationColor\":");
		append(json, tm.attenuation_color, 3);
	}
	if (tm.attenuation_distance != FLT_MAX)
	{
		comma(json);
		append(json, "\"attenuationDistance\":");
		append(json, tm.attenuation_distance);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_emissive_strength& tm)
{
	(void)data;

	comma(json);
	append(json, "\"KHR_materials_emissive_strength\":{");
	if (tm.emissive_strength != 1)
	{
		comma(json);
		append(json, "\"emissiveStrength\":");
		append(json, tm.emissive_strength);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_iridescence& tm, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_iridescence\":{");
	if (tm.iridescence_factor != 0)
	{
		comma(json);
		append(json, "\"iridescenceFactor\":");
		append(json, tm.iridescence_factor);
	}
	if (tm.iridescence_texture.texture)
	{
		comma(json);
		append(json, "\"iridescenceTexture\":");
		writeTextureInfo(json, data, tm.iridescence_texture, qt, textures);
	}
	if (tm.iridescence_ior != 1.3f)
	{
		comma(json);
		append(json, "\"iridescenceIor\":");
		append(json, tm.iridescence_ior);
	}
	if (tm.iridescence_thickness_min != 100.f)
	{
		comma(json);
		append(json, "\"iridescenceThicknessMinimum\":");
		append(json, tm.iridescence_thickness_min);
	}
	if (tm.iridescence_thickness_max != 400.f)
	{
		comma(json);
		append(json, "\"iridescenceThicknessMaximum\":");
		append(json, tm.iridescence_thickness_max);
	}
	if (tm.iridescence_thickness_texture.texture)
	{
		comma(json);
		append(json, "\"iridescenceThicknessTexture\":");
		writeTextureInfo(json, data, tm.iridescence_thickness_texture, qt, textures);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_anisotropy& tm, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_anisotropy\":{");
	if (tm.anisotropy_strength != 0)
	{
		comma(json);
		append(json, "\"anisotropyStrength\":");
		append(json, tm.anisotropy_strength);
	}
	if (tm.anisotropy_rotation != 0)
	{
		comma(json);
		append(json, "\"anisotropyRotation\":");
		append(json, tm.anisotropy_rotation);
	}
	if (tm.anisotropy_texture.texture)
	{
		comma(json);
		append(json, "\"anisotropyTexture\":");
		writeTextureInfo(json, data, tm.anisotropy_texture, qt, textures);
	}
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_dispersion& tm)
{
	(void)data;

	comma(json);
	append(json, "\"KHR_materials_dispersion\":{");
	append(json, "\"dispersion\":");
	append(json, tm.dispersion);
	append(json, "}");
}

static void writeMaterialComponent(std::string& json, const cgltf_data* data, const cgltf_diffuse_transmission& tm, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	comma(json);
	append(json, "\"KHR_materials_diffuse_transmission\":{");
	if (tm.diffuse_transmission_factor != 0)
	{
		comma(json);
		append(json, "\"diffuseTransmissionFactor\":");
		append(json, tm.diffuse_transmission_factor);
	}
	if (tm.diffuse_transmission_texture.texture)
	{
		comma(json);
		append(json, "\"diffuseTransmissionTexture\":");
		writeTextureInfo(json, data, tm.diffuse_transmission_texture, qt, textures);
	}
	if (memcmp(tm.diffuse_transmission_color_factor, white, sizeof(float) * 3) != 0)
	{
		comma(json);
		append(json, "\"diffuseTransmissionColorFactor\":");
		append(json, tm.diffuse_transmission_color_factor, 3);
	}
	if (tm.diffuse_transmission_color_texture.texture)
	{
		comma(json);
		append(json, "\"diffuseTransmissionColorTexture\":");
		writeTextureInfo(json, data, tm.diffuse_transmission_color_texture, qt, textures);
	}
	append(json, "}");
}

void writeMaterial(std::string& json, const cgltf_data* data, const cgltf_material& material, const QuantizationPosition* qp, const QuantizationTexture* qt, std::vector<TextureInfo>& textures)
{
	if (material.name && *material.name)
	{
		comma(json);
		append(json, "\"name\":\"");
		append(json, material.name);
		append(json, "\"");
	}

	if (material.has_pbr_metallic_roughness)
	{
		writeMaterialComponent(json, data, material.pbr_metallic_roughness, qt, textures);
	}

	if (material.normal_texture.texture)
	{
		comma(json);
		append(json, "\"normalTexture\":");
		writeTextureInfo(json, data, material.normal_texture, qt, textures, "scale");
	}

	if (material.occlusion_texture.texture)
	{
		comma(json);
		append(json, "\"occlusionTexture\":");
		writeTextureInfo(json, data, material.occlusion_texture, qt, textures, "strength");
	}

	if (material.emissive_texture.texture)
	{
		comma(json);
		append(json, "\"emissiveTexture\":");
		writeTextureInfo(json, data, material.emissive_texture, qt, textures);
	}

	if (memcmp(material.emissive_factor, black, 12) != 0)
	{
		comma(json);
		append(json, "\"emissiveFactor\":");
		append(json, material.emissive_factor, 3);
	}

	if (material.alpha_mode != cgltf_alpha_mode_opaque)
	{
		comma(json);
		append(json, "\"alphaMode\":\"");
		append(json, alphaMode(material.alpha_mode));
		append(json, "\"");
	}

	if (material.alpha_cutoff != 0.5f)
	{
		comma(json);
		append(json, "\"alphaCutoff\":");
		append(json, material.alpha_cutoff);
	}

	if (material.double_sided)
	{
		comma(json);
		append(json, "\"doubleSided\":true");
	}

	if (material.has_pbr_specular_glossiness || material.has_clearcoat || material.has_transmission || material.has_ior || material.has_specular ||
	    material.has_sheen || material.has_volume || material.has_emissive_strength || material.has_iridescence || material.has_anisotropy ||
	    material.has_dispersion || material.has_diffuse_transmission || material.unlit)
	{
		comma(json);
		append(json, "\"extensions\":{");

		if (material.has_pbr_specular_glossiness)
			writeMaterialComponent(json, data, material.pbr_specular_glossiness, qt, textures);

		if (material.has_clearcoat)
			writeMaterialComponent(json, data, material.clearcoat, qt, textures);

		if (material.has_transmission)
			writeMaterialComponent(json, data, material.transmission, qt, textures);

		if (material.has_ior)
			writeMaterialComponent(json, data, material.ior);

		if (material.has_specular)
			writeMaterialComponent(json, data, material.specular, qt, textures);

		if (material.has_sheen)
			writeMaterialComponent(json, data, material.sheen, qt, textures);

		if (material.has_volume)
			writeMaterialComponent(json, data, material.volume, qp, qt, textures);

		if (material.has_emissive_strength)
			writeMaterialComponent(json, data, material.emissive_strength);

		if (material.has_iridescence)
			writeMaterialComponent(json, data, material.iridescence, qt, textures);

		if (material.has_anisotropy)
			writeMaterialComponent(json, data, material.anisotropy, qt, textures);

		if (material.has_dispersion)
			writeMaterialComponent(json, data, material.dispersion);

		if (material.has_diffuse_transmission)
			writeMaterialComponent(json, data, material.diffuse_transmission, qt, textures);

		if (material.unlit)
		{
			comma(json);
			append(json, "\"KHR_materials_unlit\":{}");
		}

		append(json, "}");
	}
}

size_t getBufferView(std::vector<BufferView>& views, BufferView::Kind kind, StreamFormat::Filter filter, BufferView::Compression compression, size_t stride, int variant)
{
	if (variant >= 0)
	{
		for (size_t i = 0; i < views.size(); ++i)
		{
			BufferView& v = views[i];

			if (v.kind == kind && v.filter == filter && v.compression == compression && v.stride == stride && v.variant == variant)
				return i;
		}
	}

	BufferView view = {kind, filter, compression, stride, variant};
	views.push_back(view);

	return views.size() - 1;
}

void writeBufferView(std::string& json, BufferView::Kind kind, StreamFormat::Filter filter, size_t count, size_t stride, size_t bin_offset, size_t bin_size, BufferView::Compression compression, size_t compressed_offset, size_t compressed_size)
{
	assert(bin_size == count * stride);

	// when compression is enabled, we store uncompressed data in buffer 1 and compressed data in buffer 0
	// when compression is disabled, we store uncompressed data in buffer 0
	size_t buffer = compression != BufferView::Compression_None ? 1 : 0;

	append(json, "{\"buffer\":");
	append(json, buffer);
	append(json, ",\"byteOffset\":");
	append(json, bin_offset);
	append(json, ",\"byteLength\":");
	append(json, bin_size);
	if (kind == BufferView::Kind_Vertex)
	{
		append(json, ",\"byteStride\":");
		append(json, stride);
	}
	if (kind == BufferView::Kind_Vertex || kind == BufferView::Kind_Index)
	{
		append(json, ",\"target\":");
		append(json, (kind == BufferView::Kind_Vertex) ? "34962" : "34963");
	}
	if (compression != BufferView::Compression_None)
	{
		append(json, ",\"extensions\":{");
		append(json, "\"EXT_meshopt_compression\":{");
		append(json, "\"buffer\":0");
		append(json, ",\"byteOffset\":");
		append(json, size_t(compressed_offset));
		append(json, ",\"byteLength\":");
		append(json, size_t(compressed_size));
		append(json, ",\"byteStride\":");
		append(json, stride);
		append(json, ",\"mode\":\"");
		append(json, compressionMode(compression));
		append(json, "\"");
		if (filter != StreamFormat::Filter_None)
		{
			append(json, ",\"filter\":\"");
			append(json, compressionFilter(filter));
			append(json, "\"");
		}
		append(json, ",\"count\":");
		append(json, count);
		append(json, "}}");
	}
	append(json, "}");
}

static void writeAccessor(std::string& json, size_t view, size_t offset, cgltf_type type, cgltf_component_type component_type, bool normalized, size_t count, const float* min = NULL, const float* max = NULL, size_t numminmax = 0)
{
	append(json, "{\"bufferView\":");
	append(json, view);
	append(json, ",\"byteOffset\":");
	append(json, offset);
	append(json, ",\"componentType\":");
	append(json, componentType(component_type));
	append(json, ",\"count\":");
	append(json, count);
	append(json, ",\"type\":\"");
	append(json, shapeType(type));
	append(json, "\"");

	if (normalized)
	{
		append(json, ",\"normalized\":true");
	}

	if (min && max)
	{
		assert(numminmax);

		append(json, ",\"min\":");
		append(json, min, numminmax);
		append(json, ",\"max\":");
		append(json, max, numminmax);
	}

	append(json, "}");
}

static void writeEmbeddedImage(std::string& json, std::vector<BufferView>& views, const char* data, size_t size, const char* mime_type, TextureKind kind)
{
	size_t view = getBufferView(views, BufferView::Kind_Image, StreamFormat::Filter_None, BufferView::Compression_None, 1, -1 - kind);

	assert(views[view].data.empty());
	views[view].data.assign(data, size);

	append(json, "\"bufferView\":");
	append(json, view);
	append(json, ",\"mimeType\":\"");
	append(json, mime_type);
	append(json, "\"");
}

static std::string decodeUri(const char* uri)
{
	std::string result = uri;

	if (!result.empty())
	{
		cgltf_decode_uri(&result[0]);
		result.resize(strlen(result.c_str()));
	}

	return result;
}

void writeSampler(std::string& json, const cgltf_sampler& sampler)
{
	if (sampler.mag_filter != 0)
	{
		comma(json);
		append(json, "\"magFilter\":");
		append(json, size_t(sampler.mag_filter));
	}
	if (sampler.min_filter != 0)
	{
		comma(json);
		append(json, "\"minFilter\":");
		append(json, size_t(sampler.min_filter));
	}
	if (sampler.wrap_s != 10497)
	{
		comma(json);
		append(json, "\"wrapS\":");
		append(json, size_t(sampler.wrap_s));
	}
	if (sampler.wrap_t != 10497)
	{
		comma(json);
		append(json, "\"wrapT\":");
		append(json, size_t(sampler.wrap_t));
	}
}

static void writeImageError(std::string& json, const char* action, size_t index, const char* uri, const char* reason)
{
	append(json, "\"uri\":\"");
	append(json, "data:image/png;base64,ERR/");
	append(json, "\"");

	fprintf(stderr, "Warning: unable to %s image %d (%s), skipping%s%s%s\n", action, int(index), uri ? uri : "embedded", reason ? " (" : "", reason ? reason : "", reason ? ")" : "");
}

static void writeImageData(std::string& json, std::vector<BufferView>& views, size_t index, const char* uri, const char* mime_type, const std::string& contents, const char* output_path, TextureKind kind, bool embed)
{
	bool dataUri = uri && strncmp(uri, "data:", 5) == 0;

	if (!embed && uri && !dataUri && output_path)
	{
		std::string file_name = getFileName(uri) + mimeExtension(mime_type);
		std::string file_path = getFullPath(decodeUri(file_name.c_str()).c_str(), output_path);

		if (writeFile(file_path.c_str(), contents))
		{
			append(json, "\"uri\":\"");
			append(json, file_name);
			append(json, "\"");
		}
		else
		{
			writeImageError(json, "save", int(index), uri, file_path.c_str());
		}
	}
	else
	{
		writeEmbeddedImage(json, views, contents.c_str(), contents.size(), mime_type, kind);
	}
}

void writeImage(std::string& json, std::vector<BufferView>& views, const cgltf_image& image, const ImageInfo& info, const std::string* encoded, size_t index, const char* input_path, const char* output_path, const Settings& settings)
{
	if (image.name && *image.name)
	{
		append(json, "\"name\":\"");
		append(json, image.name);
		append(json, "\",");
	}

	if (encoded)
	{
		// image was pre-encoded via encodeImages (which might have failed!)
		if (encoded->compare(0, 5, "error") == 0)
			writeImageError(json, "encode", int(index), image.uri, encoded->c_str());
		else
			writeImageData(json, views, index, image.uri, "image/ktx2", *encoded, output_path, info.kind, settings.texture_embed);
		return;
	}

	bool dataUri = image.uri && strncmp(image.uri, "data:", 5) == 0;

	if (image.uri && !dataUri && settings.texture_ref)
	{
		// fast-path: we don't need to read the image to memory
		append(json, "\"uri\":\"");
		append(json, image.uri);
		append(json, "\"");
		return;
	}

	std::string img_data;
	std::string mime_type;
	if (!readImage(image, input_path, img_data, mime_type))
	{
		writeImageError(json, "read", index, image.uri, NULL);
		return;
	}

	writeImageData(json, views, index, image.uri, mime_type.c_str(), img_data, output_path, info.kind, settings.texture_embed);
}

void writeTexture(std::string& json, const cgltf_texture& texture, const ImageInfo* info, cgltf_data* data, const Settings& settings)
{
	if (texture.sampler)
	{
		append(json, "\"sampler\":");
		append(json, size_t(texture.sampler - data->samplers));
	}

	if (texture.image)
	{
		if (info && settings.texture_mode[info->kind] != TextureMode_Raw)
		{
			comma(json);
			append(json, "\"extensions\":{\"KHR_texture_basisu\":{\"source\":");
			append(json, size_t(texture.image - data->images));
			append(json, "}}");

			return; // skip input basisu image if present, as we replace it with the one we encoded
		}
		else
		{
			comma(json);
			append(json, "\"source\":");
			append(json, size_t(texture.image - data->images));
		}
	}

	if (texture.basisu_image)
	{
		comma(json);
		append(json, "\"extensions\":{\"KHR_texture_basisu\":{\"source\":");
		append(json, size_t(texture.basisu_image - data->images));
		append(json, "}}");
	}
	else if (texture.webp_image)
	{
		comma(json);
		append(json, "\"extensions\":{\"EXT_texture_webp\":{\"source\":");
		append(json, size_t(texture.webp_image - data->images));
		append(json, "}}");
	}
}

void writeMeshAttributes(std::string& json, std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const Mesh& mesh, int target, const QuantizationPosition& qp, const QuantizationTexture& qt, const Settings& settings)
{
	std::string scratch;

	for (size_t j = 0; j < mesh.streams.size(); ++j)
	{
		const Stream& stream = mesh.streams[j];

		if (stream.target != target)
			continue;

		scratch.clear();
		StreamFormat format = writeVertexStream(scratch, stream, qp, qt, settings);
		BufferView::Compression compression = settings.compress ? BufferView::Compression_Attribute : BufferView::Compression_None;

		size_t view = getBufferView(views, BufferView::Kind_Vertex, format.filter, compression, format.stride, stream.type);
		size_t offset = views[view].data.size();
		views[view].data += scratch;

		comma(json_accessors);
		if (stream.type == cgltf_attribute_type_position)
		{
			float min[3] = {};
			float max[3] = {};
			getPositionBounds(min, max, stream, qp, settings);

			writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, stream.data.size(), min, max, 3);
		}
		else
		{
			writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, stream.data.size());
		}

		size_t vertex_accr = accr_offset++;

		comma(json);
		append(json, "\"");
		if (stream.custom_name)
		{
			append(json, stream.custom_name);
		}
		else
		{
			append(json, attributeType(stream.type));
			if (stream.type != cgltf_attribute_type_position && stream.type != cgltf_attribute_type_normal && stream.type != cgltf_attribute_type_tangent)
			{
				append(json, "_");
				append(json, size_t(stream.index));
			}
		}
		append(json, "\":");
		append(json, vertex_accr);
	}
}

size_t writeMeshIndices(std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const std::vector<unsigned int>& indices, cgltf_primitive_type type, const Settings& settings)
{
	std::string scratch;
	StreamFormat format = writeIndexStream(scratch, indices);
	BufferView::Compression compression = settings.compress ? (type == cgltf_primitive_type_triangles ? BufferView::Compression_Index : BufferView::Compression_IndexSequence) : BufferView::Compression_None;

	size_t view = getBufferView(views, BufferView::Kind_Index, StreamFormat::Filter_None, compression, format.stride);
	size_t offset = views[view].data.size();
	views[view].data += scratch;

	comma(json_accessors);
	writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, indices.size());

	size_t index_accr = accr_offset++;

	return index_accr;
}

void writeMeshGeometry(std::string& json, std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const Mesh& mesh, const QuantizationPosition& qp, const QuantizationTexture& qt, const Settings& settings)
{
	append(json, "{\"attributes\":{");
	writeMeshAttributes(json, views, json_accessors, accr_offset, mesh, 0, qp, qt, settings);
	append(json, "}");
	if (mesh.type != cgltf_primitive_type_triangles)
	{
		append(json, ",\"mode\":");
		append(json, size_t(mesh.type - cgltf_primitive_type_points));
	}
	if (mesh.targets)
	{
		append(json, ",\"targets\":[");
		for (size_t j = 0; j < mesh.targets; ++j)
		{
			comma(json);
			append(json, "{");
			writeMeshAttributes(json, views, json_accessors, accr_offset, mesh, int(1 + j), qp, qt, settings);
			append(json, "}");
		}
		append(json, "]");
	}

	if (!mesh.indices.empty())
	{
		size_t index_accr = writeMeshIndices(views, json_accessors, accr_offset, mesh.indices, mesh.type, settings);

		append(json, ",\"indices\":");
		append(json, index_accr);
	}
}

static size_t writeAnimationTime(std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const std::vector<float>& time, const Settings& settings)
{
	std::string scratch;
	StreamFormat format = writeTimeStream(scratch, time);
	BufferView::Compression compression = settings.compress ? BufferView::Compression_Attribute : BufferView::Compression_None;

	size_t view = getBufferView(views, BufferView::Kind_Time, StreamFormat::Filter_None, compression, format.stride);
	size_t offset = views[view].data.size();
	views[view].data += scratch;

	comma(json_accessors);
	writeAccessor(json_accessors, view, offset, cgltf_type_scalar, format.component_type, format.normalized, time.size(), &time.front(), &time.back(), 1);

	size_t time_accr = accr_offset++;

	return time_accr;
}

static size_t writeAnimationTime(std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, float mint, int frames, float period, const Settings& settings)
{
	std::vector<float> time(frames);

	for (int j = 0; j < frames; ++j)
		time[j] = mint + float(j) * period;

	return writeAnimationTime(views, json_accessors, accr_offset, time, settings);
}

size_t writeJointBindMatrices(std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const cgltf_skin& skin, const QuantizationPosition& qp, const Settings& settings)
{
	std::string scratch;

	for (size_t j = 0; j < skin.joints_count; ++j)
	{
		float transform[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

		if (skin.inverse_bind_matrices)
		{
			cgltf_accessor_read_float(skin.inverse_bind_matrices, j, transform, 16);
		}

		if (settings.quantize && !settings.pos_float)
		{
			// pos_offset has to be applied first, thus it results in an offset rotated by the bind matrix
			transform[12] += qp.offset[0] * transform[0] + qp.offset[1] * transform[4] + qp.offset[2] * transform[8];
			transform[13] += qp.offset[0] * transform[1] + qp.offset[1] * transform[5] + qp.offset[2] * transform[9];
			transform[14] += qp.offset[0] * transform[2] + qp.offset[1] * transform[6] + qp.offset[2] * transform[10];

			// node_scale will be applied before the rotation/scale from transform
			for (int k = 0; k < 12; ++k)
				transform[k] *= qp.node_scale;
		}

		scratch.append(reinterpret_cast<const char*>(transform), sizeof(transform));
	}

	BufferView::Compression compression = settings.compress ? BufferView::Compression_Attribute : BufferView::Compression_None;

	size_t view = getBufferView(views, BufferView::Kind_Skin, StreamFormat::Filter_None, compression, 64);
	size_t offset = views[view].data.size();
	views[view].data += scratch;

	comma(json_accessors);
	writeAccessor(json_accessors, view, offset, cgltf_type_mat4, cgltf_component_type_r_32f, false, skin.joints_count);

	size_t matrix_accr = accr_offset++;

	return matrix_accr;
}

static void writeInstanceData(std::vector<BufferView>& views, std::string& json_accessors, cgltf_animation_path_type type, const std::vector<Attr>& data, const Settings& settings)
{
	BufferView::Compression compression = settings.compress ? BufferView::Compression_Attribute : BufferView::Compression_None;

	std::string scratch;
	StreamFormat format = writeKeyframeStream(scratch, type, data, settings);

	size_t view = getBufferView(views, BufferView::Kind_Instance, format.filter, compression, format.stride, type);
	size_t offset = views[view].data.size();
	views[view].data += scratch;

	comma(json_accessors);
	writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, data.size());
}

size_t writeInstances(std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const std::vector<Transform>& transforms, const QuantizationPosition& qp, const Settings& settings)
{
	std::vector<Attr> position, rotation, scale;
	position.resize(transforms.size());
	rotation.resize(transforms.size());
	scale.resize(transforms.size());

	for (size_t i = 0; i < transforms.size(); ++i)
	{
		decomposeTransform(position[i].f, rotation[i].f, scale[i].f, transforms[i].data);

		if (settings.quantize && !settings.pos_float)
		{
			const float* transform = transforms[i].data;

			// pos_offset has to be applied first, thus it results in an offset rotated by the instance matrix
			position[i].f[0] += qp.offset[0] * transform[0] + qp.offset[1] * transform[4] + qp.offset[2] * transform[8];
			position[i].f[1] += qp.offset[0] * transform[1] + qp.offset[1] * transform[5] + qp.offset[2] * transform[9];
			position[i].f[2] += qp.offset[0] * transform[2] + qp.offset[1] * transform[6] + qp.offset[2] * transform[10];

			// node_scale will be applied before the rotation/scale from transform
			scale[i].f[0] *= qp.node_scale;
			scale[i].f[1] *= qp.node_scale;
			scale[i].f[2] *= qp.node_scale;
		}
	}

	writeInstanceData(views, json_accessors, cgltf_animation_path_type_translation, position, settings);
	writeInstanceData(views, json_accessors, cgltf_animation_path_type_rotation, rotation, settings);
	writeInstanceData(views, json_accessors, cgltf_animation_path_type_scale, scale, settings);

	size_t result = accr_offset;
	accr_offset += 3;
	return result;
}

void writeMeshNode(std::string& json, size_t mesh_offset, cgltf_node* node, cgltf_skin* skin, cgltf_data* data, const QuantizationPosition* qp)
{
	comma(json);
	append(json, "{\"mesh\":");
	append(json, mesh_offset);
	if (skin)
	{
		append(json, ",\"skin\":");
		append(json, size_t(skin - data->skins));
	}
	if (qp)
	{
		append(json, ",\"translation\":");
		append(json, qp->offset, 3);
		append(json, ",\"scale\":[");
		append(json, qp->node_scale);
		append(json, ",");
		append(json, qp->node_scale);
		append(json, ",");
		append(json, qp->node_scale);
		append(json, "]");
	}
	if (node && node->weights_count)
	{
		append(json, ",\"weights\":");
		append(json, node->weights, node->weights_count);
	}
	append(json, "}");
}

void writeMeshNodeInstanced(std::string& json, size_t mesh_offset, size_t accr_offset)
{
	comma(json);
	append(json, "{\"mesh\":");
	append(json, mesh_offset);
	append(json, ",\"extensions\":{\"EXT_mesh_gpu_instancing\":{\"attributes\":{");

	comma(json);
	append(json, "\"TRANSLATION\":");
	append(json, accr_offset + 0);

	comma(json);
	append(json, "\"ROTATION\":");
	append(json, accr_offset + 1);

	comma(json);
	append(json, "\"SCALE\":");
	append(json, accr_offset + 2);

	append(json, "}}}");
	append(json, "}");
}

void writeSkin(std::string& json, const cgltf_skin& skin, size_t matrix_accr, const std::vector<NodeInfo>& nodes, cgltf_data* data)
{
	comma(json);
	append(json, "{");
	if (skin.name && *skin.name)
	{
		append(json, "\"name\":\"");
		append(json, skin.name);
		append(json, "\",");
	}
	append(json, "\"joints\":[");
	for (size_t j = 0; j < skin.joints_count; ++j)
	{
		comma(json);
		append(json, size_t(nodes[skin.joints[j] - data->nodes].remap));
	}
	append(json, "]");
	append(json, ",\"inverseBindMatrices\":");
	append(json, matrix_accr);
	if (skin.skeleton)
	{
		comma(json);
		append(json, "\"skeleton\":");
		append(json, size_t(nodes[skin.skeleton - data->nodes].remap));
	}
	append(json, "}");
}

void writeNode(std::string& json, const cgltf_node& node, const std::vector<NodeInfo>& nodes, cgltf_data* data)
{
	const NodeInfo& ni = nodes[&node - data->nodes];

	if (node.name && *node.name)
	{
		comma(json);
		append(json, "\"name\":\"");
		append(json, node.name);
		append(json, "\"");
	}
	if (node.has_translation)
	{
		comma(json);
		append(json, "\"translation\":");
		append(json, node.translation, 3);
	}
	if (node.has_rotation)
	{
		comma(json);
		append(json, "\"rotation\":");
		append(json, node.rotation, 4);
	}
	if (node.has_scale)
	{
		comma(json);
		append(json, "\"scale\":");
		append(json, node.scale, 3);
	}
	if (node.has_matrix)
	{
		comma(json);
		append(json, "\"matrix\":");
		append(json, node.matrix, 16);
	}

	bool has_children = !ni.mesh_nodes.empty();
	for (size_t j = 0; j < node.children_count; ++j)
		has_children |= nodes[node.children[j] - data->nodes].keep;

	if (has_children)
	{
		comma(json);
		append(json, "\"children\":[");
		for (size_t j = 0; j < node.children_count; ++j)
		{
			const NodeInfo& ci = nodes[node.children[j] - data->nodes];

			if (ci.keep)
			{
				comma(json);
				append(json, size_t(ci.remap));
			}
		}
		for (size_t j = 0; j < ni.mesh_nodes.size(); ++j)
		{
			comma(json);
			append(json, ni.mesh_nodes[j]);
		}
		append(json, "]");
	}
	if (ni.has_mesh)
	{
		comma(json);
		append(json, "\"mesh\":");
		append(json, ni.mesh_index);
		if (ni.mesh_skin)
		{
			append(json, ",\"skin\":");
			append(json, size_t(ni.mesh_skin - data->skins));
		}
		if (node.weights_count)
		{
			append(json, ",\"weights\":");
			append(json, node.weights, node.weights_count);
		}
	}
	if (node.camera)
	{
		comma(json);
		append(json, "\"camera\":");
		append(json, size_t(node.camera - data->cameras));
	}
	if (node.light)
	{
		comma(json);
		append(json, "\"extensions\":{\"KHR_lights_punctual\":{\"light\":");
		append(json, size_t(node.light - data->lights));
		append(json, "}}");
	}
}

void writeAnimation(std::string& json, std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const Animation& animation, size_t i, cgltf_data* data, const std::vector<NodeInfo>& nodes, const Settings& settings)
{
	std::vector<const Track*> tracks;

	for (size_t j = 0; j < animation.tracks.size(); ++j)
	{
		const Track& track = animation.tracks[j];

		const NodeInfo& ni = nodes[track.node - data->nodes];

		if (!ni.keep)
			continue;

		if (!settings.anim_const && (ni.animated_path_mask & (1 << track.path)) == 0)
			continue;

		tracks.push_back(&track);
	}

	if (tracks.empty())
	{
		char index[16];
		snprintf(index, sizeof(index), "%d", int(i));

		fprintf(stderr, "Warning: ignoring animation %s because it has no tracks with motion; use -ac to override\n", animation.name && *animation.name ? animation.name : index);
		return;
	}

	bool needs_time = false;
	bool needs_pose = false;

	for (size_t j = 0; j < tracks.size(); ++j)
	{
		const Track& track = *tracks[j];

#ifndef NDEBUG
		size_t keyframe_size = (track.interpolation == cgltf_interpolation_type_cubic_spline) ? 3 : 1;
		size_t time_size = track.constant ? 1 : (track.time.empty() ? animation.frames : track.time.size());

		assert(track.data.size() == keyframe_size * track.components * time_size);
#endif

		needs_time = needs_time || (track.time.empty() && !track.constant);
		needs_pose = needs_pose || track.constant;
	}

	bool needs_range = needs_pose && !needs_time && animation.frames > 1;

	needs_pose = needs_pose && !(needs_range && tracks.size() == 1);

	assert(int(needs_time) + int(needs_pose) + int(needs_range) <= 2);

	float animation_period = 1.f / float(settings.anim_freq);
	float animation_length = float(animation.frames - 1) * animation_period;

	size_t time_accr = needs_time ? writeAnimationTime(views, json_accessors, accr_offset, animation.start, animation.frames, animation_period, settings) : 0;
	size_t pose_accr = needs_pose ? writeAnimationTime(views, json_accessors, accr_offset, animation.start, 1, 0.f, settings) : 0;
	size_t range_accr = needs_range ? writeAnimationTime(views, json_accessors, accr_offset, animation.start, 2, animation_length, settings) : 0;

	std::string json_samplers;
	std::string json_channels;

	size_t track_offset = 0;

	size_t last_track_time_accr = 0;
	const Track* last_track_time = NULL;

	for (size_t j = 0; j < tracks.size(); ++j)
	{
		const Track& track = *tracks[j];

		bool range = needs_range && j == 0;
		int range_size = range ? 2 : 1;

		size_t track_time_accr = time_accr;
		if (!track.time.empty())
		{
			// reuse time accessors between consecutive tracks if possible
			if (last_track_time && track.time == last_track_time->time)
				track_time_accr = last_track_time_accr;
			else
			{
				track_time_accr = writeAnimationTime(views, json_accessors, accr_offset, track.time, settings);
				last_track_time_accr = track_time_accr;
				last_track_time = &track;
			}
		}

		std::string scratch;
		StreamFormat format = writeKeyframeStream(scratch, track.path, track.data, settings, track.interpolation == cgltf_interpolation_type_cubic_spline);

		if (range)
		{
			assert(range_size == 2);
			scratch += scratch;
		}

		BufferView::Compression compression = settings.compress && track.path != cgltf_animation_path_type_weights ? BufferView::Compression_Attribute : BufferView::Compression_None;

		size_t view = getBufferView(views, BufferView::Kind_Keyframe, format.filter, compression, format.stride, track.path);
		size_t offset = views[view].data.size();
		views[view].data += scratch;

		comma(json_accessors);
		writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, track.data.size() * range_size);

		size_t data_accr = accr_offset++;

		comma(json_samplers);
		append(json_samplers, "{\"input\":");
		append(json_samplers, range ? range_accr : (track.constant ? pose_accr : track_time_accr));
		append(json_samplers, ",\"output\":");
		append(json_samplers, data_accr);
		if (track.interpolation != cgltf_interpolation_type_linear)
		{
			append(json_samplers, ",\"interpolation\":\"");
			append(json_samplers, interpolationType(track.interpolation));
			append(json_samplers, "\"");
		}
		append(json_samplers, "}");

		const NodeInfo& tni = nodes[track.node - data->nodes];
		size_t target_node = size_t(tni.remap);

		// when animating morph weights, quantization may move mesh assignments to a mesh node in which case we need to move the animation output
		if (track.path == cgltf_animation_path_type_weights && tni.mesh_nodes.size() == 1)
			target_node = tni.mesh_nodes[0];

		comma(json_channels);
		append(json_channels, "{\"sampler\":");
		append(json_channels, track_offset);
		append(json_channels, ",\"target\":{\"node\":");
		append(json_channels, target_node);
		append(json_channels, ",\"path\":\"");
		append(json_channels, animationPath(track.path));
		append(json_channels, "\"}}");

		track_offset++;
	}

	comma(json);
	append(json, "{");
	if (animation.name && *animation.name)
	{
		append(json, "\"name\":\"");
		append(json, animation.name);
		append(json, "\",");
	}
	append(json, "\"samplers\":[");
	append(json, json_samplers);
	append(json, "],\"channels\":[");
	append(json, json_channels);
	append(json, "]}");
}

void writeCamera(std::string& json, const cgltf_camera& camera)
{
	comma(json);
	append(json, "{");

	switch (camera.type)
	{
	case cgltf_camera_type_perspective:
		append(json, "\"type\":\"perspective\",\"perspective\":{");
		append(json, "\"yfov\":");
		append(json, camera.data.perspective.yfov);
		append(json, ",\"znear\":");
		append(json, camera.data.perspective.znear);
		if (camera.data.perspective.aspect_ratio != 0.f)
		{
			append(json, ",\"aspectRatio\":");
			append(json, camera.data.perspective.aspect_ratio);
		}
		if (camera.data.perspective.zfar != 0.f)
		{
			append(json, ",\"zfar\":");
			append(json, camera.data.perspective.zfar);
		}
		append(json, "}");
		break;

	case cgltf_camera_type_orthographic:
		append(json, "\"type\":\"orthographic\",\"orthographic\":{");
		append(json, "\"xmag\":");
		append(json, camera.data.orthographic.xmag);
		append(json, ",\"ymag\":");
		append(json, camera.data.orthographic.ymag);
		append(json, ",\"znear\":");
		append(json, camera.data.orthographic.znear);
		append(json, ",\"zfar\":");
		append(json, camera.data.orthographic.zfar);
		append(json, "}");
		break;

	default:
		fprintf(stderr, "Warning: skipping camera of unknown type\n");
	}

	append(json, "}");
}

void writeLight(std::string& json, const cgltf_light& light)
{
	comma(json);
	append(json, "{\"type\":\"");
	append(json, lightType(light.type));
	append(json, "\"");
	if (memcmp(light.color, white, 12) != 0)
	{
		comma(json);
		append(json, "\"color\":");
		append(json, light.color, 3);
	}
	if (light.intensity != 1.f)
	{
		comma(json);
		append(json, "\"intensity\":");
		append(json, light.intensity);
	}
	if (light.range != 0.f)
	{
		comma(json);
		append(json, "\"range\":");
		append(json, light.range);
	}
	if (light.type == cgltf_light_type_spot)
	{
		comma(json);
		append(json, "\"spot\":{");
		append(json, "\"innerConeAngle\":");
		append(json, light.spot_inner_cone_angle);
		append(json, ",\"outerConeAngle\":");
		append(json, light.spot_outer_cone_angle == 0.f ? 0.78539816339f : light.spot_outer_cone_angle);
		append(json, "}");
	}
	append(json, "}");
}

void writeArray(std::string& json, const char* name, const std::string& contents)
{
	if (contents.empty())
		return;

	comma(json);
	append(json, "\"");
	append(json, name);
	append(json, "\":[");
	append(json, contents);
	append(json, "]");
}

void writeExtensions(std::string& json, const ExtensionInfo* extensions, size_t count)
{
	bool used_extensions = false;
	bool required_extensions = false;

	for (size_t i = 0; i < count; ++i)
	{
		used_extensions |= extensions[i].used;
		required_extensions |= extensions[i].used && extensions[i].required;
	}

	if (used_extensions)
	{
		comma(json);
		append(json, "\"extensionsUsed\":[");
		for (size_t i = 0; i < count; ++i)
			if (extensions[i].used)
			{
				comma(json);
				append(json, "\"");
				append(json, extensions[i].name);
				append(json, "\"");
			}
		append(json, "]");
	}

	if (required_extensions)
	{
		comma(json);
		append(json, "\"extensionsRequired\":[");
		for (size_t i = 0; i < count; ++i)
			if (extensions[i].used && extensions[i].required)
			{
				comma(json);
				append(json, "\"");
				append(json, extensions[i].name);
				append(json, "\"");
			}
		append(json, "]");
	}
}

void writeExtras(std::string& json, const cgltf_extras& extras)
{
	if (!extras.data)
		return;

	comma(json);
	append(json, "\"extras\":");
	appendJson(json, extras.data);
}

void writeScene(std::string& json, const cgltf_scene& scene, const std::string& roots, const Settings& settings)
{
	comma(json);
	append(json, "{");
	if (scene.name && *scene.name)
	{
		append(json, "\"name\":\"");
		append(json, scene.name);
		append(json, "\"");
	}
	if (!roots.empty())
	{
		comma(json);
		append(json, "\"nodes\":[");
		append(json, roots);
		append(json, "]");
	}
	if (settings.keep_extras)
		writeExtras(json, scene.extras);
	append(json, "}");
}
