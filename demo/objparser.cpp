#include "objparser.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>

static int fixupIndex(int index, size_t size)
{
	return (index >= 0) ? index - 1 : size + index;
}

static const char* parseFace(const char* s, int& vi, int& vti, int& vni)
{
	while (*s == ' ' || *s == '\t')
		s++;

	if (unsigned(*s - '0') > 9 && *s != '-')
		return s;

	vi = strtol(s, (char**)&s, 10);

	if (*s != '/')
		return s;
	s++;

	if (*s != '/')
		vti = strtol(s, (char**)&s, 10);

	if (*s != '/')
		return s;
	s++;

	vni = strtol(s, (char**)&s, 10);

	return s;
}

bool objParseLine(ObjFile& result, const char* line)
{
	if (line[0] == 'v' && line[1] == ' ')
	{
		float x, y, z;
		float w = 1;

		int r = sscanf(line, "v %f %f %f %f", &x, &y, &z, &w);
		if (r != 3 && r != 4)
			return false;

		result.v.push_back(x);
		result.v.push_back(y);
		result.v.push_back(z);
		result.v.push_back(w);

		return true;
	}
	else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ')
	{
		float u, v;
		float w = 1;

		int r = sscanf(line, "vt %f %f %f", &u, &v, &w);
		if (r != 2 && r != 3)
			return false;

		result.vt.push_back(u);
		result.vt.push_back(v);
		result.vt.push_back(w);

		return true;
	}
	else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
	{
		float x, y, z;

		int r = sscanf(line, "vn %f %f %f", &x, &y, &z);
		if (r != 3)
			return false;

		result.vn.push_back(x);
		result.vn.push_back(y);
		result.vn.push_back(z);

		return true;
	}
	else if (line[0] == 'f' && line[1] == ' ')
	{
		const char* s = line + 2;
		int fv = 0;

		size_t v = result.v.size() / 4;
		size_t vt = result.vt.size() / 3;
		size_t vn = result.vn.size() / 3;

		while (*s)
		{
			int vi = 0, vti = 0, vni = 0;
			s = parseFace(s, vi, vti, vni);

			if (vi == 0)
				break;

			result.f.push_back(fixupIndex(vi, v));
			result.f.push_back(fixupIndex(vti, vt));
			result.f.push_back(fixupIndex(vni, vn));

			fv++;
		}

		if (fv < 3)
			return false;

		result.fv.push_back(fv);

		return true;
	}
	else
	{
		// ignore unknown line
		return true;
	}
}

bool objParseFile(ObjFile& result, const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return false;

	bool success = true;

	char line[1024];
	while (fgets(line, sizeof(line), file))
	{
		if (!objParseLine(result, line))
		{
			success = false;
			break;
		}
	}

	fclose(file);
	return success;
}

void objTriangulate(ObjFile& result)
{
	size_t total_indices = 0;

	for (size_t face = 0; face < result.fv.size(); ++face)
	{
		int fv = result.fv[face];

		assert(fv >= 3);
		total_indices += (fv - 2) * 3;
	}

	std::vector<int> f(total_indices * 3);

	size_t read = 0;
	size_t write = 0;

	for (size_t face = 0; face < result.fv.size(); ++face)
	{
		int fv = result.fv[face];

		for (int i = 0; i + 2 < fv; ++i)
		{
			f[write + 0] = result.f[read + 0];
			f[write + 1] = result.f[read + 1];
			f[write + 2] = result.f[read + 2];

			f[write + 3] = result.f[read + i * 3 + 3];
			f[write + 4] = result.f[read + i * 3 + 4];
			f[write + 5] = result.f[read + i * 3 + 5];

			f[write + 6] = result.f[read + i * 3 + 6];
			f[write + 7] = result.f[read + i * 3 + 7];
			f[write + 8] = result.f[read + i * 3 + 8];

			write += 9;
		}

		read += fv * 3;
	}

	assert(read == result.f.size());
	assert(write == total_indices * 3);

	result.f.swap(f);
	std::vector<char>(total_indices / 3, 3).swap(result.fv);
}
