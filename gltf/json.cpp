// This file is part of gltfpack; see gltfpack.h for version/license details
#include "gltfpack.h"

#include <stdio.h>

void comma(std::string& s)
{
	char ch = s.empty() ? 0 : s[s.size() - 1];

	if (ch != 0 && ch != '[' && ch != '{')
		s += ",";
}

void append(std::string& s, size_t v)
{
	char buf[32];
	sprintf(buf, "%zu", v);
	s += buf;
}

void append(std::string& s, float v)
{
	char buf[512];
	sprintf(buf, "%.9g", v);
	s += buf;
}

void append(std::string& s, const char* v)
{
	s += v;
}

void append(std::string& s, const std::string& v)
{
	s += v;
}