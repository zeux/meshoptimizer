// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include "../extern/fast_obj.h"
#include "../src/meshoptimizer.h"

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

static void fixupUri(char* uri)
{
	// Some .obj paths come with back slashes, that are invalid as URI separators and won't open on macOS/Linux when embedding textures
	for (char* s = uri; *s; ++s)
		if (*s == '\\')
			*s = '/';
}

static void parseMaterialsObj(fastObjMesh* obj, cgltf_data* data)
{
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
		data->images[i].uri = strdup(textures[i].c_str());
		fixupUri(data->images[i].uri);
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
		const fastObjMaterial& om = obj->materials[mi];
		cgltf_material& gm = data->materials[mi];

		if (om.name)
			gm.name = strdup(om.name);

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
		else
		{
			gm.pbr_metallic_roughness.base_color_factor[0] = om.Kd[0];
			gm.pbr_metallic_roughness.base_color_factor[1] = om.Kd[1];
			gm.pbr_metallic_roughness.base_color_factor[2] = om.Kd[2];
		}

		if (om.map_d.name)
		{
			if (om.map_Kd.name && strcmp(om.map_Kd.name, om.map_d.name) != 0)
				fprintf(stderr, "Warning: material has different diffuse and alpha textures (Kd: %s, d: %s) and might not render correctly\n", om.map_Kd.name, om.map_d.name);

			gm.alpha_mode = cgltf_alpha_mode_blend;
		}
		else if (om.d < 1.0f)
		{
			gm.pbr_metallic_roughness.base_color_factor[3] = om.d;

			gm.alpha_mode = cgltf_alpha_mode_blend;
		}
	}
}

static void parseNodesObj(fastObjMesh* obj, cgltf_data* data)
{
	data->nodes = (cgltf_node*)calloc(obj->object_count, sizeof(cgltf_node));
	data->nodes_count = obj->object_count;

	for (unsigned int oi = 0; oi < obj->object_count; ++oi)
	{
		const fastObjGroup& og = obj->objects[oi];
		cgltf_node* node = &data->nodes[oi];

		if (og.name)
			node->name = strdup(og.name);

		node->rotation[3] = 1.0f;
		node->scale[0] = 1.0f;
		node->scale[1] = 1.0f;
		node->scale[2] = 1.0f;
	}

	data->scenes = (cgltf_scene*)calloc(1, sizeof(cgltf_scene));
	data->scenes_count = 1;

	data->scene = data->scenes;

	data->scenes->nodes = (cgltf_node**)calloc(obj->object_count, sizeof(cgltf_node*));
	data->scenes->nodes_count = obj->object_count;

	for (unsigned int oi = 0; oi < obj->object_count; ++oi)
		data->scenes->nodes[oi] = &data->nodes[oi];
}

static void parseMeshObj(fastObjMesh* obj, unsigned int face_offset, unsigned int face_vertex_offset, unsigned int face_count, unsigned int face_vertex_count, unsigned int index_count, Mesh& mesh)
{
	std::vector<unsigned int> remap(face_vertex_count);
	size_t unique_vertices = meshopt_generateVertexRemap(remap.data(), nullptr, face_vertex_count, &obj->indices[face_vertex_offset], face_vertex_count, sizeof(fastObjIndex));

	int pos_stream = 0;
	int nrm_stream = obj->normal_count > 1 ? 1 : -1;
	int tex_stream = obj->texcoord_count > 1 ? 1 + (nrm_stream >= 0) : -1;
	int col_stream = obj->color_count > 1 ? 1 + (nrm_stream >= 0) + (tex_stream >= 0) : -1;

	mesh.streams.resize(1 + (nrm_stream >= 0) + (tex_stream >= 0) + (col_stream >= 0));

	mesh.streams[pos_stream].type = cgltf_attribute_type_position;
	mesh.streams[pos_stream].data.resize(unique_vertices);

	if (nrm_stream >= 0)
	{
		mesh.streams[nrm_stream].type = cgltf_attribute_type_normal;
		mesh.streams[nrm_stream].data.resize(unique_vertices);
	}

	if (tex_stream >= 0)
	{
		mesh.streams[tex_stream].type = cgltf_attribute_type_texcoord;
		mesh.streams[tex_stream].data.resize(unique_vertices);
	}

	if (col_stream >= 0)
	{
		mesh.streams[col_stream].type = cgltf_attribute_type_color;
		mesh.streams[col_stream].data.resize(unique_vertices);
	}

	mesh.indices.resize(index_count);

	for (unsigned int vi = 0; vi < face_vertex_count; ++vi)
	{
		unsigned int target = remap[vi];
		// TODO: this fills every target vertex multiple times

		fastObjIndex ii = obj->indices[face_vertex_offset + vi];

		Attr p = {{obj->positions[ii.p * 3 + 0], obj->positions[ii.p * 3 + 1], obj->positions[ii.p * 3 + 2]}};
		mesh.streams[pos_stream].data[target] = p;

		if (nrm_stream >= 0)
		{
			Attr n = {{obj->normals[ii.n * 3 + 0], obj->normals[ii.n * 3 + 1], obj->normals[ii.n * 3 + 2]}};
			mesh.streams[nrm_stream].data[target] = n;
		}

		if (tex_stream >= 0)
		{
			Attr t = {{obj->texcoords[ii.t * 2 + 0], 1.f - obj->texcoords[ii.t * 2 + 1]}};
			mesh.streams[tex_stream].data[target] = t;
		}

		if (col_stream >= 0)
		{
			Attr c = {{obj->colors[ii.p * 3 + 0], obj->colors[ii.p * 3 + 1], obj->colors[ii.p * 3 + 2]}};
			mesh.streams[col_stream].data[target] = c;
		}
	}

	unsigned int vertex_offset = 0;
	unsigned int index_offset = 0;

	for (unsigned int fi = 0; fi < face_count; ++fi)
	{
		unsigned int face_vertices = obj->face_vertices[face_offset + fi];

		for (unsigned int vi = 2; vi < face_vertices; ++vi)
		{
			size_t to = index_offset + (vi - 2) * 3;

			mesh.indices[to + 0] = remap[vertex_offset];
			mesh.indices[to + 1] = remap[vertex_offset + vi - 1];
			mesh.indices[to + 2] = remap[vertex_offset + vi];
		}

		vertex_offset += face_vertices;
		index_offset += (face_vertices - 2) * 3;
	}

	assert(vertex_offset == face_vertex_count);
	assert(index_offset == index_count);
}

static void parseMeshGroupObj(fastObjMesh* obj, const fastObjGroup& og, cgltf_data* data, cgltf_node* node, std::vector<Mesh>& meshes)
{
	unsigned int face_vertex_offset = og.index_offset;
	unsigned int face_end_offset = og.face_offset + og.face_count;

	for (unsigned int face_offset = og.face_offset; face_offset < face_end_offset; )
	{
		unsigned int mi = obj->face_materials[face_offset];

		unsigned int face_count = 0;
		unsigned int face_vertex_count = 0;
		unsigned int index_count = 0;

		for (unsigned int fj = face_offset; fj < face_end_offset && obj->face_materials[fj] == mi; ++fj)
		{
			face_count += 1;
			face_vertex_count += obj->face_vertices[fj];
			index_count += (obj->face_vertices[fj] - 2) * 3;
		}

		meshes.push_back(Mesh());
		Mesh& mesh = meshes.back();

		if (data->materials_count)
		{
			assert(mi < data->materials_count);
			mesh.material = &data->materials[mi];
		}

		mesh.type = cgltf_primitive_type_triangles;
		mesh.targets = 0;

		if (node)
			mesh.nodes.push_back(node);

		parseMeshObj(obj, face_offset, face_vertex_offset, face_count, face_vertex_count, index_count, mesh);

		face_offset += face_count;
		face_vertex_offset += face_vertex_count;
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

	cgltf_data* data = (cgltf_data*)calloc(1, sizeof(cgltf_data));
	data->memory.free_func = defaultFree;

	parseMaterialsObj(obj, data);

	parseNodesObj(obj, data);
	assert(data->nodes_count == obj->object_count);

	for (unsigned int oi = 0; oi < obj->object_count; ++oi)
		parseMeshGroupObj(obj, obj->objects[oi], data, &data->nodes[oi], meshes);

	fast_obj_destroy(obj);

	return data;
}
