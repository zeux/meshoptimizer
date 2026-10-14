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
	assert(mesh.splats && mesh.type == cgltf_primitive_type_points && mesh.indices.empty());

	const Stream* positions = getStream(mesh, cgltf_attribute_type_position, Splat_Position);
	const Stream* opacity = getStream(mesh, cgltf_attribute_type_custom, Splat_Opacity);
	if (!positions || !opacity)
		return;

	// sort splats by position using Morton order
	size_t vertex_count = mesh.streams[0].data.size();

	std::vector<unsigned int> remap(vertex_count);
	meshopt_spatialSortRemap(&remap[0], positions->data[0].f, vertex_count, sizeof(Attr));

	size_t result_count = vertex_count;

	// when quantization is enabled, we can remove splats with low opacity
	// for now this is gated behind -cc so that -c produces original splat set
	if (settings.quantize && settings.compressmore)
	{
		float opacitymin = 2.f / float(1 << settings.col_bits);

		// mark slots of surviving splats and number them in the remapped order (removed splats use ~0u)
		std::vector<unsigned int> slots(vertex_count, ~0u);

		for (size_t i = 0; i < vertex_count; ++i)
			if (opacity->data[i].f[0] >= opacitymin)
				slots[remap[i]] = 0;

		result_count = 0;
		for (size_t i = 0; i < vertex_count; ++i)
			if (slots[i] != ~0u)
				slots[i] = unsigned(result_count++);

		// update old->new remap; surviving splats get the new linearized index
		for (size_t i = 0; i < vertex_count; ++i)
			remap[i] = slots[remap[i]];
	}

	for (size_t i = 0; i < mesh.streams.size(); ++i)
	{
		assert(mesh.streams[i].data.size() == vertex_count);

		meshopt_remapVertexBuffer(&mesh.streams[i].data[0], &mesh.streams[i].data[0], vertex_count, sizeof(Attr), &remap[0]);
		mesh.streams[i].data.resize(result_count);
	}

	// filter streams with excessive SH data
	if (settings.splats_dim < 3)
	{
		size_t write = 0;

		for (Stream& stream : mesh.streams)
		{
			if (stream.type == cgltf_attribute_type_custom && stream.splats >= Splat_SH1 && stream.splats <= Splat_SH3 && (stream.splats - Splat_SH0) > settings.splats_dim)
				continue;

			// the following code is roughly equivalent to streams[write] = std::move(stream), but is safe against self-move
			std::vector<Attr> data;
			data.swap(stream.data);

			mesh.streams[write] = stream;
			mesh.streams[write].data.swap(data);
			write++;
		}

		mesh.streams.resize(write);
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

static StreamFormat writeSplatStreamExpSH(std::string& bin, const Stream& stream, int bits, int range)
{
	// values in (-2^range, 2^range) are encoded with fixed 2^-bits precision (like meshopt_EncodeExpClamped, but we need custom range support)
	// to reduce the cost of sign flips on upper mantissa bytes, we try to shift mantissa left and leave low bytes as zero
	int mbits = bits + range;
	int shift = mbits + 1 <= 8 ? 16 : (mbits + 1 <= 16 ? 8 : 0);
	float scale = ldexpf(1.f, -range);

	bin.reserve(bin.size() + stream.data.size() * sizeof(float) * 3);

	for (size_t i = 0; i < stream.data.size(); ++i)
	{
		const Attr& a = stream.data[i];

		float v[3] = {a.f[0] * scale, a.f[1] * scale, a.f[2] * scale};
		unsigned int r[3];
		meshopt_encodeFilterExp(r, 1, sizeof(r), mbits + 1, v, meshopt_EncodeExpClamped);

		for (int k = 0; k < 3; ++k)
		{
			int m = int(r[k] << 8) >> 8;
			int e = int(r[k]) >> 24;

			// rounding may produce mantissa 2^mbits; clamp it so that m with sign bit fits into mbits+1 bits and shifting does not overflow
			int mmax = 1 << mbits;
			m = std::max(-(mmax - 1), std::min(m, mmax - 1));

			r[k] = ((unsigned(m) << shift) & 0xffffff) | (unsigned(e + range - shift) << 24);
		}

		bin.append(reinterpret_cast<const char*>(r), sizeof(r));
	}

	StreamFormat format = {cgltf_type_vec3, cgltf_component_type_r_32f, false, sizeof(float) * 3, StreamFormat::Filter_Exp};
	return format;
}

static void encodeSnorm(void* destination, size_t count, size_t stride, int bits, const float* data)
{
	assert(stride == 4 || stride == 8);
	assert(bits >= 1 && bits <= 16);

	signed char* d8 = static_cast<signed char*>(destination);
	short* d16 = static_cast<short*>(destination);

	// to support bits != 8/16, we need to adjust the quantization scale so that division by 127/32767 during dequantization does not lose precision
	// this is a no-op if bits matches stride, as step=1 and scale=1.0f
	int range = stride == 4 ? 128 : 32768;
	int step = 1 << ((stride == 4 ? 8 : 16) - bits);
	float scale = float(range - 1) / float(range - step);

	for (size_t i = 0; i < count; ++i)
	{
		const float* v = &data[i * 4];

		int fx = meshopt_quantizeSnorm(v[0] * scale, bits) * step;
		int fy = meshopt_quantizeSnorm(v[1] * scale, bits) * step;
		int fz = meshopt_quantizeSnorm(v[2] * scale, bits) * step;
		int fw = meshopt_quantizeSnorm(v[3] * scale, bits) * step;

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
	assert(stream.type == cgltf_attribute_type_position || stream.splats);

	SplatAttribute attr = stream.type == cgltf_attribute_type_position ? Splat_Position : SplatAttribute(stream.splats);

	if (attr == Splat_Position)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);

		int bits = settings.pos_bits;
		return writeSplatStreamFloat(bin, stream, cgltf_type_vec3, 3, settings.compress, bits, meshopt_EncodeExpSharedComponent);
	}
	else if (attr == Splat_Rotation)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec4, 4);

		StreamFormat::Filter filter = settings.compress ? StreamFormat::Filter_Quat : StreamFormat::Filter_None;
		int bits = settings.nrm_bits + (settings.compressmore ? 0 : 1);
		bits = std::max(4, std::min(bits, 16)); // quaternion filter supports 4-16 bits

		size_t offset = bin.size();
		size_t stride = 8;
		bin.resize(bin.size() + stream.data.size() * stride);

		if (filter == StreamFormat::Filter_Quat)
			meshopt_encodeFilterQuat(&bin[offset], stream.data.size(), stride, bits, stream.data[0].f);
		else
			encodeSnorm(&bin[offset], stream.data.size(), stride, 16, stream.data[0].f);

		if (filter == StreamFormat::Filter_Quat && bits == 8 && settings.compressmore)
		{
			// with 8-bit components, we left-align the 16-bit components to keep the least significant byte as 0; this compresses a little better
			// technically, .w now encodes a scale that's not quite 1.0; this works but is out of spec. in the future we could adjust the quantizer so that .w can encode 0x7ffc
			for (size_t i = 0; i < stream.data.size(); ++i)
			{
				short v[4];
				memcpy(v, &bin[offset + i * 8], sizeof(v));

				for (int k = 0; k < 3; ++k)
					v[k] = short(unsigned(v[k]) << 8);
				v[3] = short(((v[3] | 3) << 8) | (v[3] & 3));

				memcpy(&bin[offset + i * 8], v, sizeof(v));
			}
		}

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

		if (!settings.compress)
			return writeSplatStreamFloat(bin, stream, cgltf_type_vec3, 3, false, settings.col_bits, meshopt_EncodeExpSharedComponent);

		int bits = settings.col_bits - 2 - (settings.compressmore ? 1 : 0); // -2 accounts for a larger range [-2..2] with a sign bit
		bits = std::max(bits, 1);
		return writeSplatStreamExpSH(bin, stream, bits, /* range= */ 1);
	}
	else if (attr == Splat_SH1 || attr == Splat_SH2 || attr == Splat_SH3)
	{
		if (!settings.quantize)
			return writeSplatStreamRaw(bin, stream, cgltf_type_vec3, 3);

		if (settings.compressmore)
		{
			// when using compressmore (-cc) we encode SH1-3 as normalized int8 to reduce decoding memory pressure and file sizes
			// this is outside of the spec, but de-facto safe for many viewers and represent a significant spec gap that should be closed
			size_t offset = bin.size();
			size_t stride = 4;
			bin.resize(bin.size() + stream.data.size() * stride);

			int bits = settings.col_bits - (attr == Splat_SH1 ? 3 : 4); // 5/4/4 at default settings, including sign bit
			bits = std::max(1, std::min(bits, 8));
			encodeSnorm(&bin[offset], stream.data.size(), stride, bits, stream.data[0].f);

			StreamFormat format = {cgltf_type_vec3, cgltf_component_type_r_8, true, 4, StreamFormat::Filter_None};
			return format;
		}
		else
		{
			int bits = settings.col_bits - (attr == Splat_SH1 ? 2 : 3); // 6/5/5 at default settings, excluding sign bit
			bits = std::max(bits, 1);

			if (!settings.compress)
				return writeSplatStreamFloat(bin, stream, cgltf_type_vec3, 3, false, bits, meshopt_EncodeExpClamped);

			return writeSplatStreamExpSH(bin, stream, bits, /* range= */ 0);
		}
	}
	else
	{
		assert(!"Unexpected splat stream type");
		return StreamFormat{};
	}
}
