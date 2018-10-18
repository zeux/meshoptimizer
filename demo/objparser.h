#pragma once

#include <stddef.h>

class ObjFile
{
public:
	float* v; // positions; stride 3 (xyz)
	size_t v_size, v_cap;

	float* vt; // texture coordinates; stride 3 (uvw)
	size_t vt_size, vt_cap;

	float* vn; // vertex normals; stride 3 (xyz)
	size_t vn_size, vn_cap;

	int* f; // face elements; stride 9 (3 groups of indices into v/vt/vn)
	size_t f_size, f_cap;

	ObjFile();
	~ObjFile();

private:
	ObjFile(const ObjFile&);
	ObjFile& operator=(const ObjFile&);
};

void objParseLine(ObjFile& result, const char* line);
bool objParseFile(ObjFile& result, const char* path);

bool objValidate(const ObjFile& result);