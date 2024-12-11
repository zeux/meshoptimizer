// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <algorithm>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

#include "../src/meshoptimizer.h"

struct Bounds
{
	Attr min, max;

	Bounds()
	{
		min.f[0] = min.f[1] = min.f[2] = min.f[3] = +FLT_MAX;
		max.f[0] = max.f[1] = max.f[2] = max.f[3] = -FLT_MAX;
	}

	bool isValid() const
	{
		return min.f[0] <= max.f[0] && min.f[1] <= max.f[1] && min.f[2] <= max.f[2] && min.f[3] <= max.f[3];
	}
};

static void updateAttributeBounds(const Mesh& mesh, cgltf_attribute_type type, Bounds& b)
{
	Attr pad = {};

	for (size_t j = 0; j < mesh.streams.size(); ++j)
	{
		const Stream& s = mesh.streams[j];

		if (s.type == type)
		{
			if (s.target == 0)
			{
				for (size_t k = 0; k < s.data.size(); ++k)
				{
					const Attr& a = s.data[k];

					b.min.f[0] = std::min(b.min.f[0], a.f[0]);
					b.min.f[1] = std::min(b.min.f[1], a.f[1]);
					b.min.f[2] = std::min(b.min.f[2], a.f[2]);
					b.min.f[3] = std::min(b.min.f[3], a.f[3]);

					b.max.f[0] = std::max(b.max.f[0], a.f[0]);
					b.max.f[1] = std::max(b.max.f[1], a.f[1]);
					b.max.f[2] = std::max(b.max.f[2], a.f[2]);
					b.max.f[3] = std::max(b.max.f[3], a.f[3]);
				}
			}
			else
			{
				for (size_t k = 0; k < s.data.size(); ++k)
				{
					const Attr& a = s.data[k];

					pad.f[0] = std::max(pad.f[0], fabsf(a.f[0]));
					pad.f[1] = std::max(pad.f[1], fabsf(a.f[1]));
					pad.f[2] = std::max(pad.f[2], fabsf(a.f[2]));
					pad.f[3] = std::max(pad.f[3], fabsf(a.f[3]));
				}
			}
		}
	}

	for (int k = 0; k < 4; ++k)
	{
		b.min.f[k] -= pad.f[k];
		b.max.f[k] += pad.f[k];
	}
}

QuantizationPosition prepareQuantizationPosition(const std::vector<Mesh>& meshes, const Settings& settings)
{
	QuantizationPosition result = {};

	result.bits = settings.pos_bits;
	result.normalized = settings.pos_normalized;

	Bounds b;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		updateAttributeBounds(meshes[i], cgltf_attribute_type_position, b);
	}

	if (b.isValid())
	{
		result.offset[0] = b.min.f[0];
		result.offset[1] = b.min.f[1];
		result.offset[2] = b.min.f[2];
		result.scale = std::max(b.max.f[0] - b.min.f[0], std::max(b.max.f[1] - b.min.f[1], b.max.f[2] - b.min.f[2]));
	}

	result.node_scale = result.scale / float((1 << result.bits) - 1) * (result.normalized ? 65535.f : 1.f);

	return result;
}

static size_t follow(std::vector<size_t>& parents, size_t index)
{
	while (index != parents[index])
	{
		size_t parent = parents[index];

		parents[index] = parents[parent];
		index = parent;
	}

	return index;
}

void prepareQuantizationTexture(cgltf_data* data, std::vector<QuantizationTexture>& result, std::vector<size_t>& indices, const std::vector<Mesh>& meshes, const Settings& settings)
{
	// use union-find to associate each material with a canonical material
	// this is necessary because any set of materials that are used on the same mesh must use the same quantization
	std::vector<size_t> parents(result.size());

	for (size_t i = 0; i < parents.size(); ++i)
		parents[i] = i;

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		if (!mesh.material && mesh.variants.empty())
			continue;

		size_t root = follow(parents, (mesh.material ? mesh.material : mesh.variants[0].material) - data->materials);

		for (size_t j = 0; j < mesh.variants.size(); ++j)
		{
			size_t var = follow(parents, mesh.variants[j].material - data->materials);

			parents[var] = root;
		}

		indices[i] = root;
	}

	// compute canonical material bounds based on meshes that use them
	std::vector<Bounds> bounds(result.size());

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		const Mesh& mesh = meshes[i];

		if (!mesh.material && mesh.variants.empty())
			continue;

		indices[i] = follow(parents, indices[i]);
		updateAttributeBounds(mesh, cgltf_attribute_type_texcoord, bounds[indices[i]]);
	}

	// update all material data using canonical bounds
	for (size_t i = 0; i < result.size(); ++i)
	{
		QuantizationTexture& qt = result[i];

		qt.bits = settings.tex_bits;
		qt.normalized = true;

		const Bounds& b = bounds[follow(parents, i)];

		if (b.isValid())
		{
			qt.offset[0] = b.min.f[0];
			qt.offset[1] = b.min.f[1];
			qt.scale[0] = b.max.f[0] - b.min.f[0];
			qt.scale[1] = b.max.f[1] - b.min.f[1];
		}
	}
}

void getPositionBounds(float min[3], float max[3], const Stream& stream, const QuantizationPosition& qp, const Settings& settings)
{
	assert(stream.type == cgltf_attribute_type_position);
	assert(stream.data.size() > 0);

	min[0] = min[1] = min[2] = FLT_MAX;
	max[0] = max[1] = max[2] = -FLT_MAX;

	for (size_t i = 0; i < stream.data.size(); ++i)
	{
		const Attr& a = stream.data[i];

		for (int k = 0; k < 3; ++k)
		{
			min[k] = std::min(min[k], a.f[k]);
			max[k] = std::max(max[k], a.f[k]);
		}
	}

	if (settings.quantize)
	{
		if (settings.pos_float)
		{
			for (int k = 0; k < 3; ++k)
			{
				min[k] = meshopt_quantizeFloat(min[k], qp.bits);
				max[k] = meshopt_quantizeFloat(max[k], qp.bits);
			}
		}
		else
		{
			float pos_rscale = qp.scale == 0.f ? 0.f : 1.f / qp.scale * (stream.target > 0 && qp.normalized ? 32767.f / 65535.f : 1.f);

			for (int k = 0; k < 3; ++k)
			{
				if (stream.target == 0)
				{
					min[k] = float(meshopt_quantizeUnorm((min[k] - qp.offset[k]) * pos_rscale, qp.bits));
					max[k] = float(meshopt_quantizeUnorm((max[k] - qp.offset[k]) * pos_rscale, qp.bits));
				}
				else
				{
					min[k] = (min[k] >= 0.f ? 1.f : -1.f) * float(meshopt_quantizeUnorm(fabsf(min[k]) * pos_rscale, qp.bits));
					max[k] = (max[k] >= 0.f ? 1.f : -1.f) * float(meshopt_quantizeUnorm(fabsf(max[k]) * pos_rscale, qp.bits));
				}
			}
		}
	}
}

static void renormalizeWeights(uint8_t (&w)[4])
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

static StreamFormat writeVertexStreamRaw(std::string& bin, const Stream& stream, cgltf_type type, size_t components)
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

static StreamFormat writeVertexStreamFloat(std::string& bin, const Stream& stream, cgltf_type type, int components, const Settings& settings, int bits, meshopt_EncodeExpMode mode)
{
	assert(components >= 1 && components <= 4);

	StreamFormat::Filter filter = settings.compress ? StreamFormat::Filter_Exp : StreamFormat::Filter_None;

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

static int quantizeColor(float v, int bytebits, int bits)
{
	int result = meshopt_quantizeUnorm(v, bytebits);

	// replicate the top bit into the low significant bits
	const int mask = (1 << (bytebits - bits)) - 1;

	return (result & ~mask) | (mask & -(result >> (bytebits - 1)));
}

StreamFormat writeVertexStream(std::string& bin, const Stream& stream, const QuantizationPosition& qp, const QuantizationTexture& qt, const Settings& settings)
{
	if (stream.type == cgltf_attribute_type_position)
	{
		if (!settings.quantize)
			return writeVertexStreamRaw(bin, stream, cgltf_type_vec3, 3);

		if (settings.pos_float)
			return writeVertexStreamFloat(bin, stream, cgltf_type_vec3, 3, settings, qp.bits, settings.compressmore ? meshopt_EncodeExpSharedComponent : meshopt_EncodeExpSeparate);

		if (stream.target == 0)
		{
			float pos_rscale = qp.scale == 0.f ? 0.f : 1.f / qp.scale;

			for (size_t i = 0; i < stream.data.size(); ++i)
			{
				const Attr& a = stream.data[i];

				uint16_t v[4] = {
				    uint16_t(meshopt_quantizeUnorm((a.f[0] - qp.offset[0]) * pos_rscale, qp.bits)),
				    uint16_t(meshopt_quantizeUnorm((a.f[1] - qp.offset[1]) * pos_rscale, qp.bits)),
				    uint16_t(meshopt_quantizeUnorm((a.f[2] - qp.offset[2]) * pos_rscale, qp.bits)),
				    0};
				bin.append(reinterpret_cast<const char*>(v), sizeof(v));
			}

			StreamFormat format = {cgltf_type_vec3, cgltf_component_type_r_16u, qp.normalized, 8};
			return format;
		}
		else
		{
			float pos_rscale = qp.scale == 0.f ? 0.f : 1.f / qp.scale * (qp.normalized ? 32767.f / 65535.f : 1.f);

			int maxv = 0;

			for (size_t i = 0; i < stream.data.size(); ++i)
			{
				const Attr& a = stream.data[i];

				maxv = std::max(maxv, meshopt_quantizeUnorm(fabsf(a.f[0]) * pos_rscale, qp.bits));
				maxv = std::max(maxv, meshopt_quantizeUnorm(fabsf(a.f[1]) * pos_rscale, qp.bits));
				maxv = std::max(maxv, meshopt_quantizeUnorm(fabsf(a.f[2]) * pos_rscale, qp.bits));
			}

			if (maxv <= 127 && !qp.normalized)
			{
				for (size_t i = 0; i < stream.data.size(); ++i)
				{
					const Attr& a = stream.data[i];

					int8_t v[4] = {
					    int8_t((a.f[0] >= 0.f ? 1 : -1) * meshopt_quantizeUnorm(fabsf(a.f[0]) * pos_rscale, qp.bits)),
					    int8_t((a.f[1] >= 0.f ? 1 : -1) * meshopt_quantizeUnorm(fabsf(a.f[1]) * pos_rscale, qp.bits)),
					    int8_t((a.f[2] >= 0.f ? 1 : -1) * meshopt_quantizeUnorm(fabsf(a.f[2]) * pos_rscale, qp.bits)),
					    0};
					bin.append(reinterpret_cast<const char*>(v), sizeof(v));
				}

				StreamFormat format = {cgltf_type_vec3, cgltf_component_type_r_8, false, 4};
				return format;
			}
			else
			{
				for (size_t i = 0; i < stream.data.size(); ++i)
				{
					const Attr& a = stream.data[i];

					int16_t v[4] = {
					    int16_t((a.f[0] >= 0.f ? 1 : -1) * meshopt_quantizeUnorm(fabsf(a.f[0]) * pos_rscale, qp.bits)),
					    int16_t((a.f[1] >= 0.f ? 1 : -1) * meshopt_quantizeUnorm(fabsf(a.f[1]) * pos_rscale, qp.bits)),
					    int16_t((a.f[2] >= 0.f ? 1 : -1) * meshopt_quantizeUnorm(fabsf(a.f[2]) * pos_rscale, qp.bits)),
					    0};
					bin.append(reinterpret_cast<const char*>(v), sizeof(v));
				}

				StreamFormat format = {cgltf_type_vec3, cgltf_component_type_r_16, qp.normalized, 8};
				return format;
			}
		}
	}
	else if (stream.type == cgltf_attribute_type_texcoord)
	{
		if (!settings.quantize)
			return writeVertexStreamRaw(bin, stream, cgltf_type_vec2, 2);

		// expand the encoded range to ensure it covers [0..1) interval
		// this can slightly reduce precision but we should not need more precision inside 0..1, and this significantly improves compressed size when using encodeExpOne
		if (settings.tex_float)
			return writeVertexStreamFloat(bin, stream, cgltf_type_vec2, 2, settings, qt.bits, settings.compressmore ? meshopt_EncodeExpSharedComponent : meshopt_EncodeExpClamped);

		float uv_rscale[2] = {
		    qt.scale[0] == 0.f ? 0.f : 1.f / qt.scale[0],
		    qt.scale[1] == 0.f ? 0.f : 1.f / qt.scale[1],
		};

		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint16_t v[2] = {
			    uint16_t(meshopt_quantizeUnorm((a.f[0] - qt.offset[0]) * uv_rscale[0], qt.bits)),
			    uint16_t(meshopt_quantizeUnorm((a.f[1] - qt.offset[1]) * uv_rscale[1], qt.bits)),
			};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_vec2, cgltf_component_type_r_16u, qt.normalized, 4};
		return format;
	}
	else if (stream.type == cgltf_attribute_type_normal)
	{
		if (!settings.quantize)
			return writeVertexStreamRaw(bin, stream, cgltf_type_vec3, 3);

		// expand the encoded range to ensure it covers [0..1) interval
		if (settings.nrm_float)
			return writeVertexStreamFloat(bin, stream, cgltf_type_vec3, 3, settings, settings.nrm_bits, settings.compressmore || stream.target ? meshopt_EncodeExpSharedComponent : meshopt_EncodeExpClamped);

		bool oct = settings.compressmore && stream.target == 0;
		int bits = settings.nrm_bits;

		StreamFormat::Filter filter = oct ? StreamFormat::Filter_Oct : StreamFormat::Filter_None;

		size_t offset = bin.size();
		size_t stride = bits > 8 ? 8 : 4;
		bin.resize(bin.size() + stream.data.size() * stride);

		if (oct)
			meshopt_encodeFilterOct(&bin[offset], stream.data.size(), stride, bits, stream.data[0].f);
		else
			encodeSnorm(&bin[offset], stream.data.size(), stride, bits, stream.data[0].f);

		cgltf_component_type component_type = bits > 8 ? cgltf_component_type_r_16 : cgltf_component_type_r_8;
		StreamFormat format = {cgltf_type_vec3, component_type, true, stride, filter};
		return format;
	}
	else if (stream.type == cgltf_attribute_type_tangent)
	{
		if (!settings.quantize)
			return writeVertexStreamRaw(bin, stream, cgltf_type_vec4, 4);

		bool oct = settings.compressmore && stream.target == 0;
		int bits = (settings.nrm_bits > 8) ? 8 : settings.nrm_bits;

		StreamFormat::Filter filter = oct ? StreamFormat::Filter_Oct : StreamFormat::Filter_None;

		size_t offset = bin.size();
		size_t stride = 4;
		bin.resize(bin.size() + stream.data.size() * stride);

		if (oct)
			meshopt_encodeFilterOct(&bin[offset], stream.data.size(), stride, bits, stream.data[0].f);
		else
			encodeSnorm(&bin[offset], stream.data.size(), stride, bits, stream.data[0].f);

		cgltf_type type = (stream.target == 0) ? cgltf_type_vec4 : cgltf_type_vec3;
		StreamFormat format = {type, cgltf_component_type_r_8, true, 4, filter};
		return format;
	}
	else if (stream.type == cgltf_attribute_type_color)
	{
		int bits = settings.col_bits;

		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			if (bits > 8)
			{
				uint16_t v[4] = {
				    uint16_t(quantizeColor(a.f[0], 16, bits)),
				    uint16_t(quantizeColor(a.f[1], 16, bits)),
				    uint16_t(quantizeColor(a.f[2], 16, bits)),
				    uint16_t(quantizeColor(a.f[3], 16, bits))};
				bin.append(reinterpret_cast<const char*>(v), sizeof(v));
			}
			else
			{
				uint8_t v[4] = {
				    uint8_t(quantizeColor(a.f[0], 8, bits)),
				    uint8_t(quantizeColor(a.f[1], 8, bits)),
				    uint8_t(quantizeColor(a.f[2], 8, bits)),
				    uint8_t(quantizeColor(a.f[3], 8, bits))};
				bin.append(reinterpret_cast<const char*>(v), sizeof(v));
			}
		}

		if (bits > 8)
		{
			StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_16u, true, 8};
			return format;
		}
		else
		{
			StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_8u, true, 4};
			return format;
		}
	}
	else if (stream.type == cgltf_attribute_type_weights)
	{
		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			float ws = a.f[0] + a.f[1] + a.f[2] + a.f[3];
			float wsi = (ws == 0.f) ? 0.f : 1.f / ws;

			uint8_t v[4] = {
			    uint8_t(meshopt_quantizeUnorm(a.f[0] * wsi, 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[1] * wsi, 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[2] * wsi, 8)),
			    uint8_t(meshopt_quantizeUnorm(a.f[3] * wsi, 8))};

			if (wsi != 0.f)
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
	else if (stream.type == cgltf_attribute_type_custom)
	{
		// note: _custom is equivalent to _ID, as such the data contains scalar integers
		if (!settings.compressmore)
			return writeVertexStreamRaw(bin, stream, cgltf_type_scalar, 1);

		unsigned int maxv = 0;

		for (size_t i = 0; i < stream.data.size(); ++i)
			maxv = std::max(maxv, unsigned(stream.data[i].f[0]));

		// exp encoding uses a signed mantissa with only 23 significant bits; input glTF encoding may encode indices losslessly up to 2^24
		if (maxv >= (1 << 23))
			return writeVertexStreamRaw(bin, stream, cgltf_type_scalar, 1);

		for (size_t i = 0; i < stream.data.size(); ++i)
		{
			const Attr& a = stream.data[i];

			uint32_t id = uint32_t(a.f[0]);
			uint32_t v = id; // exp encoding of integers in [0..2^23-1] range is equivalent to the integer itself

			bin.append(reinterpret_cast<const char*>(&v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_scalar, cgltf_component_type_r_32f, false, 4, StreamFormat::Filter_Exp};
		return format;
	}
	else
	{
		return writeVertexStreamRaw(bin, stream, cgltf_type_vec4, 4);
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

StreamFormat writeKeyframeStream(std::string& bin, cgltf_animation_path_type type, const std::vector<Attr>& data, const Settings& settings, bool has_tangents)
{
	if (type == cgltf_animation_path_type_rotation)
	{
		StreamFormat::Filter filter = settings.compressmore && !has_tangents ? StreamFormat::Filter_Quat : StreamFormat::Filter_None;

		size_t offset = bin.size();
		size_t stride = 8;
		bin.resize(bin.size() + data.size() * stride);

		if (filter == StreamFormat::Filter_Quat)
			meshopt_encodeFilterQuat(&bin[offset], data.size(), stride, settings.rot_bits, data[0].f);
		else
			encodeSnorm(&bin[offset], data.size(), stride, 16, data[0].f);

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_16, true, 8, filter};
		return format;
	}
	else if (type == cgltf_animation_path_type_weights)
	{
		for (size_t i = 0; i < data.size(); ++i)
		{
			const Attr& a = data[i];

			uint8_t v[1] = {uint8_t(meshopt_quantizeUnorm(a.f[0], 8))};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_scalar, cgltf_component_type_r_8u, true, 1};
		return format;
	}
	else if (type == cgltf_animation_path_type_translation || type == cgltf_animation_path_type_scale)
	{
		StreamFormat::Filter filter = settings.compressmore ? StreamFormat::Filter_Exp : StreamFormat::Filter_None;
		int bits = (type == cgltf_animation_path_type_translation) ? settings.trn_bits : settings.scl_bits;

		size_t offset = bin.size();

		for (size_t i = 0; i < data.size(); ++i)
		{
			const Attr& a = data[i];

			float v[3] = {a.f[0], a.f[1], a.f[2]};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		if (filter == StreamFormat::Filter_Exp)
			meshopt_encodeFilterExp(&bin[offset], data.size(), 12, bits, reinterpret_cast<const float*>(&bin[offset]), meshopt_EncodeExpSharedVector);

		StreamFormat format = {cgltf_type_vec3, cgltf_component_type_r_32f, false, 12, filter};
		return format;
	}
	else
	{
		for (size_t i = 0; i < data.size(); ++i)
		{
			const Attr& a = data[i];

			float v[4] = {a.f[0], a.f[1], a.f[2], a.f[3]};
			bin.append(reinterpret_cast<const char*>(v), sizeof(v));
		}

		StreamFormat format = {cgltf_type_vec4, cgltf_component_type_r_32f, false, 16};
		return format;
	}
}

void compressVertexStream(std::string& bin, const std::string& data, size_t count, size_t stride)
{
	assert(data.size() == count * stride);

	std::vector<unsigned char> compressed(meshopt_encodeVertexBufferBound(count, stride));
	size_t size = meshopt_encodeVertexBuffer(&compressed[0], compressed.size(), data.c_str(), count, stride);

	bin.append(reinterpret_cast<const char*>(&compressed[0]), size);
}

void compressIndexStream(std::string& bin, const std::string& data, size_t count, size_t stride)
{
	assert(stride == 2 || stride == 4);
	assert(data.size() == count * stride);
	assert(count % 3 == 0);

	std::vector<unsigned char> compressed(meshopt_encodeIndexBufferBound(count, count));
	size_t size = 0;

	if (stride == 2)
		size = meshopt_encodeIndexBuffer(&compressed[0], compressed.size(), reinterpret_cast<const uint16_t*>(data.c_str()), count);
	else
		size = meshopt_encodeIndexBuffer(&compressed[0], compressed.size(), reinterpret_cast<const uint32_t*>(data.c_str()), count);

	bin.append(reinterpret_cast<const char*>(&compressed[0]), size);
}

void compressIndexSequence(std::string& bin, const std::string& data, size_t count, size_t stride)
{
	assert(stride == 2 || stride == 4);
	assert(data.size() == count * stride);

	std::vector<unsigned char> compressed(meshopt_encodeIndexSequenceBound(count, count));
	size_t size = 0;

	if (stride == 2)
		size = meshopt_encodeIndexSequence(&compressed[0], compressed.size(), reinterpret_cast<const uint16_t*>(data.c_str()), count);
	else
		size = meshopt_encodeIndexSequence(&compressed[0], compressed.size(), reinterpret_cast<const uint32_t*>(data.c_str()), count);

	bin.append(reinterpret_cast<const char*>(&compressed[0]), size);
}
