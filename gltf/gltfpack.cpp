// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <algorithm>
#include <unordered_map>

#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __wasi__
#include <unistd.h>
#endif

#include "../src/meshoptimizer.h"

std::string getVersion()
{
	char result[32];
	snprintf(result, sizeof(result), "%d.%d", MESHOPTIMIZER_VERSION / 1000, (MESHOPTIMIZER_VERSION % 1000) / 10);
	return result;
}

static void finalizeBufferViews(std::string& json, std::vector<BufferView>& views, std::string& bin, std::string* fallback, size_t& fallback_size)
{
	for (size_t i = 0; i < views.size(); ++i)
	{
		BufferView& view = views[i];

		size_t bin_offset = bin.size();
		size_t fallback_offset = fallback_size;

		size_t count = view.data.size() / view.stride;

		if (view.compression == BufferView::Compression_None)
		{
			bin += view.data;
		}
		else
		{
			switch (view.compression)
			{
			case BufferView::Compression_Attribute:
				compressVertexStream(bin, view.data, count, view.stride);
				break;
			case BufferView::Compression_Index:
				compressIndexStream(bin, view.data, count, view.stride);
				break;
			case BufferView::Compression_IndexSequence:
				compressIndexSequence(bin, view.data, count, view.stride);
				break;
			default:
				assert(!"Unknown compression type");
			}

			if (fallback)
				*fallback += view.data;
			fallback_size += view.data.size();
		}

		size_t raw_offset = (view.compression != BufferView::Compression_None) ? fallback_offset : bin_offset;

		comma(json);
		writeBufferView(json, view.kind, view.filter, count, view.stride, raw_offset, view.data.size(), view.compression, bin_offset, bin.size() - bin_offset);

		// record written bytes for statistics
		view.bytes = bin.size() - bin_offset;

		// align each bufferView by 4 bytes
		bin.resize((bin.size() + 3) & ~3);
		if (fallback)
			fallback->resize((fallback->size() + 3) & ~3);
		fallback_size = (fallback_size + 3) & ~3;
	}
}

static void printMeshStats(const std::vector<Mesh>& meshes, const char* name)
{
	size_t mesh_triangles = 0;
	size_t mesh_vertices = 0;
	size_t total_triangles = 0;
	size_t total_instances = 0;
	size_t total_draws = 0;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		size_t triangles = mesh.type == cgltf_primitive_type_triangles ? mesh.indices.size() / 3 : 0;

		mesh_triangles += triangles;
		mesh_vertices += mesh.streams.empty() ? 0 : mesh.streams[0].data.size();

		size_t instances = std::max(size_t(1), mesh.nodes.size() + mesh.instances.size());

		total_triangles += triangles * instances;
		total_instances += instances;
		total_draws += std::max(size_t(1), mesh.nodes.size());
	}

	printf("%s: %d mesh primitives (%d triangles, %d vertices); %d draw calls (%d instances, %lld triangles)\n", name,
	    int(meshes.size()), int(mesh_triangles), int(mesh_vertices),
	    int(total_draws), int(total_instances), (long long)total_triangles);
}

static void printSceneStats(const std::vector<BufferView>& views, const std::vector<Mesh>& meshes, size_t node_offset, size_t mesh_offset, size_t material_offset, size_t json_size, size_t bin_size)
{
	size_t bytes[BufferView::Kind_Count] = {};

	for (size_t i = 0; i < views.size(); ++i)
	{
		const BufferView& view = views[i];
		bytes[view.kind] += view.bytes;
	}

	printf("output: %d nodes, %d meshes (%d primitives), %d materials\n", int(node_offset), int(mesh_offset), int(meshes.size()), int(material_offset));
	printf("output: JSON %d bytes, buffers %d bytes\n", int(json_size), int(bin_size));
	printf("output: buffers: vertex %d bytes, index %d bytes, skin %d bytes, time %d bytes, keyframe %d bytes, instance %d bytes, image %d bytes\n",
	    int(bytes[BufferView::Kind_Vertex]), int(bytes[BufferView::Kind_Index]), int(bytes[BufferView::Kind_Skin]),
	    int(bytes[BufferView::Kind_Time]), int(bytes[BufferView::Kind_Keyframe]), int(bytes[BufferView::Kind_Instance]),
	    int(bytes[BufferView::Kind_Image]));
}

static void printAttributeStats(const std::vector<BufferView>& views, BufferView::Kind kind, const char* name)
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
		case BufferView::Kind_Instance:
			variant = animationPath(cgltf_animation_path_type(view.variant));
			break;

		default:;
		}

		size_t count = view.data.size() / view.stride;

		printf("stats: %s %s: compressed %d bytes (%.1f bits), raw %d bytes (%d bits)\n",
		    name, variant,
		    int(view.bytes), double(view.bytes) / double(count) * 8,
		    int(view.data.size()), int(view.stride * 8));
	}
}

static void printImageStats(const std::vector<BufferView>& views, TextureKind kind, const char* name)
{
	size_t bytes = 0;
	size_t count = 0;

	for (size_t i = 0; i < views.size(); ++i)
	{
		const BufferView& view = views[i];

		if (view.kind != BufferView::Kind_Image)
			continue;

		if (view.variant != -1 - kind)
			continue;

		count += 1;
		bytes += view.data.size();
	}

	if (count)
		printf("stats: image %s: %d bytes in %d images\n", name, int(bytes), int(count));
}

static bool printReport(const char* path, const std::vector<BufferView>& views, const std::vector<Mesh>& meshes, size_t node_count, size_t mesh_count, size_t texture_count, size_t material_count, size_t animation_count, size_t json_size, size_t bin_size)
{
	size_t bytes[BufferView::Kind_Count] = {};

	for (size_t i = 0; i < views.size(); ++i)
	{
		const BufferView& view = views[i];
		bytes[view.kind] += view.bytes;
	}

	size_t total_triangles = 0;
	size_t total_instances = 0;
	size_t total_draws = 0;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		size_t triangles = mesh.type == cgltf_primitive_type_triangles ? mesh.indices.size() / 3 : 0;
		size_t instances = std::max(size_t(1), mesh.nodes.size() + mesh.instances.size());

		total_triangles += triangles * instances;
		total_instances += instances;
		total_draws += std::max(size_t(1), mesh.nodes.size());
	}

	FILE* out = fopen(path, "wb");
	if (!out)
		return false;

	fprintf(out, "{\n");
	fprintf(out, "\t\"generator\": \"gltfpack %s\",\n", getVersion().c_str());
	fprintf(out, "\t\"scene\": {\n");
	fprintf(out, "\t\t\"nodeCount\": %d,\n", int(node_count));
	fprintf(out, "\t\t\"meshCount\": %d,\n", int(mesh_count));
	fprintf(out, "\t\t\"materialCount\": %d,\n", int(material_count));
	fprintf(out, "\t\t\"textureCount\": %d,\n", int(texture_count));
	fprintf(out, "\t\t\"animationCount\": %d\n", int(animation_count));
	fprintf(out, "\t},\n");
	fprintf(out, "\t\"render\": {\n");
	fprintf(out, "\t\t\"drawCount\": %d,\n", int(total_draws));
	fprintf(out, "\t\t\"instanceCount\": %d,\n", int(total_instances));
	fprintf(out, "\t\t\"triangleCount\": %lld\n", (long long)total_triangles);
	fprintf(out, "\t},\n");
	fprintf(out, "\t\"data\": {\n");
	fprintf(out, "\t\t\"json\": %d,\n", int(json_size));
	fprintf(out, "\t\t\"binary\": %d,\n", int(bin_size));
	fprintf(out, "\t\t\"buffers\": {\n");
	fprintf(out, "\t\t\t\"vertex\": %d,\n", int(bytes[BufferView::Kind_Vertex]));
	fprintf(out, "\t\t\t\"index\": %d,\n", int(bytes[BufferView::Kind_Index]));
	fprintf(out, "\t\t\t\"animation\": %d,\n", int(bytes[BufferView::Kind_Time] + bytes[BufferView::Kind_Keyframe]));
	fprintf(out, "\t\t\t\"transform\": %d,\n", int(bytes[BufferView::Kind_Skin] + bytes[BufferView::Kind_Instance]));
	fprintf(out, "\t\t\t\"image\": %d\n", int(bytes[BufferView::Kind_Image]));
	fprintf(out, "\t\t}\n");
	fprintf(out, "\t}\n");
	fprintf(out, "}\n");

	int rc = fclose(out);
	return rc == 0;
}

static bool canTransformMesh(const Mesh& mesh)
{
	// volume thickness is specified in mesh coordinate space; to avoid modifying materials we prohibit transforming meshes with volume materials
	if (mesh.material && mesh.material->has_volume && mesh.material->volume.thickness_factor > 0.f)
		return false;

	return true;
}

static void detachMesh(Mesh& mesh, cgltf_data* data, const std::vector<NodeInfo>& nodes, const Settings& settings)
{
	// mesh is already instanced, skip
	if (!mesh.instances.empty())
		return;

	// mesh is already world space, skip
	if (mesh.nodes.empty())
		return;

	// note: when -kn is specified, we keep mesh-node attachment so that named nodes can be transformed
	if (settings.keep_nodes)
		return;

	// we keep skinned meshes or meshes with morph targets as is
	// in theory we could transform both, but in practice transforming morph target meshes is more involved,
	// and reparenting skinned meshes leads to incorrect bounding box generated in three.js
	if (mesh.skin || mesh.targets)
		return;

	bool any_animated = false;
	for (size_t j = 0; j < mesh.nodes.size(); ++j)
		any_animated |= nodes[mesh.nodes[j] - data->nodes].animated;

	// animated meshes will be anchored to the same node that they used to be in to retain the animation
	if (any_animated)
		return;

	int scene = nodes[mesh.nodes[0] - data->nodes].scene;
	bool any_other_scene = false;
	for (size_t j = 0; j < mesh.nodes.size(); ++j)
		any_other_scene |= scene != nodes[mesh.nodes[j] - data->nodes].scene;

	// we only merge instances when all nodes have a single consistent scene
	if (scene < 0 || any_other_scene)
		return;

	// we only merge multiple instances together if requested
	// this often makes the scenes faster to render by reducing the draw call count, but can result in larger files
	if (mesh.nodes.size() > 1 && !settings.mesh_merge && !settings.mesh_instancing)
		return;

	// mesh has duplicate geometry; detaching it would increase the size due to unique world-space transforms
	if (mesh.nodes.size() == 1 && mesh.geometry_duplicate && !settings.mesh_merge)
		return;

	// prefer instancing if possible, use merging otherwise
	if (mesh.nodes.size() > 1 && settings.mesh_instancing)
	{
		mesh.instances.resize(mesh.nodes.size());

		for (size_t j = 0; j < mesh.nodes.size(); ++j)
			cgltf_node_transform_world(mesh.nodes[j], mesh.instances[j].data);

		mesh.nodes.clear();
		mesh.scene = scene;
	}
	else if (canTransformMesh(mesh))
	{
		mergeMeshInstances(mesh);

		assert(mesh.nodes.empty());
		mesh.scene = scene;
	}
}

static bool isExtensionSupported(const ExtensionInfo* extensions, size_t count, const char* name)
{
	for (size_t i = 0; i < count; ++i)
		if (strcmp(extensions[i].name, name) == 0)
			return true;

	return false;
}

namespace std
{
template <>
struct hash<std::pair<uint64_t, uint64_t> >
{
	size_t operator()(const std::pair<uint64_t, uint64_t>& x) const
	{
		return std::hash<uint64_t>()(x.first ^ x.second);
	}
};
} // namespace std

static size_t process(cgltf_data* data, const char* input_path, const char* output_path, const char* report_path, std::vector<Mesh>& meshes, std::vector<Animation>& animations, const Settings& settings, std::string& json, std::string& bin, std::string& fallback, size_t& fallback_size)
{
	if (settings.verbose)
	{
		printf("input: %d nodes, %d meshes (%d primitives), %d materials, %d skins, %d animations, %d images\n",
		    int(data->nodes_count), int(data->meshes_count), int(meshes.size()), int(data->materials_count), int(data->skins_count), int(animations.size()), int(data->images_count));
		printMeshStats(meshes, "input");
	}

	for (size_t i = 0; i < animations.size(); ++i)
		processAnimation(animations[i], settings);

	std::vector<NodeInfo> nodes(data->nodes_count);

	markScenes(data, nodes);
	markAnimated(data, nodes, animations);

	mergeMeshMaterials(data, meshes, settings);
	if (settings.mesh_dedup)
		dedupMeshes(meshes, settings);

	for (size_t i = 0; i < meshes.size(); ++i)
		detachMesh(meshes[i], data, nodes, settings);

	// material information is required for mesh and image processing
	std::vector<MaterialInfo> materials(data->materials_count);
	std::vector<TextureInfo> textures(data->textures_count);
	std::vector<ImageInfo> images(data->images_count);

	analyzeMaterials(data, materials, textures, images);

	mergeTextures(data, textures);

	optimizeMaterials(data, input_path, images);

	// streams need to be filtered before mesh merging (or processing) to make sure we can merge meshes with redundant streams
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];
		MaterialInfo mi = mesh.material ? materials[mesh.material - data->materials] : MaterialInfo();

		// merge material requirements across all variants
		for (size_t j = 0; j < mesh.variants.size(); ++j)
		{
			MaterialInfo vi = materials[mesh.variants[j].material - data->materials];

			mi.needs_tangents |= vi.needs_tangents;
			mi.texture_set_mask |= vi.texture_set_mask;
			mi.unlit &= vi.unlit;
		}

		if (!settings.keep_attributes)
			filterStreams(mesh, mi);
	}

	mergeMeshes(meshes, settings);
	filterEmptyMeshes(meshes);

	markNeededNodes(data, nodes, meshes, animations, settings);
	markNeededMaterials(data, materials, meshes, settings);

#ifndef NDEBUG
	std::vector<Mesh> debug_meshes;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		if (mesh.type != cgltf_primitive_type_triangles)
			continue;

		if (settings.simplify_debug > 0)
		{
			Mesh kinds = {};
			Mesh loops = {};
			debugSimplify(mesh, kinds, loops, settings.simplify_debug, settings.simplify_error, settings.simplify_attributes, settings.quantize && !settings.nrm_float);
			debug_meshes.push_back(kinds);
			debug_meshes.push_back(loops);
		}

		if (settings.meshlet_debug > 0)
		{
			Mesh meshlets = {};
			debugMeshlets(mesh, meshlets, settings.meshlet_debug);
			debug_meshes.push_back(meshlets);
		}
	}
#endif

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];
		processMesh(mesh, settings);

		if (mesh.geometry_duplicate)
			hashMesh(mesh);
	}

#ifndef NDEBUG
	meshes.insert(meshes.end(), debug_meshes.begin(), debug_meshes.end());
#endif

	filterEmptyMeshes(meshes); // some meshes may become empty after processing

	QuantizationPosition qp = prepareQuantizationPosition(meshes, settings);

	std::vector<QuantizationTexture> qt_materials(materials.size());
	std::vector<size_t> qt_meshes(meshes.size(), size_t(-1));
	prepareQuantizationTexture(data, qt_materials, qt_meshes, meshes, settings);

	QuantizationTexture qt_dummy = {};
	qt_dummy.bits = settings.tex_bits;

	std::string json_images;
	std::string json_samplers;
	std::string json_textures;
	std::string json_materials;
	std::string json_accessors;
	std::string json_meshes;
	std::string json_nodes;
	std::string json_skins;
	std::vector<std::string> json_roots(data->scenes_count);
	std::string json_animations;
	std::string json_cameras;
	std::string json_extensions;

	std::vector<BufferView> views;

	bool ext_pbr_specular_glossiness = false;
	bool ext_clearcoat = false;
	bool ext_transmission = false;
	bool ext_ior = false;
	bool ext_specular = false;
	bool ext_sheen = false;
	bool ext_volume = false;
	bool ext_emissive_strength = false;
	bool ext_iridescence = false;
	bool ext_anisotropy = false;
	bool ext_dispersion = false;
	bool ext_diffuse_transmission = false;
	bool ext_unlit = false;
	bool ext_instancing = false;
	bool ext_texture_transform = false;
	bool ext_texture_basisu = false;
	bool ext_texture_webp = false;

	size_t accr_offset = 0;
	size_t node_offset = 0;
	size_t mesh_offset = 0;
	size_t texture_offset = 0;
	size_t material_offset = 0;

	for (size_t i = 0; i < data->samplers_count; ++i)
	{
		const cgltf_sampler& sampler = data->samplers[i];

		comma(json_samplers);
		append(json_samplers, "{");
		writeSampler(json_samplers, sampler);
		append(json_samplers, "}");
	}

	std::vector<std::string> encoded_images;

#ifdef WITH_BASISU
	if (data->images_count && settings.texture_ktx2)
	{
		encoded_images.resize(data->images_count);

		encodeImages(encoded_images.data(), data, images, input_path, settings);
	}
#endif

	for (size_t i = 0; i < data->images_count; ++i)
	{
		const cgltf_image& image = data->images[i];

		std::string* encoded = (encoded_images.size() && !encoded_images[i].empty()) ? &encoded_images[i] : NULL;

		comma(json_images);
		append(json_images, "{");
		writeImage(json_images, views, image, images[i], encoded, i, input_path, output_path, settings);
		append(json_images, "}");

		if (encoded)
			*encoded = std::string(); // reclaim memory early
	}

	for (size_t i = 0; i < data->textures_count; ++i)
	{
		const cgltf_texture& texture = data->textures[i];

		if (!textures[i].keep)
			continue;

		comma(json_textures);
		append(json_textures, "{");
		writeTexture(json_textures, texture, texture.image ? &images[texture.image - data->images] : NULL, data, settings);
		append(json_textures, "}");

		assert(textures[i].remap == int(texture_offset));
		texture_offset++;
		ext_texture_basisu = ext_texture_basisu || texture.has_basisu;
		ext_texture_webp = ext_texture_webp || texture.has_webp;
	}

	for (size_t i = 0; i < data->materials_count; ++i)
	{
		MaterialInfo& mi = materials[i];

		if (!mi.keep)
			continue;

		const cgltf_material& material = data->materials[i];

		comma(json_materials);
		append(json_materials, "{");
		writeMaterial(json_materials, data, material, settings.quantize && !settings.pos_float ? &qp : NULL, settings.quantize && !settings.tex_float ? &qt_materials[i] : NULL, textures);
		if (settings.keep_extras)
			writeExtras(json_materials, material.extras);
		append(json_materials, "}");

		mi.remap = int(material_offset);
		material_offset++;

		ext_pbr_specular_glossiness = ext_pbr_specular_glossiness || material.has_pbr_specular_glossiness;
		ext_clearcoat = ext_clearcoat || material.has_clearcoat;
		ext_transmission = ext_transmission || material.has_transmission;
		ext_ior = ext_ior || material.has_ior;
		ext_specular = ext_specular || material.has_specular;
		ext_sheen = ext_sheen || material.has_sheen;
		ext_volume = ext_volume || material.has_volume;
		ext_emissive_strength = ext_emissive_strength || material.has_emissive_strength;
		ext_iridescence = ext_iridescence || material.has_iridescence;
		ext_diffuse_transmission = ext_diffuse_transmission || material.has_diffuse_transmission;
		ext_anisotropy = ext_anisotropy || material.has_anisotropy;
		ext_dispersion = ext_dispersion || material.has_dispersion;
		ext_unlit = ext_unlit || material.unlit;
		ext_texture_transform = ext_texture_transform || mi.uses_texture_transform;
	}

	std::unordered_map<std::pair<uint64_t, uint64_t>, std::pair<size_t, size_t> > primitive_cache;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		comma(json_meshes);
		append(json_meshes, "{\"primitives\":[");

		size_t pi = i;
		for (; pi < meshes.size(); ++pi)
		{
			const Mesh& prim = meshes[pi];

			if (prim.scene != mesh.scene || prim.skin != mesh.skin || prim.targets != mesh.targets)
				break;

			if (pi > i && (mesh.instances.size() || prim.instances.size()))
				break;

			if (!compareMeshNodes(mesh, prim))
				break;

			if (!compareMeshTargets(mesh, prim))
				break;

			const QuantizationTexture& qt = qt_meshes[pi] == size_t(-1) ? qt_dummy : qt_materials[qt_meshes[pi]];

			comma(json_meshes);

			if (prim.geometry_duplicate)
			{
				std::pair<size_t, size_t>& primitive_json = primitive_cache[std::make_pair(prim.geometry_hash[0], prim.geometry_hash[1])];

				if (primitive_json.second)
				{
					// reuse previously written accessors
					json_meshes.append(json_meshes, primitive_json.first, primitive_json.second);
				}
				else
				{
					primitive_json.first = json_meshes.size();
					writeMeshGeometry(json_meshes, views, json_accessors, accr_offset, prim, qp, qt, settings);
					primitive_json.second = json_meshes.size() - primitive_json.first;
				}
			}
			else
			{
				writeMeshGeometry(json_meshes, views, json_accessors, accr_offset, prim, qp, qt, settings);
			}

			if (prim.material)
			{
				MaterialInfo& mi = materials[prim.material - data->materials];

				assert(mi.keep);
				append(json_meshes, ",\"material\":");
				append(json_meshes, size_t(mi.remap));
			}

			if (prim.variants.size())
			{
				append(json_meshes, ",\"extensions\":{\"KHR_materials_variants\":{\"mappings\":[");

				for (size_t j = 0; j < prim.variants.size(); ++j)
				{
					const cgltf_material_mapping& variant = prim.variants[j];
					MaterialInfo& mi = materials[variant.material - data->materials];

					assert(mi.keep);
					comma(json_meshes);
					append(json_meshes, "{\"material\":");
					append(json_meshes, size_t(mi.remap));
					append(json_meshes, ",\"variants\":[");
					append(json_meshes, size_t(variant.variant));
					append(json_meshes, "]}");
				}

				append(json_meshes, "]}}");
			}

			if (settings.keep_extras)
				writeExtras(json_meshes, prim.extras);

			append(json_meshes, "}");
		}

		append(json_meshes, "]");

		if (mesh.target_weights.size())
		{
			append(json_meshes, ",\"weights\":");
			append(json_meshes, mesh.target_weights.data(), mesh.target_weights.size());
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

		if (mesh.nodes.size())
		{
			for (size_t j = 0; j < mesh.nodes.size(); ++j)
			{
				NodeInfo& ni = nodes[mesh.nodes[j] - data->nodes];
				assert(ni.keep);

				// if we don't use position quantization, prefer attaching the mesh to its node directly
				if (!ni.has_mesh && (!settings.quantize || settings.pos_float || (qp.offset[0] == 0.f && qp.offset[1] == 0.f && qp.offset[2] == 0 && qp.node_scale == 1.f)))
				{
					ni.has_mesh = true;
					ni.mesh_index = mesh_offset;
					ni.mesh_skin = mesh.skin;
				}
				else
				{
					ni.mesh_nodes.push_back(node_offset);

					writeMeshNode(json_nodes, mesh_offset, mesh.nodes[j], mesh.skin, data, settings.quantize && !settings.pos_float ? &qp : NULL);

					node_offset++;
				}
			}
		}

		if (mesh.instances.size())
		{
			assert(mesh.scene >= 0);
			comma(json_roots[mesh.scene]);
			append(json_roots[mesh.scene], node_offset);

			size_t instance_accr = writeInstances(views, json_accessors, accr_offset, mesh.instances, qp, settings);

			assert(!mesh.skin);
			writeMeshNodeInstanced(json_nodes, mesh_offset, instance_accr);

			node_offset++;
		}

		if (mesh.nodes.empty() && mesh.instances.empty())
		{
			assert(mesh.scene >= 0);
			comma(json_roots[mesh.scene]);
			append(json_roots[mesh.scene], node_offset);

			writeMeshNode(json_nodes, mesh_offset, NULL, mesh.skin, data, settings.quantize && !settings.pos_float ? &qp : NULL);

			node_offset++;
		}

		mesh_offset++;
		ext_instancing = ext_instancing || !mesh.instances.empty();

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

		comma(json_nodes);
		append(json_nodes, "{");
		writeNode(json_nodes, node, nodes, data);
		if (settings.keep_extras)
			writeExtras(json_nodes, node.extras);
		append(json_nodes, "}");
	}

	for (size_t i = 0; i < data->scenes_count; ++i)
	{
		for (size_t j = 0; j < data->scenes[i].nodes_count; ++j)
		{
			NodeInfo& ni = nodes[data->scenes[i].nodes[j] - data->nodes];

			if (ni.keep)
			{
				comma(json_roots[i]);
				append(json_roots[i], size_t(ni.remap));
			}
		}
	}

	for (size_t i = 0; i < data->skins_count; ++i)
	{
		const cgltf_skin& skin = data->skins[i];

		size_t matrix_accr = writeJointBindMatrices(views, json_accessors, accr_offset, skin, qp, settings);

		writeSkin(json_skins, skin, matrix_accr, nodes, data);
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

	if (data->lights_count > 0)
	{
		comma(json_extensions);
		append(json_extensions, "\"KHR_lights_punctual\":{\"lights\":[");

		for (size_t i = 0; i < data->lights_count; ++i)
		{
			const cgltf_light& light = data->lights[i];

			writeLight(json_extensions, light);
		}

		append(json_extensions, "]}");
	}

	if (data->variants_count > 0)
	{
		comma(json_extensions);
		append(json_extensions, "\"KHR_materials_variants\":{\"variants\":[");

		for (size_t i = 0; i < data->variants_count; ++i)
		{
			const cgltf_material_variant& variant = data->variants[i];

			comma(json_extensions);
			append(json_extensions, "{\"name\":\"");
			append(json_extensions, variant.name);
			append(json_extensions, "\"}");
		}

		append(json_extensions, "]}");
	}

	append(json, "\"asset\":{");
	append(json, "\"version\":\"2.0\",\"generator\":\"gltfpack ");
	append(json, getVersion());
	append(json, "\"");
	writeExtras(json, data->asset.extras);
	append(json, "}");

	const ExtensionInfo extensions[] = {
	    {"KHR_mesh_quantization", settings.quantize, true},
	    {"EXT_meshopt_compression", settings.compress, !settings.fallback},
	    {"KHR_texture_transform", (settings.quantize && !settings.tex_float && !json_textures.empty()) || ext_texture_transform, false},
	    {"KHR_materials_pbrSpecularGlossiness", ext_pbr_specular_glossiness, false},
	    {"KHR_materials_clearcoat", ext_clearcoat, false},
	    {"KHR_materials_transmission", ext_transmission, false},
	    {"KHR_materials_ior", ext_ior, false},
	    {"KHR_materials_specular", ext_specular, false},
	    {"KHR_materials_sheen", ext_sheen, false},
	    {"KHR_materials_volume", ext_volume, false},
	    {"KHR_materials_emissive_strength", ext_emissive_strength, false},
	    {"KHR_materials_iridescence", ext_iridescence, false},
	    {"KHR_materials_anisotropy", ext_anisotropy, false},
	    {"KHR_materials_dispersion", ext_dispersion, false},
	    {"KHR_materials_diffuse_transmission", ext_diffuse_transmission, false},
	    {"KHR_materials_unlit", ext_unlit, false},
	    {"KHR_materials_variants", data->variants_count > 0, false},
	    {"KHR_lights_punctual", data->lights_count > 0, false},
	    {"KHR_texture_basisu", (!json_textures.empty() && settings.texture_ktx2) || ext_texture_basisu, true},
	    {"EXT_texture_webp", ext_texture_webp, true},
	    {"EXT_mesh_gpu_instancing", ext_instancing, true},
	};

	for (size_t i = 0; i < data->extensions_required_count; ++i)
	{
		const char* ext = data->extensions_required[i];

		if (!isExtensionSupported(extensions, sizeof(extensions) / sizeof(extensions[0]), ext))
			fprintf(stderr, "Warning: required extension %s is not supported and will be skipped\n", ext);
	}

	writeExtensions(json, extensions, sizeof(extensions) / sizeof(extensions[0]));

	// buffers[] array to be inserted by the caller
	size_t bufferspec_pos = json.size();

	std::string json_views;
	finalizeBufferViews(json_views, views, bin, settings.fallback ? &fallback : NULL, fallback_size);

	writeArray(json, "bufferViews", json_views);
	writeArray(json, "accessors", json_accessors);
	writeArray(json, "samplers", json_samplers);
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

		for (size_t i = 0; i < data->scenes_count; ++i)
			writeScene(json, data->scenes[i], json_roots[i], settings);

		append(json, "]");
	}

	writeArray(json, "cameras", json_cameras);

	if (data->scene)
	{
		append(json, ",\"scene\":");
		append(json, size_t(data->scene - data->scenes));
	}

	if (!json_extensions.empty())
	{
		append(json, ",\"extensions\":{");
		append(json, json_extensions);
		append(json, "}");
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
		printAttributeStats(views, BufferView::Kind_Instance, "instance");

		printImageStats(views, TextureKind_Generic, "generic");
		printImageStats(views, TextureKind_Color, "color");
		printImageStats(views, TextureKind_Normal, "normal");
		printImageStats(views, TextureKind_Attrib, "attrib");
	}

	if (report_path)
	{
		if (!printReport(report_path, views, meshes, node_offset, mesh_offset, texture_offset, material_offset, animations.size(), json.size(), bin.size()))
		{
			fprintf(stderr, "Warning: cannot save report to %s\n", report_path);
		}
	}

	return bufferspec_pos;
}

static void writeU32(FILE* out, uint32_t data)
{
	fwrite(&data, 4, 1, out);
}

static const char* getBaseName(const char* path)
{
	const char* slash = strrchr(path, '/');
	const char* backslash = strrchr(path, '\\');

	const char* rs = slash ? slash + 1 : path;
	const char* bs = backslash ? backslash + 1 : path;

	return std::max(rs, bs);
}

static std::string getBufferSpec(const char* bin_path, size_t bin_size, const char* fallback_path, size_t fallback_size, bool fallback_ref)
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
		append(json, "\"EXT_meshopt_compression\":{");
		append(json, "\"fallback\":true");
		append(json, "}}");
		append(json, "}");
	}
	append(json, "]");

	return json;
}

int gltfpack(const char* input, const char* output, const char* report, Settings settings)
{
	cgltf_data* data = NULL;
	std::vector<Mesh> meshes;
	std::vector<Animation> animations;

	std::string iext = getExtension(input);
	std::string oext = output ? getExtension(output) : "";

	if (iext == ".gltf" || iext == ".glb")
	{
		const char* error = NULL;
		data = parseGltf(input, meshes, animations, &error);

		if (error)
		{
			fprintf(stderr, "Error loading %s: %s\n", input, error);
			return 2;
		}
	}
	else if (iext == ".obj")
	{
		const char* error = NULL;
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

#ifndef WITH_BASISU
	if (data->images_count && settings.texture_ktx2)
	{
		fprintf(stderr, "Error: gltfpack was built without BasisU support, texture compression is not available\n");
#ifdef __wasi__
		fprintf(stderr, "Note: node.js builds do not support BasisU due to lack of platform features; download a native build from https://github.com/zeux/meshoptimizer/releases\n");
#endif
		return 3;
	}
#endif

	if (oext == ".glb")
	{
		settings.texture_embed = true;
	}

	if (data->images_count && !settings.texture_ref && !settings.texture_embed)
	{
		for (size_t i = 0; i < data->images_count; ++i)
		{
			const char* uri = data->images[i].uri;
			if (!uri || strncmp(uri, "data:", 5) == 0)
				continue;

			for (size_t j = 0; j < i; ++j)
			{
				const char* urj = data->images[j].uri;
				if (!urj || strncmp(urj, "data:", 5) == 0)
					continue;

				if (strcmp(uri, urj) != 0 && strcmp(getBaseName(uri), getBaseName(urj)) == 0)
				{
					fprintf(stderr, "Warning: images %s and %s share the same base name and will overwrite each other\n", uri, urj);
					break;
				}
			}
		}
	}

	std::string json, bin, fallback;
	size_t fallback_size = 0;

	json += '{';
	size_t bufferspec_pos = process(data, input, output, report, meshes, animations, settings, json, bin, fallback, fallback_size);
	json += '}';

	cgltf_free(data);

	if (!output)
	{
		return 0;
	}

	if (oext == ".gltf")
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

		std::string bufferspec = getBufferSpec(getBaseName(binpath.c_str()), bin.size(), settings.fallback ? getBaseName(fbpath.c_str()) : NULL, fallback_size, settings.compress);
		json.insert(bufferspec_pos, "," + bufferspec);

		fwrite(json.c_str(), json.size(), 1, outjson);
		fwrite(bin.c_str(), bin.size(), 1, outbin);

		if (settings.fallback)
			fwrite(fallback.c_str(), fallback.size(), 1, outfb);

		int rc = 0;
		rc |= fclose(outjson);
		rc |= fclose(outbin);
		if (outfb)
			rc |= fclose(outfb);

		if (rc)
		{
			fprintf(stderr, "Error saving %s\n", output);
			return 4;
		}
	}
	else if (oext == ".glb")
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

		std::string bufferspec = getBufferSpec(NULL, bin.size(), settings.fallback ? getBaseName(fbpath.c_str()) : NULL, fallback_size, settings.compress);
		json.insert(bufferspec_pos, "," + bufferspec);

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

		int rc = 0;
		rc |= fclose(out);
		if (outfb)
			rc |= fclose(outfb);

		if (rc)
		{
			fprintf(stderr, "Error saving %s\n", output);
			return 4;
		}
	}
	else
	{
		fprintf(stderr, "Error saving %s: unknown extension (expected .gltf or .glb)\n", output);
		return 4;
	}

	return 0;
}

Settings defaults()
{
	Settings settings = {};
	settings.quantize = true;
	settings.pos_bits = 14;
	settings.tex_bits = 12;
	settings.nrm_bits = 8;
	settings.col_bits = 8;
	settings.trn_bits = 16;
	settings.rot_bits = 12;
	settings.scl_bits = 16;
	settings.anim_freq = 30;
	settings.mesh_dedup = true;
	settings.simplify_ratio = 1.f;
	settings.simplify_error = 1e-2f;

	for (int kind = 0; kind < TextureKind__Count; ++kind)
	{
		settings.texture_mode[kind] = TextureMode_Raw;
		settings.texture_scale[kind] = 1.f;
		settings.texture_quality[kind] = 8;
	}

	return settings;
}

template <typename T>
T clamp(T v, T min, T max)
{
	return v < min ? min : (v > max ? max : v);
}

unsigned int textureMask(const char* arg)
{
	unsigned int result = 0;

	while (arg)
	{
		const char* comma = strchr(arg, ',');
		size_t seg = comma ? comma - arg - 1 : strlen(arg);

		if (strncmp(arg, "color", seg) == 0)
			result |= 1 << TextureKind_Color;
		else if (strncmp(arg, "normal", seg) == 0)
			result |= 1 << TextureKind_Normal;
		else if (strncmp(arg, "attrib", seg) == 0)
			result |= 1 << TextureKind_Attrib;
		else
			fprintf(stderr, "Warning: unrecognized texture class %.*s\n", int(seg), arg);

		arg = comma ? comma + 1 : NULL;
	}

	return result;
}

template <typename T>
void applySetting(T (&data)[TextureKind__Count], T value, unsigned int mask = ~0u)
{
	for (int kind = 0; kind < TextureKind__Count; ++kind)
		if (mask & (1 << kind))
			data[kind] = value;
}

#ifndef GLTFFUZZ
int main(int argc, char** argv)
{
#ifndef __wasi__
	setlocale(LC_ALL, "C"); // disable locale specific convention for number parsing/printing
#endif

	meshopt_encodeVertexVersion(0);
	meshopt_encodeIndexVersion(1);

	Settings settings = defaults();

	const char* input = NULL;
	const char* output = NULL;
	const char* report = NULL;
	bool help = false;
	bool test = false;
	bool require_ktx2 = false;

	std::vector<const char*> testinputs;

	for (int i = 1; i < argc; ++i)
	{
		const char* arg = argv[i];

		if (strcmp(arg, "-vp") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.pos_bits = clamp(atoi(argv[++i]), 1, 16);
		}
		else if (strcmp(arg, "-vt") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.tex_bits = clamp(atoi(argv[++i]), 1, 16);
		}
		else if (strcmp(arg, "-vn") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.nrm_bits = clamp(atoi(argv[++i]), 1, 16);
		}
		else if (strcmp(arg, "-vc") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.col_bits = clamp(atoi(argv[++i]), 1, 16);
		}
		else if (strcmp(arg, "-vpi") == 0)
		{
			settings.pos_float = false;
			settings.pos_normalized = false;
		}
		else if (strcmp(arg, "-vpn") == 0)
		{
			settings.pos_float = false;
			settings.pos_normalized = true;
		}
		else if (strcmp(arg, "-vpf") == 0)
		{
			settings.pos_float = true;
		}
		else if (strcmp(arg, "-vtf") == 0)
		{
			settings.tex_float = true;
		}
		else if (strcmp(arg, "-vnf") == 0)
		{
			settings.nrm_float = true;
		}
		else if (strcmp(arg, "-at") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.trn_bits = clamp(atoi(argv[++i]), 1, 24);
		}
		else if (strcmp(arg, "-ar") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.rot_bits = clamp(atoi(argv[++i]), 4, 16);
		}
		else if (strcmp(arg, "-as") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.scl_bits = clamp(atoi(argv[++i]), 1, 24);
		}
		else if (strcmp(arg, "-af") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.anim_freq = clamp(atoi(argv[++i]), 0, 100);
		}
		else if (strcmp(arg, "-ac") == 0)
		{
			settings.anim_const = true;
		}
		else if (strcmp(arg, "-kn") == 0)
		{
			settings.keep_nodes = true;
		}
		else if (strcmp(arg, "-km") == 0)
		{
			settings.keep_materials = true;
		}
		else if (strcmp(arg, "-ke") == 0)
		{
			settings.keep_extras = true;
		}
		else if (strcmp(arg, "-kv") == 0)
		{
			settings.keep_attributes = true;
		}
		else if (strcmp(arg, "-mdd") == 0)
		{
			fprintf(stderr, "Warning: option -mdd disables mesh deduplication and is only provided as a safety measure; it will be removed in the future\n");
			settings.mesh_dedup = false;
		}
		else if (strcmp(arg, "-mm") == 0)
		{
			settings.mesh_merge = true;
		}
		else if (strcmp(arg, "-mi") == 0)
		{
			settings.mesh_instancing = true;
		}
		else if (strcmp(arg, "-si") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.simplify_ratio = clamp(float(atof(argv[++i])), 0.f, 1.f);
		}
		else if (strcmp(arg, "-se") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.simplify_error = clamp(float(atof(argv[++i])), 0.f, 1.f);
		}
		else if (strcmp(arg, "-sa") == 0)
		{
			settings.simplify_aggressive = true;
		}
		else if (strcmp(arg, "-slb") == 0)
		{
			settings.simplify_lock_borders = true;
		}
		else if (strcmp(arg, "-sv") == 0)
		{
			settings.simplify_attributes = true;
		}
#ifndef NDEBUG
		else if (strcmp(arg, "-sd") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.simplify_debug = clamp(float(atof(argv[++i])), 0.f, 1.f);
		}
		else if (strcmp(arg, "-md") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.meshlet_debug = clamp(atoi(argv[++i]), 3, 255);
		}
#endif
		else if (strcmp(arg, "-tu") == 0)
		{
			settings.texture_ktx2 = true;

			unsigned int mask = ~0u;
			if (i + 1 < argc && isalpha(argv[i + 1][0]))
				mask = textureMask(argv[++i]);

			applySetting(settings.texture_mode, TextureMode_UASTC, mask);
		}
		else if (strcmp(arg, "-tc") == 0)
		{
			settings.texture_ktx2 = true;

			unsigned int mask = ~0u;
			if (i + 1 < argc && isalpha(argv[i + 1][0]))
				mask = textureMask(argv[++i]);

			applySetting(settings.texture_mode, TextureMode_ETC1S, mask);
		}
		else if (strcmp(arg, "-tq") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			require_ktx2 = true;

			int quality = clamp(atoi(argv[++i]), 1, 10);
			applySetting(settings.texture_quality, quality);
		}
		else if (strcmp(arg, "-tq") == 0 && i + 2 < argc && isalpha(argv[i + 1][0]) && isdigit(argv[i + 2][0]))
		{
			require_ktx2 = true;

			unsigned int mask = textureMask(argv[++i]);
			int quality = clamp(atoi(argv[++i]), 1, 10);
			applySetting(settings.texture_quality, quality, mask);
		}
		else if (strcmp(arg, "-ts") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			require_ktx2 = true;

			float scale = clamp(float(atof(argv[++i])), 0.f, 1.f);
			applySetting(settings.texture_scale, scale);
		}
		else if (strcmp(arg, "-ts") == 0 && i + 2 < argc && isalpha(argv[i + 1][0]) && isdigit(argv[i + 2][0]))
		{
			require_ktx2 = true;

			unsigned int mask = textureMask(argv[++i]);
			float scale = clamp(float(atof(argv[++i])), 0.f, 1.f);
			applySetting(settings.texture_scale, scale, mask);
		}
		else if (strcmp(arg, "-tl") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			require_ktx2 = true;

			int limit = atoi(argv[++i]);
			applySetting(settings.texture_limit, limit);
		}
		else if (strcmp(arg, "-tl") == 0 && i + 2 < argc && isalpha(argv[i + 1][0]) && isdigit(argv[i + 2][0]))
		{
			require_ktx2 = true;

			unsigned int mask = textureMask(argv[++i]);
			int limit = atoi(argv[++i]);
			applySetting(settings.texture_limit, limit, mask);
		}
		else if (strcmp(arg, "-tp") == 0)
		{
			require_ktx2 = true;

			settings.texture_pow2 = true;
		}
		else if (strcmp(arg, "-tfy") == 0)
		{
			require_ktx2 = true;

			settings.texture_flipy = true;
		}
		else if (strcmp(arg, "-tr") == 0)
		{
			settings.texture_ref = true;
		}
		else if (strcmp(arg, "-tj") == 0 && i + 1 < argc && isdigit(argv[i + 1][0]))
		{
			settings.texture_jobs = clamp(atoi(argv[++i]), 0, 128);
		}
		else if (strcmp(arg, "-noq") == 0)
		{
			// TODO: Warn if -noq is used and suggest -vpf instead; use -noqq to silence
			settings.quantize = false;
		}
		else if (strcmp(arg, "-i") == 0 && i + 1 < argc && !input)
		{
			input = argv[++i];
		}
		else if (strcmp(arg, "-o") == 0 && i + 1 < argc && !output)
		{
			output = argv[++i];
		}
		else if (strcmp(arg, "-r") == 0 && i + 1 < argc && !report)
		{
			report = argv[++i];
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
		else if (strcmp(arg, "-ce") == 0)
		{
			fprintf(stderr, "Warning: experimental compression will produce files that are not compliant with EXT_meshopt_compression\n");
			meshopt_encodeVertexVersion(1);
			settings.compress = true;
			settings.compressmore = true;
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
			test = true;
		}
		else if (arg[0] == '-')
		{
			fprintf(stderr, "Unrecognized option %s\n", arg);
			return 1;
		}
		else if (test)
		{
			testinputs.push_back(arg);
		}
		else
		{
			fprintf(stderr, "Expected option, got %s instead\n", arg);
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
		for (size_t i = 0; i < testinputs.size(); ++i)
		{
			const char* path = testinputs[i];

			printf("%s\n", path);
			gltfpack(path, NULL, NULL, settings);
		}

		return 0;
	}

	if (!input || !output || help)
	{
		fprintf(stderr, "gltfpack %s\n", getVersion().c_str());
		fprintf(stderr, "Usage: gltfpack [options] -i input -o output\n");

		if (help)
		{
			fprintf(stderr, "\nBasics:\n");
			fprintf(stderr, "\t-i file: input file to process, .obj/.gltf/.glb\n");
			fprintf(stderr, "\t-o file: output file path, .gltf/.glb\n");
			fprintf(stderr, "\t-c: produce compressed gltf/glb files (-cc for higher compression ratio)\n");
			fprintf(stderr, "\nTextures:\n");
			fprintf(stderr, "\t-tc: convert all textures to KTX2 with BasisU supercompression\n");
			fprintf(stderr, "\t-tu: use UASTC when encoding textures (much higher quality and much larger size)\n");
			fprintf(stderr, "\t-tq N: set texture encoding quality (default: 8; N should be between 1 and 10\n");
			fprintf(stderr, "\t-ts R: scale texture dimensions by the ratio R (default: 1; R should be between 0 and 1)\n");
			fprintf(stderr, "\t-tl N: limit texture dimensions to N pixels (default: 0 = no limit)\n");
			fprintf(stderr, "\t-tp: resize textures to nearest power of 2 to conform to WebGL1 restrictions\n");
			fprintf(stderr, "\t-tfy: flip textures along Y axis during BasisU supercompression\n");
			fprintf(stderr, "\t-tj N: use N threads when compressing textures\n");
			fprintf(stderr, "\t-tr: keep referring to original texture paths instead of copying/embedding images\n");
			fprintf(stderr, "\tTexture classes:\n");
			fprintf(stderr, "\t-tc C: use ETC1S when encoding textures of class C\n");
			fprintf(stderr, "\t-tu C: use UASTC when encoding textures of class C\n");
			fprintf(stderr, "\t-tq C N: set texture encoding quality for class C\n");
			fprintf(stderr, "\t-ts C R: scale texture dimensions for class C\n");
			fprintf(stderr, "\t-tl C N: limit texture dimensions for class C\n");
			fprintf(stderr, "\t... where C is a comma-separated list (no spaces) with valid values color,normal,attrib\n");
			fprintf(stderr, "\nSimplification:\n");
			fprintf(stderr, "\t-si R: simplify meshes targeting triangle/point count ratio R (default: 1; R should be between 0 and 1)\n");
			fprintf(stderr, "\t-se E: limit simplification error to E (default: 0.01 = 1%% deviation; E should be between 0 and 1)\n");
			fprintf(stderr, "\t-sa: aggressively simplify to the target ratio disregarding quality\n");
			fprintf(stderr, "\t-sv: take vertex attributes into account when simplifying meshes\n");
			fprintf(stderr, "\t-slb: lock border vertices during simplification to avoid gaps on connected meshes\n");
			fprintf(stderr, "\nVertex precision:\n");
			fprintf(stderr, "\t-vp N: use N-bit quantization for positions (default: 14; N should be between 1 and 16)\n");
			fprintf(stderr, "\t-vt N: use N-bit quantization for texture coordinates (default: 12; N should be between 1 and 16)\n");
			fprintf(stderr, "\t-vn N: use N-bit quantization for normals (default: 8; N should be between 1 and 16) and tangents (up to 8-bit)\n");
			fprintf(stderr, "\t-vc N: use N-bit quantization for colors (default: 8; N should be between 1 and 16)\n");
			fprintf(stderr, "\nVertex positions:\n");
			fprintf(stderr, "\t-vpi: use integer attributes for positions (default)\n");
			fprintf(stderr, "\t-vpn: use normalized attributes for positions\n");
			fprintf(stderr, "\t-vpf: use floating point attributes for positions\n");
			fprintf(stderr, "\nVertex attributes:\n");
			fprintf(stderr, "\t-vtf: use floating point attributes for texture coordinates\n");
			fprintf(stderr, "\t-vnf: use floating point attributes for normals\n");
			fprintf(stderr, "\t-kv: keep source vertex attributes even if they aren't used\n");
			fprintf(stderr, "\nAnimations:\n");
			fprintf(stderr, "\t-at N: use N-bit quantization for translations (default: 16; N should be between 1 and 24)\n");
			fprintf(stderr, "\t-ar N: use N-bit quantization for rotations (default: 12; N should be between 4 and 16)\n");
			fprintf(stderr, "\t-as N: use N-bit quantization for scale (default: 16; N should be between 1 and 24)\n");
			fprintf(stderr, "\t-af N: resample animations at N Hz (default: 30; use 0 to disable)\n");
			fprintf(stderr, "\t-ac: keep constant animation tracks even if they don't modify the node transform\n");
			fprintf(stderr, "\nScene:\n");
			fprintf(stderr, "\t-kn: keep named nodes and meshes attached to named nodes so that named nodes can be transformed externally\n");
			fprintf(stderr, "\t-km: keep named materials and disable named material merging\n");
			fprintf(stderr, "\t-ke: keep extras data\n");
			fprintf(stderr, "\t-mm: merge instances of the same mesh together when possible\n");
			fprintf(stderr, "\t-mi: use EXT_mesh_gpu_instancing when serializing multiple mesh instances\n");
			fprintf(stderr, "\nMiscellaneous:\n");
			fprintf(stderr, "\t-cf: produce compressed gltf/glb files with fallback for loaders that don't support compression\n");
			fprintf(stderr, "\t-noq: disable quantization; produces much larger glTF files with no extensions\n");
			fprintf(stderr, "\t-v: verbose output (print version when used without other options)\n");
			fprintf(stderr, "\t-r file: output a JSON report to file\n");
			fprintf(stderr, "\t-h: display this help and exit\n");
		}
		else
		{
			fprintf(stderr, "\nBasics:\n");
			fprintf(stderr, "\t-i file: input file to process, .obj/.gltf/.glb\n");
			fprintf(stderr, "\t-o file: output file path, .gltf/.glb\n");
			fprintf(stderr, "\t-c: produce compressed gltf/glb files (-cc for higher compression ratio)\n");
			fprintf(stderr, "\t-tc: convert all textures to KTX2 with BasisU supercompression\n");
			fprintf(stderr, "\t-si R: simplify meshes targeting triangle/point count ratio R (between 0 and 1)\n");
			fprintf(stderr, "\nRun gltfpack -h to display a full list of options\n");
		}

		return 1;
	}

	if (require_ktx2 && !settings.texture_ktx2)
	{
		fprintf(stderr, "Texture processing is only supported when texture compression is enabled via -tc/-tu\n");
		return 1;
	}

	if (settings.texture_ref && settings.texture_ktx2)
	{
		fprintf(stderr, "Option -tr currently can not be used together with -tc\n");
		return 1;
	}

	if (settings.fallback && settings.compressmore)
	{
		fprintf(stderr, "Option -cf can not be used together with -cc\n");
		return 1;
	}

	if (settings.fallback && (settings.pos_float || settings.tex_float || settings.nrm_float))
	{
		fprintf(stderr, "Option -cf can not be used together with -vpf, -vtf or -vnf\n");
		return 1;
	}

	if (settings.keep_nodes && (settings.mesh_merge || settings.mesh_instancing))
	{
		fprintf(stderr, "Warning: option -kn disables mesh merge (-mm) and mesh instancing (-mi) optimizations\n");
	}

	return gltfpack(input, output, report, settings);
}
#endif

#ifdef __wasi__
extern "C" int pack(int argc, char** argv)
{
	chdir("/gltfpack-$pwd");

	int result = main(argc, argv);
	fflush(NULL);
	return result;
}
#endif

#ifdef GLTFFUZZ
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* buffer, size_t size)
{
	Settings settings = defaults();

	settings.texture_embed = true;

	std::vector<Mesh> meshes;
	std::vector<Animation> animations;

	const char* error = NULL;
	cgltf_data* data = parseGlb(buffer, size, meshes, animations, &error);

	// this is a difficult tradeoff
	// returning 0 on files that fail to parse means that fuzzing is more incremental: files with errors are put into the corpus,
	// and the subsequent mutations may lead to discovering more interesting inputs, including valid ones that are difficult to find otherwise.
	// however, this leads to most of the corpus being invalid, and we very rarely get useful coverage for actual gltfpack processing.
	// for now we just focus on valid files, as we expect cgltf parser itself to be bulletproof as it's fuzzed separately.
	if (error)
		return -1;

	std::string json, bin, fallback;
	size_t fallback_size = 0;
	process(data, NULL, NULL, NULL, meshes, animations, settings, json, bin, fallback, fallback_size);

	cgltf_free(data);

	return 0;
}
#endif
