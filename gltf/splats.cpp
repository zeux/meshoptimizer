// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include "../extern/micro_ply.h"

#include <memory>

#include <math.h>

const char* kGltfSH[4][7] = {
    {"KHR_gaussian_splatting:SH_DEGREE_0_COEF_0"},
    {"KHR_gaussian_splatting:SH_DEGREE_1_COEF_0", "KHR_gaussian_splatting:SH_DEGREE_1_COEF_1", "KHR_gaussian_splatting:SH_DEGREE_1_COEF_2"},
    {"KHR_gaussian_splatting:SH_DEGREE_2_COEF_0", "KHR_gaussian_splatting:SH_DEGREE_2_COEF_1", "KHR_gaussian_splatting:SH_DEGREE_2_COEF_2", "KHR_gaussian_splatting:SH_DEGREE_2_COEF_3", "KHR_gaussian_splatting:SH_DEGREE_2_COEF_4"},
    {"KHR_gaussian_splatting:SH_DEGREE_3_COEF_0", "KHR_gaussian_splatting:SH_DEGREE_3_COEF_1", "KHR_gaussian_splatting:SH_DEGREE_3_COEF_2", "KHR_gaussian_splatting:SH_DEGREE_3_COEF_3", "KHR_gaussian_splatting:SH_DEGREE_3_COEF_4", "KHR_gaussian_splatting:SH_DEGREE_3_COEF_5", "KHR_gaussian_splatting:SH_DEGREE_3_COEF_6"},
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

	const char* data = static_cast<const char*>(element.data);

	result.resize(element.count);

	for (size_t i = 0; i < result.size(); ++i)
	{
		if (px)
			memcpy(&result[i].f[0], data + px->offset, sizeof(float));
		if (py)
			memcpy(&result[i].f[1], data + py->offset, sizeof(float));
		if (pz)
			memcpy(&result[i].f[2], data + pz->offset, sizeof(float));
		if (pw)
			memcpy(&result[i].f[3], data + pw->offset, sizeof(float));

		data += element.data_stride;
	}

	return true;
}

template <int N>
static bool readSHProps(std::vector<Attr> (&result)[N], const ply_element_t& vtxe, int offset, int count)
{
	assert(N == (count + 2) / 3); // VEC3

	for (int i = 0; i < count; i += 3)
	{
		char pname[3][16];
		snprintf(pname[0], sizeof(pname[0]), "f_rest_%d", offset + i + 0);
		snprintf(pname[1], sizeof(pname[1]), "f_rest_%d", offset + i + 1);
		snprintf(pname[2], sizeof(pname[2]), "f_rest_%d", offset + i + 2);

		if (!readProps(result[i / 3], vtxe, pname[0], i + 1 < count ? pname[1] : NULL, i + 2 < count ? pname[2] : NULL, NULL))
			return false;
	}

	return true;
}

static void appendStream(Mesh& mesh, std::vector<Attr>&& data, cgltf_attribute_type type, const char* custom_name = NULL)
{
	mesh.streams.push_back(Stream());
	Stream& stream = mesh.streams.back();

	stream.type = type;
	stream.custom_name = custom_name;
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
	if (!readProps(position, vtxe, "x", "y", "z", NULL) || !readProps(rotation, vtxe, "rot_0", "rot_1", "rot_2", "rot_3") ||
	    !readProps(scale, vtxe, "scale_0", "scale_1", "scale_2", NULL) || !readProps(opacity, vtxe, "opacity", NULL, NULL, NULL) ||
	    !readProps(sh0, vtxe, "f_dc_0", "f_dc_1", "f_dc_2", NULL))
	{
		*error = "invalid 3DGS PLY: missing required attributes";
		return NULL;
	}

	std::vector<Attr> sh1[3], sh2[5], sh3[7];
	bool hassh1 = readSHProps(sh1, vtxe, 0, 9);
	bool hassh2 = readSHProps(sh2, vtxe, 9, 15);
	bool hassh3 = readSHProps(sh3, vtxe, 24, 21);

	if ((hassh3 && (!hassh1 || !hassh2)) || (hassh2 && !hassh1))
	{
		*error = "invalid 3DGS PLY: inconsistent SH attributes";
		return NULL;
	}

	meshes.push_back(Mesh());
	Mesh& mesh = meshes.back();

	mesh.type = cgltf_primitive_type_points;
	mesh.splats = true;

	appendStream(mesh, std::move(position), cgltf_attribute_type_position);
	appendStream(mesh, std::move(rotation), cgltf_attribute_type_custom, "KHR_gaussian_splatting:ROTATION");
	appendStream(mesh, std::move(scale), cgltf_attribute_type_custom, "KHR_gaussian_splatting:SCALE");
	appendStream(mesh, std::move(opacity), cgltf_attribute_type_custom, "KHR_gaussian_splatting:OPACITY");
	appendStream(mesh, std::move(sh0), cgltf_attribute_type_custom, kGltfSH[0][0]); // SH_DEGREE_0_COEF_0

	if (hassh1)
		for (int k = 0; k < 3; ++k)
			appendStream(mesh, std::move(sh1[k]), cgltf_attribute_type_custom, kGltfSH[1][k]);
	if (hassh2)
		for (int k = 0; k < 5; ++k)
			appendStream(mesh, std::move(sh2[k]), cgltf_attribute_type_custom, kGltfSH[2][k]);
	if (hassh3)
		for (int k = 0; k < 7; ++k)
			appendStream(mesh, std::move(sh3[k]), cgltf_attribute_type_custom, kGltfSH[3][k]);

	cgltf_data* data = createScene();

	return data;
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

StreamFormat writeSplatStream(std::string& bin, const Stream& stream, const QuantizationPosition& qp, const Settings& settings, bool filters)
{
	(void)qp;
	(void)settings;
	(void)filters;

	if (stream.type == cgltf_attribute_type_position)
		return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);
	else if (stream.type == cgltf_attribute_type_custom)
	{
		if (strstr(stream.custom_name, "ROTATION"))
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec4, 4);
		else if (strstr(stream.custom_name, "SCALE"))
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);
		else if (strstr(stream.custom_name, "SH_DEGREE"))
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);
		else if (strstr(stream.custom_name, "OPACITY"))
			return writeSplatStreamRaw(bin, stream, cgltf_type_scalar, 1);
	}

	assert(!"Unexpected splat stream type");
	return StreamFormat{};
}
