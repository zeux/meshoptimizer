// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

struct BasisSettings
{
	int etc1s_l;
	int etc1s_q;
	int uastc_l;
	float uastc_q;
};

static const char* kMimeTypes[][2] = {
    {"image/jpeg", ".jpg"},
    {"image/jpeg", ".jpeg"},
    {"image/png", ".png"},
};

static const BasisSettings kBasisSettings[10] = {
    {1, 1, 0, 1.5f},
    {1, 6, 0, 1.f},
    {1, 20, 1, 1.0f},
    {1, 50, 1, 0.75f},
    {1, 90, 1, 0.5f},
    {1, 128, 1, 0.4f},
    {1, 160, 1, 0.34f},
    {1, 192, 1, 0.29f}, // default
    {1, 224, 2, 0.26f},
    {1, 255, 2, 0.f},
};

void analyzeImages(cgltf_data* data, std::vector<ImageInfo>& images)
{
	for (size_t i = 0; i < data->materials_count; ++i)
	{
		const cgltf_material& material = data->materials[i];

		if (material.has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;

			if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image)
				images[pbr.base_color_texture.texture->image - data->images].srgb = true;
		}

		if (material.has_pbr_specular_glossiness)
		{
			const cgltf_pbr_specular_glossiness& pbr = material.pbr_specular_glossiness;

			if (pbr.diffuse_texture.texture && pbr.diffuse_texture.texture->image)
				images[pbr.diffuse_texture.texture->image - data->images].srgb = true;
		}

		if (material.emissive_texture.texture && material.emissive_texture.texture->image)
			images[material.emissive_texture.texture->image - data->images].srgb = true;

		if (material.normal_texture.texture && material.normal_texture.texture->image)
			images[material.normal_texture.texture->image - data->images].normal_map = true;
	}
}

const char* inferMimeType(const char* path)
{
	const char* ext = strrchr(path, '.');
	if (!ext)
		return "";

	std::string extl = ext;
	for (size_t i = 0; i < extl.length(); ++i)
		extl[i] = char(tolower(extl[i]));

	for (size_t i = 0; i < sizeof(kMimeTypes) / sizeof(kMimeTypes[0]); ++i)
		if (extl == kMimeTypes[i][1])
			return kMimeTypes[i][0];

	return "";
}

static const char* mimeExtension(const char* mime_type)
{
	for (size_t i = 0; i < sizeof(kMimeTypes) / sizeof(kMimeTypes[0]); ++i)
		if (strcmp(kMimeTypes[i][0], mime_type) == 0)
			return kMimeTypes[i][1];

	return ".raw";
}

#ifdef __EMSCRIPTEN__
EM_JS(int, execute, (const char* cmd, bool ignore_stdout, bool ignore_stderr), {
	var cp = require('child_process');
	var stdio = [ 'ignore', ignore_stdout ? 'ignore' : 'inherit', ignore_stderr ? 'ignore' : 'inherit' ];
	var ret = cp.spawnSync(UTF8ToString(cmd), [], {shell:true, stdio:stdio });
	return ret.status == null ? 256 : ret.status;
});

EM_JS(const char*, readenv, (const char* name), {
	var val = process.env[UTF8ToString(name)];
	if (!val)
		return 0;
	var ret = _malloc(lengthBytesUTF8(val) + 1);
	stringToUTF8(val, ret, lengthBytesUTF8(val) + 1);
	return ret;
});
#else
static int execute(const char* cmd_, bool ignore_stdout, bool ignore_stderr)
{
#ifdef _WIN32
	std::string ignore = "nul";
#else
	std::string ignore = "/dev/null";
#endif

	std::string cmd = cmd_;

	if (ignore_stdout)
		(cmd += " >") += ignore;
	if (ignore_stderr)
		(cmd += " 2>") += ignore;

	return system(cmd.c_str());
}

static const char* readenv(const char* name)
{
	return getenv(name);
}
#endif

bool checkBasis(bool verbose)
{
	const char* basisu_path = readenv("BASISU_PATH");
	std::string cmd = basisu_path ? basisu_path : "basisu";

	cmd += " -version";

	int rc = execute(cmd.c_str(), /* ignore_stdout= */ true, /* ignore_stderr= */ true);
	if (verbose)
		printf("%s => %d\n", cmd.c_str(), rc);

	return rc == 0;
}

bool encodeBasis(const std::string& data, const char* mime_type, std::string& result, bool normal_map, bool srgb, int quality, float scale, bool pow2, bool uastc, bool verbose)
{
	(void)scale;
	(void)pow2;

	TempFile temp_input(mimeExtension(mime_type));
	TempFile temp_output(".basis");

	if (!writeFile(temp_input.path.c_str(), data))
		return false;

	const char* basisu_path = readenv("BASISU_PATH");
	std::string cmd = basisu_path ? basisu_path : "basisu";

	const BasisSettings& bs = kBasisSettings[quality <= 0 ? 0 : quality > 9 ? 9 : quality - 1];

	cmd += " -mipmap";

	if (normal_map)
	{
		cmd += " -normal_map";
		// for optimal quality we should specify seperate_rg_to_color_alpha but this requires renderer awareness
	}
	else if (!srgb)
	{
		cmd += " -linear";
	}

	if (uastc)
	{
		char settings[128];
		sprintf(settings, " -uastc_level %d -uastc_rdo_q %.2f", bs.uastc_l, bs.uastc_q);

		cmd += " -uastc";
		cmd += settings;
		cmd += " -uastc_rdo_d 1024";
	}
	else
	{
		char settings[128];
		sprintf(settings, " -comp_level %d -q %d", bs.etc1s_l, bs.etc1s_q);

		cmd += settings;
	}

	cmd += " -file ";
	cmd += temp_input.path;
	cmd += " -output_file ";
	cmd += temp_output.path;

	int rc = execute(cmd.c_str(), /* ignore_stdout= */ true, /* ignore_stderr= */ false);
	if (verbose)
		printf("%s => %d\n", cmd.c_str(), rc);

	return rc == 0 && readFile(temp_output.path.c_str(), result);
}

bool checkKtx(bool verbose)
{
	const char* toktx_path = readenv("TOKTX_PATH");
	std::string cmd = toktx_path ? toktx_path : "toktx";

	cmd += " --version";

	int rc = execute(cmd.c_str(), /* ignore_stdout= */ true, /* ignore_stderr= */ true);
	if (verbose)
		printf("%s => %d\n", cmd.c_str(), rc);

	return rc == 0;
}

static bool getDimensionsPng(const std::string& data, int& width, int& height)
{
	if (data.size() < 8 + 8 + 13 + 4)
		return false;

	const char* signature = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a";
	if (data.compare(0, 8, signature) != 0)
		return false;

	if (data.compare(12, 4, "IHDR") != 0)
		return false;

	width = unsigned(data[18]) * 256 + unsigned(data[19]);
	height = unsigned(data[22]) * 256 + unsigned(data[23]);

	return true;
}

static bool getDimensionsJpeg(const std::string& data, int& width, int& height)
{
	(void)data; (void)width; (void)height;
	return false;
}

static bool getDimensions(const std::string& data, const char* mime_type, int& width, int& height)
{
	if (strcmp(mime_type, "image/png") == 0)
		return getDimensionsPng(data, width, height);
	if (strcmp(mime_type, "image/jpeg") == 0)
		return getDimensionsJpeg(data, width, height);

	return false;
}

static int roundPow2(int value)
{
	int result = 1;

	while (result < value)
		result <<= 1;

	// to prevent odd texture sizes from increasing the size too much, we round to nearest power of 2 above a certain size
	if (value > 128 && result * 3 / 4 > value)
		result >>= 1;

	return result;
}

static int roundBlock(int value)
{
	return (value == 1 || value == 2) ? value : (value + 3) & ~3;
}

bool encodeKtx(const std::string& data, const char* mime_type, std::string& result, bool normal_map, bool srgb, int quality, float scale, bool pow2, bool uastc, bool verbose)
{
	int width = 0, height = 0;
	if (!getDimensions(data, mime_type, width, height))
		return false;

	TempFile temp_input(mimeExtension(mime_type));
	TempFile temp_output(".ktx2");

	if (!writeFile(temp_input.path.c_str(), data))
		return false;

	const char* toktx_path = readenv("TOKTX_PATH");
	std::string cmd = toktx_path ? toktx_path : "toktx";

	const BasisSettings& bs = kBasisSettings[quality <= 0 ? 0 : quality > 9 ? 9 : quality - 1];

	cmd += " --t2";
	cmd += " --2d";
	cmd += " --genmipmap";
	cmd += " --nowarn";

	int (*round)(int value) = pow2 ? roundPow2 : roundBlock;
	int newWidth = round(width * scale), newHeight = round(height * scale);

	if (newWidth != width || newHeight != height)
	{
		char wh[128];
		sprintf(wh, " --resize %dx%d", newWidth, newHeight);
		cmd += wh;
	}

	if (uastc)
	{
		char settings[128];
		sprintf(settings, " %d --uastc_rdo_q %.2f", bs.uastc_l, bs.uastc_q);

		cmd += " --uastc";
		cmd += settings;
		cmd += " --uastc_rdo_d 1024";
		cmd += " --zcmp 9";
	}
	else
	{
		char settings[128];
		sprintf(settings, " --clevel %d --qlevel %d", bs.etc1s_l, bs.etc1s_q);

		cmd += " --bcmp";
		cmd += settings;

		// for optimal quality we should specify separate_rg_to_color_alpha but this requires renderer awareness
		if (normal_map)
			cmd += " --normal_map";
	}

	if (srgb)
		cmd += " --srgb";
	else
		cmd += " --linear";

	cmd += " -- ";
	cmd += temp_output.path;
	cmd += " ";
	cmd += temp_input.path;

	int rc = execute(cmd.c_str(), /* ignore_stdout= */ false, /* ignore_stderr= */ false);
	if (verbose)
		printf("%s => %d\n", cmd.c_str(), rc);

	return rc == 0 && readFile(temp_output.path.c_str(), result);
}
