/**
 * gltfpack - version 0.13
 *
 * Copyright (C) 2016-2020, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * Report bugs and download new versions at https://github.com/zeux/meshoptimizer
 *
 * This application is distributed under the MIT License. See notice at the end of this file.
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include "../tools/cgltf.h"

#include <assert.h>

#include <string>
#include <vector>

struct Attr
{
	float f[4];
};

struct Stream
{
	cgltf_attribute_type type;
	int index;
	int target; // 0 = base mesh, 1+ = morph target

	std::vector<Attr> data;
};

struct Mesh
{
	cgltf_node* node;

	cgltf_material* material;
	cgltf_skin* skin;

	cgltf_primitive_type type;

	std::vector<Stream> streams;
	std::vector<unsigned int> indices;

	size_t targets;
	std::vector<float> target_weights;
	std::vector<const char*> target_names;
};

struct Track
{
	cgltf_node* node;
	cgltf_animation_path_type path;

	bool dummy;

	size_t components; // 1 unless path is cgltf_animation_path_type_weights

	cgltf_interpolation_type interpolation;

	std::vector<float> time; // empty for resampled or constant animations
	std::vector<Attr> data;
};

struct Animation
{
	const char* name;

	float start;
	int frames;

	std::vector<Track> tracks;
};

struct Settings
{
	int pos_bits;
	int tex_bits;
	int nrm_bits;
	bool nrm_unnormalized;

	int anim_freq;
	bool anim_const;

	bool keep_named;

	float simplify_threshold;
	bool simplify_aggressive;

	bool texture_embed;
	bool texture_basis;
	bool texture_ktx2;

	int texture_quality;

	bool compress;
	bool compressmore;
	bool fallback;

	int verbose;
};

struct QuantizationPosition
{
	float offset[3];
	float scale;
	int bits;
};

struct QuantizationTexture
{
	float offset[2];
	float scale[2];
	int bits;
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
	bool animated;

	unsigned int animated_paths;

	int remap;
	std::vector<size_t> meshes;
};

struct MaterialInfo
{
	bool keep;

	int remap;
};

struct ImageInfo
{
	bool normal_map;
	bool srgb;
};

struct ExtensionInfo
{
	const char* name;

	bool used;
	bool required;
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

	size_t bytes;
};

void processAnimation(Animation& animation, const Settings& settings);
void processMesh(Mesh& mesh, const Settings& settings);

void transformMesh(Mesh& mesh, const cgltf_node* node);
bool compareMeshTargets(const Mesh& lhs, const Mesh& rhs);
void mergeMeshes(std::vector<Mesh>& meshes, const Settings& settings);
void filterEmptyMeshes(std::vector<Mesh>& meshes);

bool usesTextureSet(const cgltf_material& material, int set);
void mergeMeshMaterials(cgltf_data* data, std::vector<Mesh>& meshes);
void markNeededMaterials(cgltf_data* data, std::vector<MaterialInfo>& materials, const std::vector<Mesh>& meshes);

QuantizationPosition prepareQuantizationPosition(const std::vector<Mesh>& meshes, const Settings& settings);
void prepareQuantizationTexture(cgltf_data* data, std::vector<QuantizationTexture>& result, const std::vector<Mesh>& meshes, const Settings& settings);
void getPositionBounds(int min[3], int max[3], const Stream& stream, const QuantizationPosition& qp);

StreamFormat writeVertexStream(std::string& bin, const Stream& stream, const QuantizationPosition& qp, const QuantizationTexture& qt, const Settings& settings, bool has_targets);
StreamFormat writeIndexStream(std::string& bin, const std::vector<unsigned int>& stream);
StreamFormat writeTimeStream(std::string& bin, const std::vector<float>& data);
StreamFormat writeKeyframeStream(std::string& bin, cgltf_animation_path_type type, const std::vector<Attr>& data);

void compressVertexStream(std::string& bin, const std::string& data, size_t count, size_t stride);
void compressIndexStream(std::string& bin, const std::string& data, size_t count, size_t stride);

cgltf_data* parseObj(const char* path, std::vector<Mesh>& meshes, const char** error);
cgltf_data* parseGltf(const char* path, std::vector<Mesh>& meshes, std::vector<Animation>& animations, const char** error);

/**
 * Copyright (c) 2016-2020 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
