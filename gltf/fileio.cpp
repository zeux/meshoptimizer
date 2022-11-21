// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

std::string getTempPrefix()
{
#if defined(_WIN32)
	const char* temp_dir = getenv("TEMP");
	std::string path = temp_dir ? temp_dir : ".";
	path += "\\gltfpack-temp";
	path += std::to_string(_getpid());
	return path;
#elif defined(__wasi__)
	return "gltfpack-temp";
#else
	std::string path = "/tmp/gltfpack-temp";
	path += std::to_string(getpid());
	return path;
#endif
}

std::string getFullPath(const char* path, const char* base_path)
{
	std::string result = base_path;

	std::string::size_type slash = result.find_last_of("/\\");
	result.erase(slash == std::string::npos ? 0 : slash + 1);

	result += path;

	return result;
}

std::string getFileName(const char* path)
{
	std::string result = path;

	std::string::size_type slash = result.find_last_of("/\\");
	if (slash != std::string::npos)
		result.erase(0, slash + 1);

	std::string::size_type dot = result.find_last_of('.');
	if (dot != std::string::npos)
		result.erase(dot);

	return result;
}

std::string getExtension(const char* path)
{
	std::string result = path;

	std::string::size_type slash = result.find_last_of("/\\");
	std::string::size_type dot = result.find_last_of('.');

	if (slash != std::string::npos && dot != std::string::npos && dot < slash)
		dot = std::string::npos;

	result.erase(0, dot);

	for (size_t i = 0; i < result.length(); ++i)
		if (unsigned(result[i] - 'A') < 26)
			result[i] = (result[i] - 'A') + 'a';

	return result;
}

bool readFile(const char* path, std::string& data)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return false;

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (length <= 0)
	{
		fclose(file);
		return false;
	}

	data.resize(length);
	size_t result = fread(&data[0], 1, data.size(), file);
	int rc = fclose(file);

	return rc == 0 && result == data.size();
}

bool writeFile(const char* path, const std::string& data)
{
	FILE* file = fopen(path, "wb");
	if (!file)
		return false;

	size_t result = fwrite(&data[0], 1, data.size(), file);
	int rc = fclose(file);

	return rc == 0 && result == data.size();
}

void removeFile(const char* path)
{
	remove(path);
}
