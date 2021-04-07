// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

const char* inferMimeType(const char* path)
{
	std::string ext = getExtension(path);

	for (size_t i = 0; i < sizeof(kMimeTypes) / sizeof(kMimeTypes[0]); ++i)
		if (ext == kMimeTypes[i][1])
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

#ifdef __wasi__
static int execute(const char* cmd, bool ignore_stdout, bool ignore_stderr)
{
	return system(cmd);
}

static std::string getExecutable(const char* name, const char* env)
{
	return name;
}
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

static std::string getExecutable(const char* name, const char* env)
{
	const char* path = getenv(env);
	path = path ? path : name;

#ifdef _WIN32
	// when the executable path contains a space, we need to quote the path ourselves
	if (path[0] != '"' && strchr(path, ' '))
		return '"' + std::string(path) + '"';
#endif

	return path;
}
#endif

bool checkBasis(bool verbose)
{
	std::string cmd = getExecutable("basisu", "BASISU_PATH");

	cmd += " -version";

	int rc = execute(cmd.c_str(), /* ignore_stdout= */ !verbose, /* ignore_stderr= */ !verbose);
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

	std::string cmd = getExecutable("basisu", "BASISU_PATH");

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
	std::string cmd = getExecutable("toktx", "TOKTX_PATH");

	cmd += " --version";

	int rc = execute(cmd.c_str(), /* ignore_stdout= */ !verbose, /* ignore_stderr= */ !verbose);
	if (verbose)
		printf("%s => %d\n", cmd.c_str(), rc);

	return rc == 0;
}

static int readInt16(const std::string& data, size_t offset)
{
	return (unsigned char)data[offset] * 256 + (unsigned char)data[offset + 1];
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

	width = readInt16(data, 18);
	height = readInt16(data, 22);

	return true;
}

static bool getDimensionsJpeg(const std::string& data, int& width, int& height)
{
	size_t offset = 0;

	// note, this can stop parsing before reaching the end but we stop at SOF anyway
	while (offset + 4 <= data.size())
	{
		if (data[offset] != '\xff')
			return false;

		char marker = data[offset + 1];

		if (marker == '\xff')
		{
			offset++;
			continue; // padding
		}

		// d0..d9 correspond to SOI, RSTn, EOI
		if (marker == 0 || unsigned(marker - '\xd0') <= 9)
		{
			offset += 2;
			continue; // no payload
		}

		// c0..c1 correspond to SOF0, SOF1
		if (marker == '\xc0' || marker == '\xc2')
		{
			if (offset + 10 > data.size())
				return false;

			width = readInt16(data, offset + 7);
			height = readInt16(data, offset + 5);

			return true;
		}

		offset += 2 + readInt16(data, offset + 2);
	}

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

static int roundBlock(int value, bool pow2)
{
	if (value == 0)
		return 4;

	if (pow2 && value > 4)
		return roundPow2(value);

	return (value + 3) & ~3;
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

	std::string cmd = getExecutable("toktx", "TOKTX_PATH");

	const BasisSettings& bs = kBasisSettings[quality <= 0 ? 0 : quality > 9 ? 9 : quality - 1];

	cmd += " --t2";
	cmd += " --2d";
	cmd += " --genmipmap";
	cmd += " --nowarn";

	int newWidth = roundBlock(int(width * scale), pow2);
	int newHeight = roundBlock(int(height * scale), pow2);

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
