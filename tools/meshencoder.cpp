// Converts .obj files to .optmesh files
// Usage: meshencoder [.obj] [.optmesh]

// Data layout:
// Header: 32b
// Material table: 4b * material_count
// Material data
// Object table: 16b * object_count
// Vertex data
// Index data

#include "../demo/objparser.h"
#include "../src/meshoptimizer.h"

#include <algorithm>
#include <vector>

#include <math.h>
#include <stdio.h>
#include <string.h>

struct Header
{
	char magic[4]; // OPTM

	unsigned int material_count;
	unsigned int object_count;

	unsigned int vertex_count;
	unsigned int index_count;
	unsigned int vertex_data_size;
	unsigned int index_data_size;

	float scale;
};

struct Material
{
	unsigned int name_length;
};

struct Object
{
	unsigned int index_offset;
	unsigned int index_count;
	unsigned int material_index;
	unsigned int reserved;
};

struct Vertex
{
	short px, py, pz, pw; // normalized signed 16-bit value
	char nx, ny, nz, nw; // normalized signed 8-bit value
	unsigned short tx, ty; // normalized unsigned 16-bit value
};

int main(int argc, char** argv)
{
	if (argc <= 2)
	{
		printf("Usage: %s [.obj] [.optmesh]\n", argv[0]);
		return 1;
	}

	const char* input = argv[1];
	const char* output = argv[2];

	ObjFile file;

	if (!objParseFile(file, input))
	{
		printf("Error loading %s: file not found\n", input);
		return 2;
	}

	if (!objValidate(file))
	{
		printf("Error loading %s: invalid file data\n", input);
		return 3;
	}

	float scale = 0.f;

	for (size_t i = 0; i < file.v_size; ++i)
		scale = std::max(scale, fabsf(file.v[i]));

	float scale_inverse = (scale == 0.f) ? 0.f : 1 / scale;

	size_t total_indices = file.f_size / 3;

	std::vector<Vertex> triangles(total_indices);

	int pos_precision = 0xfffc; // 14 bits
	int uv_precision = 0xfff0; // 12 bits

	for (size_t i = 0; i < total_indices; ++i)
	{
		int vi = file.f[i * 3 + 0];
		int vti = file.f[i * 3 + 1];
		int vni = file.f[i * 3 + 2];

		Vertex v =
		    {
		        short(meshopt_quantizeUnorm(file.v[vi * 3 + 0] * scale_inverse, 16) & pos_precision),
		        short(meshopt_quantizeUnorm(file.v[vi * 3 + 1] * scale_inverse, 16) & pos_precision),
		        short(meshopt_quantizeUnorm(file.v[vi * 3 + 2] * scale_inverse, 16) & pos_precision),
				0x7fff,

		        char(meshopt_quantizeSnorm(vni >= 0 ? file.vn[vni * 3 + 0] : 0, 8)),
		        char(meshopt_quantizeSnorm(vni >= 0 ? file.vn[vni * 3 + 1] : 0, 8)),
		        char(meshopt_quantizeSnorm(vni >= 0 ? file.vn[vni * 3 + 2] : 0, 8)),
				0,

		        (unsigned short)(meshopt_quantizeUnorm(vti >= 0 ? file.vt[vti * 3 + 0] : 0, 16) & uv_precision),
		        (unsigned short)(meshopt_quantizeUnorm(vti >= 0 ? file.vt[vti * 3 + 1] : 0, 16) & uv_precision),
		    };

		triangles[i] = v;
	}

	std::vector<unsigned int> remap(total_indices);

	size_t total_vertices = meshopt_generateVertexRemap(&remap[0], NULL, total_indices, &triangles[0], total_indices, sizeof(Vertex));

	std::vector<unsigned int> indices(total_indices);
	meshopt_remapIndexBuffer(&indices[0], NULL, total_indices, &remap[0]);

	std::vector<Vertex> vertices(total_vertices);
	meshopt_remapVertexBuffer(&vertices[0], &triangles[0], total_indices, sizeof(Vertex), &remap[0]);

	meshopt_optimizeVertexCache(&indices[0], &indices[0], indices.size(), vertices.size());
	meshopt_optimizeVertexFetch(&vertices[0], &indices[0], indices.size(), &vertices[0], vertices.size(), sizeof(Vertex));

	std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(vertices.size(), sizeof(Vertex)));
	vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), &vertices[0], vertices.size(), sizeof(Vertex)));

	std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(indices.size(), vertices.size()));
	ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), &indices[0], indices.size()));

	FILE* result = fopen(output, "wb");
	if (!result)
	{
		printf("Error saving %s: can't open file for writing\n", output);
		return 4;
	}

	Header header = {};
	memcpy(header.magic, "OPTM", 4);

	header.material_count = 0;
	header.object_count = 1;

	header.vertex_count = unsigned(vertices.size());
	header.index_count = unsigned(indices.size());
	header.vertex_data_size = unsigned(vbuf.size());
	header.index_data_size = unsigned(ibuf.size());

	header.scale = scale;

	fwrite(&header, 1, sizeof(header), result);

	Object object = {};
	object.index_offset = 0;
	object.index_count = indices.size();
	object.material_index = 0;

	fwrite(&object, 1, sizeof(object), result);

	fwrite(&vbuf[0], 1, vbuf.size(), result);
	fwrite(&ibuf[0], 1, ibuf.size(), result);
	fclose(result);

	return 0;
}
