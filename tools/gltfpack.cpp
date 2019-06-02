// gltfpack is part of meshoptimizer library; see meshoptimizer.h for version/license details
//
// gltfpack is a command-line tool that takes a glTF file as an input and can produce two types of files:
// - regular glb/gltf files that use data that has been optimized for GPU consumption using various cache optimizers
// and quantization
// - packed glb/gltf files that additionally use meshoptimizer codecs to reduce the size of vertex/index data; these
// files can be further compressed with deflate/etc.
//
// To load regular glb files, it should be sufficient to use a standard glTF loader (although note that these files
// use quantized position/texture coordinates that are technically invalid per spec; THREE.js and BabylonJS support
// these files out of the box).
// To load packed glb files, meshoptimizer vertex decoder needs to be integrated into the loader; demo/GLPLoader.js
// contains a work-in-progress loader - please note that the extension specification isn't ready yet so the format
// will change!
//
// gltfpack currently supports materials, meshes, nodes and skinning data
// gltfpack doesn't support morph targets, animation data, lights and cameras

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include "../src/meshoptimizer.h"

#include <algorithm>
#include <string>
#include <vector>
#include <map>

#include <float.h>
#include <math.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"

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
	cgltf_skin* skin;
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
	bool verbose;
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

void parseMeshesGltf(cgltf_data* data, std::vector<Mesh>& meshes)
{
	for (size_t ni = 0; ni < data->nodes_count; ++ni)
	{
		const cgltf_node& node = data->nodes[ni];

		if (!node.mesh)
			continue;

		float transform[16];
		cgltf_node_transform_world(&node, transform);

		const cgltf_mesh& mesh = *node.mesh;

		for (size_t pi = 0; pi < mesh.primitives_count; ++pi)
		{
			const cgltf_primitive& primitive = mesh.primitives[pi];

			Mesh result;

			result.material = primitive.material;
			result.skin = node.skin;

			if (cgltf_accessor* a = primitive.indices)
			{
				result.indices.resize(a->count);
				for (size_t i = 0; i < a->count; ++i)
					result.indices[i] = unsigned(cgltf_accessor_read_index(a, i));
			}

			for (size_t ai = 0; ai < primitive.attributes_count; ++ai)
			{
				const cgltf_attribute& attr = primitive.attributes[ai];

				if (attr.type == cgltf_attribute_type_invalid)
					continue;

				Stream s = {attr.type, attr.index};
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

cgltf_data* parseSceneObj(fastObjMesh* obj)
{
	cgltf_data* data = (cgltf_data*)calloc(1, sizeof(cgltf_data));
	data->memory_free = cgltf_default_free;

	std::map<std::string, size_t> textures;

	for (unsigned int mi = 0; mi < obj->material_count; ++mi)
	{
		fastObjMaterial& om = obj->materials[mi];

		if (om.map_Kd.name)
			textures[om.map_Kd.name] = 0;
	}

	size_t image_offset = 0;

	for (std::map<std::string, size_t>::iterator ti = textures.begin(); ti != textures.end(); ++ti)
		ti->second = image_offset++;

	data->images = (cgltf_image*)calloc(textures.size(), sizeof(cgltf_image));
	data->images_count = textures.size();

	for (std::map<std::string, size_t>::iterator ti = textures.begin(); ti != textures.end(); ++ti)
	{
		data->images[ti->second].uri = strdup(ti->first.c_str());
	}

	data->textures = (cgltf_texture*)calloc(textures.size(), sizeof(cgltf_texture));
	data->textures_count = textures.size();

	for (std::map<std::string, size_t>::iterator ti = textures.begin(); ti != textures.end(); ++ti)
	{
		data->textures[ti->second].image = &data->images[ti->second];
	}

	data->materials = (cgltf_material*)calloc(obj->material_count, sizeof(cgltf_material));
	data->materials_count = obj->material_count;

	for (unsigned int mi = 0; mi < obj->material_count; ++mi)
	{
		cgltf_material& gm = data->materials[mi];
		fastObjMaterial& om = obj->materials[mi];

		gm.has_pbr_metallic_roughness = true;
		gm.pbr_metallic_roughness.base_color_factor[0] = 1.0f;
		gm.pbr_metallic_roughness.base_color_factor[1] = 1.0f;
		gm.pbr_metallic_roughness.base_color_factor[2] = 1.0f;
		gm.pbr_metallic_roughness.base_color_factor[3] = 1.0f;
		gm.pbr_metallic_roughness.metallic_factor = 0.0f;
		gm.pbr_metallic_roughness.roughness_factor = 1.0f;

		gm.alpha_cutoff = 0.5f;

		if (om.map_Kd.name)
		{
			gm.pbr_metallic_roughness.base_color_texture.texture = &data->textures[textures[om.map_Kd.name]];
			gm.pbr_metallic_roughness.base_color_texture.scale = 1.0f;

			gm.alpha_mode = (om.illum == 4 || om.illum == 6 || om.illum == 7 || om.illum == 9) ? cgltf_alpha_mode_mask : cgltf_alpha_mode_opaque;
		}

		if (om.map_d.name)
		{
			gm.alpha_mode = cgltf_alpha_mode_blend;
		}
	}

	return data;
}

void parseMeshesObj(fastObjMesh* obj, cgltf_data* data, std::vector<Mesh>& meshes)
{
	unsigned int material_count = std::max(obj->material_count, 1u);

	std::vector<size_t> vertex_count(material_count);
	std::vector<size_t> index_count(material_count);

	for (unsigned int gi = 0; gi < obj->group_count; ++gi)
	{
		for (unsigned int fi = 0; fi < obj->groups[gi].face_count; ++fi)
		{
			unsigned int mi = obj->groups[gi].materials[fi];

			vertex_count[mi] += obj->groups[gi].vertices[fi];
			index_count[mi] += (obj->groups[gi].vertices[fi] - 2) * 3;
		}
	}

	std::vector<size_t> mesh_index(material_count);

	for (unsigned int mi = 0; mi < material_count; ++mi)
	{
		if (index_count[mi] == 0)
			continue;

		mesh_index[mi] = meshes.size();

		meshes.push_back(Mesh());

		Mesh& mesh = meshes.back();

		if (data->materials_count)
		{
			assert(mi < data->materials_count);
			mesh.material = &data->materials[mi];
		}

		mesh.streams.resize(3);
		mesh.streams[0].type = cgltf_attribute_type_position;
		mesh.streams[0].data.resize(vertex_count[mi]);
		mesh.streams[1].type = cgltf_attribute_type_normal;
		mesh.streams[1].data.resize(vertex_count[mi]);
		mesh.streams[2].type = cgltf_attribute_type_texcoord;
		mesh.streams[2].data.resize(vertex_count[mi]);
		mesh.indices.resize(index_count[mi]);
	}

	std::vector<size_t> vertex_offset(material_count);
	std::vector<size_t> index_offset(material_count);

	for (unsigned int gi = 0; gi < obj->group_count; ++gi)
	{
		size_t group_offset = 0;

		for (unsigned int fi = 0; fi < obj->groups[gi].face_count; ++fi)
		{
			unsigned int mi = obj->groups[gi].materials[fi];
			Mesh& mesh = meshes[mesh_index[mi]];

			size_t vo = vertex_offset[mi];
			size_t io = index_offset[mi];

			for (unsigned int vi = 0; vi < obj->groups[gi].vertices[fi]; ++vi)
			{
				fastObjIndex ii = obj->groups[gi].indices[group_offset + vi];

				Attr p = { obj->positions[ii.p * 3 + 0], obj->positions[ii.p * 3 + 1], obj->positions[ii.p * 3 + 2] };
				Attr n = { obj->normals[ii.n * 3 + 0], obj->normals[ii.n * 3 + 1], obj->normals[ii.n * 3 + 2] };
				Attr t = { obj->texcoords[ii.t * 2 + 0], 1.f - obj->texcoords[ii.t * 2 + 1] };

				mesh.streams[0].data[vo + vi] = p;
				mesh.streams[1].data[vo + vi] = n;
				mesh.streams[2].data[vo + vi] = t;
			}

			for (unsigned int vi = 2; vi < obj->groups[gi].vertices[fi]; ++vi)
			{
				size_t to = io + (vi - 2) * 3;

				mesh.indices[to + 0] = unsigned(vo);
				mesh.indices[to + 1] = unsigned(vo + vi - 1);
				mesh.indices[to + 2] = unsigned(vo + vi);
			}

			vertex_offset[mi] += obj->groups[gi].vertices[fi];
			index_offset[mi] += (obj->groups[gi].vertices[fi] - 2) * 3;
			group_offset += obj->groups[gi].vertices[fi];
		}
	}
}

bool canMerge(const Mesh& lhs, const Mesh& rhs)
{
	if (lhs.material != rhs.material)
		return false;

	if (lhs.skin != rhs.skin)
		return false;

	if (lhs.streams.size() != rhs.streams.size())
		return false;

	for (size_t i = 0; i < lhs.streams.size(); ++i)
		if (lhs.streams[i].type != rhs.streams[i].type || lhs.streams[i].index != rhs.streams[i].index)
			return false;

	return true;
}

void mergeMeshes(Mesh& target, const Mesh& mesh)
{
	assert(target.streams.size() == mesh.streams.size());

	size_t vertex_offset = target.streams[0].data.size();
	size_t index_offset = target.indices.size();

	for (size_t i = 0; i < target.streams.size(); ++i)
		target.streams[i].data.insert(target.streams[i].data.end(), mesh.streams[i].data.begin(), mesh.streams[i].data.end());

	target.indices.resize(target.indices.size() + mesh.indices.size());

	size_t index_count = mesh.indices.size();

	for (size_t i = 0; i < index_count; ++i)
		target.indices[index_offset + i] = unsigned(vertex_offset + mesh.indices[i]);
}

void mergeMeshes(std::vector<Mesh>& meshes)
{
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		for (size_t j = 0; j < i; ++j)
		{
			Mesh& target = meshes[j];

			if (target.indices.size() && canMerge(mesh, target))
			{
				mergeMeshes(target, mesh);

				mesh.streams.clear();
				mesh.indices.clear();

				break;
			}
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
		meshopt_Stream stream = {&mesh.streams[i].data[0], sizeof(Attr), sizeof(Attr)};
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

void rescaleNormal(float& nx, float& ny, float& nz)
{
	// scale the normal to make sure the largest component is +-1.0
	// this reduces the entropy of the normal by ~1.5 bits without losing precision
	// it's better to use octahedral encoding but that requires special shader support
	float nm = std::max(fabsf(nx), std::max(fabsf(ny), fabsf(nz)));
	float ns = nm == 0.f ? 0.f : 1 / nm;

	nx *= ns;
	ny *= ns;
	nz *= ns;
}

void renormalizeWeights(uint8_t (&w)[4])
{
	int sum = w[0] + w[1] + w[2] + w[3];

	if (sum == 255)
		return;

	// we assume that the total error is limited to 0.5/component = 2
	// this means that it's acceptable to adjust the max. component to compensate for the error
	int max = 0;

	for (int k = 1; k < 4; ++k)
		if (w[k] > w[max])
			max = k;

	w[max] += 255 - sum;
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
			    0};
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
			rescaleNormal(nx, ny, nz);

			int8_t v[4] = {
			    int8_t(meshopt_quantizeSnorm(nx, 8)),
			    int8_t(meshopt_quantizeSnorm(ny, 8)),
			    int8_t(meshopt_quantizeSnorm(nz, 8)),
			    0};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_8, cgltf_type_vec3), 4);
	}
	else if (stream.type == cgltf_attribute_type_tangent)
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			float nx = a.f[0], ny = a.f[1], nz = a.f[2], nw = a.f[3];
			rescaleNormal(nx, ny, nz);

			int8_t v[4] = {
			    int8_t(meshopt_quantizeSnorm(nx, 8)),
			    int8_t(meshopt_quantizeSnorm(ny, 8)),
			    int8_t(meshopt_quantizeSnorm(nz, 8)),
			    int8_t(meshopt_quantizeSnorm(nw, 8))};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_8, cgltf_type_vec4), 4);
	}
	else if (stream.type == cgltf_attribute_type_color)
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint8_t v[4] = {
			    uint8_t(meshopt_quantizeUnorm(a.f[0], 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[1], 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[2], 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[3], 8))};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_8u, cgltf_type_vec4), 4);
	}
	else if (stream.type == cgltf_attribute_type_weights)
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint8_t v[4] = {
			    uint8_t(meshopt_quantizeUnorm(a.f[0], 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[1], 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[2], 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[3], 8))};

			renormalizeWeights(v);

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
			    uint8_t(a.f[3])};
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
			    a.f[3]};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		return std::make_pair(std::make_pair(cgltf_component_type_r_32f, cgltf_type_vec4), 16);
	}
}

cgltf_component_type writeIndexStream(std::string& bin, const std::vector<unsigned int>& stream)
{
	for (size_t i = 0; i < stream.size(); ++i)
	{
		uint32_t v[1] = {stream[i]};
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
		return "\"SCALAR\"";
	case cgltf_type_vec2:
		return "\"VEC2\"";
	case cgltf_type_vec3:
		return "\"VEC3\"";
	case cgltf_type_vec4:
		return "\"VEC4\"";
	default:
		return "\"\"";
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

void writeTextureInfo(std::string& json, const cgltf_data* data, const cgltf_texture_view& view, const QuantizationParams& qp)
{
	assert(view.texture);

	json += "{\"index\":";
	json += to_string(size_t(view.texture - data->textures));
	json += ",\"texCoord\":";
	json += to_string(size_t(view.texcoord));
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
	static const float white[4] = {1, 1, 1, 1};
	static const float black[4] = {0, 0, 0, 0};

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

	if (material.alpha_mode != cgltf_alpha_mode_opaque)
	{
		comma(json);
		json += "\"alphaMode\":";
		json += (material.alpha_mode == cgltf_alpha_mode_blend) ? "\"BLEND\"" : "\"MASK\"";
	}

	if (material.alpha_cutoff != 0.5f)
	{
		comma(json);
		json += "\"alphaCutoff\":";
		json += to_string(material.alpha_cutoff);
	}

	if (material.double_sided)
	{
		comma(json);
		json += "\"doubleSided\":true";
	}

	if (material.has_pbr_specular_glossiness || material.unlit)
	{
		comma(json);
		json += "\"extensions\":{";

		if (material.has_pbr_specular_glossiness)
		{
			const cgltf_pbr_specular_glossiness& pbr = material.pbr_specular_glossiness;

			comma(json);
			json += "\"KHR_materials_pbrSpecularGlossiness\":{";
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
			json += "}";
		}
		if (material.unlit)
		{
			comma(json);
			json += "\"KHR_materials_unlit\":{}";
		}

		json += "}";
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

void writeBufferView(std::string& json, cgltf_buffer_view_type type, size_t count, size_t stride, size_t bin_offset, size_t bin_size, bool compressed)
{
	assert(compressed || bin_size == count * stride);
	json += "{\"buffer\":0";
	json += ",\"byteLength\":";
	json += to_string(bin_size);
	json += ",\"byteOffset\":";
	json += to_string(bin_offset);
	if (type == cgltf_buffer_view_type_vertices)
	{
		json += ",\"byteStride\":";
		json += to_string(stride);
	}
	json += ",\"target\":";
	json += (type == cgltf_buffer_view_type_vertices) ? "34962" : "34963";
	if (compressed)
	{
		json += ",\"extensions\":{";
		json += "\"KHR_meshopt_compression\":{";
		json += "\"mode\":";
		json += (type == cgltf_buffer_view_type_vertices) ? "\"VERTEX\"" : "\"INDEX\"";
		json += ",\"count\":";
		json += to_string(count);
		json += ",\"byteStride\":";
		json += to_string(stride);
		json += "}}";
	}
	json += "}";
}

bool process(Scene& scene, const Settings& settings, std::string& json, std::string& bin)
{
	cgltf_data* data = scene.data;

	if (settings.verbose)
	{
		printf("input: %d nodes, %d meshes, %d skins\n", int(scene.data->nodes_count), int(scene.data->meshes_count), int(scene.data->skins_count));
	}

	mergeMeshes(scene.meshes);

	for (size_t i = 0; i < scene.meshes.size(); ++i)
	{
		Mesh& mesh = scene.meshes[i];

		if (mesh.indices.empty())
			continue;

		reindexMesh(mesh);
		optimizeMesh(mesh);
	}

	if (settings.verbose)
	{
		size_t triangles = 0;
		size_t vertices = 0;

		for (size_t i = 0; i < scene.meshes.size(); ++i)
		{
			const Mesh& mesh = scene.meshes[i];

			triangles += mesh.indices.size() / 3;
			vertices += mesh.streams.empty() ? 0 : mesh.streams[0].data.size();
		}

		printf("meshes: %d triangles, %d vertices\n", int(triangles), int(vertices));
	}

	QuantizationParams qp = prepareQuantization(scene, settings);

	std::string json_images;
	std::string json_textures;
	std::string json_materials;
	std::string json_buffer_views;
	std::string json_accessors;
	std::string json_meshes;
	std::string json_nodes;
	std::string json_skins;
	std::string json_roots;

	bool has_pbr_specular_glossiness = false;

	size_t view_offset = 0;
	size_t node_offset = 0;
	size_t mesh_offset = 0;

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

		if (mesh.indices.empty())
			continue;

		std::string json_attributes;

		for (size_t j = 0; j < mesh.streams.size(); ++j)
		{
			const Stream& stream = mesh.streams[j];

			if (stream.type == cgltf_attribute_type_texcoord && (!mesh.material || !usesTextureSet(*mesh.material, stream.index)))
				continue;

			if ((stream.type == cgltf_attribute_type_joints || stream.type == cgltf_attribute_type_weights) && !mesh.skin)
				continue;

			size_t bin_offset = bin.size();
			std::pair<std::pair<cgltf_component_type, cgltf_type>, size_t> p = writeVertexStream(bin, stream, qp);

			if (settings.compress)
				compressVertexStream(bin, bin_offset, stream.data.size(), p.second);

			comma(json_buffer_views);
			writeBufferView(json_buffer_views, cgltf_buffer_view_type_vertices, stream.data.size(), p.second, bin_offset, bin.size() - bin_offset, settings.compress);

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
				// TODO: compute precise min/max (validator isn't happy with imprecise data for some reason)
				size_t maxp = (1 << qp.pos_bits) - 1;

				json_accessors += ",\"min\":[0,0,0],\"max\":[";
				json_accessors += to_string(maxp);
				json_accessors += ",";
				json_accessors += to_string(maxp);
				json_accessors += ",";
				json_accessors += to_string(maxp);
				json_accessors += "]";
			}
			else if (stream.type == cgltf_attribute_type_normal || stream.type == cgltf_attribute_type_tangent || stream.type == cgltf_attribute_type_color || stream.type == cgltf_attribute_type_weights)
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

			size_t index_size = (p == cgltf_component_type_r_16u) ? 2 : 4;

			if (settings.compress)
				compressIndexStream(bin, bin_offset, mesh.indices.size(), index_size);

			comma(json_buffer_views);
			writeBufferView(json_buffer_views, cgltf_buffer_view_type_indices, mesh.indices.size(), index_size, bin_offset, bin.size() - bin_offset, settings.compress);

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
		json_nodes += to_string(mesh_offset);
		if (mesh.skin)
		{
			comma(json_nodes);
			json_nodes += "\"skin\":";
			json_nodes += to_string(size_t(mesh.skin - data->skins));
		}
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
		json_roots += to_string(node_offset);

		node_offset++;
		mesh_offset++;
	}

	std::vector<bool> node_keep(data->nodes_count);
	std::vector<bool> node_named(data->nodes_count);
	std::vector<int> node_remap(data->nodes_count, -1);

	for (size_t i = 0; i < data->skins_count; ++i)
	{
		const cgltf_skin& skin = data->skins[i];

		// for now we keep all joints directly referenced by the skin and the entire ancestry tree; we keep names for joints as well
		for (size_t j = 0; j < skin.joints_count; ++j)
		{
			cgltf_node* joint = skin.joints[j];

			for (cgltf_node* node = joint; node; node = node->parent)
				node_keep[node - data->nodes] = true;

			node_named[joint - data->nodes] = true;
		}
	}

	for (size_t i = 0; i < data->nodes_count; ++i)
	{
		if (node_keep[i])
		{
			node_remap[i] = int(node_offset);

			node_offset++;
		}
	}

	for (size_t i = 0; i < data->nodes_count; ++i)
	{
		if (node_keep[i])
		{
			const cgltf_node& node = data->nodes[i];

			if (!node.parent)
			{
				comma(json_roots);
				json_roots += to_string(size_t(node_remap[i]));
			}

			comma(json_nodes);
			json_nodes += "{";
			if (node.name && node_named[i])
			{
				comma(json_nodes);
				json_nodes += "\"name\":\"";
				json_nodes += node.name;
				json_nodes += "\"";
			}
			if (node.has_translation)
			{
				comma(json_nodes);
				json_nodes += "\"translation\":[";
				json_nodes += to_string(node.translation[0]);
				json_nodes += ",";
				json_nodes += to_string(node.translation[1]);
				json_nodes += ",";
				json_nodes += to_string(node.translation[2]);
				json_nodes += "]";
			}
			if (node.has_rotation)
			{
				comma(json_nodes);
				json_nodes += "\"rotation\":[";
				json_nodes += to_string(node.rotation[0]);
				json_nodes += ",";
				json_nodes += to_string(node.rotation[1]);
				json_nodes += ",";
				json_nodes += to_string(node.rotation[2]);
				json_nodes += ",";
				json_nodes += to_string(node.rotation[3]);
				json_nodes += "]";
			}
			if (node.has_scale)
			{
				comma(json_nodes);
				json_nodes += "\"scale\":[";
				json_nodes += to_string(node.scale[0]);
				json_nodes += ",";
				json_nodes += to_string(node.scale[1]);
				json_nodes += ",";
				json_nodes += to_string(node.scale[2]);
				json_nodes += "]";
			}
			if (node.has_matrix)
			{
				comma(json_nodes);
				json_nodes += "\"matrix\":[";
				for (int k = 0; k < 16; ++k)
				{
					json_nodes += to_string(node.matrix[k]);
					if (k != 15)
						json_nodes += ",";
				}
				json_nodes += "]";
			}
			if (node.children_count)
			{
				comma(json_nodes);
				json_nodes += "\"children\":[";
				for (size_t j = 0; j < node.children_count; ++j)
				{
					ptrdiff_t child = node.children[j] - data->nodes;
					if (node_keep[child])
					{
						comma(json_nodes);
						json_nodes += to_string(size_t(node_remap[child]));
					}
				}
				json_nodes += "]";
			}
			json_nodes += "}";
		}
	}

	for (size_t i = 0; i < data->skins_count; ++i)
	{
		const cgltf_skin& skin = data->skins[i];

		size_t bin_offset = bin.size();

		for (size_t j = 0; j < skin.joints_count; ++j)
		{
			float transform[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

			if (skin.inverse_bind_matrices)
			{
				cgltf_accessor_read_float(skin.inverse_bind_matrices, j, transform, 16);
			}

			float node_scale = qp.pos_scale / float((1 << qp.pos_bits) - 1);

			for (int k = 0; k < 12; ++k)
				transform[k] *= node_scale;

			transform[12] += qp.pos_offset[0];
			transform[13] += qp.pos_offset[1];
			transform[14] += qp.pos_offset[2];

			bin.append(reinterpret_cast<const char*>(transform), sizeof(transform));
		}

		comma(json_buffer_views);
		json_buffer_views += "{\"buffer\":0";
		json_buffer_views += ",\"byteLength\":";
		json_buffer_views += to_string(bin.size() - bin_offset);
		json_buffer_views += ",\"byteOffset\":";
		json_buffer_views += to_string(bin_offset);
		json_buffer_views += "}";

		comma(json_accessors);
		json_accessors += "{\"bufferView\":";
		json_accessors += to_string(view_offset);
		json_accessors += ",\"componentType\":";
		json_accessors += componentType(cgltf_component_type_r_32f);
		json_accessors += ",\"count\":";
		json_accessors += to_string(skin.joints_count);
		json_accessors += ",\"type\":\"MAT4\"";
		json_accessors += "}";

		size_t matrix_view = view_offset;

		view_offset++;

		comma(json_skins);

		json_skins += "{";
		json_skins += "\"joints\":[";
		for (size_t j = 0; j < skin.joints_count; ++j)
		{
			comma(json_skins);
			json_skins += to_string(size_t(node_remap[skin.joints[j] - data->nodes]));
		}
		json_skins += "]";
		json_skins += ",\"inverseBindMatrices\":";
		json_skins += to_string(matrix_view);
		if (skin.skeleton)
		{
			comma(json_skins);
			json_skins += "\"skeleton\":";
			json_skins += to_string(size_t(node_remap[skin.skeleton - data->nodes]));
		}
		json_skins += "}";
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
	json += ",\"skins\":[";
	json += json_skins;
	json += "]";
	json += ",\"asset\":{\"version\":\"2.0\", \"generator\":\"gltfpack\"}";
	json += ",\"extensionsUsed\":[";
	json += "\"KHR_quantized_geometry\"";
	if (settings.compress)
	{
		comma(json);
		json += "\"KHR_meshopt_compression\"";
	}
	if (!json_textures.empty())
	{
		comma(json);
		json += "\"KHR_texture_transform\"";
	}
	if (has_pbr_specular_glossiness)
	{
		comma(json);
		json += "\"KHR_materials_pbrSpecularGlossiness\"";
	}
	json += "]";

	if (settings.verbose)
	{
		printf("output: %d nodes, %d meshes\n", int(node_offset), int(mesh_offset));
		printf("output: JSON %d bytes, buffers %d bytes\n", int(json.size()), int(bin.size()));
	}

	return true;
}

void writeU32(FILE* out, uint32_t data)
{
	fwrite(&data, 4, 1, out);
}

int main(int argc, char** argv)
{
	Settings settings = {};
	settings.pos_bits = 14;
	settings.uv_bits = 12;

	const char* input = 0;
	const char* output = 0;

	for (int i = 1; i < argc; ++i)
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
		else if (strcmp(arg, "-v") == 0)
		{
			settings.verbose = true;
		}
		else if (arg[0] == '-')
		{
			fprintf(stderr, "Unrecognized option %s\n", arg);
			return 1;
		}
		else if (!input)
		{
			input = arg;
		}
		else if (!output)
		{
			output = arg;
		}
		else
		{
			fprintf(stderr, "Too many filenames, expected two\n");
			return 1;
		}
	}

	if (!input || !output)
	{
		fprintf(stderr, "Usage: gltfpack [options] input output\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "-vpN: use N-bit quantization for position (default: 14; N should be between 1 and 16)\n");
		fprintf(stderr, "-vtN: use N-bit quantization for texture corodinates (default: 12; N should be between 1 and 16)\n");
		fprintf(stderr, "-c: produce compressed glb files\n");
		fprintf(stderr, "-v: verbose output\n");

		return 1;
	}


	Scene scene = {};

	const char* iext = strrchr(input, '.');

	if (iext && (strcmp(iext, ".gltf") == 0 || strcmp(iext, ".GLTF") == 0 || strcmp(iext, ".glb") == 0 || strcmp(iext, ".GLB") == 0))
	{
		cgltf_options options = {};
		cgltf_result result = cgltf_parse_file(&options, input, &scene.data);
		result = (result == cgltf_result_success) ? cgltf_validate(scene.data) : result;
		result = (result == cgltf_result_success) ? cgltf_load_buffers(&options, scene.data, input) : result;

		if (result != cgltf_result_success)
		{
			fprintf(stderr, "Error loading %s: %s\n", input, getError(result));
			return 2;
		}

		parseMeshesGltf(scene.data, scene.meshes);
	}
	else if (iext && (strcmp(iext, ".obj") == 0 || strcmp(iext, ".OBJ") == 0))
	{
		fastObjMesh* obj = fast_obj_read(input);

		if (!obj)
		{
			fprintf(stderr, "Error loading %s: file not found\n", input);
			return 2;
		}

		scene.data = parseSceneObj(obj);
		parseMeshesObj(obj, scene.data, scene.meshes);

		fast_obj_destroy(obj);
	}
	else
	{
		fprintf(stderr, "Error loading %s: unknown extension (expected .gltf or .glb or .obj)\n", input);
		return 2;
	}

	std::string json, bin;
	if (!process(scene, settings, json, bin))
	{
		fprintf(stderr, "Error processing %s\n", input);
		return 3;
	}

	const char* oext = strrchr(output, '.');

	if (oext && (strcmp(oext, ".gltf") == 0 || strcmp(oext, ".GLTF") == 0))
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
	else if (oext && (strcmp(oext, ".glb") == 0 || strcmp(oext, ".GLB") == 0))
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
		writeU32(out, uint32_t(12 + 8 + json.size() + 8 + bin.size()));

		writeU32(out, uint32_t(json.size()));
		writeU32(out, 0x4E4F534A);
		fwrite(json.c_str(), json.size(), 1, out);

		writeU32(out, uint32_t(bin.size()));
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
