// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* kMimeTypes[][2] = {
    {"image/jpeg", ".jpg"},
    {"image/jpeg", ".jpeg"},
    {"image/png", ".png"},
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

bool checkBasis()
{
	const char* basisu_path = getenv("BASISU_PATH");
	std::string cmd = basisu_path ? basisu_path : "basisu";

	cmd += " -version";

#ifdef _WIN32
	cmd += " >nul 2>nul";
#else
	cmd += " >/dev/null 2>/dev/null";
#endif

	int rc = system(cmd.c_str());

	return rc == 0;
}

bool encodeBasis(const std::string& data, const char* mime_type, std::string& result, bool normal_map, bool srgb, int quality, bool uastc)
{
	TempFile temp_input(mimeExtension(mime_type));
	TempFile temp_output(".basis");

	if (!writeFile(temp_input.path.c_str(), data))
		return false;

	const char* basisu_path = getenv("BASISU_PATH");
	std::string cmd = basisu_path ? basisu_path : "basisu";

	char ql[16];
	sprintf(ql, "%d", (quality * 255 + 50) / 100);

	cmd += " -q ";
	cmd += ql;

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
		cmd += " -uastc";
	}

	cmd += " -file ";
	cmd += temp_input.path;
	cmd += " -output_file ";
	cmd += temp_output.path;

#ifdef _WIN32
	cmd += " >nul";
#else
	cmd += " >/dev/null";
#endif

	int rc = system(cmd.c_str());

	return rc == 0 && readFile(temp_output.path.c_str(), result);
}
