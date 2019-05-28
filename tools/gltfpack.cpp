#include "../src/meshoptimizer.h"

#include <string>
#include <vector>

#include <float.h>
#include <math.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

struct Attr
{
	float f[4];
};

struct Stream
{
	cgltf_attribute_type type;
	int index;

	std::vector<Attr> data;
};

struct Mesh
{
	cgltf_material* material;
	std::vector<Stream> streams;
	std::vector<unsigned int> indices;
};

struct Scene
{
	cgltf_data* data;
	std::vector<Mesh> meshes;

	~Scene()
	{
		cgltf_free(data);
	}
};

struct Settings
{
	int pos_bits;
	int uv_bits;
	bool compress;
};

struct QuantizationParams
{
	float pos_offset[3];
	float pos_scale;
	int pos_bits;

	float uv_offset[2];
	float uv_scale[2];
	int uv_bits;
};

const char* getError(cgltf_result result)
{
	switch (result)
	{
	case cgltf_result_file_not_found:
		return "file not found";

	case cgltf_result_io_error:
		return "I/O error";

	case cgltf_result_invalid_json:
		return "invalid JSON";

	case cgltf_result_invalid_gltf:
		return "invalid GLTF";

	case cgltf_result_out_of_memory:
		return "out of memory";

	default:
		return "unknown error";
	}
}

cgltf_accessor* getAccessor(const cgltf_attribute* attributes, size_t attribute_count, cgltf_attribute_type type, int index = 0)
{
	for (size_t i = 0; i < attribute_count; ++i)
		if (attributes[i].type == type && attributes[i].index == index)
			return attributes[i].data;

	return 0;
}

void transformPosition(float* ptr, const float* transform)
{
	float x = ptr[0] * transform[0] + ptr[1] * transform[4] + ptr[2] * transform[8] + transform[12];
	float y = ptr[0] * transform[1] + ptr[1] * transform[5] + ptr[2] * transform[9] + transform[13];
	float z = ptr[0] * transform[2] + ptr[1] * transform[6] + ptr[2] * transform[10] + transform[14];

	ptr[0] = x;
	ptr[1] = y;
	ptr[2] = z;
}

void transformNormal(float* ptr, const float* transform)
{
	float x = ptr[0] * transform[0] + ptr[1] * transform[4] + ptr[2] * transform[8];
	float y = ptr[0] * transform[1] + ptr[1] * transform[5] + ptr[2] * transform[9];
	float z = ptr[0] * transform[2] + ptr[1] * transform[6] + ptr[2] * transform[10];

	float l = sqrtf(x * x + y * y + z * z);
	float s = (l == 0.f) ? 0.f : 1 / l;

	ptr[0] = x * s;
	ptr[1] = y * s;
	ptr[2] = z * s;
}

void parseMeshes(cgltf_data* data, std::vector<Mesh>& meshes)
{
	for (size_t ni = 0; ni < data->nodes_count; ++ni)
	{
		if (!data->nodes[ni].mesh)
			continue;

		float transform[16];
		cgltf_node_transform_world(&data->nodes[ni], transform);

		const cgltf_mesh& mesh = *data->nodes[ni].mesh;

		for (size_t pi = 0; pi < mesh.primitives_count; ++pi)
		{
			const cgltf_primitive& primitive = mesh.primitives[pi];

			Mesh result;

			result.material = primitive.material;

			if (cgltf_accessor* a = primitive.indices)
			{
				result.indices.resize(a->count);
				for (size_t i = 0; i < a->count; ++i)
					result.indices[i] = cgltf_accessor_read_index(a, i);
			}

			for (size_t ai = 0; ai < primitive.attributes_count; ++ai)
			{
				const cgltf_attribute& attr = primitive.attributes[ai];

				if (attr.type == cgltf_attribute_type_invalid)
					continue;

				Stream s = { attr.type, attr.index };
				s.data.resize(attr.data->count);

				for (size_t i = 0; i < attr.data->count; ++i)
					cgltf_accessor_read_float(attr.data, i, s.data[i].f, 4);

				if (attr.type == cgltf_attribute_type_position)
				{
					for (size_t i = 0; i < attr.data->count; ++i)
						transformPosition(s.data[i].f, transform);
				}
				else if (attr.type == cgltf_attribute_type_normal || attr.type == cgltf_attribute_type_tangent)
				{
					for (size_t i = 0; i < attr.data->count; ++i)
						transformNormal(s.data[i].f, transform);
				}

				result.streams.push_back(s);
			}

			if (result.indices.size() && result.streams.size())
				meshes.push_back(result);
		}
	}
}

void reindexMesh(Mesh& mesh)
{
	size_t total_vertices = mesh.streams[0].data.size();
	size_t total_indices = mesh.indices.size();

	std::vector<meshopt_Stream> streams;
	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		meshopt_Stream stream = { &mesh.streams[i].data[0], sizeof(Attr), sizeof(Attr) };
		streams.push_back(stream);
	}

	std::vector<unsigned int> remap(total_indices);
	size_t unique_vertices = meshopt_generateVertexRemapMulti(&remap[0], &mesh.indices[0], total_indices, total_vertices, &streams[0], streams.size());

	meshopt_remapIndexBuffer(&mesh.indices[0], &mesh.indices[0], total_indices, &remap[0]);

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], total_vertices, sizeof(Attr), &remap[0]);
		mesh.streams[i].data.resize(unique_vertices);
	}
}

void optimizeMesh(Mesh& mesh)
{
	size_t vertex_count = mesh.streams[0].data.size();

	meshopt_optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), vertex_count);

	std::vector<unsigned int> remap(vertex_count);
	size_t unique_vertices = meshopt_optimizeVertexFetchRemap(&remap[0], &mesh.indices[0], mesh.indices.size(), vertex_count);
	assert(unique_vertices == vertex_count);

	meshopt_remapIndexBuffer(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &remap[0]);

	for (size_t i = 0; i < mesh.streams.size(); ++i)
		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], vertex_count, sizeof(Attr), &remap[0]);
}

bool getAttributeBounds(const Scene& scene, cgltf_attribute_type type, Attr& min, Attr& max)
{
	min.f[0] = min.f[1] = min.f[2] = min.f[3] = +FLT_MAX;
	max.f[0] = max.f[1] = max.f[2] = max.f[3] = -FLT_MAX;

	bool valid = false;

	for (size_t i = 0; i < scene.meshes.size(); ++i)
	{
		const Mesh& mesh = scene.meshes[i];

		for (size_t j = 0; j < mesh.streams.size(); ++j)
		{
			const Stream& s = mesh.streams[j];

			if (s.type == type)
			{
				for (size_t k = 0; k < s.data.size(); ++k)
				{
					const Attr& a = s.data[k];

					min.f[0] = std::min(min.f[0], a.f[0]);
					min.f[1] = std::min(min.f[1], a.f[1]);
					min.f[2] = std::min(min.f[2], a.f[2]);
					min.f[3] = std::min(min.f[3], a.f[3]);

					max.f[0] = std::max(max.f[0], a.f[0]);
					max.f[1] = std::max(max.f[1], a.f[1]);
					max.f[2] = std::max(max.f[2], a.f[2]);
					max.f[3] = std::max(max.f[3], a.f[3]);

					valid = true;
				}
			}
		}
	}

	return valid;
}

QuantizationParams prepareQuantization(const Scene& scene, const Settings& settings)
{
	QuantizationParams result = {};

	result.pos_bits = settings.pos_bits;

	Attr pos_min, pos_max;
	if (getAttributeBounds(scene, cgltf_attribute_type_position, pos_min, pos_max))
	{
		result.pos_offset[0] = pos_min.f[0];
		result.pos_offset[1] = pos_min.f[1];
		result.pos_offset[2] = pos_min.f[2];
		result.pos_scale = std::max(pos_max.f[0] - pos_min.f[0], std::max(pos_max.f[1] - pos_min.f[1], pos_max.f[2] - pos_min.f[2]));
	}

	result.uv_bits = settings.uv_bits;

	Attr uv_min, uv_max;
	if (getAttributeBounds(scene, cgltf_attribute_type_texcoord, uv_min, uv_max))
	{
		result.uv_offset[0] = uv_min.f[0];
		result.uv_offset[1] = uv_min.f[1];
		result.uv_scale[0] = uv_max.f[0] - uv_min.f[0];
		result.uv_scale[1] = uv_max.f[1] - uv_min.f[1];
	}

	return result;
}

std::pair<std::pair<cgltf_component_type, cgltf_type>, size_t> writeVertexStream(std::string& bin, const Stream& stream, const QuantizationParams& params)
{
	if (stream.type == cgltf_attribute_type_position)
	{
		float pos_rscale = params.pos_scale == 0.f ? 0.f : 1.f / params.pos_scale;

		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint16_t v[4] = {
				uint16_t(meshopt_quantizeUnorm((a.f[0] - params.pos_offset[0]) * pos_rscale, params.pos_bits)),
				uint16_t(meshopt_quantizeUnorm((a.f[1] - params.pos_offset[1]) * pos_rscale, params.pos_bits)),
				uint16_t(meshopt_quantizeUnorm((a.f[2] - params.pos_offset[2]) * pos_rscale, params.pos_bits)),
				0
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_16u, cgltf_type_vec3), 8);
	}
	else if (stream.type == cgltf_attribute_type_texcoord)
	{
		float uv_rscale[2] = {
			params.uv_scale[0] == 0.f ? 0.f : 1.f / params.uv_scale[0],
			params.uv_scale[1] == 0.f ? 0.f : 1.f / params.uv_scale[1],
		};

		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint16_t v[2] = {
				uint16_t(meshopt_quantizeUnorm((a.f[0] - params.uv_offset[0]) * uv_rscale[0], params.uv_bits)),
				uint16_t(meshopt_quantizeUnorm((a.f[1] - params.uv_offset[1]) * uv_rscale[1], params.uv_bits)),
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_16u, cgltf_type_vec2), 4);
	}
	else if (stream.type == cgltf_attribute_type_normal || stream.type == cgltf_attribute_type_tangent)
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			float nx = a.f[0], ny = a.f[1], nz = a.f[2];

			// scale the normal to make sure the largest component is +-1.0
			// this reduces the entropy of the normal by ~1.5 bits without losing precision
			// it's better to use octahedral encoding but that requires special shader support
			float nm = std::max(fabsf(nx), std::max(fabsf(ny), fabsf(nz)));
			float ns = nm == 0.f ? 0.f : 1 / nm;

			nx *= ns;
			ny *= ns;
			nz *= ns;

			int8_t v[4] = {
				int8_t(meshopt_quantizeSnorm(nx, 8)),
				int8_t(meshopt_quantizeSnorm(ny, 8)),
				int8_t(meshopt_quantizeSnorm(nz, 8)),
				0
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_8, cgltf_type_vec3), 4);
	}
	else if (stream.type == cgltf_attribute_type_color || stream.type == cgltf_attribute_type_weights)
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint8_t v[4] = {
				uint8_t(meshopt_quantizeUnorm(a.f[0], 8)),
				uint8_t(meshopt_quantizeUnorm(a.f[1], 8)),
				uint8_t(meshopt_quantizeUnorm(a.f[2], 8)),
				uint8_t(meshopt_quantizeUnorm(a.f[3], 8))
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_8u, cgltf_type_vec4), 4);
	}
	else if (stream.type == cgltf_attribute_type_joints)
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint8_t v[4] = {
				uint8_t(a.f[0]),
				uint8_t(a.f[1]),
				uint8_t(a.f[2]),
				uint8_t(a.f[3])
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_8u, cgltf_type_vec4), 4);
	}
	else
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			float v[4] = {
				a.f[0],
				a.f[1],
				a.f[2],
				a.f[3]
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_32f, cgltf_type_vec4), 16);
	}
}

cgltf_component_type writeIndexStream(std::string& bin, const std::vector<unsigned int>& stream)
{
	for (size_t i = 0; i < stream.size(); ++i)
	{
		uint32_t v[1] = { stream[i] };
		bin.append(reinterpret_cast<const char*>(v), sizeof(v));
	}

	return cgltf_component_type_r_32u;
}

void compressVertexStream(std::string& bin, size_t offset, size_t count, size_t stride)
{
	std::vector<unsigned char> compressed(meshopt_encodeVertexBufferBound(count, stride));
	size_t size = meshopt_encodeVertexBuffer(&compressed[0], compressed.size(), bin.c_str() + offset, count, stride);

	bin.erase(offset);
	bin.append(reinterpret_cast<const char*>(&compressed[0]), size);
}

void compressIndexStream(std::string& bin, size_t offset, size_t count, size_t stride)
{
	assert(stride == 4);

	std::vector<unsigned char> compressed(meshopt_encodeIndexBufferBound(count, count * 3));
	size_t size = meshopt_encodeIndexBuffer(&compressed[0], compressed.size(), reinterpret_cast<const unsigned int*>(bin.c_str() + offset), count);

	bin.erase(offset);
	bin.append(reinterpret_cast<const char*>(&compressed[0]), size);
}

void comma(std::string& s)
{
	char ch = s.empty() ? 0 : s[s.size() - 1];

	if (ch != 0 && ch != '[' && ch != '{')
		s += ',';
}

std::string to_string(size_t v)
{
	char buf[32];
	sprintf(buf, "%zu", v);
	return buf;
}

std::string to_string(float v)
{
	char buf[512];
	sprintf(buf, "%.7g", v);
	return buf;
}

const char* componentType(cgltf_component_type type)
{
	switch (type)
	{
		case cgltf_component_type_r_8: return "5120";
		case cgltf_component_type_r_8u: return "5121";
		case cgltf_component_type_r_16: return "5122";
		case cgltf_component_type_r_16u: return "5123";
		case cgltf_component_type_r_32u: return "5125";
		case cgltf_component_type_r_32f: return "5126";
		default: return "0";
	}
}

const char* shapeType(cgltf_type type)
{
	switch (type)
	{
	case cgltf_type_scalar: return "\"SCALAR\"";
	case cgltf_type_vec2: return "\"VEC2\"";
	case cgltf_type_vec3: return "\"VEC3\"";
	case cgltf_type_vec4: return "\"VEC4\"";
	default: return "\"\"";
	}
}

const char* attributeType(cgltf_attribute_type type)
{
	switch (type)
	{
	case cgltf_attribute_type_position: return "POSITION";
	case cgltf_attribute_type_normal: return "NORMAL";
	case cgltf_attribute_type_tangent: return "TANGENT";
	case cgltf_attribute_type_texcoord: return "TEXCOORD";
	case cgltf_attribute_type_color: return "COLOR";
	case cgltf_attribute_type_joints: return "JOINTS";
	case cgltf_attribute_type_weights: return "WEIGHTS";
	default: return "ATTRIBUTE";
	}
}

void writeTextureInfo(std::string& json, const cgltf_data* data, const cgltf_texture_view& view, const QuantizationParams& qp)
{
	assert(view.texture);

	json += "{\"index\":";
	json += to_string(size_t(view.texture - data->textures));
	json += ",\"extensions\":{\"KHR_texture_transform\":{";
	json += "\"offset\":[";
	json += to_string(qp.uv_offset[0]);
	json += ",";
	json += to_string(qp.uv_offset[1]);
	json += "],\"scale\":[";
	json += to_string(qp.uv_scale[0] / float((1 << qp.uv_bits) - 1));
	json += ",";
	json += to_string(qp.uv_scale[1] / float((1 << qp.uv_bits) - 1));
	json += "]}}}";
}

void writeMaterialInfo(std::string& json, const cgltf_data* data, const cgltf_material& material, const QuantizationParams& qp)
{
	static const float white[4] = { 1, 1, 1, 1 };
	static const float black[4] = { 0, 0, 0, 0 };

	if (material.has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;

		comma(json);
		json += "\"pbrMetallicRoughness\":{";
		if (memcmp(pbr.base_color_factor, white, 16) != 0)
		{
			comma(json);
			json += "\"baseColorFactor\":[";
			json += to_string(pbr.base_color_factor[0]);
			json += ",";
			json += to_string(pbr.base_color_factor[1]);
			json += ",";
			json += to_string(pbr.base_color_factor[2]);
			json += ",";
			json += to_string(pbr.base_color_factor[3]);
			json += "]";
		}
		if (pbr.base_color_texture.texture)
		{
			comma(json);
			json += "\"baseColorTexture\":";
			writeTextureInfo(json, data, pbr.base_color_texture, qp);
		}
		if (pbr.metallic_factor != 1)
		{
			comma(json);
			json += "\"metallicFactor\":";
			json += to_string(pbr.metallic_factor);
		}
		if (pbr.roughness_factor != 1)
		{
			comma(json);
			json += "\"roughnessFactor\":";
			json += to_string(pbr.roughness_factor);
		}
		if (pbr.metallic_roughness_texture.texture)
		{
			comma(json);
			json += "\"metallicRoughnessTexture\":";
			writeTextureInfo(json, data, pbr.metallic_roughness_texture, qp);
		}
		json += "}";
	}

	if (material.has_pbr_specular_glossiness)
	{
		const cgltf_pbr_specular_glossiness& pbr = material.pbr_specular_glossiness;

		comma(json);
		json += "\"extensions\":{\"KHR_materials_pbrSpecularGlossiness\":{";
		if (pbr.diffuse_texture.texture)
		{
			comma(json);
			json += "\"diffuseTexture\":";
			writeTextureInfo(json, data, pbr.diffuse_texture, qp);
		}
		if (pbr.specular_glossiness_texture.texture)
		{
			comma(json);
			json += "\"specularGlossinessTexture\":";
			writeTextureInfo(json, data, pbr.specular_glossiness_texture, qp);
		}
		if (memcmp(pbr.diffuse_factor, white, 16) != 0)
		{
			comma(json);
			json += "\"diffuseFactor\":[";
			json += to_string(pbr.diffuse_factor[0]);
			json += ",";
			json += to_string(pbr.diffuse_factor[1]);
			json += ",";
			json += to_string(pbr.diffuse_factor[2]);
			json += ",";
			json += to_string(pbr.diffuse_factor[3]);
			json += "]";
		}
		if (memcmp(pbr.specular_factor, white, 12) != 0)
		{
			comma(json);
			json += "\"specularFactor\":[";
			json += to_string(pbr.specular_factor[0]);
			json += ",";
			json += to_string(pbr.specular_factor[1]);
			json += ",";
			json += to_string(pbr.specular_factor[2]);
			json += "]";
		}
		if (pbr.glossiness_factor != 1)
		{
			comma(json);
			json += "\"glossinessFactor\":";
			json += to_string(pbr.glossiness_factor);
		}
		json += "}}";
	}

	if (material.normal_texture.texture)
	{
		comma(json);
		json += "\"normalTexture\":";
		writeTextureInfo(json, data, material.normal_texture, qp);
	}

	if (material.occlusion_texture.texture)
	{
		comma(json);
		json += "\"occlusionTexture\":";
		writeTextureInfo(json, data, material.occlusion_texture, qp);
	}

	if (material.emissive_texture.texture)
	{
		comma(json);
		json += "\"emissiveTexture\":";
		writeTextureInfo(json, data, material.emissive_texture, qp);
	}

	if (memcmp(material.emissive_factor, black, 12) != 0)
	{
		comma(json);
		json += "\"emissiveFactor\":[";
		json += to_string(material.emissive_factor[0]);
		json += ",";
		json += to_string(material.emissive_factor[1]);
		json += ",";
		json += to_string(material.emissive_factor[2]);
		json += "]";
	}
}

bool process(Scene& scene, const Settings& settings, std::string& json, std::string& bin)
{
	cgltf_data* data = scene.data;

	parseMeshes(data, scene.meshes);

	for (size_t i = 0; i < scene.meshes.size(); ++i)
	{
		reindexMesh(scene.meshes[i]);
		optimizeMesh(scene.meshes[i]);
	}

	QuantizationParams qp = prepareQuantization(scene, settings);

	std::string json_images;
	std::string json_textures;
	std::string json_materials;
	std::string json_buffer_views;
	std::string json_accessors;
	std::string json_meshes;
	std::string json_nodes;
	std::string json_roots;

	bool has_pbr_specular_glossiness = false;

	size_t view_offset = 0;

	for (size_t i = 0; i < scene.data->images_count; ++i)
	{
		const cgltf_image& image = scene.data->images[i];

		comma(json_images);
		json_images += "{\"uri\":\"";
		json_images += image.uri;
		json_images += "\"}";
	}

	for (size_t i = 0; i < scene.data->textures_count; ++i)
	{
		const cgltf_texture& texture = scene.data->textures[i];

		comma(json_textures);
		json_textures += "{";
		if (texture.image)
		{
			json_textures += "\"source\":";
			json_textures += to_string(size_t(texture.image - scene.data->images));
		}
		json_textures += "}";
	}

	for (size_t i = 0; i < scene.data->materials_count; ++i)
	{
		const cgltf_material& material = scene.data->materials[i];

		comma(json_materials);
		json_materials += "{";
		writeMaterialInfo(json_materials, scene.data, material, qp);
		json_materials += "}";

		has_pbr_specular_glossiness = has_pbr_specular_glossiness || material.has_pbr_specular_glossiness;
	}

	for (size_t i = 0; i < scene.meshes.size(); ++i)
	{
		const Mesh& mesh = scene.meshes[i];

		std::string json_attributes;

		for (size_t j = 0; j < mesh.streams.size(); ++j)
		{
			const Stream& stream = mesh.streams[j];

			size_t bin_offset = bin.size();
			std::pair<std::pair<cgltf_component_type, cgltf_type>, size_t> p = writeVertexStream(bin, stream, qp);

			if (settings.compress)
				compressVertexStream(bin, bin_offset, stream.data.size(), p.second);

			comma(json_buffer_views);
			json_buffer_views += "{\"buffer\":0";
			json_buffer_views += ",\"byteLength\":";
			json_buffer_views += to_string(bin.size() - bin_offset),
			json_buffer_views += ",\"byteOffset\":";
			json_buffer_views += to_string(bin_offset),
			json_buffer_views += ",\"byteStride\":";
			json_buffer_views += to_string(p.second);
			json_buffer_views += ",\"target\":34962";
			json_buffer_views += "}";

			comma(json_accessors);
			json_accessors += "{\"bufferView\":";
			json_accessors += to_string(view_offset);
			json_accessors += ",\"componentType\":";
			json_accessors += componentType(p.first.first);
			json_accessors += ",\"count\":";
			json_accessors += to_string(stream.data.size());
			json_accessors += ",\"type\":";
			json_accessors += shapeType(p.first.second);

			if (stream.type == cgltf_attribute_type_position)
			{
				size_t maxp = (1 << qp.pos_bits) - 1;

				json_accessors += ",\"min\":[0,0,0],\"max\":[";
				json_accessors += to_string(maxp);
				json_accessors += ",";
				json_accessors += to_string(maxp);
				json_accessors += ",";
				json_accessors += to_string(maxp);
				json_accessors += "]";
			}
			else if (stream.type == cgltf_attribute_type_normal || stream.type == cgltf_attribute_type_tangent || stream.type == cgltf_attribute_type_color)
			{
				json_accessors += ",\"normalized\":true";
			}

			json_accessors += "}";

			comma(json_attributes);
			json_attributes += "\"";
			json_attributes += attributeType(stream.type);
			if (stream.type != cgltf_attribute_type_position && stream.type != cgltf_attribute_type_normal && stream.type != cgltf_attribute_type_tangent)
			{
				json_attributes += "_";
				json_attributes += to_string(size_t(stream.index));
			}
			json_attributes += "\":";
			json_attributes += to_string(view_offset);

			view_offset++;
		}

		size_t index_view = 0;

		{
			size_t bin_offset = bin.size();
			cgltf_component_type p = writeIndexStream(bin, mesh.indices);

			if (settings.compress)
				compressIndexStream(bin, bin_offset, mesh.indices.size(), p == cgltf_component_type_r_16u ? 2 : 4);

			comma(json_buffer_views);
			json_buffer_views += "{\"buffer\":0";
			json_buffer_views += ",\"byteLength\":";
			json_buffer_views += to_string(bin.size() - bin_offset),
			json_buffer_views += ",\"byteOffset\":";
			json_buffer_views += to_string(bin_offset),
			json_buffer_views += ",\"target\":34963";
			json_buffer_views += "}";

			comma(json_accessors);
			json_accessors += "{\"bufferView\":";
			json_accessors += to_string(view_offset);
			json_accessors += ",\"componentType\":";
			json_accessors += componentType(p);
			json_accessors += ",\"count\":";
			json_accessors += to_string(mesh.indices.size());
			json_accessors += ",\"type\":\"SCALAR\"";
			json_accessors += "}";

			index_view = view_offset;

			view_offset++;
		}

		comma(json_meshes);
		json_meshes += "{\"primitives\":[{\"attributes\":{";
		json_meshes += json_attributes;
		json_meshes += "},\"indices\":";
		json_meshes += to_string(index_view);
		if (mesh.material)
		{
			json_meshes += ",\"material\":";
			json_meshes += to_string(size_t(mesh.material - scene.data->materials));
		}
		json_meshes += "}]}";

		float node_scale = qp.pos_scale / float((1 << qp.pos_bits) - 1);

		comma(json_nodes);
		json_nodes += "{\"mesh\":";
		json_nodes += to_string(i);
		json_nodes += ",\"translation\":[";
		json_nodes += to_string(qp.pos_offset[0]);
		json_nodes += ",";
		json_nodes += to_string(qp.pos_offset[1]);
		json_nodes += ",";
		json_nodes += to_string(qp.pos_offset[2]);
		json_nodes += "],\"scale\":[";
		json_nodes += to_string(node_scale);
		json_nodes += ",";
		json_nodes += to_string(node_scale);
		json_nodes += ",";
		json_nodes += to_string(node_scale);
		json_nodes += "]";
		json_nodes += "}";

		comma(json_roots);
		json_roots += to_string(i);
	}

	json += "\"bufferViews\":[";
	json += json_buffer_views;
	json += "],\"accessors\":[";
	json += json_accessors;
	json += "],\"images\":[";
	json += json_images;
	json += "],\"textures\":[";
	json += json_textures;
	json += "],\"materials\":[";
	json += json_materials;
	json += "],\"meshes\":[";
	json += json_meshes;
	json += "],\"nodes\":[";
	json += json_nodes;
	json += "],\"scenes\":[";
	json += "{\"nodes\":[";
	json += json_roots;
	json += "]}";
	json += "],\"scene\":0";
	json += ",\"asset\":{\"version\":\"2.0\"}";
	json += ",\"extensionsUsed\":[";
	json += "\"KHR_texture_transform\"";
	if (has_pbr_specular_glossiness)
	{
		comma(json);
		json += "\"KHR_materials_pbrSpecularGlossiness\"";
	}
	json += "]";

	return true;
}

void writeU32(FILE* out, uint32_t data)
{
	fwrite(&data, 4, 1, out);
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage: gltfpack [options] input output\n");
		return 1;
	}

	Settings settings = {};
	settings.pos_bits = 14;
	settings.uv_bits = 12;
	settings.compress = false;

	for (int i = 1; i < argc - 2; ++i)
	{
		const char* arg = argv[i];

		if (strncmp(arg, "-vp", 3) == 0 && isdigit(arg[3]))
		{
			settings.pos_bits = atoi(arg + 3);
		}
		else if (strncmp(arg, "-vt", 3) == 0 && isdigit(arg[3]))
		{
			settings.uv_bits = atoi(arg + 3);
		}
		else if (strcmp(arg, "-c") == 0)
		{
			settings.compress = true;
		}
		else
		{
			fprintf(stderr, "Unrecognized option %s\n", argv[i]);
			return 1;
		}
	}

	const char* input = argv[argc - 2];
	const char* output = argv[argc - 1];

	Scene scene = {};

	cgltf_options options = {};
	cgltf_result result = cgltf_parse_file(&options, input, &scene.data);
	result = (result == cgltf_result_success) ? cgltf_validate(scene.data) : result;
	result = (result == cgltf_result_success) ? cgltf_load_buffers(&options, scene.data, input) : result;

	if (result != cgltf_result_success)
	{
		fprintf(stderr, "Error loading %s: %s\n", input, getError(result));
		return 2;
	}

	std::string json, bin;
	if (!process(scene, settings, json, bin))
	{
		fprintf(stderr, "Error processing %s\n", input);
		return 3;
	}

	const char* ext = strrchr(output, '.');

	if (ext && (strcmp(ext, ".gltf") == 0 || strcmp(ext, ".GLTF") == 0))
	{
		std::string binpath = output;
		binpath.replace(binpath.size() - 5, 5, ".bin");

		std::string binname = binpath;
		std::string::size_type slash = binname.find_last_of("/\\");
		if (slash != std::string::npos)
			binname.erase(0, slash);

		FILE* outjson = fopen(output, "wb");
		FILE* outbin = fopen(binpath.c_str(), "wb");
		if (!outjson || !outbin)
		{
			fprintf(stderr, "Error saving %s\n", output);
			return 4;
		}

		fprintf(outjson, "{\"buffers\":[{\"uri\":\"%s\",\"byteLength\":%zu}],", binname.c_str(), bin.size());
		fwrite(json.c_str(), json.size(), 1, outjson);
		fprintf(outjson, "}");

		fwrite(bin.c_str(), bin.size(), 1, outbin);

		fclose(outjson);
		fclose(outbin);
	}
	else if (ext && (strcmp(ext, ".glb") == 0 || strcmp(ext, ".GLB") == 0))
	{
		FILE* out = fopen(output, "wb");
		if (!out)
		{
			fprintf(stderr, "Error saving %s\n", output);
			return 4;
		}

		char bufferspec[32];
		sprintf(bufferspec, "{\"buffers\":[{\"byteLength\":%zu}],", bin.size());

		json.insert(0, bufferspec);
		json.push_back('}');

		while (json.size() % 4)
			json.push_back(' ');

		while (bin.size() % 4)
			bin.push_back('\0');

		writeU32(out, 0x46546C67);
		writeU32(out, 2);
		writeU32(out, 12 + 8 + json.size() + 8 + bin.size());

		writeU32(out, json.size());
		writeU32(out, 0x4E4F534A);
		fwrite(json.c_str(), json.size(), 1, out);

		writeU32(out, bin.size());
		writeU32(out, 0x004E4942);
		fwrite(bin.c_str(), bin.size(), 1, out);

		fclose(out);
	}
	else
	{
		fprintf(stderr, "Error saving %s: unknown extension (expected .gltf or .glb)\n", output);
		return 4;
	}

	return 0;
}
