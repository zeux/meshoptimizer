// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <algorithm>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/meshoptimizer.h"

std::string getVersion()
{
	char result[32];
	sprintf(result, "%d.%d", MESHOPTIMIZER_VERSION / 1000, (MESHOPTIMIZER_VERSION % 1000) / 10);
	return result;
}

void comma(std::string& s)
{
	char ch = s.empty() ? 0 : s[s.size() - 1];

	if (ch != 0 && ch != '[' && ch != '{')
		s += ",";
}

void append(std::string& s, size_t v)
{
	char buf[32];
	sprintf(buf, "%zu", v);
	s += buf;
}

void append(std::string& s, float v)
{
	char buf[512];
	sprintf(buf, "%.9g", v);
	s += buf;
}

void append(std::string& s, const char* v)
{
	s += v;
}

void append(std::string& s, const std::string& v)
{
	s += v;
}

const char* componentType(cgltf_component_type type)
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

const char* shapeType(cgltf_type type)
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

const char* lightType(cgltf_light_type type)
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

void writeTextureInfo(std::string& json, const cgltf_data* data, const cgltf_texture_view& view, const QuantizationTexture& qt)
{
	assert(view.texture);

	cgltf_texture_transform transform = {};

	if (view.has_transform)
	{
		transform = view.transform;
	}
	else
	{
		transform.scale[0] = transform.scale[1] = 1.f;
	}

	transform.offset[0] += qt.offset[0];
	transform.offset[1] += qt.offset[1];
	transform.scale[0] *= qt.scale[0] / float((1 << qt.bits) - 1);
	transform.scale[1] *= qt.scale[1] / float((1 << qt.bits) - 1);

	append(json, "{\"index\":");
	append(json, size_t(view.texture - data->textures));
	append(json, ",\"texCoord\":");
	append(json, size_t(view.texcoord));
	append(json, ",\"extensions\":{\"KHR_texture_transform\":{");
	append(json, "\"offset\":[");
	append(json, transform.offset[0]);
	append(json, ",");
	append(json, transform.offset[1]);
	append(json, "],\"scale\":[");
	append(json, transform.scale[0]);
	append(json, ",");
	append(json, transform.scale[1]);
	append(json, "]");
	if (transform.rotation != 0.f)
	{
		append(json, ",\"rotation\":");
		append(json, transform.rotation);
	}
	append(json, "}}}");
}

void writeMaterialInfo(std::string& json, const cgltf_data* data, const cgltf_material& material, const QuantizationTexture& qt)
{
	static const float white[4] = {1, 1, 1, 1};
	static const float black[4] = {0, 0, 0, 0};

	if (material.name && *material.name)
	{
		comma(json);
		append(json, "\"name\":\"");
		append(json, material.name);
		append(json, "\"");
	}

	if (material.has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;

		comma(json);
		append(json, "\"pbrMetallicRoughness\":{");
		if (memcmp(pbr.base_color_factor, white, 16) != 0)
		{
			comma(json);
			append(json, "\"baseColorFactor\":[");
			append(json, pbr.base_color_factor[0]);
			append(json, ",");
			append(json, pbr.base_color_factor[1]);
			append(json, ",");
			append(json, pbr.base_color_factor[2]);
			append(json, ",");
			append(json, pbr.base_color_factor[3]);
			append(json, "]");
		}
		if (pbr.base_color_texture.texture)
		{
			comma(json);
			append(json, "\"baseColorTexture\":");
			writeTextureInfo(json, data, pbr.base_color_texture, qt);
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
			writeTextureInfo(json, data, pbr.metallic_roughness_texture, qt);
		}
		append(json, "}");
	}

	if (material.normal_texture.texture)
	{
		comma(json);
		append(json, "\"normalTexture\":");
		writeTextureInfo(json, data, material.normal_texture, qt);
	}

	if (material.occlusion_texture.texture)
	{
		comma(json);
		append(json, "\"occlusionTexture\":");
		writeTextureInfo(json, data, material.occlusion_texture, qt);
	}

	if (material.emissive_texture.texture)
	{
		comma(json);
		append(json, "\"emissiveTexture\":");
		writeTextureInfo(json, data, material.emissive_texture, qt);
	}

	if (memcmp(material.emissive_factor, black, 12) != 0)
	{
		comma(json);
		append(json, "\"emissiveFactor\":[");
		append(json, material.emissive_factor[0]);
		append(json, ",");
		append(json, material.emissive_factor[1]);
		append(json, ",");
		append(json, material.emissive_factor[2]);
		append(json, "]");
	}

	if (material.alpha_mode != cgltf_alpha_mode_opaque)
	{
		comma(json);
		append(json, "\"alphaMode\":");
		append(json, (material.alpha_mode == cgltf_alpha_mode_blend) ? "\"BLEND\"" : "\"MASK\"");
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

	if (material.has_pbr_specular_glossiness || material.unlit)
	{
		comma(json);
		append(json, "\"extensions\":{");

		if (material.has_pbr_specular_glossiness)
		{
			const cgltf_pbr_specular_glossiness& pbr = material.pbr_specular_glossiness;

			comma(json);
			append(json, "\"KHR_materials_pbrSpecularGlossiness\":{");
			if (pbr.diffuse_texture.texture)
			{
				comma(json);
				append(json, "\"diffuseTexture\":");
				writeTextureInfo(json, data, pbr.diffuse_texture, qt);
			}
			if (pbr.specular_glossiness_texture.texture)
			{
				comma(json);
				append(json, "\"specularGlossinessTexture\":");
				writeTextureInfo(json, data, pbr.specular_glossiness_texture, qt);
			}
			if (memcmp(pbr.diffuse_factor, white, 16) != 0)
			{
				comma(json);
				append(json, "\"diffuseFactor\":[");
				append(json, pbr.diffuse_factor[0]);
				append(json, ",");
				append(json, pbr.diffuse_factor[1]);
				append(json, ",");
				append(json, pbr.diffuse_factor[2]);
				append(json, ",");
				append(json, pbr.diffuse_factor[3]);
				append(json, "]");
			}
			if (memcmp(pbr.specular_factor, white, 12) != 0)
			{
				comma(json);
				append(json, "\"specularFactor\":[");
				append(json, pbr.specular_factor[0]);
				append(json, ",");
				append(json, pbr.specular_factor[1]);
				append(json, ",");
				append(json, pbr.specular_factor[2]);
				append(json, "]");
			}
			if (pbr.glossiness_factor != 1)
			{
				comma(json);
				append(json, "\"glossinessFactor\":");
				append(json, pbr.glossiness_factor);
			}
			append(json, "}");
		}
		if (material.unlit)
		{
			comma(json);
			append(json, "\"KHR_materials_unlit\":{}");
		}

		append(json, "}");
	}
}

size_t getBufferView(std::vector<BufferView>& views, BufferView::Kind kind, int variant, size_t stride, bool compressed)
{
	if (variant >= 0)
	{
		for (size_t i = 0; i < views.size(); ++i)
			if (views[i].kind == kind && views[i].variant == variant && views[i].stride == stride && views[i].compressed == compressed)
				return i;
	}

	BufferView view = {kind, variant, stride, compressed};
	views.push_back(view);

	return views.size() - 1;
}

void writeBufferView(std::string& json, BufferView::Kind kind, size_t count, size_t stride, size_t bin_offset, size_t bin_size, int compression, size_t compressed_offset, size_t compressed_size)
{
	assert(bin_size == count * stride);

	// when compression is enabled, we store uncompressed data in buffer 1 and compressed data in buffer 0
	// when compression is disabled, we store uncompressed data in buffer 0
	size_t buffer = compression >= 0 ? 1 : 0;

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
	if (compression >= 0)
	{
		append(json, ",\"extensions\":{");
		append(json, "\"MESHOPT_compression\":{");
		append(json, "\"buffer\":0");
		append(json, ",\"byteOffset\":");
		append(json, size_t(compressed_offset));
		append(json, ",\"byteLength\":");
		append(json, size_t(compressed_size));
		append(json, ",\"byteStride\":");
		append(json, stride);
		append(json, ",\"mode\":");
		append(json, size_t(compression));
		append(json, ",\"count\":");
		append(json, count);
		append(json, "}}");
	}
	append(json, "}");
}

void writeAccessor(std::string& json, size_t view, size_t offset, cgltf_type type, cgltf_component_type component_type, bool normalized, size_t count, const float* min = 0, const float* max = 0, size_t numminmax = 0)
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

		append(json, ",\"min\":[");
		for (size_t k = 0; k < numminmax; ++k)
		{
			comma(json);
			append(json, min[k]);
		}
		append(json, "],\"max\":[");
		for (size_t k = 0; k < numminmax; ++k)
		{
			comma(json);
			append(json, max[k]);
		}
		append(json, "]");
	}

	append(json, "}");
}

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

		if (mesh.node)
		{
			NodeInfo& ni = nodes[mesh.node - data->nodes];

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

bool parseDataUri(const char* uri, std::string& mime_type, std::string& result)
{
	if (strncmp(uri, "data:", 5) == 0)
	{
		const char* comma = strchr(uri, ',');

		if (comma && comma - uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0)
		{
			const char* base64 = comma + 1;
			size_t base64_size = strlen(base64);
			size_t size = base64_size - base64_size / 4;

			if (base64_size >= 2)
			{
				size -= base64[base64_size - 2] == '=';
				size -= base64[base64_size - 1] == '=';
			}

			void* data = 0;

			cgltf_options options = {};
			cgltf_result res = cgltf_load_buffer_base64(&options, size, base64, &data);

			if (res != cgltf_result_success)
				return false;

			mime_type = std::string(uri + 5, comma - 7);
			result = std::string(static_cast<const char*>(data), size);

			free(data);

			return true;
		}
	}

	return false;
}

void writeEmbeddedImage(std::string& json, std::vector<BufferView>& views, const char* data, size_t size, const char* mime_type)
{
	size_t view = getBufferView(views, BufferView::Kind_Image, -1, 1, false);

	assert(views[view].data.empty());
	views[view].data.assign(data, size);

	append(json, "\"bufferView\":");
	append(json, view);
	append(json, ",\"mimeType\":\"");
	append(json, mime_type);
	append(json, "\"");
}

void writeImage(std::string& json, std::vector<BufferView>& views, const cgltf_image& image, const ImageInfo& info, size_t index, const char* input_path, const char* output_path, const Settings& settings)
{
	std::string img_data;
	std::string mime_type;

	if (image.uri && parseDataUri(image.uri, mime_type, img_data))
	{
		// we will re-embed img_data below
	}
	else if (image.buffer_view && image.buffer_view->buffer->data && image.mime_type)
	{
		const cgltf_buffer_view* view = image.buffer_view;

		img_data.assign(static_cast<const char*>(view->buffer->data) + view->offset, view->size);
		mime_type = image.mime_type;
	}
	else if (image.uri && settings.texture_embed)
	{
		std::string full_path = getFullPath(image.uri, input_path);

		if (!readFile(full_path.c_str(), img_data))
		{
			fprintf(stderr, "Warning: unable to read image %s, skipping\n", image.uri);
		}

		mime_type = inferMimeType(image.uri);
	}

	if (!img_data.empty())
	{
		if (settings.texture_basis)
		{
			std::string encoded;

			if (encodeBasis(img_data, encoded, info.normal_map, info.srgb, settings.texture_quality))
			{
				if (settings.texture_ktx2)
					encoded = basisToKtx(encoded, info.srgb);

				writeEmbeddedImage(json, views, encoded.c_str(), encoded.size(), settings.texture_ktx2 ? "image/ktx2" : "image/basis");
			}
			else
			{
				fprintf(stderr, "Warning: unable to encode image %d with Basis, skipping\n", int(index));
			}
		}
		else
		{
			writeEmbeddedImage(json, views, img_data.c_str(), img_data.size(), mime_type.c_str());
		}
	}
	else if (image.uri)
	{
		if (settings.texture_basis)
		{
			std::string full_path = getFullPath(image.uri, input_path);
			std::string basis_path = getFileName(image.uri) + (settings.texture_ktx2 ? ".ktx2" : ".basis");
			std::string basis_full_path = getFullPath(basis_path.c_str(), output_path);

			if (readFile(full_path.c_str(), img_data))
			{
				std::string encoded;

				if (encodeBasis(img_data, encoded, info.normal_map, info.srgb, settings.texture_quality))
				{
					if (settings.texture_ktx2)
						encoded = basisToKtx(encoded, info.srgb);

					if (writeFile(basis_full_path.c_str(), encoded))
					{
						append(json, "\"uri\":\"");
						append(json, basis_path);
						append(json, "\"");
					}
					else
					{
						fprintf(stderr, "Warning: unable to save Basis image %s, skipping\n", image.uri);
					}
				}
				else
				{
					fprintf(stderr, "Warning: unable to encode image %s with Basis, skipping\n", image.uri);
				}
			}
			else
			{
				fprintf(stderr, "Warning: unable to read image %s, skipping\n", image.uri);
			}
		}
		else
		{
			append(json, "\"uri\":\"");
			append(json, image.uri);
			append(json, "\"");
		}
	}
	else
	{
		fprintf(stderr, "Warning: ignoring image %d since it has no URI and no valid buffer data\n", int(index));
	}
}

void writeTexture(std::string& json, const cgltf_texture& texture, cgltf_data* data, const Settings& settings)
{
	if (texture.image)
	{
		if (settings.texture_ktx2)
		{
			append(json, "\"extensions\":{\"KHR_texture_basisu\":{\"source\":");
			append(json, size_t(texture.image - data->images));
			append(json, "}}");
		}
		else
		{
			append(json, "\"source\":");
			append(json, size_t(texture.image - data->images));
		}
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
		StreamFormat format = writeVertexStream(scratch, stream, qp, qt, settings, mesh.targets > 0);

		size_t view = getBufferView(views, BufferView::Kind_Vertex, stream.type, format.stride, settings.compress);
		size_t offset = views[view].data.size();
		views[view].data += scratch;

		comma(json_accessors);
		if (stream.type == cgltf_attribute_type_position)
		{
			int min[3] = {};
			int max[3] = {};
			getPositionBounds(min, max, stream, qp);

			float minf[3] = {float(min[0]), float(min[1]), float(min[2])};
			float maxf[3] = {float(max[0]), float(max[1]), float(max[2])};

			writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, stream.data.size(), minf, maxf, 3);
		}
		else
		{
			writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, stream.data.size());
		}

		size_t vertex_accr = accr_offset++;

		comma(json);
		append(json, "\"");
		append(json, attributeType(stream.type));
		if (stream.type != cgltf_attribute_type_position && stream.type != cgltf_attribute_type_normal && stream.type != cgltf_attribute_type_tangent)
		{
			append(json, "_");
			append(json, size_t(stream.index));
		}
		append(json, "\":");
		append(json, vertex_accr);
	}
}

size_t writeMeshIndices(std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, const Mesh& mesh, const Settings& settings)
{
	std::string scratch;
	StreamFormat format = writeIndexStream(scratch, mesh.indices);

	size_t view = getBufferView(views, BufferView::Kind_Index, 0, format.stride, settings.compress);
	size_t offset = views[view].data.size();
	views[view].data += scratch;

	comma(json_accessors);
	writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, mesh.indices.size());

	size_t index_accr = accr_offset++;

	return index_accr;
}

size_t writeAnimationTime(std::vector<BufferView>& views, std::string& json_accessors, size_t& accr_offset, float mint, int frames, const Settings& settings)
{
	std::vector<float> time(frames);

	for (int j = 0; j < frames; ++j)
		time[j] = mint + float(j) / settings.anim_freq;

	std::string scratch;
	StreamFormat format = writeTimeStream(scratch, time);

	size_t view = getBufferView(views, BufferView::Kind_Time, 0, format.stride, settings.compress);
	size_t offset = views[view].data.size();
	views[view].data += scratch;

	comma(json_accessors);
	writeAccessor(json_accessors, view, offset, cgltf_type_scalar, format.component_type, format.normalized, frames, &time.front(), &time.back(), 1);

	size_t time_accr = accr_offset++;

	return time_accr;
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

		float node_scale = qp.scale / float((1 << qp.bits) - 1);

		// pos_offset has to be applied first, thus it results in an offset rotated by the bind matrix
		transform[12] += qp.offset[0] * transform[0] + qp.offset[1] * transform[4] + qp.offset[2] * transform[8];
		transform[13] += qp.offset[0] * transform[1] + qp.offset[1] * transform[5] + qp.offset[2] * transform[9];
		transform[14] += qp.offset[0] * transform[2] + qp.offset[1] * transform[6] + qp.offset[2] * transform[10];

		// node_scale will be applied before the rotation/scale from transform
		for (int k = 0; k < 12; ++k)
			transform[k] *= node_scale;

		scratch.append(reinterpret_cast<const char*>(transform), sizeof(transform));
	}

	size_t view = getBufferView(views, BufferView::Kind_Skin, 0, 64, settings.compress);
	size_t offset = views[view].data.size();
	views[view].data += scratch;

	comma(json_accessors);
	writeAccessor(json_accessors, view, offset, cgltf_type_mat4, cgltf_component_type_r_32f, false, skin.joints_count);

	size_t matrix_accr = accr_offset++;

	return matrix_accr;
}

void writeMeshNode(std::string& json, size_t mesh_offset, const Mesh& mesh, cgltf_data* data, const QuantizationPosition& qp)
{
	float node_scale = qp.scale / float((1 << qp.bits) - 1);

	comma(json);
	append(json, "{\"mesh\":");
	append(json, mesh_offset);
	if (mesh.skin)
	{
		comma(json);
		append(json, "\"skin\":");
		append(json, size_t(mesh.skin - data->skins));
	}
	append(json, ",\"translation\":[");
	append(json, qp.offset[0]);
	append(json, ",");
	append(json, qp.offset[1]);
	append(json, ",");
	append(json, qp.offset[2]);
	append(json, "],\"scale\":[");
	append(json, node_scale);
	append(json, ",");
	append(json, node_scale);
	append(json, ",");
	append(json, node_scale);
	append(json, "]");
	if (mesh.node && mesh.node->weights_count)
	{
		append(json, ",\"weights\":[");
		for (size_t j = 0; j < mesh.node->weights_count; ++j)
		{
			comma(json);
			append(json, mesh.node->weights[j]);
		}
		append(json, "]");
	}
	append(json, "}");
}

void writeNode(std::string& json, const cgltf_node& node, const std::vector<NodeInfo>& nodes, cgltf_data* data)
{
	const NodeInfo& ni = nodes[&node - data->nodes];

	comma(json);
	append(json, "{");
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
		append(json, "\"translation\":[");
		append(json, node.translation[0]);
		append(json, ",");
		append(json, node.translation[1]);
		append(json, ",");
		append(json, node.translation[2]);
		append(json, "]");
	}
	if (node.has_rotation)
	{
		comma(json);
		append(json, "\"rotation\":[");
		append(json, node.rotation[0]);
		append(json, ",");
		append(json, node.rotation[1]);
		append(json, ",");
		append(json, node.rotation[2]);
		append(json, ",");
		append(json, node.rotation[3]);
		append(json, "]");
	}
	if (node.has_scale)
	{
		comma(json);
		append(json, "\"scale\":[");
		append(json, node.scale[0]);
		append(json, ",");
		append(json, node.scale[1]);
		append(json, ",");
		append(json, node.scale[2]);
		append(json, "]");
	}
	if (node.has_matrix)
	{
		comma(json);
		append(json, "\"matrix\":[");
		for (int k = 0; k < 16; ++k)
		{
			comma(json);
			append(json, node.matrix[k]);
		}
		append(json, "]");
	}
	if (node.children_count || !ni.meshes.empty())
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
		for (size_t j = 0; j < ni.meshes.size(); ++j)
		{
			comma(json);
			append(json, ni.meshes[j]);
		}
		append(json, "]");
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
	append(json, "}");
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

		if (!settings.anim_const && (ni.animated_paths & (1 << track.path)) == 0)
			continue;

		tracks.push_back(&track);
	}

	if (tracks.empty())
	{
		fprintf(stderr, "Warning: ignoring animation %d because it has no valid tracks\n", int(i));
		return;
	}

	bool needs_time = false;
	bool needs_pose = false;

	for (size_t j = 0; j < tracks.size(); ++j)
	{
		const Track& track = *tracks[j];

		bool tc = track.data.size() == track.components;

		needs_time = needs_time || !tc;
		needs_pose = needs_pose || tc;
	}

	size_t time_accr = needs_time ? writeAnimationTime(views, json_accessors, accr_offset, animation.start, animation.frames, settings) : 0;
	size_t pose_accr = needs_pose ? writeAnimationTime(views, json_accessors, accr_offset, animation.start, 1, settings) : 0;

	std::string json_samplers;
	std::string json_channels;

	size_t track_offset = 0;

	for (size_t j = 0; j < tracks.size(); ++j)
	{
		const Track& track = *tracks[j];

		bool tc = track.data.size() == track.components;

		std::string scratch;
		StreamFormat format = writeKeyframeStream(scratch, track.path, track.data);

		size_t view = getBufferView(views, BufferView::Kind_Keyframe, track.path, format.stride, settings.compress && track.path != cgltf_animation_path_type_weights);
		size_t offset = views[view].data.size();
		views[view].data += scratch;

		comma(json_accessors);
		writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, track.data.size());

		size_t data_accr = accr_offset++;

		comma(json_samplers);
		append(json_samplers, "{\"input\":");
		append(json_samplers, tc ? pose_accr : time_accr);
		append(json_samplers, ",\"output\":");
		append(json_samplers, data_accr);
		append(json_samplers, "}");

		const NodeInfo& tni = nodes[track.node - data->nodes];
		size_t target_node = size_t(tni.remap);

		if (track.path == cgltf_animation_path_type_weights)
		{
			assert(tni.meshes.size() == 1);
			target_node = tni.meshes[0];
		}

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
	static const float white[3] = {1, 1, 1};

	comma(json);
	append(json, "{\"type\":\"");
	append(json, lightType(light.type));
	append(json, "\"");
	if (memcmp(light.color, white, sizeof(white)) != 0)
	{
		comma(json);
		append(json, "\"color\":[");
		append(json, light.color[0]);
		append(json, ",");
		append(json, light.color[1]);
		append(json, ",");
		append(json, light.color[2]);
		append(json, "]");
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

void finalizeBufferViews(std::string& json, std::vector<BufferView>& views, std::string& bin, std::string& fallback)
{
	for (size_t i = 0; i < views.size(); ++i)
	{
		BufferView& view = views[i];

		size_t bin_offset = bin.size();
		size_t fallback_offset = fallback.size();

		size_t count = view.data.size() / view.stride;

		int compression = -1;

		if (view.compressed)
		{
			if (view.kind == BufferView::Kind_Index)
			{
				compressIndexStream(bin, view.data, count, view.stride);
				compression = 1;
			}
			else
			{
				compressVertexStream(bin, view.data, count, view.stride);
				compression = 0;
			}

			fallback += view.data;
		}
		else
		{
			bin += view.data;
		}

		size_t raw_offset = (compression >= 0) ? fallback_offset : bin_offset;

		comma(json);
		writeBufferView(json, view.kind, count, view.stride, raw_offset, view.data.size(), compression, bin_offset, bin.size() - bin_offset);

		// record written bytes for statistics
		view.bytes = bin.size() - bin_offset;

		// align each bufferView by 4 bytes
		bin.resize((bin.size() + 3) & ~3);
		fallback.resize((fallback.size() + 3) & ~3);
	}
}

void printMeshStats(const std::vector<Mesh>& meshes, const char* name)
{
	size_t triangles = 0;
	size_t vertices = 0;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		triangles += mesh.indices.size() / 3;
		vertices += mesh.streams.empty() ? 0 : mesh.streams[0].data.size();
	}

	printf("%s: %d triangles, %d vertices\n", name, int(triangles), int(vertices));
}

void printSceneStats(const std::vector<BufferView>& views, const std::vector<Mesh>& meshes, size_t node_offset, size_t mesh_offset, size_t material_offset, size_t json_size, size_t bin_size)
{
	size_t bytes[BufferView::Kind_Count] = {};

	for (size_t i = 0; i < views.size(); ++i)
	{
		const BufferView& view = views[i];
		bytes[view.kind] += view.bytes;
	}

	printf("output: %d nodes, %d meshes (%d primitives), %d materials\n", int(node_offset), int(mesh_offset), int(meshes.size()), int(material_offset));
	printf("output: JSON %d bytes, buffers %d bytes\n", int(json_size), int(bin_size));
	printf("output: buffers: vertex %d bytes, index %d bytes, skin %d bytes, time %d bytes, keyframe %d bytes, image %d bytes\n",
	       int(bytes[BufferView::Kind_Vertex]), int(bytes[BufferView::Kind_Index]), int(bytes[BufferView::Kind_Skin]),
	       int(bytes[BufferView::Kind_Time]), int(bytes[BufferView::Kind_Keyframe]), int(bytes[BufferView::Kind_Image]));
}

void printAttributeStats(const std::vector<BufferView>& views, BufferView::Kind kind, const char* name)
{
	for (size_t i = 0; i < views.size(); ++i)
	{
		const BufferView& view = views[i];

		if (view.kind != kind)
			continue;

		const char* variant = "unknown";

		switch (kind)
		{
		case BufferView::Kind_Vertex:
			variant = attributeType(cgltf_attribute_type(view.variant));
			break;

		case BufferView::Kind_Index:
			variant = "index";
			break;

		case BufferView::Kind_Keyframe:
			variant = animationPath(cgltf_animation_path_type(view.variant));
			break;

		default:;
		}

		size_t count = view.data.size() / view.stride;

		printf("stats: %s %s: compressed %d bytes (%.1f bits), raw %d bytes (%d bits)\n",
		       name,
		       variant,
		       int(view.bytes),
		       double(view.bytes) / double(count) * 8,
		       int(view.data.size()),
		       int(view.stride * 8));
	}
}

void process(cgltf_data* data, const char* input_path, const char* output_path, std::vector<Mesh>& meshes, std::vector<Animation>& animations, const Settings& settings, std::string& json, std::string& bin, std::string& fallback)
{
	if (settings.verbose)
	{
		printf("input: %d nodes, %d meshes (%d primitives), %d materials, %d skins, %d animations\n",
		       int(data->nodes_count), int(data->meshes_count), int(meshes.size()), int(data->materials_count), int(data->skins_count), int(animations.size()));
		printMeshStats(meshes, "input");
	}

	for (size_t i = 0; i < animations.size(); ++i)
	{
		processAnimation(animations[i], settings);
	}

	std::vector<NodeInfo> nodes(data->nodes_count);

	markAnimated(data, nodes, animations);

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		// note: when -kn is specified, we keep mesh-node attachment so that named nodes can be transformed
		if (mesh.node && !settings.keep_named)
		{
			NodeInfo& ni = nodes[mesh.node - data->nodes];

			// we transform all non-skinned non-animated meshes to world space
			// this makes sure that quantization doesn't introduce gaps if the original scene was watertight
			if (!ni.animated && !mesh.skin && mesh.targets == 0)
			{
				transformMesh(mesh, mesh.node);
				mesh.node = 0;
			}

			// skinned and animated meshes will be anchored to the same node that they used to be in
			// for animated meshes, this is important since they need to be transformed by the same animation
			// for skinned meshes, in theory this isn't important since the transform of the skinned node doesn't matter; in practice this affects generated bounding box in three.js
		}
	}

	mergeMeshMaterials(data, meshes);
	mergeMeshes(meshes, settings);
	filterEmptyMeshes(meshes);

	markNeededNodes(data, nodes, meshes, animations, settings);

	std::vector<MaterialInfo> materials(data->materials_count);

	markNeededMaterials(data, materials, meshes);

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		processMesh(meshes[i], settings);
	}

	filterEmptyMeshes(meshes); // some meshes may become empty after processing

	std::vector<ImageInfo> images(data->images_count);

	analyzeImages(data, images);

	QuantizationPosition qp = prepareQuantizationPosition(meshes, settings);

	std::vector<QuantizationTexture> qt_materials(materials.size());
	prepareQuantizationTexture(data, qt_materials, meshes, settings);

	QuantizationTexture qt_dummy = {};
	qt_dummy.bits = settings.tex_bits;

	std::string json_images;
	std::string json_textures;
	std::string json_materials;
	std::string json_accessors;
	std::string json_meshes;
	std::string json_nodes;
	std::string json_skins;
	std::string json_roots;
	std::string json_animations;
	std::string json_cameras;
	std::string json_lights;

	std::vector<BufferView> views;

	bool ext_pbr_specular_glossiness = false;
	bool ext_unlit = false;

	size_t accr_offset = 0;
	size_t node_offset = 0;
	size_t mesh_offset = 0;
	size_t material_offset = 0;

	for (size_t i = 0; i < data->images_count; ++i)
	{
		const cgltf_image& image = data->images[i];

		if (settings.verbose && settings.texture_basis)
		{
			const char* uri = image.uri;
			bool embedded = !uri || strncmp(uri, "data:", 5) == 0;

			printf("image %d (%s) is being encoded with Basis\n", int(i), embedded ? "embedded" : uri);
		}

		comma(json_images);
		append(json_images, "{");
		writeImage(json_images, views, image, images[i], i, input_path, output_path, settings);
		append(json_images, "}");
	}

	for (size_t i = 0; i < data->textures_count; ++i)
	{
		const cgltf_texture& texture = data->textures[i];

		comma(json_textures);
		append(json_textures, "{");
		writeTexture(json_textures, texture, data, settings);
		append(json_textures, "}");
	}

	for (size_t i = 0; i < data->materials_count; ++i)
	{
		MaterialInfo& mi = materials[i];

		if (!mi.keep)
			continue;

		const cgltf_material& material = data->materials[i];

		comma(json_materials);
		append(json_materials, "{");
		writeMaterialInfo(json_materials, data, material, qt_materials[i]);
		append(json_materials, "}");

		mi.remap = int(material_offset);
		material_offset++;

		ext_pbr_specular_glossiness = ext_pbr_specular_glossiness || material.has_pbr_specular_glossiness;
		ext_unlit = ext_unlit || material.unlit;
	}

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		comma(json_meshes);
		append(json_meshes, "{\"primitives\":[");

		size_t pi = i;
		for (; pi < meshes.size(); ++pi)
		{
			const Mesh& prim = meshes[pi];

			if (prim.node != mesh.node || prim.skin != mesh.skin || prim.targets != mesh.targets)
				break;

			if (!compareMeshTargets(mesh, prim))
				break;

			const QuantizationTexture& qt = prim.material ? qt_materials[prim.material - data->materials] : qt_dummy;

			comma(json_meshes);
			append(json_meshes, "{\"attributes\":{");
			writeMeshAttributes(json_meshes, views, json_accessors, accr_offset, prim, 0, qp, qt, settings);
			append(json_meshes, "}");
			append(json_meshes, ",\"mode\":");
			append(json_meshes, size_t(prim.type));

			if (mesh.targets)
			{
				append(json_meshes, ",\"targets\":[");
				for (size_t j = 0; j < mesh.targets; ++j)
				{
					comma(json_meshes);
					append(json_meshes, "{");
					writeMeshAttributes(json_meshes, views, json_accessors, accr_offset, prim, int(1 + j), qp, qt, settings);
					append(json_meshes, "}");
				}
				append(json_meshes, "]");
			}

			if (!prim.indices.empty())
			{
				size_t index_accr = writeMeshIndices(views, json_accessors, accr_offset, prim, settings);

				append(json_meshes, ",\"indices\":");
				append(json_meshes, index_accr);
			}

			if (prim.material)
			{
				MaterialInfo& mi = materials[prim.material - data->materials];

				assert(mi.keep);
				append(json_meshes, ",\"material\":");
				append(json_meshes, size_t(mi.remap));
			}

			append(json_meshes, "}");
		}

		append(json_meshes, "]");

		if (mesh.target_weights.size())
		{
			append(json_meshes, ",\"weights\":[");
			for (size_t j = 0; j < mesh.target_weights.size(); ++j)
			{
				comma(json_meshes);
				append(json_meshes, mesh.target_weights[j]);
			}
			append(json_meshes, "]");
		}

		if (mesh.target_names.size())
		{
			append(json_meshes, ",\"extras\":{\"targetNames\":[");
			for (size_t j = 0; j < mesh.target_names.size(); ++j)
			{
				comma(json_meshes);
				append(json_meshes, "\"");
				append(json_meshes, mesh.target_names[j]);
				append(json_meshes, "\"");
			}
			append(json_meshes, "]}");
		}

		append(json_meshes, "}");

		writeMeshNode(json_nodes, mesh_offset, mesh, data, qp);

		if (mesh.node)
		{
			NodeInfo& ni = nodes[mesh.node - data->nodes];

			assert(ni.keep);
			ni.meshes.push_back(node_offset);
		}
		else
		{
			comma(json_roots);
			append(json_roots, node_offset);
		}

		node_offset++;
		mesh_offset++;

		// skip all meshes that we've written in this iteration
		assert(pi > i);
		i = pi - 1;
	}

	remapNodes(data, nodes, node_offset);

	for (size_t i = 0; i < data->nodes_count; ++i)
	{
		NodeInfo& ni = nodes[i];

		if (!ni.keep)
			continue;

		const cgltf_node& node = data->nodes[i];

		if (!node.parent)
		{
			comma(json_roots);
			append(json_roots, size_t(ni.remap));
		}

		writeNode(json_nodes, node, nodes, data);
	}

	for (size_t i = 0; i < data->skins_count; ++i)
	{
		const cgltf_skin& skin = data->skins[i];

		size_t matrix_accr = writeJointBindMatrices(views, json_accessors, accr_offset, skin, qp, settings);

		comma(json_skins);
		append(json_skins, "{");
		append(json_skins, "\"joints\":[");
		for (size_t j = 0; j < skin.joints_count; ++j)
		{
			comma(json_skins);
			append(json_skins, size_t(nodes[skin.joints[j] - data->nodes].remap));
		}
		append(json_skins, "]");
		append(json_skins, ",\"inverseBindMatrices\":");
		append(json_skins, matrix_accr);
		if (skin.skeleton)
		{
			comma(json_skins);
			append(json_skins, "\"skeleton\":");
			append(json_skins, size_t(nodes[skin.skeleton - data->nodes].remap));
		}
		append(json_skins, "}");
	}

	for (size_t i = 0; i < animations.size(); ++i)
	{
		const Animation& animation = animations[i];

		writeAnimation(json_animations, views, json_accessors, accr_offset, animation, i, data, nodes, settings);
	}

	for (size_t i = 0; i < data->cameras_count; ++i)
	{
		const cgltf_camera& camera = data->cameras[i];

		writeCamera(json_cameras, camera);
	}

	for (size_t i = 0; i < data->lights_count; ++i)
	{
		const cgltf_light& light = data->lights[i];

		writeLight(json_lights, light);
	}

	append(json, "\"asset\":{");
	append(json, "\"version\":\"2.0\",\"generator\":\"gltfpack ");
	append(json, getVersion());
	append(json, "\"");
	if (data->asset.extras.start_offset)
	{
		append(json, ",\"extras\":");
		json.append(data->json + data->asset.extras.start_offset, data->json + data->asset.extras.end_offset);
	}
	append(json, "}");

	const ExtensionInfo extensions[] = {
	    {"KHR_mesh_quantization", true, true},
	    {"MESHOPT_compression", settings.compress, !settings.fallback},
	    {"KHR_texture_transform", !json_textures.empty(), false},
	    {"KHR_materials_pbrSpecularGlossiness", ext_pbr_specular_glossiness, false},
	    {"KHR_materials_unlit", ext_unlit, false},
	    {"KHR_lights_punctual", data->lights_count > 0, false},
	    {"KHR_texture_basisu", !json_textures.empty() && settings.texture_ktx2, true},
	};

	writeExtensions(json, extensions, sizeof(extensions) / sizeof(extensions[0]));

	std::string json_views;
	finalizeBufferViews(json_views, views, bin, fallback);

	writeArray(json, "bufferViews", json_views);
	writeArray(json, "accessors", json_accessors);
	writeArray(json, "images", json_images);
	writeArray(json, "textures", json_textures);
	writeArray(json, "materials", json_materials);
	writeArray(json, "meshes", json_meshes);
	writeArray(json, "skins", json_skins);
	writeArray(json, "animations", json_animations);
	writeArray(json, "nodes", json_nodes);

	if (!json_roots.empty())
	{
		append(json, ",\"scenes\":[");
		append(json, "{\"nodes\":[");
		append(json, json_roots);
		append(json, "]}]");
	}

	writeArray(json, "cameras", json_cameras);

	if (!json_lights.empty())
	{
		append(json, ",\"extensions\":{\"KHR_lights_punctual\":{\"lights\":[");
		append(json, json_lights);
		append(json, "]}}");
	}
	if (!json_roots.empty())
	{
		append(json, ",\"scene\":0");
	}

	if (settings.verbose)
	{
		printMeshStats(meshes, "output");
		printSceneStats(views, meshes, node_offset, mesh_offset, material_offset, json.size(), bin.size());
	}

	if (settings.verbose > 1)
	{
		printAttributeStats(views, BufferView::Kind_Vertex, "vertex");
		printAttributeStats(views, BufferView::Kind_Index, "index");
		printAttributeStats(views, BufferView::Kind_Keyframe, "keyframe");
	}
}

void writeU32(FILE* out, uint32_t data)
{
	fwrite(&data, 4, 1, out);
}

const char* getBaseName(const char* path)
{
	const char* slash = strrchr(path, '/');
	const char* backslash = strrchr(path, '\\');

	const char* rs = slash ? slash + 1 : path;
	const char* bs = backslash ? backslash + 1 : path;

	return std::max(rs, bs);
}

std::string getBufferSpec(const char* bin_path, size_t bin_size, const char* fallback_path, size_t fallback_size, bool fallback_ref)
{
	std::string json;
	append(json, "\"buffers\":[");
	append(json, "{");
	if (bin_path)
	{
		append(json, "\"uri\":\"");
		append(json, bin_path);
		append(json, "\"");
	}
	comma(json);
	append(json, "\"byteLength\":");
	append(json, bin_size);
	append(json, "}");
	if (fallback_ref)
	{
		comma(json);
		append(json, "{");
		if (fallback_path)
		{
			append(json, "\"uri\":\"");
			append(json, fallback_path);
			append(json, "\"");
		}
		comma(json);
		append(json, "\"byteLength\":");
		append(json, fallback_size);
		append(json, ",\"extensions\":{");
		append(json, "\"MESHOPT_compression\":{");
		append(json, "\"fallback\":true");
		append(json, "}}");
		append(json, "}");
	}
	append(json, "]");

	return json;
}

int gltfpack(const char* input, const char* output, const Settings& settings)
{
	cgltf_data* data = 0;
	std::vector<Mesh> meshes;
	std::vector<Animation> animations;

	const char* iext = strrchr(input, '.');

	if (iext && (strcmp(iext, ".gltf") == 0 || strcmp(iext, ".GLTF") == 0 || strcmp(iext, ".glb") == 0 || strcmp(iext, ".GLB") == 0))
	{
		const char* error = 0;
		data = parseGltf(input, meshes, animations, &error);

		if (error)
		{
			fprintf(stderr, "Error loading %s: %s\n", input, error);
			return 2;
		}
	}
	else if (iext && (strcmp(iext, ".obj") == 0 || strcmp(iext, ".OBJ") == 0))
	{
		const char* error = 0;
		data = parseObj(input, meshes, &error);

		if (!data)
		{
			fprintf(stderr, "Error loading %s: %s\n", input, error);
			return 2;
		}
	}
	else
	{
		fprintf(stderr, "Error loading %s: unknown extension (expected .gltf or .glb or .obj)\n", input);
		return 2;
	}

	if (data->images_count && settings.texture_basis)
	{
		if (!checkBasis())
		{
			fprintf(stderr, "Error: basisu is not present in PATH or BASISU_PATH is not set\n");
			return 3;
		}
	}

	std::string json, bin, fallback;
	process(data, input, output, meshes, animations, settings, json, bin, fallback);

	cgltf_free(data);

	if (!output)
	{
		return 0;
	}

	const char* oext = strrchr(output, '.');

	if (oext && (strcmp(oext, ".gltf") == 0 || strcmp(oext, ".GLTF") == 0))
	{
		std::string binpath = output;
		binpath.replace(binpath.size() - 5, 5, ".bin");

		std::string fbpath = output;
		fbpath.replace(fbpath.size() - 5, 5, ".fallback.bin");

		FILE* outjson = fopen(output, "wb");
		FILE* outbin = fopen(binpath.c_str(), "wb");
		FILE* outfb = settings.fallback ? fopen(fbpath.c_str(), "wb") : NULL;
		if (!outjson || !outbin || (!outfb && settings.fallback))
		{
			fprintf(stderr, "Error saving %s\n", output);
			return 4;
		}

		std::string bufferspec = getBufferSpec(getBaseName(binpath.c_str()), bin.size(), settings.fallback ? getBaseName(fbpath.c_str()) : NULL, fallback.size(), settings.compress);

		fprintf(outjson, "{");
		fwrite(bufferspec.c_str(), bufferspec.size(), 1, outjson);
		fprintf(outjson, ",");
		fwrite(json.c_str(), json.size(), 1, outjson);
		fprintf(outjson, "}");

		fwrite(bin.c_str(), bin.size(), 1, outbin);

		if (settings.fallback)
			fwrite(fallback.c_str(), fallback.size(), 1, outfb);

		fclose(outjson);
		fclose(outbin);
		if (outfb)
			fclose(outfb);
	}
	else if (oext && (strcmp(oext, ".glb") == 0 || strcmp(oext, ".GLB") == 0))
	{
		std::string fbpath = output;
		fbpath.replace(fbpath.size() - 4, 4, ".fallback.bin");

		FILE* out = fopen(output, "wb");
		FILE* outfb = settings.fallback ? fopen(fbpath.c_str(), "wb") : NULL;
		if (!out || (!outfb && settings.fallback))
		{
			fprintf(stderr, "Error saving %s\n", output);
			return 4;
		}

		std::string bufferspec = getBufferSpec(NULL, bin.size(), settings.fallback ? getBaseName(fbpath.c_str()) : NULL, fallback.size(), settings.compress);

		json.insert(0, "{" + bufferspec + ",");
		json.push_back('}');

		while (json.size() % 4)
			json.push_back(' ');

		while (bin.size() % 4)
			bin.push_back('\0');

		writeU32(out, 0x46546C67);
		writeU32(out, 2);
		writeU32(out, uint32_t(12 + 8 + json.size() + 8 + bin.size()));

		writeU32(out, uint32_t(json.size()));
		writeU32(out, 0x4E4F534A);
		fwrite(json.c_str(), json.size(), 1, out);

		writeU32(out, uint32_t(bin.size()));
		writeU32(out, 0x004E4942);
		fwrite(bin.c_str(), bin.size(), 1, out);

		if (settings.fallback)
			fwrite(fallback.c_str(), fallback.size(), 1, outfb);

		fclose(out);
		if (outfb)
			fclose(outfb);
	}
	else
	{
		fprintf(stderr, "Error saving %s: unknown extension (expected .gltf or .glb)\n", output);
		return 4;
	}

	return 0;
}

int main(int argc, char** argv)
{
	meshopt_encodeIndexVersion(1);

	Settings settings = {};
	settings.pos_bits = 14;
	settings.tex_bits = 12;
	settings.nrm_bits = 8;
	settings.anim_freq = 30;
	settings.simplify_threshold = 1.f;
	settings.texture_quality = 50;

	const char* input = 0;
	const char* output = 0;
	bool help = false;
	int test = 0;

	for (int i = 1; i < argc; ++i)
	{
		const char* arg = argv[i];

		if (strcmp(arg, "-vp") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.pos_bits = atoi(argv[++i]);
		}
		else if (strcmp(arg, "-vt") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.tex_bits = atoi(argv[++i]);
		}
		else if (strcmp(arg, "-vn") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.nrm_bits = atoi(argv[++i]);
		}
		else if (strcmp(arg, "-vu") == 0)
		{
			settings.nrm_unnormalized = true;
		}
		else if (strcmp(arg, "-af") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.anim_freq = atoi(argv[++i]);
		}
		else if (strcmp(arg, "-ac") == 0)
		{
			settings.anim_const = true;
		}
		else if (strcmp(arg, "-kn") == 0)
		{
			settings.keep_named = true;
		}
		else if (strcmp(arg, "-si") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.simplify_threshold = float(atof(argv[++i]));
		}
		else if (strcmp(arg, "-sa") == 0)
		{
			settings.simplify_aggressive = true;
		}
		else if (strcmp(arg, "-te") == 0)
		{
			settings.texture_embed = true;
		}
		else if (strcmp(arg, "-tb") == 0)
		{
			settings.texture_basis = true;
		}
		else if (strcmp(arg, "-tc") == 0)
		{
			settings.texture_basis = true;
			settings.texture_ktx2 = true;
		}
		else if (strcmp(arg, "-tq") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.texture_quality = atoi(argv[++i]);
		}
		else if (strcmp(arg, "-i") == 0 && i + 1 < argc && !input)
		{
			input = argv[++i];
		}
		else if (strcmp(arg, "-o") == 0 && i + 1 < argc && !output)
		{
			output = argv[++i];
		}
		else if (strcmp(arg, "-c") == 0)
		{
			settings.compress = true;
		}
		else if (strcmp(arg, "-cc") == 0)
		{
			settings.compress = true;
			settings.compressmore = true;
		}
		else if (strcmp(arg, "-cf") == 0)
		{
			settings.compress = true;
			settings.fallback = true;
		}
		else if (strcmp(arg, "-v") == 0)
		{
			settings.verbose = 1;
		}
		else if (strcmp(arg, "-vv") == 0)
		{
			settings.verbose = 2;
		}
		else if (strcmp(arg, "-h") == 0)
		{
			help = true;
		}
		else if (strcmp(arg, "-test") == 0)
		{
			test = i + 1;
			break;
		}
		else
		{
			fprintf(stderr, "Unrecognized option %s\n", arg);
			return 1;
		}
	}

	// shortcut for gltfpack -v
	if (settings.verbose && argc == 2)
	{
		printf("gltfpack %s\n", getVersion().c_str());
		return 0;
	}

	if (test)
	{
		for (int i = test; i < argc; ++i)
		{
			printf("%s\n", argv[i]);
			gltfpack(argv[i], NULL, settings);
		}

		return 0;
	}

	if (!input || !output || help)
	{
		fprintf(stderr, "gltfpack %s\n", getVersion().c_str());
		fprintf(stderr, "Usage: gltfpack [options] -i input -o output\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "-i file: input file to process, .obj/.gltf/.glb\n");
		fprintf(stderr, "-o file: output file path, .gltf/.glb\n");
		fprintf(stderr, "-vp N: use N-bit quantization for positions (default: 14; N should be between 1 and 16)\n");
		fprintf(stderr, "-vt N: use N-bit quantization for texture corodinates (default: 12; N should be between 1 and 16)\n");
		fprintf(stderr, "-vn N: use N-bit quantization for normals and tangents (default: 8; N should be between 1 and 16)\n");
		fprintf(stderr, "-vu: use unnormalized normal/tangent vectors to improve compression (default: off)\n");
		fprintf(stderr, "-af N: resample animations at N Hz (default: 30)\n");
		fprintf(stderr, "-ac: keep constant animation tracks even if they don't modify the node transform\n");
		fprintf(stderr, "-kn: keep named nodes and meshes attached to named nodes so that named nodes can be transformed externally\n");
		fprintf(stderr, "-si R: simplify meshes to achieve the ratio R (default: 1; R should be between 0 and 1)\n");
		fprintf(stderr, "-sa: aggressively simplify to the target ratio disregarding quality\n");
		fprintf(stderr, "-te: embed all textures into main buffer\n");
		fprintf(stderr, "-tb: convert all textures to Basis Universal format (with basisu executable)\n");
		fprintf(stderr, "-tc: convert all textures to KTX2 with BasisU supercompression (using basisu executable)\n");
		fprintf(stderr, "-tq N: set texture encoding quality (default: 50; N should be between 1 and 100\n");
		fprintf(stderr, "-c: produce compressed gltf/glb files\n");
		fprintf(stderr, "-cc: produce compressed gltf/glb files with higher compression ratio\n");
		fprintf(stderr, "-cf: produce compressed gltf/glb files with fallback for loaders that don't support compression\n");
		fprintf(stderr, "-v: verbose output (print version when used without other options)\n");
		fprintf(stderr, "-h: display this help and exit\n");

		return 1;
	}

	return gltfpack(input, output, settings);
}
