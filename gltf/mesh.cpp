// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <algorithm>
#include <unordered_map>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "../src/meshoptimizer.h"

static float inverseTranspose(float* result, const float* transform)
{
	float m[4][4] = {};
	memcpy(m, transform, 16 * sizeof(float));

	float det =
	    m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
	    m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
	    m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

	float invdet = (det == 0.f) ? 0.f : 1.f / det;

	float r[4][4] = {};

	r[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invdet;
	r[1][0] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invdet;
	r[2][0] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invdet;
	r[0][1] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invdet;
	r[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invdet;
	r[2][1] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * invdet;
	r[0][2] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invdet;
	r[1][2] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * invdet;
	r[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invdet;

	r[3][3] = 1.f;

	memcpy(result, r, 16 * sizeof(float));

	return det;
}

static void transformPosition(float* res, const float* ptr, const float* transform)
{
	float x = ptr[0] * transform[0] + ptr[1] * transform[4] + ptr[2] * transform[8] + transform[12];
	float y = ptr[0] * transform[1] + ptr[1] * transform[5] + ptr[2] * transform[9] + transform[13];
	float z = ptr[0] * transform[2] + ptr[1] * transform[6] + ptr[2] * transform[10] + transform[14];

	res[0] = x;
	res[1] = y;
	res[2] = z;
}

static void transformNormal(float* res, const float* ptr, const float* transform)
{
	float x = ptr[0] * transform[0] + ptr[1] * transform[4] + ptr[2] * transform[8];
	float y = ptr[0] * transform[1] + ptr[1] * transform[5] + ptr[2] * transform[9];
	float z = ptr[0] * transform[2] + ptr[1] * transform[6] + ptr[2] * transform[10];

	float l = sqrtf(x * x + y * y + z * z);
	float s = (l == 0.f) ? 0.f : 1 / l;

	res[0] = x * s;
	res[1] = y * s;
	res[2] = z * s;
}

// assumes mesh & target are structurally identical
static void transformMesh(Mesh& target, const Mesh& mesh, const cgltf_node* node)
{
	assert(target.streams.size() == mesh.streams.size());
	assert(target.indices.size() == mesh.indices.size());

	float transform[16];
	cgltf_node_transform_world(node, transform);

	float transforminvt[16];
	float det = inverseTranspose(transforminvt, transform);

	for (size_t si = 0; si < mesh.streams.size(); ++si)
	{
		const Stream& source = mesh.streams[si];
		Stream& stream = target.streams[si];

		assert(source.type == stream.type);
		assert(source.data.size() == stream.data.size());

		if (stream.type == cgltf_attribute_type_position)
		{
			for (size_t i = 0; i < stream.data.size(); ++i)
				transformPosition(stream.data[i].f, source.data[i].f, transform);
		}
		else if (stream.type == cgltf_attribute_type_normal)
		{
			for (size_t i = 0; i < stream.data.size(); ++i)
				transformNormal(stream.data[i].f, source.data[i].f, transforminvt);
		}
		else if (stream.type == cgltf_attribute_type_tangent)
		{
			for (size_t i = 0; i < stream.data.size(); ++i)
				transformNormal(stream.data[i].f, source.data[i].f, transform);
		}
	}

	// copy indices so that we can modify them below
	target.indices = mesh.indices;

	if (det < 0 && mesh.type == cgltf_primitive_type_triangles)
	{
		// negative scale means we need to flip face winding
		for (size_t i = 0; i < target.indices.size(); i += 3)
			std::swap(target.indices[i + 0], target.indices[i + 1]);
	}
}

bool compareMeshTargets(const Mesh& lhs, const Mesh& rhs)
{
	if (lhs.targets != rhs.targets)
		return false;

	if (lhs.target_weights.size() != rhs.target_weights.size())
		return false;

	for (size_t i = 0; i < lhs.target_weights.size(); ++i)
		if (lhs.target_weights[i] != rhs.target_weights[i])
			return false;

	if (lhs.target_names.size() != rhs.target_names.size())
		return false;

	for (size_t i = 0; i < lhs.target_names.size(); ++i)
		if (strcmp(lhs.target_names[i], rhs.target_names[i]) != 0)
			return false;

	return true;
}

bool compareMeshVariants(const Mesh& lhs, const Mesh& rhs)
{
	if (lhs.variants.size() != rhs.variants.size())
		return false;

	for (size_t i = 0; i < lhs.variants.size(); ++i)
	{
		if (lhs.variants[i].variant != rhs.variants[i].variant)
			return false;

		if (lhs.variants[i].material != rhs.variants[i].material)
			return false;
	}

	return true;
}

bool compareMeshNodes(const Mesh& lhs, const Mesh& rhs)
{
	if (lhs.nodes.size() != rhs.nodes.size())
		return false;

	for (size_t i = 0; i < lhs.nodes.size(); ++i)
		if (lhs.nodes[i] != rhs.nodes[i])
			return false;

	return true;
}

static bool compareTransforms(const Transform& lhs, const Transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(Transform)) == 0;
}

static bool canMergeMeshNodes(cgltf_node* lhs, cgltf_node* rhs, const Settings& settings)
{
	if (lhs == rhs)
		return true;

	if (lhs->parent != rhs->parent)
		return false;

	bool lhs_transform = lhs->has_translation | lhs->has_rotation | lhs->has_scale | lhs->has_matrix | (!!lhs->weights);
	bool rhs_transform = rhs->has_translation | rhs->has_rotation | rhs->has_scale | rhs->has_matrix | (!!rhs->weights);

	if (lhs_transform || rhs_transform)
		return false;

	if (settings.keep_nodes)
	{
		if (lhs->name && *lhs->name)
			return false;

		if (rhs->name && *rhs->name)
			return false;
	}

	// we can merge nodes that don't have transforms of their own and have the same parent
	// this is helpful when instead of splitting mesh into primitives, DCCs split mesh into mesh nodes
	return true;
}

static bool canMergeMeshes(const Mesh& lhs, const Mesh& rhs, const Settings& settings)
{
	if (lhs.scene != rhs.scene)
		return false;

	if (lhs.nodes.size() != rhs.nodes.size())
		return false;

	for (size_t i = 0; i < lhs.nodes.size(); ++i)
		if (!canMergeMeshNodes(lhs.nodes[i], rhs.nodes[i], settings))
			return false;

	if (lhs.instances.size() != rhs.instances.size())
		return false;

	for (size_t i = 0; i < lhs.instances.size(); ++i)
		if (!compareTransforms(lhs.instances[i], rhs.instances[i]))
			return false;

	if (lhs.material != rhs.material)
		return false;

	if (lhs.skin != rhs.skin)
		return false;

	if (lhs.type != rhs.type)
		return false;

	if (!compareMeshTargets(lhs, rhs))
		return false;

	if (!compareMeshVariants(lhs, rhs))
		return false;

	if (lhs.indices.empty() != rhs.indices.empty())
		return false;

	if (lhs.streams.size() != rhs.streams.size())
		return false;

	if (settings.keep_extras && !areExtrasEqual(lhs.extras, rhs.extras))
		return false;

	for (size_t i = 0; i < lhs.streams.size(); ++i)
		if (lhs.streams[i].type != rhs.streams[i].type || lhs.streams[i].index != rhs.streams[i].index || lhs.streams[i].target != rhs.streams[i].target)
			return false;

	return true;
}

static void mergeMeshes(Mesh& target, const Mesh& mesh)
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

static void hashUpdate(uint64_t hash[2], const void* data, size_t size)
{
#define ROTL64(x, r) (((x) << (r)) | ((x) >> (64 - (r))))

	// MurMurHash3 128-bit
	const uint64_t c1 = 0x87c37b91114253d5ull;
	const uint64_t c2 = 0x4cf5ad432745937full;

	uint64_t h1 = hash[0], h2 = hash[1];

	size_t offset = 0;

	// body
	for (; offset + 16 <= size; offset += 16)
	{
		uint64_t k1, k2;
		memcpy(&k1, static_cast<const char*>(data) + offset + 0, 8);
		memcpy(&k2, static_cast<const char*>(data) + offset + 8, 8);

		k1 *= c1, k1 = ROTL64(k1, 31), k1 *= c2;
		h1 ^= k1, h1 = ROTL64(h1, 27), h1 += h2;
		h1 = h1 * 5 + 0x52dce729;
		k2 *= c2, k2 = ROTL64(k2, 33), k2 *= c1;
		h2 ^= k2, h2 = ROTL64(h2, 31), h2 += h1;
		h2 = h2 * 5 + 0x38495ab5;
	}

	// tail
	if (offset < size)
	{
		uint64_t tail[2] = {};
		memcpy(tail, static_cast<const char*>(data) + offset, size - offset);

		uint64_t k1 = tail[0], k2 = tail[1];

		k1 *= c1, k1 = ROTL64(k1, 31), k1 *= c2;
		h1 ^= k1;
		k2 *= c2, k2 = ROTL64(k2, 33), k2 *= c1;
		h2 ^= k2;
	}

	h1 ^= size;
	h2 ^= size;

	hash[0] = h1;
	hash[1] = h2;

#undef ROTL64
}

void hashMesh(Mesh& mesh)
{
	mesh.geometry_hash[0] = mesh.geometry_hash[1] = 41;

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		const Stream& stream = mesh.streams[i];

		int meta[3] = {stream.type, stream.index, stream.target};
		hashUpdate(mesh.geometry_hash, meta, sizeof(meta));

		if (stream.custom_name)
			hashUpdate(mesh.geometry_hash, stream.custom_name, strlen(stream.custom_name));

		hashUpdate(mesh.geometry_hash, stream.data.data(), stream.data.size() * sizeof(Attr));
	}

	if (!mesh.indices.empty())
		hashUpdate(mesh.geometry_hash, mesh.indices.data(), mesh.indices.size() * sizeof(unsigned int));

	int meta[4] = {int(mesh.streams.size()), mesh.streams.empty() ? 0 : int(mesh.streams[0].data.size()), int(mesh.indices.size()), mesh.type};
	hashUpdate(mesh.geometry_hash, meta, sizeof(meta));
}

static bool canDedupMesh(const Mesh& mesh, const Settings& settings)
{
	// empty mesh
	if (mesh.streams.empty())
		return false;

	// world-space mesh
	if (mesh.nodes.empty() && mesh.instances.empty())
		return false;

	// has extras
	if (settings.keep_extras && mesh.extras.data)
		return false;

	// to simplify dedup we ignore complex target setups for now
	if (!mesh.target_weights.empty() || !mesh.target_names.empty() || !mesh.variants.empty())
		return false;

	return true;
}

void dedupMeshes(std::vector<Mesh>& meshes, const Settings& settings)
{
	std::unordered_map<uint64_t, int> hashes;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		hashMesh(mesh);
		hashes[mesh.geometry_hash[0] ^ mesh.geometry_hash[1]]++;
	}

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& target = meshes[i];

		if (!canDedupMesh(target, settings))
			continue;

		if (hashes[target.geometry_hash[0] ^ target.geometry_hash[1]] <= 1)
			continue;

		for (size_t j = i + 1; j < meshes.size(); ++j)
		{
			Mesh& mesh = meshes[j];

			if (mesh.geometry_hash[0] != target.geometry_hash[0] || mesh.geometry_hash[1] != target.geometry_hash[1])
				continue;

			if (!canDedupMesh(mesh, settings))
				continue;

			if (mesh.scene != target.scene || mesh.material != target.material || mesh.skin != target.skin)
			{
				// mark both meshes as having duplicate geometry; we don't use this in dedupMeshes but it's useful later in the pipeline
				target.geometry_duplicate = true;
				mesh.geometry_duplicate = true;
				continue;
			}

			// basic sanity test; these should be included in geometry hash
			assert(mesh.streams.size() == target.streams.size());
			assert(mesh.streams[0].data.size() == target.streams[0].data.size());
			assert(mesh.indices.size() == target.indices.size());

			target.nodes.insert(target.nodes.end(), mesh.nodes.begin(), mesh.nodes.end());
			target.instances.insert(target.instances.end(), mesh.instances.begin(), mesh.instances.end());

			mesh.streams.clear();
			mesh.indices.clear();
			mesh.nodes.clear();
			mesh.instances.clear();
		}
	}

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& target = meshes[i];
		if (target.nodes.size() <= 1)
			continue;

		std::sort(target.nodes.begin(), target.nodes.end());
		target.nodes.erase(std::unique(target.nodes.begin(), target.nodes.end()), target.nodes.end());
	}
}

void mergeMeshInstances(Mesh& mesh)
{
	if (mesh.nodes.empty())
		return;

	// fast-path: for single instance meshes we transform in-place
	if (mesh.nodes.size() == 1)
	{
		transformMesh(mesh, mesh, mesh.nodes[0]);
		mesh.nodes.clear();
		return;
	}

	Mesh base = mesh;
	Mesh transformed = base;

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		mesh.streams[i].data.clear();
		mesh.streams[i].data.reserve(base.streams[i].data.size() * mesh.nodes.size());
	}

	mesh.indices.clear();
	mesh.indices.reserve(base.indices.size() * mesh.nodes.size());

	for (size_t i = 0; i < mesh.nodes.size(); ++i)
	{
		transformMesh(transformed, base, mesh.nodes[i]);
		mergeMeshes(mesh, transformed);
	}

	mesh.nodes.clear();
}

void mergeMeshes(std::vector<Mesh>& meshes, const Settings& settings)
{
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& target = meshes[i];

		if (target.streams.empty())
			continue;

		size_t target_vertices = target.streams[0].data.size();
		size_t target_indices = target.indices.size();

		size_t last_merged = i;

		for (size_t j = i + 1; j < meshes.size(); ++j)
		{
			Mesh& mesh = meshes[j];

			if (!mesh.streams.empty() && canMergeMeshes(target, mesh, settings))
			{
				target_vertices += mesh.streams[0].data.size();
				target_indices += mesh.indices.size();
				last_merged = j;
			}
		}

		for (size_t j = 0; j < target.streams.size(); ++j)
			target.streams[j].data.reserve(target_vertices);

		target.indices.reserve(target_indices);

		for (size_t j = i + 1; j <= last_merged; ++j)
		{
			Mesh& mesh = meshes[j];

			if (!mesh.streams.empty() && canMergeMeshes(target, mesh, settings))
			{
				mergeMeshes(target, mesh);

				mesh.streams.clear();
				mesh.indices.clear();
				mesh.nodes.clear();
				mesh.instances.clear();
			}
		}

		assert(target.streams[0].data.size() == target_vertices);
		assert(target.indices.size() == target_indices);
	}
}

void filterEmptyMeshes(std::vector<Mesh>& meshes)
{
	size_t write = 0;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh& mesh = meshes[i];

		if (mesh.streams.empty())
			continue;

		if (mesh.streams[0].data.empty())
			continue;

		if (mesh.type != cgltf_primitive_type_points && mesh.indices.empty())
			continue;

		// the following code is roughly equivalent to meshes[write] = std::move(mesh)
		std::vector<Stream> streams;
		streams.swap(mesh.streams);

		std::vector<unsigned int> indices;
		indices.swap(mesh.indices);

		meshes[write] = mesh;
		meshes[write].streams.swap(streams);
		meshes[write].indices.swap(indices);

		write++;
	}

	meshes.resize(write);
}

static bool isConstant(const std::vector<Attr>& data, const Attr& value, float tolerance = 0.01f)
{
	for (size_t i = 0; i < data.size(); ++i)
	{
		const Attr& a = data[i];

		if (fabsf(a.f[0] - value.f[0]) > tolerance || fabsf(a.f[1] - value.f[1]) > tolerance || fabsf(a.f[2] - value.f[2]) > tolerance || fabsf(a.f[3] - value.f[3]) > tolerance)
			return false;
	}

	return true;
}

void filterStreams(Mesh& mesh, const MaterialInfo& mi)
{
	bool morph_normal = false;
	bool morph_tangent = false;
	int keep_texture_set = -1;

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		Stream& stream = mesh.streams[i];

		if (stream.target)
		{
			morph_normal = morph_normal || (stream.type == cgltf_attribute_type_normal && !isConstant(stream.data, {0, 0, 0, 0}));
			morph_tangent = morph_tangent || (stream.type == cgltf_attribute_type_tangent && !isConstant(stream.data, {0, 0, 0, 0}));
		}

		if (stream.type == cgltf_attribute_type_texcoord && stream.index < 32 && (mi.texture_set_mask & (1u << stream.index)) != 0)
		{
			keep_texture_set = std::max(keep_texture_set, stream.index);
		}
	}

	size_t write = 0;

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		Stream& stream = mesh.streams[i];

		if (stream.type == cgltf_attribute_type_texcoord && stream.index > keep_texture_set)
			continue;

		if (stream.type == cgltf_attribute_type_normal && mi.unlit)
			continue;

		if (stream.type == cgltf_attribute_type_tangent && !mi.needs_tangents)
			continue;

		if ((stream.type == cgltf_attribute_type_joints || stream.type == cgltf_attribute_type_weights) && !mesh.skin)
			continue;

		if (stream.type == cgltf_attribute_type_color && isConstant(stream.data, {1, 1, 1, 1}))
			continue;

		if (stream.target && stream.type == cgltf_attribute_type_normal && !morph_normal)
			continue;

		if (stream.target && stream.type == cgltf_attribute_type_tangent && !morph_tangent)
			continue;

		if (mesh.type == cgltf_primitive_type_points && stream.type == cgltf_attribute_type_normal && !stream.data.empty() && isConstant(stream.data, stream.data[0]))
			continue;

		// the following code is roughly equivalent to streams[write] = std::move(stream)
		std::vector<Attr> data;
		data.swap(stream.data);

		mesh.streams[write] = stream;
		mesh.streams[write].data.swap(data);

		write++;
	}

	mesh.streams.resize(write);
}

struct QuantizedTBN
{
	int8_t nx, ny, nz, nw;
	int8_t tx, ty, tz, tw;
};

static void quantizeTBN(QuantizedTBN* target, size_t offset, const Attr* source, size_t size, int bits)
{
	int8_t* target8 = reinterpret_cast<int8_t*>(target) + offset;

	for (size_t i = 0; i < size; ++i)
	{
		target8[i * sizeof(QuantizedTBN) + 0] = int8_t(meshopt_quantizeSnorm(source[i].f[0], bits));
		target8[i * sizeof(QuantizedTBN) + 1] = int8_t(meshopt_quantizeSnorm(source[i].f[1], bits));
		target8[i * sizeof(QuantizedTBN) + 2] = int8_t(meshopt_quantizeSnorm(source[i].f[2], bits));
		target8[i * sizeof(QuantizedTBN) + 3] = int8_t(meshopt_quantizeSnorm(source[i].f[3], bits));
	}
}

static void reindexMesh(Mesh& mesh, bool quantize_tbn)
{
	size_t total_vertices = mesh.streams[0].data.size();
	size_t total_indices = mesh.indices.size();

	std::vector<QuantizedTBN> qtbn;

	std::vector<meshopt_Stream> streams;
	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		const Stream& attr = mesh.streams[i];
		if (attr.target)
			continue;

		assert(attr.data.size() == total_vertices);

		if (quantize_tbn && (attr.type == cgltf_attribute_type_normal || attr.type == cgltf_attribute_type_tangent))
		{
			if (qtbn.empty())
			{
				qtbn.resize(total_vertices);

				meshopt_Stream stream = {&qtbn[0], sizeof(QuantizedTBN), sizeof(QuantizedTBN)};
				streams.push_back(stream);
			}

			size_t offset = attr.type == cgltf_attribute_type_normal ? offsetof(QuantizedTBN, nx) : offsetof(QuantizedTBN, tx);
			quantizeTBN(&qtbn[0], offset, &attr.data[0], total_vertices, /* bits= */ 8);
		}
		else
		{
			meshopt_Stream stream = {&attr.data[0], sizeof(Attr), sizeof(Attr)};
			streams.push_back(stream);
		}
	}

	if (streams.empty())
		return;

	std::vector<unsigned int> remap(total_vertices);
	size_t unique_vertices = meshopt_generateVertexRemapMulti(&remap[0], &mesh.indices[0], total_indices, total_vertices, &streams[0], streams.size());
	assert(unique_vertices <= total_vertices);

	meshopt_remapIndexBuffer(&mesh.indices[0], &mesh.indices[0], total_indices, &remap[0]);

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		assert(mesh.streams[i].data.size() == total_vertices);

		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], total_vertices, sizeof(Attr), &remap[0]);
		mesh.streams[i].data.resize(unique_vertices);
	}
}

static void filterTriangles(Mesh& mesh)
{
	assert(mesh.type == cgltf_primitive_type_triangles);

	unsigned int* indices = &mesh.indices[0];
	size_t total_indices = mesh.indices.size();

	size_t write = 0;

	for (size_t i = 0; i < total_indices; i += 3)
	{
		unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];

		if (a != b && a != c && b != c)
		{
			indices[write + 0] = a;
			indices[write + 1] = b;
			indices[write + 2] = c;
			write += 3;
		}
	}

	mesh.indices.resize(write);
}

static Stream* getStream(Mesh& mesh, cgltf_attribute_type type, int index = 0)
{
	for (size_t i = 0; i < mesh.streams.size(); ++i)
		if (mesh.streams[i].type == type && mesh.streams[i].index == index)
			return &mesh.streams[i];

	return NULL;
}

static void simplifyAttributes(std::vector<float>& attrs, float* attrw, size_t stride, Mesh& mesh)
{
	assert(stride >= 6); // normal + color

	size_t vertex_count = mesh.streams[0].data.size();

	attrs.resize(vertex_count * stride);
	float* data = attrs.data();

	if (const Stream* attr = getStream(mesh, cgltf_attribute_type_normal))
	{
		const Attr* a = attr->data.data();

		for (size_t i = 0; i < vertex_count; ++i)
		{
			data[i * stride + 0] = a[i].f[0];
			data[i * stride + 1] = a[i].f[1];
			data[i * stride + 2] = a[i].f[2];
		}

		attrw[0] = attrw[1] = attrw[2] = 0.5f;
	}

	if (const Stream* attr = getStream(mesh, cgltf_attribute_type_color))
	{
		const Attr* a = attr->data.data();

		for (size_t i = 0; i < vertex_count; ++i)
		{
			data[i * stride + 3] = a[i].f[0] * a[i].f[3];
			data[i * stride + 4] = a[i].f[1] * a[i].f[3];
			data[i * stride + 5] = a[i].f[2] * a[i].f[3];
		}

		attrw[3] = attrw[4] = attrw[5] = 1.0f;
	}
}

static void simplifyUvSplit(Mesh& mesh, std::vector<unsigned int>& remap)
{
	assert(mesh.type == cgltf_primitive_type_triangles);
	assert(!mesh.indices.empty());

	const Stream* uv = getStream(mesh, cgltf_attribute_type_texcoord);
	if (!uv)
		return;

	size_t vertex_count = uv->data.size();

	std::vector<unsigned char> uvsign(mesh.indices.size() / 3);
	std::vector<unsigned char> flipseam(vertex_count);

	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		unsigned int a = mesh.indices[i + 0];
		unsigned int b = mesh.indices[i + 1];
		unsigned int c = mesh.indices[i + 2];

		const Attr& va = uv->data[a];
		const Attr& vb = uv->data[b];
		const Attr& vc = uv->data[c];

		float uvarea = (vb.f[0] - va.f[0]) * (vc.f[1] - va.f[1]) - (vc.f[0] - va.f[0]) * (vb.f[1] - va.f[1]);
		unsigned char flag = uvarea > 0 ? 1 : (uvarea < 0 ? 2 : 0);

		uvsign[i / 3] = flag;
		flipseam[a] |= flag;
		flipseam[b] |= flag;
		flipseam[c] |= flag;
	}

	std::vector<unsigned int> split(vertex_count);
	size_t splits = 0;

	remap.resize(vertex_count);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		remap[i] = unsigned(i);

		if (flipseam[i] == 3)
		{
			assert(remap.size() == vertex_count + splits);
			remap.push_back(unsigned(i));

			split[i] = unsigned(vertex_count + splits);
			splits++;

			for (size_t k = 0; k < mesh.streams.size(); ++k)
				mesh.streams[k].data.push_back(mesh.streams[k].data[i]);
		}
	}

	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		unsigned int a = mesh.indices[i + 0];
		unsigned int b = mesh.indices[i + 1];
		unsigned int c = mesh.indices[i + 2];

		unsigned char sign = uvsign[i / 3];

		if (flipseam[a] == 3 && sign == 2)
			mesh.indices[i + 0] = split[a];
		if (flipseam[b] == 3 && sign == 2)
			mesh.indices[i + 1] = split[b];
		if (flipseam[c] == 3 && sign == 2)
			mesh.indices[i + 2] = split[c];
	}
}

static void simplifyMesh(Mesh& mesh, float threshold, float error, bool attributes, bool aggressive, bool lock_borders, bool debug = false)
{
	enum
	{
		meshopt_SimplifyInternalDebug = 1 << 30
	};

	assert(mesh.type == cgltf_primitive_type_triangles);

	if (mesh.indices.empty())
		return;

	const Stream* positions = getStream(mesh, cgltf_attribute_type_position);
	if (!positions)
		return;

	std::vector<unsigned int> uvremap;
	if (attributes)
		simplifyUvSplit(mesh, uvremap);

	size_t vertex_count = mesh.streams[0].data.size();

	size_t target_index_count = size_t(double(size_t(mesh.indices.size() / 3)) * threshold) * 3;
	float target_error = error;
	float target_error_aggressive = 1e-1f;

	unsigned int options = 0;
	if (lock_borders)
		options |= meshopt_SimplifyLockBorder;
	else
		options |= meshopt_SimplifyPrune;

	if (debug)
		options |= meshopt_SimplifyInternalDebug;

	std::vector<unsigned int> indices(mesh.indices.size());

	if (attributes)
	{
		float attrw[6] = {};
		std::vector<float> attrs;
		simplifyAttributes(attrs, attrw, sizeof(attrw) / sizeof(attrw[0]), mesh);

		indices.resize(meshopt_simplifyWithAttributes(&indices[0], &mesh.indices[0], mesh.indices.size(), positions->data[0].f, vertex_count, sizeof(Attr), attrs.data(), sizeof(attrw), attrw, sizeof(attrw) / sizeof(attrw[0]), NULL, target_index_count, target_error, options));
	}
	else
	{
		indices.resize(meshopt_simplify(&indices[0], &mesh.indices[0], mesh.indices.size(), positions->data[0].f, vertex_count, sizeof(Attr), target_index_count, target_error, options));
	}

	mesh.indices.swap(indices);

	// Note: if the simplifier got stuck, we can try to reindex without normals/tangents and retry
	// For now we simply fall back to aggressive simplifier instead

	// if the precise simplifier got "stuck", we'll try to simplify using the sloppy simplifier; this is only used when aggressive simplification is enabled as it breaks attribute discontinuities
	if (aggressive && mesh.indices.size() > target_index_count)
	{
		indices.resize(meshopt_simplifySloppy(&indices[0], &mesh.indices[0], mesh.indices.size(), positions->data[0].f, vertex_count, sizeof(Attr), target_index_count, target_error_aggressive));
		mesh.indices.swap(indices);
	}

	if (uvremap.size() && mesh.indices.size() && !debug)
		meshopt_remapIndexBuffer(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &uvremap[0]);
}

static void optimizeMesh(Mesh& mesh, bool compressmore)
{
	assert(mesh.type == cgltf_primitive_type_triangles);

	if (mesh.indices.empty())
		return;

	size_t vertex_count = mesh.streams[0].data.size();

	if (compressmore)
		meshopt_optimizeVertexCacheStrip(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), vertex_count);
	else
		meshopt_optimizeVertexCache(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), vertex_count);

	std::vector<unsigned int> remap(vertex_count);
	size_t unique_vertices = meshopt_optimizeVertexFetchRemap(&remap[0], &mesh.indices[0], mesh.indices.size(), vertex_count);
	assert(unique_vertices <= vertex_count);

	meshopt_remapIndexBuffer(&mesh.indices[0], &mesh.indices[0], mesh.indices.size(), &remap[0]);

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		assert(mesh.streams[i].data.size() == vertex_count);

		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], vertex_count, sizeof(Attr), &remap[0]);
		mesh.streams[i].data.resize(unique_vertices);
	}
}

struct BoneInfluence
{
	float i;
	float w;
};

struct BoneInfluenceWeightPredicate
{
	bool operator()(const BoneInfluence& lhs, const BoneInfluence& rhs) const
	{
		return lhs.w > rhs.w;
	}
};

static void filterBones(Mesh& mesh)
{
	const int kMaxGroups = 8;

	std::pair<Stream*, Stream*> groups[kMaxGroups];
	int group_count = 0;

	// gather all joint/weight groups; each group contains 4 bone influences
	for (int i = 0; i < kMaxGroups; ++i)
	{
		Stream* jg = getStream(mesh, cgltf_attribute_type_joints, int(i));
		Stream* wg = getStream(mesh, cgltf_attribute_type_weights, int(i));

		if (!jg || !wg)
			break;

		groups[group_count++] = std::make_pair(jg, wg);
	}

	if (group_count == 0)
		return;

	// weights below cutoff can't be represented in quantized 8-bit storage
	const float weight_cutoff = 0.5f / 255.f;

	size_t vertex_count = mesh.streams[0].data.size();

	BoneInfluence inf[kMaxGroups * 4] = {};

	for (size_t i = 0; i < vertex_count; ++i)
	{
		int count = 0;

		// gather all bone influences for this vertex
		for (int j = 0; j < group_count; ++j)
		{
			const Attr& ja = groups[j].first->data[i];
			const Attr& wa = groups[j].second->data[i];

			for (int k = 0; k < 4; ++k)
				if (wa.f[k] > weight_cutoff)
				{
					inf[count].i = ja.f[k];
					inf[count].w = wa.f[k];
					count++;
				}
		}

		// pick top 4 influences; this also sorts resulting influences by weight which helps renderers that use influence subset in shader LODs
		std::sort(inf, inf + count, BoneInfluenceWeightPredicate());

		// copy the top 4 influences back into stream 0 - we will remove other streams at the end
		Attr& ja = groups[0].first->data[i];
		Attr& wa = groups[0].second->data[i];

		for (int k = 0; k < 4; ++k)
		{
			if (k < count)
			{
				ja.f[k] = inf[k].i;
				wa.f[k] = inf[k].w;
			}
			else
			{
				ja.f[k] = 0.f;
				wa.f[k] = 0.f;
			}
		}
	}

	// remove redundant weight/joint streams
	for (size_t i = 0; i < mesh.streams.size();)
	{
		Stream& s = mesh.streams[i];

		if ((s.type == cgltf_attribute_type_joints || s.type == cgltf_attribute_type_weights) && s.index > 0)
			mesh.streams.erase(mesh.streams.begin() + i);
		else
			++i;
	}
}

static void simplifyPointMesh(Mesh& mesh, float threshold)
{
	assert(mesh.type == cgltf_primitive_type_points);

	if (threshold >= 1)
		return;

	const Stream* positions = getStream(mesh, cgltf_attribute_type_position);
	if (!positions)
		return;

	const Stream* colors = getStream(mesh, cgltf_attribute_type_color);

	size_t vertex_count = mesh.streams[0].data.size();

	size_t target_vertex_count = size_t(double(vertex_count) * threshold);

	const float color_weight = 1;

	std::vector<unsigned int> indices(target_vertex_count);
	if (target_vertex_count)
		indices.resize(meshopt_simplifyPoints(&indices[0], positions->data[0].f, vertex_count, sizeof(Attr), colors ? colors->data[0].f : NULL, sizeof(Attr), color_weight, target_vertex_count));

	std::vector<Attr> scratch(indices.size());

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		std::vector<Attr>& data = mesh.streams[i].data;

		assert(data.size() == vertex_count);

		for (size_t j = 0; j < indices.size(); ++j)
			scratch[j] = data[indices[j]];

		data = scratch;
	}
}

static void sortPointMesh(Mesh& mesh)
{
	assert(mesh.type == cgltf_primitive_type_points);

	const Stream* positions = getStream(mesh, cgltf_attribute_type_position);
	if (!positions)
		return;

	// skip spatial sort in presence of custom attributes, since when they refer to mesh features their order is more important to preserve for compression efficiency
	if (getStream(mesh, cgltf_attribute_type_custom))
		return;

	size_t vertex_count = mesh.streams[0].data.size();

	std::vector<unsigned int> remap(vertex_count);
	meshopt_spatialSortRemap(&remap[0], positions->data[0].f, vertex_count, sizeof(Attr));

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		assert(mesh.streams[i].data.size() == vertex_count);

		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], vertex_count, sizeof(Attr), &remap[0]);
	}
}

void processMesh(Mesh& mesh, const Settings& settings)
{
	switch (mesh.type)
	{
	case cgltf_primitive_type_points:
		assert(mesh.indices.empty());
		simplifyPointMesh(mesh, settings.simplify_ratio);
		sortPointMesh(mesh);
		break;

	case cgltf_primitive_type_lines:
		break;

	case cgltf_primitive_type_triangles:
		filterBones(mesh);
		reindexMesh(mesh, settings.quantize && !settings.nrm_float);
		filterTriangles(mesh);
		if (settings.simplify_ratio < 1)
			simplifyMesh(mesh, settings.simplify_ratio, settings.simplify_error, settings.simplify_attributes, settings.simplify_aggressive, settings.simplify_lock_borders);
		optimizeMesh(mesh, settings.compressmore);
		break;

	default:
		assert(!"Unknown primitive type");
	}
}

#ifndef NDEBUG
void debugSimplify(const Mesh& source, Mesh& kinds, Mesh& loops, float ratio, float error, bool attributes, bool quantize_tbn)
{
	Mesh mesh = source;
	assert(mesh.type == cgltf_primitive_type_triangles);

	// note: it's important to follow the same pipeline as processMesh
	// otherwise the result won't match
	filterBones(mesh);
	reindexMesh(mesh, quantize_tbn);
	filterTriangles(mesh);

	simplifyMesh(mesh, ratio, error, attributes, /* aggressive= */ false, /* lock_borders= */ false, /* debug= */ true);

	// color palette for display
	static const Attr kPalette[] = {
	    {0.5f, 0.5f, 0.5f, 1.f}, // manifold
	    {0.f, 0.f, 1.f, 1.f},    // border
	    {0.f, 1.f, 0.f, 1.f},    // seam
	    {0.f, 1.f, 1.f, 1.f},    // complex
	    {1.f, 0.f, 0.f, 1.f},    // locked
	};

	// prepare meshes
	kinds.nodes = mesh.nodes;
	kinds.skin = mesh.skin;

	loops.nodes = mesh.nodes;
	loops.skin = mesh.skin;

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		const Stream& stream = mesh.streams[i];

		if (stream.target == 0 && (stream.type == cgltf_attribute_type_position || stream.type == cgltf_attribute_type_joints || stream.type == cgltf_attribute_type_weights))
		{
			kinds.streams.push_back(stream);
			loops.streams.push_back(stream);
		}
	}

	size_t vertex_count = mesh.streams[0].data.size();

	// transform kind/loop data into lines & points
	Stream colors = {cgltf_attribute_type_color};
	colors.data.resize(vertex_count, kPalette[0]);

	kinds.type = cgltf_primitive_type_points;
	loops.type = cgltf_primitive_type_lines;

	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		for (int k = 0; k < 3; ++k)
		{
			const unsigned int mask = (1 << 28) - 1;
			unsigned int v = mesh.indices[i + k];
			unsigned int next = mesh.indices[i + (k + 1) % 3];
			unsigned int vk = (v >> 28) & 7;
			unsigned int vl = v >> 31;

			if (vk)
			{
				colors.data[v & mask] = kPalette[vk];
				kinds.indices.push_back(v & mask);
			}

			if (vl)
			{
				loops.indices.push_back(v & mask);
				loops.indices.push_back(next & mask);
			}
		}
	}

	kinds.streams.push_back(colors);
	loops.streams.push_back(colors);
}

void debugMeshlets(const Mesh& source, Mesh& meshlets, int max_vertices)
{
	Mesh mesh = source;
	assert(mesh.type == cgltf_primitive_type_triangles);

	reindexMesh(mesh, /* quantize_tbn= */ true);

	const Stream* positions = getStream(mesh, cgltf_attribute_type_position);
	assert(positions);

	const float cone_weight = 0.f;

	size_t max_triangles = (max_vertices * 2 + 3) & ~3;
	size_t max_meshlets = meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> ml(max_meshlets);
	std::vector<unsigned int> mlv(max_meshlets * max_vertices);
	std::vector<unsigned char> mlt(max_meshlets * max_triangles * 3);
	ml.resize(meshopt_buildMeshlets(&ml[0], &mlv[0], &mlt[0], &mesh.indices[0], mesh.indices.size(), positions->data[0].f, positions->data.size(), sizeof(Attr), max_vertices, max_triangles, cone_weight));

	// generate meshlet meshes, using unique colors
	meshlets.nodes = mesh.nodes;

	Stream mv = {cgltf_attribute_type_position};
	Stream mc = {cgltf_attribute_type_color};

	for (size_t i = 0; i < ml.size(); ++i)
	{
		const meshopt_Meshlet& m = ml[i];

		unsigned int h = unsigned(i);
		h ^= h >> 13;
		h *= 0x5bd1e995;
		h ^= h >> 15;

		Attr c = {{float(h & 0xff) / 255.f, float((h >> 8) & 0xff) / 255.f, float((h >> 16) & 0xff) / 255.f, 1.f}};

		unsigned int offset = unsigned(mv.data.size());

		for (size_t j = 0; j < m.vertex_count; ++j)
		{
			mv.data.push_back(positions->data[mlv[m.vertex_offset + j]]);
			mc.data.push_back(c);
		}

		for (size_t j = 0; j < m.triangle_count; ++j)
		{
			meshlets.indices.push_back(offset + mlt[m.triangle_offset + j * 3 + 0]);
			meshlets.indices.push_back(offset + mlt[m.triangle_offset + j * 3 + 1]);
			meshlets.indices.push_back(offset + mlt[m.triangle_offset + j * 3 + 2]);
		}
	}

	meshlets.type = cgltf_primitive_type_triangles;
	meshlets.streams.push_back(mv);
	meshlets.streams.push_back(mc);
}
#endif
