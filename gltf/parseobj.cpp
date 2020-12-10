// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include "../extern/fast_obj.h"

#include <algorithm>

#include <stdlib.h>
#include <string.h>

static void defaultFree(void*, void* p)
{
	free(p);
}

static int textureIndex(const std::vector<std::string>& textures, const char* name)
{
	for (size_t i = 0; i < textures.size(); ++i)
		if (textures[i] == name)
			return int(i);

	return -1;
}

static cgltf_data* parseSceneObj(fastObjMesh* obj)
{
	cgltf_data* data = (cgltf_data*)calloc(1, sizeof(cgltf_data));
	data->memory.free = defaultFree;

	std::vector<std::string> textures;

	for (unsigned int mi = 0; mi < obj->material_count; ++mi)
	{
		fastObjMaterial& om = obj->materials[mi];

		if (om.map_Kd.name && textureIndex(textures, om.map_Kd.name) < 0)
			textures.push_back(om.map_Kd.name);
	}

	data->images = (cgltf_image*)calloc(textures.size(), sizeof(cgltf_image));
	data->images_count = textures.size();

	for (size_t i = 0; i < textures.size(); ++i)
	{
		data->images[i].uri = (char*)malloc(textures[i].size() + 1);
		strcpy(data->images[i].uri, textures[i].c_str());
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

	data->scenes = (cgltf_scene*)calloc(1, sizeof(cgltf_scene));
	data->scenes_count = 1;

	return data;
}

static void parseMeshesObj(fastObjMesh* obj, cgltf_data* data, std::vector<Mesh>& meshes)
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

		mesh.type = cgltf_primitive_type_triangles;

		mesh.streams.resize(3);
		mesh.streams[0].type = cgltf_attribute_type_position;
		mesh.streams[0].data.resize(vertex_count[mi]);
		mesh.streams[1].type = cgltf_attribute_type_normal;
		mesh.streams[1].data.resize(vertex_count[mi]);
		mesh.streams[2].type = cgltf_attribute_type_texcoord;
		mesh.streams[2].data.resize(vertex_count[mi]);
		mesh.indices.resize(index_count[mi]);
		mesh.targets = 0;
	}

	std::vector<char> mesh_normals(meshes.size());
	std::vector<char> mesh_texcoords(meshes.size());

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

			mesh_normals[mesh_index[mi]] |= ii.n > 0;
			mesh_texcoords[mesh_index[mi]] |= ii.t > 0;
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

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		assert(mesh.streams.size() == 3);
		assert(mesh.streams[1].type == cgltf_attribute_type_normal);
		assert(mesh.streams[2].type == cgltf_attribute_type_texcoord);

		if (!mesh_texcoords[i])
			mesh.streams.erase(mesh.streams.begin() + 2);

		if (!mesh_normals[i])
			mesh.streams.erase(mesh.streams.begin() + 1);
	}
}

cgltf_data* parseObj(const char* path, std::vector<Mesh>& meshes, const char** error)
{
	fastObjMesh* obj = fast_obj_read(path);

	if (!obj)
	{
		*error = "file not found";
		return 0;
	}

	cgltf_data* data = parseSceneObj(obj);
	parseMeshesObj(obj, data, meshes);

	fast_obj_destroy(obj);

	return data;
}
