#pragma once

#include <vector>

struct ObjFile
{
	std::vector<float> v; // positions; stride 3 (xyz)
	std::vector<float> vt; // texture coordinates; stride 3 (uvw)
	std::vector<float> vn; // vertex normals; stride 3 (xyz)

	std::vector<char> fv; // face vertex count
	std::vector<int> f; // face elements; stride defined by fv (*3 since f contains indices into v/vt/vn)
};

void objParseLine(ObjFile& result, const char* line);
bool objParseFile(ObjFile& result, const char* path);

bool objValidate(const ObjFile& result);
void objTriangulate(ObjFile& result);
