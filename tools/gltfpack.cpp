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
// To load packed glb files, meshoptimizer vertex decoder needs to be integrated into the loader; demo/GLTFLoader.js
// contains a work-in-progress loader - please note that the extension specification isn't ready yet so the format
// will change!
//
// gltfpack supports materials, meshes, nodes, skinning and animations
// gltfpack doesn't support morph targets, lights and cameras

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

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cgltf.h"
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
	cgltf_node* node;

	cgltf_material* material;
	cgltf_skin* skin;

	std::vector<Stream> streams;
	std::vector<unsigned int> indices;
};

struct Settings
{
	int pos_bits;
	int tex_bits;
	int nrm_bits;
	bool nrm_unit;

	int anim_freq;
	bool anim_const;

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

struct StreamFormat
{
	cgltf_type type;
	cgltf_component_type component_type;
	bool normalized;
	size_t stride;
};

struct NodeInfo
{
	bool keep;
	bool named;
	bool animated;

	unsigned int animated_paths;

	int remap;
	std::vector<size_t> meshes;
};

struct BufferView
{
	enum Kind
	{
		Kind_Vertex,
		Kind_Index,
		Kind_Skin,
		Kind_Time,
		Kind_Keyframe,
		Kind_Image,
		Kind_Count
	};

	Kind kind;
	int variant;
	size_t stride;
	bool compressed;

	std::string data;
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

void transformMesh(Mesh& mesh, const cgltf_node* node)
{
	float transform[16];
	cgltf_node_transform_world(node, transform);

	for (size_t si = 0; si < mesh.streams.size(); ++si)
	{
		Stream& stream = mesh.streams[si];

		if (stream.type == cgltf_attribute_type_position)
		{
			for (size_t i = 0; i < stream.data.size(); ++i)
				transformPosition(stream.data[i].f, transform);
		}
		else if (stream.type == cgltf_attribute_type_normal || stream.type == cgltf_attribute_type_tangent)
		{
			for (size_t i = 0; i < stream.data.size(); ++i)
				transformNormal(stream.data[i].f, transform);
		}
	}
}

void parseMeshesGltf(cgltf_data* data, std::vector<Mesh>& meshes)
{
	for (size_t ni = 0; ni < data->nodes_count; ++ni)
	{
		cgltf_node& node = data->nodes[ni];

		if (!node.mesh)
			continue;

		const cgltf_mesh& mesh = *node.mesh;
		int mesh_id = int(&mesh - data->meshes);

		for (size_t pi = 0; pi < mesh.primitives_count; ++pi)
		{
			const cgltf_primitive& primitive = mesh.primitives[pi];

			if (!primitive.indices)
			{
				fprintf(stderr, "Warning: ignoring primitive %d of mesh %d because it has no index data\n", int(pi), mesh_id);
				continue;
			}

			if (primitive.type != cgltf_primitive_type_triangles)
			{
				fprintf(stderr, "Warning: ignoring primitive %d of mesh %d because type %d is not supported\n", int(pi), mesh_id, primitive.type);
				continue;
			}

			if (primitive.targets_count)
			{
				fprintf(stderr, "Warning: ignoring %d morph targets in primitive %d of mesh %d\n", int(primitive.targets_count), int(pi), mesh_id);
			}

			Mesh result;

			result.node = &node;

			result.material = primitive.material;
			result.skin = node.skin;

			result.indices.resize(primitive.indices->count);
			for (size_t i = 0; i < primitive.indices->count; ++i)
				result.indices[i] = unsigned(cgltf_accessor_read_index(primitive.indices, i));

			for (size_t ai = 0; ai < primitive.attributes_count; ++ai)
			{
				const cgltf_attribute& attr = primitive.attributes[ai];

				if (attr.type == cgltf_attribute_type_invalid)
				{
					fprintf(stderr, "Warning: ignoring unknown attribute %s in primitive %d of mesh %d\n", attr.name, int(pi), mesh_id);
					continue;
				}

				Stream s = {attr.type, attr.index};
				s.data.resize(attr.data->count);

				for (size_t i = 0; i < attr.data->count; ++i)
					cgltf_accessor_read_float(attr.data, i, s.data[i].f, 4);

				result.streams.push_back(s);
			}

			meshes.push_back(result);
		}
	}
}

void defaultFree(void*, void* p)
{
	free(p);
}

size_t textureIndex(const std::vector<std::string>& textures, const std::string& name)
{
	std::vector<std::string>::const_iterator it = std::lower_bound(textures.begin(), textures.end(), name);
	assert(it != textures.end());
	assert(*it == name);

	return size_t(it - textures.begin());
}

cgltf_data* parseSceneObj(fastObjMesh* obj)
{
	cgltf_data* data = (cgltf_data*)calloc(1, sizeof(cgltf_data));
	data->memory_free = defaultFree;

	std::vector<std::string> textures;

	for (unsigned int mi = 0; mi < obj->material_count; ++mi)
	{
		fastObjMaterial& om = obj->materials[mi];

		if (om.map_Kd.name)
			textures.push_back(om.map_Kd.name);
	}

	std::sort(textures.begin(), textures.end());
	textures.erase(std::unique(textures.begin(), textures.end()), textures.end());

	data->images = (cgltf_image*)calloc(textures.size(), sizeof(cgltf_image));
	data->images_count = textures.size();

	for (size_t i = 0; i < textures.size(); ++i)
	{
		data->images[i].uri = strdup(textures[i].c_str());
	}

	data->textures = (cgltf_texture*)calloc(textures.size(), sizeof(cgltf_texture));
	data->textures_count = textures.size();

	for (size_t i = 0; i < textures.size(); ++i)
	{
		data->textures[i].image = &data->images[i];
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
			gm.pbr_metallic_roughness.base_color_texture.texture = &data->textures[textureIndex(textures, om.map_Kd.name)];
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

	for (unsigned int fi = 0; fi < obj->face_count; ++fi)
	{
		unsigned int mi = obj->face_materials[fi];

		vertex_count[mi] += obj->face_vertices[fi];
		index_count[mi] += (obj->face_vertices[fi] - 2) * 3;
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

	size_t group_offset = 0;

	for (unsigned int fi = 0; fi < obj->face_count; ++fi)
	{
		unsigned int mi = obj->face_materials[fi];
		Mesh& mesh = meshes[mesh_index[mi]];

		size_t vo = vertex_offset[mi];
		size_t io = index_offset[mi];

		for (unsigned int vi = 0; vi < obj->face_vertices[fi]; ++vi)
		{
			fastObjIndex ii = obj->indices[group_offset + vi];

			Attr p = {{obj->positions[ii.p * 3 + 0], obj->positions[ii.p * 3 + 1], obj->positions[ii.p * 3 + 2]}};
			Attr n = {{obj->normals[ii.n * 3 + 0], obj->normals[ii.n * 3 + 1], obj->normals[ii.n * 3 + 2]}};
			Attr t = {{obj->texcoords[ii.t * 2 + 0], 1.f - obj->texcoords[ii.t * 2 + 1]}};

			mesh.streams[0].data[vo + vi] = p;
			mesh.streams[1].data[vo + vi] = n;
			mesh.streams[2].data[vo + vi] = t;
		}

		for (unsigned int vi = 2; vi < obj->face_vertices[fi]; ++vi)
		{
			size_t to = io + (vi - 2) * 3;

			mesh.indices[to + 0] = unsigned(vo);
			mesh.indices[to + 1] = unsigned(vo + vi - 1);
			mesh.indices[to + 2] = unsigned(vo + vi);
		}

		vertex_offset[mi] += obj->face_vertices[fi];
		index_offset[mi] += (obj->face_vertices[fi] - 2) * 3;
		group_offset += obj->face_vertices[fi];
	}
}

bool canMerge(const Mesh& lhs, const Mesh& rhs)
{
	if (lhs.node != rhs.node)
	{
		if (!lhs.node || !rhs.node)
			return false;

		if (lhs.node->parent != rhs.node->parent)
			return false;

		bool lhs_transform = lhs.node->has_translation | lhs.node->has_rotation | lhs.node->has_scale | lhs.node->has_matrix;
		bool rhs_transform = rhs.node->has_translation | rhs.node->has_rotation | rhs.node->has_scale | rhs.node->has_matrix;

		if (lhs_transform || rhs_transform)
			return false;

		// we can merge nodes that don't have transforms of their own and have the same parent
		// this is helpful when instead of splitting mesh into primitives, DCCs split mesh into mesh nodes
	}

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
	(void)unique_vertices;

	meshopt_remapIndexBuffer(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &remap[0]);

	for (size_t i = 0; i < mesh.streams.size(); ++i)
		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], vertex_count, sizeof(Attr), &remap[0]);
}

bool getAttributeBounds(const std::vector<Mesh>& meshes, cgltf_attribute_type type, Attr& min, Attr& max)
{
	min.f[0] = min.f[1] = min.f[2] = min.f[3] = +FLT_MAX;
	max.f[0] = max.f[1] = max.f[2] = max.f[3] = -FLT_MAX;

	bool valid = false;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

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

QuantizationParams prepareQuantization(const std::vector<Mesh>& meshes, const Settings& settings)
{
	QuantizationParams result = {};

	result.pos_bits = settings.pos_bits;

	Attr pos_min, pos_max;
	if (getAttributeBounds(meshes, cgltf_attribute_type_position, pos_min, pos_max))
	{
		result.pos_offset[0] = pos_min.f[0];
		result.pos_offset[1] = pos_min.f[1];
		result.pos_offset[2] = pos_min.f[2];
		result.pos_scale = std::max(pos_max.f[0] - pos_min.f[0], std::max(pos_max.f[1] - pos_min.f[1], pos_max.f[2] - pos_min.f[2]));
	}

	result.uv_bits = settings.tex_bits;

	Attr uv_min, uv_max;
	if (getAttributeBounds(meshes, cgltf_attribute_type_texcoord, uv_min, uv_max))
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

	w[max] += uint8_t(255 - sum);
}

StreamFormat writeVertexStream(std::string& bin, const Stream& stream, const QuantizationParams& params, const Settings& settings)
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
			    1};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		// note: vec4 is used instead of vec3 to avoid three.js bug with interleaved buffers (#16802)
		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_16u, false, 8};
		return format;
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

		StreamFormat format = {cgltf_type_vec2, cgltf_component_type_r_16u, false, 4};
		return format;
	}
	else if (stream.type == cgltf_attribute_type_normal)
	{
		int bits = settings.nrm_unit ? 8 : settings.nrm_bits;

		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			float nx = a.f[0], ny = a.f[1], nz = a.f[2];

			if (!settings.nrm_unit)
				rescaleNormal(nx, ny, nz);

			int8_t v[4] = {
			    int8_t(meshopt_quantizeSnorm(nx, bits)),
			    int8_t(meshopt_quantizeSnorm(ny, bits)),
			    int8_t(meshopt_quantizeSnorm(nz, bits)),
			    0};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		// note: vec4 is used instead of vec3 to avoid three.js bug with interleaved buffers (#16802)
		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_8, true, 4};
		return format;
	}
	else if (stream.type == cgltf_attribute_type_tangent)
	{
		int bits = settings.nrm_unit ? 8 : settings.nrm_bits;

		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			float nx = a.f[0], ny = a.f[1], nz = a.f[2], nw = a.f[3];

			if (!settings.nrm_unit)
				rescaleNormal(nx, ny, nz);

			int8_t v[4] = {
			    int8_t(meshopt_quantizeSnorm(nx, bits)),
			    int8_t(meshopt_quantizeSnorm(ny, bits)),
			    int8_t(meshopt_quantizeSnorm(nz, bits)),
			    int8_t(meshopt_quantizeSnorm(nw, 8))};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_8, true, 4};
		return format;
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

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_8u, true, 4};
		return format;
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

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_8u, true, 4};
		return format;
	}
	else if (stream.type == cgltf_attribute_type_joints)
	{
		unsigned int maxj = 0;

		for (size_t i = 0; i < stream.data.size(); ++i)
			maxj = std::max(maxj, unsigned(stream.data[i].f[0]));

		assert(maxj <= 65535);

		if (maxj <= 255)
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

			StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_8u, false, 4};
			return format;
		}
		else
		{
			for (size_t i = 0; i < stream.data.size(); ++i)
			{
				const Attr& a = stream.data[i];

				uint16_t v[4] = {
				    uint16_t(a.f[0]),
				    uint16_t(a.f[1]),
				    uint16_t(a.f[2]),
				    uint16_t(a.f[3])};
				bin.append(reinterpret_cast<const char*>(v), sizeof(v));
			}

			StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_16u, false, 8};
			return format;
		}
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

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_32f, false, 16};
		return format;
	}
}

void getPositionBounds(uint16_t min[3], uint16_t max[3], const Stream& stream, const QuantizationParams& params)
{
	assert(stream.type == cgltf_attribute_type_position);
	assert(stream.data.size() > 0);

	min[0] = min[1] = min[2] = 0xffff;
	max[0] = max[1] = max[2] = 0;

	float pos_rscale = params.pos_scale == 0.f ? 0.f : 1.f / params.pos_scale;

	for (size_t i = 0; i < stream.data.size(); ++i)
	{
		const Attr& a = stream.data[i];

		for (int k = 0; k < 3; ++k)
		{
			uint16_t v = uint16_t(meshopt_quantizeUnorm((a.f[k] - params.pos_offset[k]) * pos_rscale, params.pos_bits));

			min[k] = std::min(min[k], v);
			max[k] = std::max(max[k], v);
		}
	}
}

StreamFormat writeIndexStream(std::string& bin, const std::vector<unsigned int>& stream)
{
	unsigned int maxi = 0;
	for (size_t i = 0; i < stream.size(); ++i)
		maxi = std::max(maxi, stream[i]);

	// save 16-bit indices if we can; note that we can't use restart index (65535)
	if (maxi < 65535)
	{
		for (size_t i = 0; i < stream.size(); ++i)
		{
			uint16_t v[1] = {uint16_t(stream[i])};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_scalar, cgltf_component_type_r_16u, false, 2};
		return format;
	}
	else
	{
		for (size_t i = 0; i < stream.size(); ++i)
		{
			uint32_t v[1] = {stream[i]};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_scalar, cgltf_component_type_r_32u, false, 4};
		return format;
	}
}

StreamFormat writeTimeStream(std::string& bin, const std::vector<float>& data)
{
	for (size_t i = 0; i < data.size(); ++i)
	{
		float v[1] = {data[i]};
		bin.append(reinterpret_cast<const char*>(v), sizeof(v));
	}

	StreamFormat format = {cgltf_type_scalar, cgltf_component_type_r_32f, false, 4};
	return format;
}

StreamFormat writeKeyframeStream(std::string& bin, cgltf_animation_path_type type, const std::vector<Attr>& data)
{
	if (type == cgltf_animation_path_type_rotation)
	{
		for (size_t i = 0; i < data.size(); ++i)
		{
			const Attr& a = data[i];

			int16_t v[4] = {
			    int16_t(meshopt_quantizeSnorm(a.f[0], 16)),
			    int16_t(meshopt_quantizeSnorm(a.f[1], 16)),
			    int16_t(meshopt_quantizeSnorm(a.f[2], 16)),
			    int16_t(meshopt_quantizeSnorm(a.f[3], 16)),
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_16, true, 8};
		return format;
	}
	else
	{
		for (size_t i = 0; i < data.size(); ++i)
		{
			const Attr& a = data[i];

			float v[3] = {a.f[0], a.f[1], a.f[2]};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_vec3, cgltf_component_type_r_32f, false, 12};
		return format;
	}
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
	assert(stride == 2 || stride == 4);

	std::vector<unsigned char> compressed(meshopt_encodeIndexBufferBound(count, count * 3));
	size_t size = 0;

	if (stride == 2)
		size = meshopt_encodeIndexBuffer(&compressed[0], compressed.size(), reinterpret_cast<const uint16_t*>(bin.c_str() + offset), count);
	else
		size = meshopt_encodeIndexBuffer(&compressed[0], compressed.size(), reinterpret_cast<const uint32_t*>(bin.c_str() + offset), count);

	bin.erase(offset);
	bin.append(reinterpret_cast<const char*>(&compressed[0]), size);
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
		return "\"SCALAR\"";
	case cgltf_type_vec2:
		return "\"VEC2\"";
	case cgltf_type_vec3:
		return "\"VEC3\"";
	case cgltf_type_vec4:
		return "\"VEC4\"";
	case cgltf_type_mat2:
		return "\"MAT2\"";
	case cgltf_type_mat3:
		return "\"MAT3\"";
	case cgltf_type_mat4:
		return "\"MAT4\"";
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

const char* animationPath(cgltf_animation_path_type type)
{
	switch (type)
	{
	case cgltf_animation_path_type_translation:
		return "\"translation\"";
	case cgltf_animation_path_type_rotation:
		return "\"rotation\"";
	case cgltf_animation_path_type_scale:
		return "\"scale\"";
	default:
		return "\"\"";
	}
}

void writeTextureInfo(std::string& json, const cgltf_data* data, const cgltf_texture_view& view, const QuantizationParams& qp)
{
	assert(view.texture);

	append(json, "{\"index\":");
	append(json, size_t(view.texture - data->textures));
	append(json, ",\"texCoord\":");
	append(json, size_t(view.texcoord));
	append(json, ",\"extensions\":{\"KHR_texture_transform\":{");
	append(json, "\"offset\":[");
	append(json, qp.uv_offset[0]);
	append(json, ",");
	append(json, qp.uv_offset[1]);
	append(json, "],\"scale\":[");
	append(json, qp.uv_scale[0] / float((1 << qp.uv_bits) - 1));
	append(json, ",");
	append(json, qp.uv_scale[1] / float((1 << qp.uv_bits) - 1));
	append(json, "]}}}");
}

void writeMaterialInfo(std::string& json, const cgltf_data* data, const cgltf_material& material, const QuantizationParams& qp)
{
	static const float white[4] = {1, 1, 1, 1};
	static const float black[4] = {0, 0, 0, 0};

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
			writeTextureInfo(json, data, pbr.base_color_texture, qp);
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
			writeTextureInfo(json, data, pbr.metallic_roughness_texture, qp);
		}
		append(json, "}");
	}

	if (material.normal_texture.texture)
	{
		comma(json);
		append(json, "\"normalTexture\":");
		writeTextureInfo(json, data, material.normal_texture, qp);
	}

	if (material.occlusion_texture.texture)
	{
		comma(json);
		append(json, "\"occlusionTexture\":");
		writeTextureInfo(json, data, material.occlusion_texture, qp);
	}

	if (material.emissive_texture.texture)
	{
		comma(json);
		append(json, "\"emissiveTexture\":");
		writeTextureInfo(json, data, material.emissive_texture, qp);
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
				writeTextureInfo(json, data, pbr.diffuse_texture, qp);
			}
			if (pbr.specular_glossiness_texture.texture)
			{
				comma(json);
				append(json, "\"specularGlossinessTexture\":");
				writeTextureInfo(json, data, pbr.specular_glossiness_texture, qp);
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

void writeBufferView(std::string& json, BufferView::Kind kind, size_t count, size_t stride, size_t bin_offset, size_t bin_size, bool compressed)
{
	append(json, "{\"buffer\":0");
	append(json, ",\"byteLength\":");
	append(json, bin_size);
	append(json, ",\"byteOffset\":");
	append(json, bin_offset);
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
	if (compressed)
	{
		append(json, ",\"extensions\":{");
		append(json, "\"KHR_meshopt_compression\":{");
		append(json, "\"count\":");
		append(json, count);
		append(json, ",\"byteStride\":");
		append(json, stride);
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
	append(json, ",\"type\":");
	append(json, shapeType(type));

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

float getDelta(const Attr& l, const Attr& r, cgltf_animation_path_type type)
{
	if (type == cgltf_animation_path_type_rotation)
	{
		float error = 1.f - fabsf(l.f[0] * r.f[0] + l.f[1] * r.f[1] + l.f[2] * r.f[2] + l.f[3] * r.f[3]);

		return error;
	}
	else
	{
		float error = 0;
		for (int k = 0; k < 4; ++k)
			error += fabsf(r.f[k] - l.f[k]);

		return error;
	}
}

bool isTrackConstant(const cgltf_animation_sampler& sampler, cgltf_animation_path_type type)
{
	const float tolerance = 1e-3f;

	Attr first = {};
	cgltf_accessor_read_float(sampler.output, 0, first.f, 4);

	for (size_t i = 1; i < sampler.output->count; ++i)
	{
		Attr attr = {};
		cgltf_accessor_read_float(sampler.output, i, attr.f, 4);

		if (getDelta(first, attr, type) > tolerance)
			return false;
	}

	return true;
}

Attr interpolateLinear(const Attr& l, const Attr& r, float t, cgltf_animation_path_type type)
{
	if (type == cgltf_animation_path_type_rotation)
	{
		// Approximating slerp, https://zeux.io/2015/07/23/approximating-slerp/
		// We also handle quaternion double-cover
		float ca = l.f[0] * r.f[0] + l.f[1] * r.f[1] + l.f[2] * r.f[2] + l.f[3] * r.f[3];

		float d = fabsf(ca);
		float A = 1.0904f + d * (-3.2452f + d * (3.55645f - d * 1.43519f));
		float B = 0.848013f + d * (-1.06021f + d * 0.215638f);
		float k = A * (t - 0.5f) * (t - 0.5f) + B;
		float ot = t + t * (t - 0.5f) * (t - 1) * k;

		float t0 = 1 - ot;
		float t1 = ca > 0 ? ot : -ot;

		Attr lerp = {{
		    l.f[0] * t0 + r.f[0] * t1,
		    l.f[1] * t0 + r.f[1] * t1,
		    l.f[2] * t0 + r.f[2] * t1,
		    l.f[3] * t0 + r.f[3] * t1,
		}};

		float len = sqrtf(lerp.f[0] * lerp.f[0] + lerp.f[1] * lerp.f[1] + lerp.f[2] * lerp.f[2] + lerp.f[3] * lerp.f[3]);

		if (len > 0.f)
		{
			lerp.f[0] /= len;
			lerp.f[1] /= len;
			lerp.f[2] /= len;
			lerp.f[3] /= len;
		}

		return lerp;
	}
	else
	{
		Attr lerp = {{
		    l.f[0] * (1 - t) + r.f[0] * t,
		    l.f[1] * (1 - t) + r.f[1] * t,
		    l.f[2] * (1 - t) + r.f[2] * t,
		    l.f[3] * (1 - t) + r.f[3] * t,
		}};

		return lerp;
	}
}

Attr interpolateHermite(const Attr& v0, const Attr& t0, const Attr& v1, const Attr& t1, float t, float dt, cgltf_animation_path_type type)
{
	float s0 = 1 + t * t * (2 * t - 3);
	float s1 = t + t * t * (t - 2);
	float s2 = 1 - s0;
	float s3 = t * t * (t - 1);

	float ts1 = dt * s1;
	float ts3 = dt * s3;

	Attr lerp = {{
	    s0 * v0.f[0] + ts1 * t0.f[0] + s2 * v1.f[0] + ts3 * t1.f[0],
	    s0 * v0.f[1] + ts1 * t0.f[1] + s2 * v1.f[1] + ts3 * t1.f[1],
	    s0 * v0.f[2] + ts1 * t0.f[2] + s2 * v1.f[2] + ts3 * t1.f[2],
	    s0 * v0.f[3] + ts1 * t0.f[3] + s2 * v1.f[3] + ts3 * t1.f[3],
	}};

	if (type == cgltf_animation_path_type_rotation)
	{
		float len = sqrtf(lerp.f[0] * lerp.f[0] + lerp.f[1] * lerp.f[1] + lerp.f[2] * lerp.f[2] + lerp.f[3] * lerp.f[3]);

		if (len > 0.f)
		{
			lerp.f[0] /= len;
			lerp.f[1] /= len;
			lerp.f[2] /= len;
			lerp.f[3] /= len;
		}
	}

	return lerp;
}

void resampleKeyframes(std::vector<Attr>& data, const cgltf_animation_sampler& sampler, cgltf_animation_path_type type, int frames, float mint, int freq)
{
	size_t cursor = 0;

	for (int i = 0; i < frames; ++i)
	{
		float time = mint + float(i) / freq;

		while (cursor + 1 < sampler.input->count)
		{
			float next_time = 0;
			cgltf_accessor_read_float(sampler.input, cursor + 1, &next_time, 1);

			if (next_time > time)
				break;

			cursor++;
		}

		if (cursor + 1 < sampler.input->count)
		{
			float cursor_time = 0;
			float next_time = 0;
			cgltf_accessor_read_float(sampler.input, cursor + 0, &cursor_time, 1);
			cgltf_accessor_read_float(sampler.input, cursor + 1, &next_time, 1);

			float range = next_time - cursor_time;
			float inv_range = (range == 0.f) ? 0.f : 1.f / (next_time - cursor_time);
			float t = std::max(0.f, std::min(1.f, (time - cursor_time) * inv_range));

			switch (sampler.interpolation)
			{
			case cgltf_interpolation_type_linear:
			{
				Attr v0 = {};
				Attr v1 = {};
				cgltf_accessor_read_float(sampler.output, cursor + 0, v0.f, 4);
				cgltf_accessor_read_float(sampler.output, cursor + 1, v1.f, 4);
				data.push_back(interpolateLinear(v0, v1, t, type));
			}
			break;

			case cgltf_interpolation_type_step:
			{
				Attr v = {};
				cgltf_accessor_read_float(sampler.output, cursor, v.f, 4);
				data.push_back(v);
			}
			break;

			case cgltf_interpolation_type_cubic_spline:
			{
				Attr v0 = {};
				Attr b0 = {};
				Attr a1 = {};
				Attr v1 = {};
				cgltf_accessor_read_float(sampler.output, cursor * 3 + 1, v0.f, 4);
				cgltf_accessor_read_float(sampler.output, cursor * 3 + 2, b0.f, 4);
				cgltf_accessor_read_float(sampler.output, cursor * 3 + 3, a1.f, 4);
				cgltf_accessor_read_float(sampler.output, cursor * 3 + 4, v1.f, 4);
				data.push_back(interpolateHermite(v0, b0, v1, a1, t, range, type));
			}
			break;

			default:
				assert(!"Unknown interpolation type");
			}
		}
		else
		{
			size_t offset = sampler.interpolation == cgltf_interpolation_type_cubic_spline ? cursor * 3 + 1 : cursor;

			Attr v = {};
			cgltf_accessor_read_float(sampler.output, offset, v.f, 4);
			data.push_back(v);
		}
	}
}

void markAnimated(cgltf_data* data, std::vector<NodeInfo>& nodes)
{
	for (size_t i = 0; i < data->animations_count; ++i)
	{
		const cgltf_animation& animation = data->animations[i];

		for (size_t j = 0; j < animation.channels_count; ++j)
		{
			const cgltf_animation_channel& channel = animation.channels[j];
			const cgltf_animation_sampler& sampler = *channel.sampler;

			if (!channel.target_node)
				continue;

			NodeInfo& ni = nodes[channel.target_node - data->nodes];

			if (channel.target_path != cgltf_animation_path_type_translation && channel.target_path != cgltf_animation_path_type_rotation && channel.target_path != cgltf_animation_path_type_scale)
				continue;

			// mark nodes that have animation tracks that change their base transform as animated
			if (!isTrackConstant(sampler, channel.target_path))
			{
				ni.animated_paths |= (1 << channel.target_path);
			}
			else
			{
				Attr base = {};

				switch (channel.target_path)
				{
				case cgltf_animation_path_type_translation:
					memcpy(base.f, channel.target_node->translation, 3 * sizeof(float));
					break;
				case cgltf_animation_path_type_rotation:
					memcpy(base.f, channel.target_node->rotation, 4 * sizeof(float));
					break;
				case cgltf_animation_path_type_scale:
					memcpy(base.f, channel.target_node->scale, 3 * sizeof(float));
					break;
				default:;
				}

				Attr first = {};
				cgltf_accessor_read_float(sampler.output, 0, first.f, 4);

				const float tolerance = 1e-3f;

				if (getDelta(base, first, channel.target_path) > tolerance)
				{
					ni.animated_paths |= (1 << channel.target_path);
				}
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

void markNeeded(cgltf_data* data, std::vector<NodeInfo>& nodes, const std::vector<Mesh>& meshes)
{
	// mark all joints as kept & named (names might be important to manipulate externally)
	for (size_t i = 0; i < data->skins_count; ++i)
	{
		const cgltf_skin& skin = data->skins[i];

		// for now we keep all joints directly referenced by the skin and the entire ancestry tree; we keep names for joints as well
		for (size_t j = 0; j < skin.joints_count; ++j)
		{
			NodeInfo& ni = nodes[skin.joints[j] - data->nodes];

			ni.keep = true;
			ni.named = true;
		}
	}

	// mark all animated nodes as kept
	for (size_t i = 0; i < data->animations_count; ++i)
	{
		const cgltf_animation& animation = data->animations[i];

		for (size_t j = 0; j < animation.channels_count; ++j)
		{
			const cgltf_animation_channel& channel = animation.channels[j];

			if (channel.target_node)
			{
				NodeInfo& ni = nodes[channel.target_node - data->nodes];

				ni.keep = true;
			}
		}
	}

	// mark all mesh nodes as kept
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		if (mesh.indices.empty())
			continue;

		if (mesh.node)
		{
			NodeInfo& ni = nodes[mesh.node - data->nodes];

			ni.keep = true;
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
	views[view].data.append(data, size);

	// each chunk must be aligned to 4 bytes
	views[view].data.resize((views[view].data.size() + 3) & ~3);

	append(json, "\"bufferView\":");
	append(json, view);
	append(json, ",\"mimeType\":\"");
	append(json, mime_type);
	append(json, "\"");
}

bool process(cgltf_data* data, std::vector<Mesh>& meshes, const Settings& settings, std::string& json, std::string& bin)
{
	if (settings.verbose)
	{
		printf("input: %d nodes, %d meshes, %d skins, %d animations\n",
		       int(data->nodes_count), int(data->meshes_count), int(data->skins_count), int(data->animations_count));
	}

	std::vector<NodeInfo> nodes(data->nodes_count);

	markAnimated(data, nodes);

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		if (mesh.node)
		{
			NodeInfo& ni = nodes[mesh.node - data->nodes];

			// we transform all non-skinned non-animated meshes to world space
			// this makes sure that quantization doesn't introduce gaps if the original scene was watertight
			if (!ni.animated && !mesh.skin)
			{
				transformMesh(mesh, mesh.node);
				mesh.node = 0;
			}

			// skinned and animated meshes will be anchored to the same node that they used to be in
			// for animated meshes, this is important since they need to be transformed by the same animation
			// for skinned meshes, in theory this isn't important since the transform of the skinned node doesn't matter; in practice this affects generated bounding box in three.js
		}
	}

	mergeMeshes(meshes);

	markNeeded(data, nodes, meshes);

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		if (mesh.indices.empty())
			continue;

		reindexMesh(mesh);
		optimizeMesh(mesh);
	}

	if (settings.verbose)
	{
		size_t triangles = 0;
		size_t vertices = 0;

		for (size_t i = 0; i < meshes.size(); ++i)
		{
			const Mesh& mesh = meshes[i];

			triangles += mesh.indices.size() / 3;
			vertices += mesh.streams.empty() ? 0 : mesh.streams[0].data.size();
		}

		printf("meshes: %d triangles, %d vertices\n", int(triangles), int(vertices));
	}

	QuantizationParams qp = prepareQuantization(meshes, settings);

	std::string json_images;
	std::string json_textures;
	std::string json_materials;
	std::string json_accessors;
	std::string json_meshes;
	std::string json_nodes;
	std::string json_skins;
	std::string json_roots;
	std::string json_animations;

	std::vector<BufferView> views;
	std::string scratch;

	bool ext_pbr_specular_glossiness = false;
	bool ext_unlit = false;

	size_t accr_offset = 0;
	size_t node_offset = 0;
	size_t mesh_offset = 0;

	for (size_t i = 0; i < data->images_count; ++i)
	{
		const cgltf_image& image = data->images[i];

		comma(json_images);
		append(json_images, "{");
		if (image.uri)
		{
			std::string mime_type;
			std::string img;

			if (parseDataUri(image.uri, mime_type, img))
			{
				writeEmbeddedImage(json_images, views, img.c_str(), img.size(), mime_type.c_str());
			}
			else
			{
				append(json_images, "\"uri\":\"");
				append(json_images, image.uri);
				append(json_images, "\"");
			}
		}
		else if (image.buffer_view && image.buffer_view->buffer->data && image.mime_type)
		{
			const char* img = static_cast<const char*>(image.buffer_view->buffer->data) + image.buffer_view->offset;
			size_t size = image.buffer_view->size;

			writeEmbeddedImage(json_images, views, img, size, image.mime_type);
		}
		else
		{
			fprintf(stderr, "Warning: ignoring image %d since it has no URI and no valid buffer data\n", int(i));
		}

		append(json_images, "}");
	}

	for (size_t i = 0; i < data->textures_count; ++i)
	{
		const cgltf_texture& texture = data->textures[i];

		comma(json_textures);
		append(json_textures, "{");
		if (texture.image)
		{
			append(json_textures, "\"source\":");
			append(json_textures, size_t(texture.image - data->images));
		}
		append(json_textures, "}");
	}

	for (size_t i = 0; i < data->materials_count; ++i)
	{
		const cgltf_material& material = data->materials[i];

		comma(json_materials);
		append(json_materials, "{");
		writeMaterialInfo(json_materials, data, material, qp);
		append(json_materials, "}");

		ext_pbr_specular_glossiness = ext_pbr_specular_glossiness || material.has_pbr_specular_glossiness;
		ext_unlit = ext_unlit || material.unlit;
	}

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

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

			scratch.clear();
			StreamFormat format = writeVertexStream(scratch, stream, qp, settings);

			size_t view = getBufferView(views, BufferView::Kind_Vertex, 0, format.stride, settings.compress);
			size_t offset = views[view].data.size();
			views[view].data += scratch;

			comma(json_accessors);
			if (stream.type == cgltf_attribute_type_position)
			{
				uint16_t min[3] = {};
				uint16_t max[3] = {};
				getPositionBounds(min, max, stream, qp);

				// note: vec4 is used instead of vec3 to avoid three.js bug with interleaved buffers (#16802)
				float minf[4] = {float(min[0]), float(min[1]), float(min[2]), 1};
				float maxf[4] = {float(max[0]), float(max[1]), float(max[2]), 1};

				writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, stream.data.size(), minf, maxf, 4);
			}
			else
			{
				writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, stream.data.size());
			}

			size_t vertex_accr = accr_offset++;

			comma(json_attributes);
			append(json_attributes, "\"");
			append(json_attributes, attributeType(stream.type));
			if (stream.type != cgltf_attribute_type_position && stream.type != cgltf_attribute_type_normal && stream.type != cgltf_attribute_type_tangent)
			{
				append(json_attributes, "_");
				append(json_attributes, size_t(stream.index));
			}
			append(json_attributes, "\":");
			append(json_attributes, vertex_accr);
		}

		size_t index_accr = 0;

		{
			scratch.clear();
			StreamFormat format = writeIndexStream(scratch, mesh.indices);

			// TODO: ideally variant would be 0 but this hurts index compression
			size_t view = getBufferView(views, BufferView::Kind_Index, -1, format.stride, settings.compress);
			size_t offset = views[view].data.size();
			views[view].data += scratch;

			comma(json_accessors);
			writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, mesh.indices.size());

			index_accr = accr_offset++;
		}

		comma(json_meshes);
		append(json_meshes, "{\"primitives\":[{\"attributes\":{");
		append(json_meshes, json_attributes);
		append(json_meshes, "},\"indices\":");
		append(json_meshes, index_accr);
		if (mesh.material)
		{
			append(json_meshes, ",\"material\":");
			append(json_meshes, size_t(mesh.material - data->materials));
		}
		append(json_meshes, "}]}");

		float node_scale = qp.pos_scale / float((1 << qp.pos_bits) - 1);

		comma(json_nodes);
		append(json_nodes, "{\"mesh\":");
		append(json_nodes, mesh_offset);
		if (mesh.skin)
		{
			comma(json_nodes);
			append(json_nodes, "\"skin\":");
			append(json_nodes, size_t(mesh.skin - data->skins));
		}
		append(json_nodes, ",\"translation\":[");
		append(json_nodes, qp.pos_offset[0]);
		append(json_nodes, ",");
		append(json_nodes, qp.pos_offset[1]);
		append(json_nodes, ",");
		append(json_nodes, qp.pos_offset[2]);
		append(json_nodes, "],\"scale\":[");
		append(json_nodes, node_scale);
		append(json_nodes, ",");
		append(json_nodes, node_scale);
		append(json_nodes, ",");
		append(json_nodes, node_scale);
		append(json_nodes, "]");
		append(json_nodes, "}");

		if (mesh.node)
		{
			assert(nodes[mesh.node - data->nodes].keep);
			nodes[mesh.node - data->nodes].meshes.push_back(node_offset);
		}
		else
		{
			comma(json_roots);
			append(json_roots, node_offset);
		}

		node_offset++;
		mesh_offset++;
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

		comma(json_nodes);
		append(json_nodes, "{");
		if (node.name && ni.named)
		{
			comma(json_nodes);
			append(json_nodes, "\"name\":\"");
			append(json_nodes, node.name);
			append(json_nodes, "\"");
		}
		if (node.has_translation)
		{
			comma(json_nodes);
			append(json_nodes, "\"translation\":[");
			append(json_nodes, node.translation[0]);
			append(json_nodes, ",");
			append(json_nodes, node.translation[1]);
			append(json_nodes, ",");
			append(json_nodes, node.translation[2]);
			append(json_nodes, "]");
		}
		if (node.has_rotation)
		{
			comma(json_nodes);
			append(json_nodes, "\"rotation\":[");
			append(json_nodes, node.rotation[0]);
			append(json_nodes, ",");
			append(json_nodes, node.rotation[1]);
			append(json_nodes, ",");
			append(json_nodes, node.rotation[2]);
			append(json_nodes, ",");
			append(json_nodes, node.rotation[3]);
			append(json_nodes, "]");
		}
		if (node.has_scale)
		{
			comma(json_nodes);
			append(json_nodes, "\"scale\":[");
			append(json_nodes, node.scale[0]);
			append(json_nodes, ",");
			append(json_nodes, node.scale[1]);
			append(json_nodes, ",");
			append(json_nodes, node.scale[2]);
			append(json_nodes, "]");
		}
		if (node.has_matrix)
		{
			comma(json_nodes);
			append(json_nodes, "\"matrix\":[");
			for (int k = 0; k < 16; ++k)
			{
				comma(json_nodes);
				append(json_nodes, node.matrix[k]);
			}
			append(json_nodes, "]");
		}
		if (node.children_count || !ni.meshes.empty())
		{
			comma(json_nodes);
			append(json_nodes, "\"children\":[");
			for (size_t j = 0; j < node.children_count; ++j)
			{
				NodeInfo& ci = nodes[node.children[j] - data->nodes];

				if (ci.keep)
				{
					comma(json_nodes);
					append(json_nodes, size_t(ci.remap));
				}
			}
			for (size_t j = 0; j < ni.meshes.size(); ++j)
			{
				comma(json_nodes);
				append(json_nodes, ni.meshes[j]);
			}
			append(json_nodes, "]");
		}
		append(json_nodes, "}");
	}

	for (size_t i = 0; i < data->skins_count; ++i)
	{
		const cgltf_skin& skin = data->skins[i];

		scratch.clear();

		for (size_t j = 0; j < skin.joints_count; ++j)
		{
			float transform[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

			if (skin.inverse_bind_matrices)
			{
				cgltf_accessor_read_float(skin.inverse_bind_matrices, j, transform, 16);
			}

			float node_scale = qp.pos_scale / float((1 << qp.pos_bits) - 1);

			// pos_offset has to be applied first, thus it results in an offset rotated by the bind matrix
			transform[12] += qp.pos_offset[0] * transform[0] + qp.pos_offset[1] * transform[4] + qp.pos_offset[2] * transform[8];
			transform[13] += qp.pos_offset[0] * transform[1] + qp.pos_offset[1] * transform[5] + qp.pos_offset[2] * transform[9];
			transform[14] += qp.pos_offset[0] * transform[2] + qp.pos_offset[1] * transform[6] + qp.pos_offset[2] * transform[10];

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

	for (size_t i = 0; i < data->animations_count; ++i)
	{
		const cgltf_animation& animation = data->animations[i];

		std::string json_samplers;
		std::string json_channels;

		std::vector<const cgltf_animation_channel*> tracks;

		for (size_t j = 0; j < animation.channels_count; ++j)
		{
			const cgltf_animation_channel& channel = animation.channels[j];

			if (!channel.target_node)
			{
				fprintf(stderr, "Warning: ignoring channel %d of animation %d because it has no target node\n", int(j), int(i));
				continue;
			}

			NodeInfo& ni = nodes[channel.target_node - data->nodes];

			if (!ni.keep)
				continue;

			if (channel.target_path != cgltf_animation_path_type_translation && channel.target_path != cgltf_animation_path_type_rotation && channel.target_path != cgltf_animation_path_type_scale)
			{
				fprintf(stderr, "Warning: ignoring channel %d of animation %d because target path %d is not supported\n", int(j), int(i), channel.target_path);
				continue;
			}

			if (!settings.anim_const && (ni.animated_paths & (1 << channel.target_path)) == 0)
				continue;

			tracks.push_back(&channel);
		}

		if (tracks.empty())
		{
			fprintf(stderr, "Warning: ignoring animation %d because it has no valid tracks\n", int(i));
			continue;
		}

		float mint = 0, maxt = 0;
		bool needs_time = false;
		bool needs_pose = false;

		for (size_t j = 0; j < tracks.size(); ++j)
		{
			const cgltf_animation_channel& channel = *tracks[j];
			const cgltf_animation_sampler& sampler = *channel.sampler;

			mint = std::min(mint, sampler.input->min[0]);
			maxt = std::max(maxt, sampler.input->max[0]);

			bool tc = isTrackConstant(sampler, channel.target_path);

			needs_time = needs_time || !tc;
			needs_pose = needs_pose || tc;
		}

		// round the number of frames to nearest but favor the "up" direction
		// this means that at 10 Hz resampling, we will try to preserve the last frame <10ms
		// but if the last frame is <2ms we favor just removing this data
		int frames = 1 + int((maxt - mint) * settings.anim_freq + 0.8f);

		size_t time_accr = 0;

		if (needs_time)
		{
			std::vector<float> time(frames);

			for (int j = 0; j < frames; ++j)
				time[j] = mint + float(j) / settings.anim_freq;

			scratch.clear();
			StreamFormat format = writeTimeStream(scratch, time);

			size_t view = getBufferView(views, BufferView::Kind_Time, 0, format.stride, settings.compress);
			size_t offset = views[view].data.size();
			views[view].data += scratch;

			comma(json_accessors);
			writeAccessor(json_accessors, view, offset, cgltf_type_scalar, format.component_type, format.normalized, frames, &time.front(), &time.back(), 1);

			time_accr = accr_offset++;
		}

		size_t pose_accr = 0;

		if (needs_pose)
		{
			std::vector<float> pose(1, mint);

			scratch.clear();
			StreamFormat format = writeTimeStream(scratch, pose);

			size_t view = getBufferView(views, BufferView::Kind_Time, 0, format.stride, settings.compress);
			size_t offset = views[view].data.size();
			views[view].data += scratch;

			comma(json_accessors);
			writeAccessor(json_accessors, view, offset, cgltf_type_scalar, format.component_type, format.normalized, 1, &pose.front(), &pose.back(), 1);

			pose_accr = accr_offset++;
		}

		size_t track_offset = 0;

		for (size_t j = 0; j < tracks.size(); ++j)
		{
			const cgltf_animation_channel& channel = *tracks[j];
			const cgltf_animation_sampler& sampler = *channel.sampler;

			bool tc = isTrackConstant(sampler, channel.target_path);

			std::vector<Attr> track;
			if (tc)
			{
				Attr pose = {};
				cgltf_accessor_read_float(sampler.output, 0, pose.f, 4);
				track.push_back(pose);
			}
			else
			{
				resampleKeyframes(track, sampler, channel.target_path, frames, mint, settings.anim_freq);
			}

			scratch.clear();
			StreamFormat format = writeKeyframeStream(scratch, channel.target_path, track);

			size_t view = getBufferView(views, BufferView::Kind_Keyframe, channel.target_path, format.stride, settings.compress);
			size_t offset = views[view].data.size();
			views[view].data += scratch;

			comma(json_accessors);
			writeAccessor(json_accessors, view, offset, format.type, format.component_type, format.normalized, track.size());

			size_t data_accr = accr_offset++;

			comma(json_samplers);
			append(json_samplers, "{\"input\":");
			append(json_samplers, tc ? pose_accr : time_accr);
			append(json_samplers, ",\"output\":");
			append(json_samplers, data_accr);
			append(json_samplers, "}");

			comma(json_channels);
			append(json_channels, "{\"sampler\":");
			append(json_channels, track_offset);
			append(json_channels, ",\"target\":{\"node\":");
			append(json_channels, size_t(nodes[channel.target_node - data->nodes].remap));
			append(json_channels, ",\"path\":");
			append(json_channels, animationPath(channel.target_path));
			append(json_channels, "}}");

			track_offset++;
		}

		comma(json_animations);
		append(json_animations, "{");
		if (animation.name)
		{
			append(json_animations, "\"name\":\"");
			append(json_animations, animation.name);
			append(json_animations, "\",");
		}
		append(json_animations, "\"samplers\":[");
		append(json_animations, json_samplers);
		append(json_animations, "],\"channels\":[");
		append(json_animations, json_channels);
		append(json_animations, "]}");
	}

	append(json, "\"asset\":{\"version\":\"2.0\", \"generator\":\"gltfpack\"}");
	append(json, ",\"extensionsUsed\":[");
	append(json, "\"KHR_quantized_geometry\"");
	if (settings.compress)
	{
		comma(json);
		append(json, "\"KHR_meshopt_compression\"");
	}
	if (!json_textures.empty())
	{
		comma(json);
		append(json, "\"KHR_texture_transform\"");
	}
	if (ext_pbr_specular_glossiness)
	{
		comma(json);
		append(json, "\"KHR_materials_pbrSpecularGlossiness\"");
	}
	if (ext_unlit)
	{
		comma(json);
		append(json, "\"KHR_materials_unlit\"");
	}
	append(json, "]");

	size_t bytes[BufferView::Kind_Count] = {};
	size_t bytes_raw[BufferView::Kind_Count] = {};

	if (!views.empty())
	{
		append(json, ",\"bufferViews\":[");
		for (size_t i = 0; i < views.size(); ++i)
		{
			BufferView& view = views[i];

			size_t offset = bin.size();
			bin += view.data;
			bin.resize((bin.size() + 3) & ~3);

			size_t count = view.data.size() / view.stride;

			if (view.compressed)
			{
				if (view.kind == BufferView::Kind_Index)
					compressIndexStream(bin, offset, count, view.stride);
				else
					compressVertexStream(bin, offset, count, view.stride);
			}

			comma(json);
			writeBufferView(json, view.kind, count, view.stride, offset, bin.size() - offset, view.compressed);

			bytes[view.kind] += bin.size() - offset;
			bytes_raw[view.kind] += view.data.size();
		}
		append(json, "]");
	}
	if (!json_accessors.empty())
	{
		append(json, ",\"accessors\":[");
		append(json, json_accessors);
		append(json, "]");
	}
	if (!json_images.empty())
	{
		append(json, ",\"images\":[");
		append(json, json_images);
		append(json, "]");
	}
	if (!json_textures.empty())
	{
		append(json, ",\"textures\":[");
		append(json, json_textures);
		append(json, "]");
	}
	if (!json_materials.empty())
	{
		append(json, ",\"materials\":[");
		append(json, json_materials);
		append(json, "]");
	}
	if (!json_meshes.empty())
	{
		append(json, ",\"meshes\":[");
		append(json, json_meshes);
		append(json, "]");
	}
	if (!json_skins.empty())
	{
		append(json, ",\"skins\":[");
		append(json, json_skins);
		append(json, "]");
	}
	if (!json_animations.empty())
	{
		append(json, ",\"animations\":[");
		append(json, json_animations);
		append(json, "]");
	}
	if (!json_roots.empty())
	{
		append(json, ",\"nodes\":[");
		append(json, json_nodes);
		append(json, "],\"scenes\":[");
		append(json, "{\"nodes\":[");
		append(json, json_roots);
		append(json, "]}");
		append(json, "],\"scene\":0");
	}

	if (settings.verbose)
	{
		printf("output: %d nodes, %d meshes\n", int(node_offset), int(mesh_offset));
		printf("output: JSON %d bytes, buffers %d bytes\n", int(json.size()), int(bin.size()));
		printf("output: buffers: vertex %d bytes, index %d bytes, skin %d bytes, time %d bytes, keyframe %d bytes, image %d bytes\n",
		       int(bytes[BufferView::Kind_Vertex]), int(bytes[BufferView::Kind_Index]), int(bytes[BufferView::Kind_Skin]),
		       int(bytes[BufferView::Kind_Time]), int(bytes[BufferView::Kind_Keyframe]), int(bytes[BufferView::Kind_Image]));
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
	settings.tex_bits = 12;
	settings.nrm_bits = 8;
	settings.anim_freq = 30;

	const char* input = 0;
	const char* output = 0;
	bool help = false;

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
			settings.nrm_unit = true;
		}
		else if (strcmp(arg, "-af") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.anim_freq = atoi(argv[++i]);
		}
		else if (strcmp(arg, "-ac") == 0)
		{
			settings.anim_const = true;
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
		else if (strcmp(arg, "-v") == 0)
		{
			settings.verbose = true;
		}
		else if (strcmp(arg, "-h") == 0)
		{
			help = true;
		}
		else
		{
			fprintf(stderr, "Unrecognized option %s\n", arg);
			return 1;
		}
	}

	if (!input || !output || help)
	{
		fprintf(stderr, "Usage: gltfpack [options] -i input -o output\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "-i file: input file to process, .obj/.gltf/.glb\n");
		fprintf(stderr, "-o file: output file path, .gltf/.glb\n");
		fprintf(stderr, "-vp N: use N-bit quantization for positions (default: 14; N should be between 1 and 16)\n");
		fprintf(stderr, "-vt N: use N-bit quantization for texture corodinates (default: 12; N should be between 1 and 16)\n");
		fprintf(stderr, "-vn N: use N-bit quantization for normals and tangents (default: 8; N should be between 1 and 8)\n");
		fprintf(stderr, "-vu: use unit-length normal/tangent vectors (default: off)\n");
		fprintf(stderr, "-af N: resample animations at N Hz (default: 30)\n");
		fprintf(stderr, "-ac: keep constant animation tracks even if they don't modify the node transform\n");
		fprintf(stderr, "-c: produce compressed glb files\n");
		fprintf(stderr, "-v: verbose output\n");
		fprintf(stderr, "-h: display this help and exit\n");

		return 1;
	}

	cgltf_data* data = 0;
	std::vector<Mesh> meshes;

	const char* iext = strrchr(input, '.');

	if (iext && (strcmp(iext, ".gltf") == 0 || strcmp(iext, ".GLTF") == 0 || strcmp(iext, ".glb") == 0 || strcmp(iext, ".GLB") == 0))
	{
		cgltf_options options = {};
		cgltf_result result = cgltf_parse_file(&options, input, &data);
		result = (result == cgltf_result_success) ? cgltf_validate(data) : result;
		result = (result == cgltf_result_success) ? cgltf_load_buffers(&options, data, input) : result;

		if (result != cgltf_result_success)
		{
			fprintf(stderr, "Error loading %s: %s\n", input, getError(result));
			cgltf_free(data);
			return 2;
		}

		parseMeshesGltf(data, meshes);
	}
	else if (iext && (strcmp(iext, ".obj") == 0 || strcmp(iext, ".OBJ") == 0))
	{
		fastObjMesh* obj = fast_obj_read(input);

		if (!obj)
		{
			fprintf(stderr, "Error loading %s: file not found\n", input);
			cgltf_free(data);
			return 2;
		}

		data = parseSceneObj(obj);
		parseMeshesObj(obj, data, meshes);

		fast_obj_destroy(obj);
	}
	else
	{
		fprintf(stderr, "Error loading %s: unknown extension (expected .gltf or .glb or .obj)\n", input);
		return 2;
	}

	std::string json, bin;
	if (!process(data, meshes, settings, json, bin))
	{
		fprintf(stderr, "Error processing %s\n", input);
		cgltf_free(data);
		return 3;
	}

	cgltf_free(data);

	const char* oext = strrchr(output, '.');

	if (oext && (strcmp(oext, ".gltf") == 0 || strcmp(oext, ".GLTF") == 0))
	{
		std::string binpath = output;
		binpath.replace(binpath.size() - 5, 5, ".bin");

		std::string binname = binpath;
		std::string::size_type slash = binname.find_last_of("/\\");
		if (slash != std::string::npos)
			binname.erase(0, slash + 1);

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

		char bufferspec[64];
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
