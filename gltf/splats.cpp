// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <memory>

#include <math.h>

#include "../extern/micro_ply.h"
#include "../src/meshoptimizer.h"

enum SplatAttribute
{
	Splat_Position = 0,
	Splat_Rotation,
	Splat_Scale,
	Splat_Opacity,
	Splat_SH0,
	Splat_SH1,
	Splat_SH2,
	Splat_SH3,
};

static void defaultFree(void*, void* p)
{
	free(p);
}

static const ply_prop_t* findPropFloat(const ply_element_t& element, const char* name)
{
	if (!name)
		return NULL;

	for (int32_t i = 0; i < element.property_count; ++i)
	{
		const ply_prop_t& prop = element.properties[i];

		if (strcmp(prop.name, name) == 0 && prop.type == ply_prop_decimal && prop.bytes == sizeof(float))
			return &prop;
	}

	return NULL;
}

static bool readProps(std::vector<Attr>& result, const ply_element_t& element, const char* x, const char* y, const char* z, const char* w)
{
	const ply_prop_t* px = findPropFloat(element, x);
	const ply_prop_t* py = findPropFloat(element, y);
	const ply_prop_t* pz = findPropFloat(element, z);
	const ply_prop_t* pw = findPropFloat(element, w);

	if ((x && !px) || (y && !py) || (z && !pz) || (w && !pw))
		return false;

	size_t count = size_t(element.count);

	result.resize(count);

	const char* data = static_cast<const char*>(element.data);
	Attr* target = result.data();

	for (size_t i = 0; i < count; ++i)
	{
		if (px)
			memcpy(&target[i].f[0], data + px->offset, sizeof(float));
		if (py)
			memcpy(&target[i].f[1], data + py->offset, sizeof(float));
		if (pz)
			memcpy(&target[i].f[2], data + pz->offset, sizeof(float));
		if (pw)
			memcpy(&target[i].f[3], data + pw->offset, sizeof(float));

		data += element.data_stride;
	}

	return true;
}

static int countSHProps(const ply_element_t& element)
{
	for (int i = 0; i <= 45; ++i)
	{
		char pname[16];
		snprintf(pname, sizeof(pname), "f_rest_%d", i);

		if (!findPropFloat(element, pname))
			return i;
	}

	// too many props; f_rest_45 implies >3 bands
	return -1;
}

template <int N>
static bool readSHProps(std::vector<Attr> (&result)[N], const ply_element_t& vtxe, int offset, int stride)
{
	for (int i = 0; i < N; ++i)
	{
		char pname[3][16];
		snprintf(pname[0], sizeof(pname[0]), "f_rest_%d", offset + i + stride * 0);
		snprintf(pname[1], sizeof(pname[1]), "f_rest_%d", offset + i + stride * 1);
		snprintf(pname[2], sizeof(pname[2]), "f_rest_%d", offset + i + stride * 2);

		if (!readProps(result[i], vtxe, pname[0], pname[1], pname[2], NULL))
			return false;
	}

	return true;
}

static void convertSplats(std::vector<Attr>& position, std::vector<Attr>& rotation, std::vector<Attr>& scale, std::vector<Attr>& opacity, std::vector<Attr> (&sh1)[3], std::vector<Attr> (&sh2)[5], std::vector<Attr> (&sh3)[7], int shdim)
{
	for (size_t i = 0; i < position.size(); ++i)
	{
		// PLY splats store scale in log units, and opacity before the sigmoid activation
		for (int k = 0; k < 3; ++k)
			scale[i].f[k] = expf(scale[i].f[k]);

		opacity[i].f[0] = 1.f / (1.f + expf(-opacity[i].f[0]));

		// PLY splats are conventionally encoded in COLMAP coordinate space, with -X right -Y up -Z forward; glTF uses -X right +Y up +Z forward
		// the conversion requires a fixup of positions, rotations, and SH bands, flipping Y & Z signs
		position[i].f[1] *= -1;
		position[i].f[2] *= -1;

		rotation[i].f[1] *= -1;
		rotation[i].f[2] *= -1;

		// PLY rotation quaternions may or may not be normalized; normalize them just in case
		float ql = sqrtf(rotation[i].f[0] * rotation[i].f[0] + rotation[i].f[1] * rotation[i].f[1] + rotation[i].f[2] * rotation[i].f[2] + rotation[i].f[3] * rotation[i].f[3]);

		for (int k = 0; k < 4; ++k)
			rotation[i].f[k] /= ql;

		// for SH conversion, since coordinate space xform is flipping Y & Z, we need to flip coefficients for bands that encode odd-length products of Y & Z
		for (int k = 0; k < 3; ++k)
		{
			if (shdim >= 1)
			{
				sh1[0][i].f[k] *= -1;
				sh1[1][i].f[k] *= -1;
			}

			if (shdim >= 2)
			{
				sh2[0][i].f[k] *= -1;
				sh2[3][i].f[k] *= -1;
			}

			if (shdim >= 3)
			{
				sh3[0][i].f[k] *= -1;
				sh3[2][i].f[k] *= -1;
				sh3[3][i].f[k] *= -1;
				sh3[5][i].f[k] *= -1;
			}
		}
	}
}

static void appendStream(Mesh& mesh, std::vector<Attr>&& data, cgltf_attribute_type type, SplatAttribute attr, int index = 0)
{
	mesh.streams.push_back(Stream());
	Stream& stream = mesh.streams.back();

	stream.type = type;
	stream.index = index;
	stream.splats = attr;
	stream.data.swap(data);
}

static cgltf_data* createScene()
{
	cgltf_data* data = (cgltf_data*)calloc(1, sizeof(cgltf_data));
	data->memory.free_func = defaultFree;

	data->scenes = (cgltf_scene*)calloc(1, sizeof(cgltf_scene));
	data->scenes_count = 1;

	data->scene = data->scenes;

	return data;
}

cgltf_data* parseSplats(const char* path, std::vector<Mesh>& meshes, const char** error)
{
	std::string plydata;
	if (!readFile(path, plydata))
	{
		*error = "file not found";
		return NULL;
	}

	ply_file_t file;
	if (!ply_read(plydata.data(), plydata.size(), &file))
	{
		*error = "invalid PLY";
		return NULL;
	}

	std::unique_ptr<ply_file_t, void (*)(ply_file_t*)> file_storage(&file, ply_free);

	if (file.count != 1 || strcmp(file.elements[0].name, "vertex") != 0 || file.elements[0].list_data)
	{
		*error = "invalid 3DGS PLY: expected vertex array";
		return NULL;
	}

	ply_element_t vtxe = file.elements[0];

	std::vector<Attr> position, rotation, scale, opacity, sh0;
	if (!readProps(position, vtxe, "x", "y", "z", NULL) || !readProps(rotation, vtxe, "rot_1", "rot_2", "rot_3", "rot_0") ||
	    !readProps(scale, vtxe, "scale_0", "scale_1", "scale_2", NULL) || !readProps(opacity, vtxe, "opacity", NULL, NULL, NULL) ||
	    !readProps(sh0, vtxe, "f_dc_0", "f_dc_1", "f_dc_2", NULL))
	{
		*error = "invalid 3DGS PLY: missing required attributes";
		return NULL;
	}

	int shprops = countSHProps(vtxe);
	int shdim = shprops == 45 ? 3 : (shprops == 24 ? 2 : (shprops == 9 ? 1 : (shprops == 0 ? 0 : -1)));

	std::vector<Attr> sh1[3], sh2[5], sh3[7];
	if (shdim < 0 ||
	    (shdim >= 1 && !readSHProps(sh1, vtxe, 0, shprops / 3)) ||
	    (shdim >= 2 && !readSHProps(sh2, vtxe, 3, shprops / 3)) ||
	    (shdim >= 3 && !readSHProps(sh3, vtxe, 8, shprops / 3)))
	{
		*error = "invalid 3DGS PLY: inconsistent SH attributes";
		return NULL;
	}

	convertSplats(position, rotation, scale, opacity, sh1, sh2, sh3, shdim);

	meshes.push_back(Mesh());
	Mesh& mesh = meshes.back();

	mesh.type = cgltf_primitive_type_points;
	mesh.splats = true;

	appendStream(mesh, std::move(position), cgltf_attribute_type_position, Splat_Position);
	appendStream(mesh, std::move(rotation), cgltf_attribute_type_custom, Splat_Rotation);
	appendStream(mesh, std::move(scale), cgltf_attribute_type_custom, Splat_Scale);
	appendStream(mesh, std::move(opacity), cgltf_attribute_type_custom, Splat_Opacity);
	appendStream(mesh, std::move(sh0), cgltf_attribute_type_custom, Splat_SH0, 0);

	if (shdim >= 1)
		for (int k = 0; k < 3; ++k)
			appendStream(mesh, std::move(sh1[k]), cgltf_attribute_type_custom, Splat_SH1, k);

	if (shdim >= 2)
		for (int k = 0; k < 5; ++k)
			appendStream(mesh, std::move(sh2[k]), cgltf_attribute_type_custom, Splat_SH2, k);

	if (shdim >= 3)
		for (int k = 0; k < 7; ++k)
			appendStream(mesh, std::move(sh3[k]), cgltf_attribute_type_custom, Splat_SH3, k);

	cgltf_data* data = createScene();

	return data;
}

static Stream* getStream(Mesh& mesh, cgltf_attribute_type type, SplatAttribute attr, int index = 0)
{
	for (size_t i = 0; i < mesh.streams.size(); ++i)
		if (mesh.streams[i].type == type && mesh.streams[i].index == index && mesh.streams[i].splats == attr)
			return &mesh.streams[i];

	return NULL;
}

void processSplatMesh(Mesh& mesh, const Settings& settings)
{
	(void)settings;

	assert(mesh.splats && mesh.type == cgltf_primitive_type_points && mesh.indices.empty());

	const Stream* positions = getStream(mesh, cgltf_attribute_type_position, Splat_Position);
	if (!positions)
		return;

	// sort splats by position using Morton order
	size_t vertex_count = mesh.streams[0].data.size();

	std::vector<unsigned int> remap(vertex_count);
	meshopt_spatialSortRemap(&remap[0], positions->data[0].f, vertex_count, sizeof(Attr));

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		assert(mesh.streams[i].data.size() == vertex_count);

		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], vertex_count, sizeof(Attr), &remap[0]);
	}
}

const char* splatAttributeType(int type)
{
	switch (SplatAttribute(type))
	{
	case Splat_Position:
		return "POSITION";
	case Splat_Rotation:
		return "ROTATION";
	case Splat_Scale:
		return "SCALE";
	case Splat_Opacity:
		return "OPACITY";
	case Splat_SH0:
		return "SH_DEGREE_0_COEF";
	case Splat_SH1:
		return "SH_DEGREE_1_COEF";
	case Splat_SH2:
		return "SH_DEGREE_2_COEF";
	case Splat_SH3:
		return "SH_DEGREE_3_COEF";
	}

	assert(!"Unknown splat attribute type");
	return NULL;
}

static StreamFormat writeSplatStreamRaw(std::string& bin, const Stream& stream, cgltf_type type, size_t components)
{
	assert(components >= 1 && components <= 4);

	for (size_t i = 0; i < stream.data.size(); ++i)
	{
		const Attr& a = stream.data[i];

		bin.append(reinterpret_cast<const char*>(a.f), sizeof(float) * components);
	}

	StreamFormat format = {type, cgltf_component_type_r_32f, false, sizeof(float) * components};
	return format;
}

static StreamFormat writeSplatStreamFloat(std::string& bin, const Stream& stream, cgltf_type type, int components, bool expf, int bits, meshopt_EncodeExpMode mode)
{
	assert(components >= 1 && components <= 4);

	StreamFormat::Filter filter = expf ? StreamFormat::Filter_Exp : StreamFormat::Filter_None;

	if (filter == StreamFormat::Filter_Exp)
	{
		size_t offset = bin.size();
		size_t stride = sizeof(float) * components;
		for (size_t i = 0; i < stream.data.size(); ++i)
			bin.append(reinterpret_cast<const char*>(stream.data[i].f), stride);

		meshopt_encodeFilterExp(&bin[offset], stream.data.size(), stride, bits + 1, reinterpret_cast<const float*>(&bin[offset]), mode);
	}
	else
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			float v[4];
			for (int k = 0; k < components; ++k)
				v[k] = meshopt_quantizeFloat(a.f[k], bits);
			bin.append(reinterpret_cast<const char*>(v), sizeof(float) * components);
		}
	}

	StreamFormat format = {type, cgltf_component_type_r_32f, false, sizeof(float) * components, filter};
	return format;
}

static void encodeSnorm(void* destination, size_t count, size_t stride, int bits, const float* data)
{
	assert(stride == 4 || stride == 8);
	assert(bits >= 1 && bits <= 16);

	signed char* d8 = static_cast<signed char*>(destination);
	short* d16 = static_cast<short*>(destination);

	for (size_t i = 0; i < count; ++i)
	{
		const float* v = &data[i * 4];

		int fx = meshopt_quantizeSnorm(v[0], bits);
		int fy = meshopt_quantizeSnorm(v[1], bits);
		int fz = meshopt_quantizeSnorm(v[2], bits);
		int fw = meshopt_quantizeSnorm(v[3], bits);

		if (stride == 4)
		{
			d8[i * 4 + 0] = (signed char)(fx);
			d8[i * 4 + 1] = (signed char)(fy);
			d8[i * 4 + 2] = (signed char)(fz);
			d8[i * 4 + 3] = (signed char)(fw);
		}
		else
		{
			d16[i * 4 + 0] = short(fx);
			d16[i * 4 + 1] = short(fy);
			d16[i * 4 + 2] = short(fz);
			d16[i * 4 + 3] = short(fw);
		}
	}
}

StreamFormat writeSplatStream(std::string& bin, const Stream& stream, const Settings& settings)
{
	assert(stream.type == cgltf_attribute_type_position || (stream.type == cgltf_attribute_type_custom && stream.splats));

	(void)settings;

	SplatAttribute attr = stream.type == cgltf_attribute_type_position ? Splat_Position : SplatAttribute(stream.splats);

	if (attr == Splat_Position)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);

		return writeSplatStreamFloat(bin, stream, cgltf_type_vec3, 3, settings.compress, settings.pos_bits, meshopt_EncodeExpSharedComponent);
	}
	else if (attr == Splat_Rotation)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec4, 4);

		StreamFormat::Filter filter = settings.compress ? StreamFormat::Filter_Quat : StreamFormat::Filter_None;
		int bits = settings.nrm_bits + 1;

		size_t offset = bin.size();
		size_t stride = 8;
		bin.resize(bin.size() + stream.data.size() * stride);

		if (filter == StreamFormat::Filter_Quat)
			meshopt_encodeFilterQuat(&bin[offset], stream.data.size(), stride, bits, stream.data[0].f);
		else
			encodeSnorm(&bin[offset], stream.data.size(), stride, 16, stream.data[0].f);

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_16, true, 8, filter};
		return format;
	}
	else if (attr == Splat_Scale)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);

		int bits = settings.tex_bits;
		return writeSplatStreamFloat(bin, stream, cgltf_type_vec3, 3, settings.compress, bits, meshopt_EncodeExpSharedComponent);
	}
	else if (attr == Splat_Opacity)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_scalar, 1);

		int bits = settings.col_bits;
		return writeSplatStreamFloat(bin, stream, cgltf_type_scalar, 1, settings.compress, bits, meshopt_EncodeExpClamped);
	}
	else if (attr == Splat_SH0)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);

		int bits = settings.col_bits;
		return writeSplatStreamFloat(bin, stream, cgltf_type_vec3, 3, settings.compress, bits, meshopt_EncodeExpSharedComponent);
	}
	else if (attr == Splat_SH1 || attr == Splat_SH2 || attr == Splat_SH3)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);

		int bits = settings.col_bits;
		if (settings.compressmore)
			bits -= (attr == Splat_SH1 ? 1 : (attr == Splat_SH2 ? 2 : 3));

		return writeSplatStreamFloat(bin, stream, cgltf_type_vec3, 3, settings.compress, bits, meshopt_EncodeExpClamped);
	}
	else
	{
		assert(!"Unexpected splat stream type");
		return StreamFormat{};
	}
}
