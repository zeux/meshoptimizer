#include "../src/meshoptimizer.h"

#include <float.h>
#include <stdint.h>
#include <stdlib.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* buffer, size_t size)
{
	if (size == 0)
		return 0;

	srand(buffer[0]);

	float vb[100][4];

	for (int i = 0; i < 100; ++i)
	{
		vb[i][0] = rand() % 10;
		vb[i][1] = rand() % 10;
		vb[i][2] = rand() % 10;
		vb[i][3] = rand() % 10;
	}

	unsigned int ib[999];
	int indices = (size - 1) < 999 ? (size - 1) / 3 * 3 : 999;

	for (int i = 0; i < indices; ++i)
		ib[i] = buffer[1 + i] % 100;

	meshopt_Meshlet ml[333];
	unsigned int mv[333 * 3];
	unsigned char mt[333 * 4];

	meshopt_buildMeshlets(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 4, /* max_triangles= */ 4, 0.f);

	meshopt_buildMeshletsScan(ml, mv, mt, ib, indices, 100, /* max_vertices= */ 4, /* max_triangles= */ 4);
	meshopt_buildMeshletsScan(ml, mv, mt, ib, indices, 100, /* max_vertices= */ 4, /* max_triangles= */ 8);

	meshopt_buildMeshletsFlex(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 4, /* min_triangles= */ 4, /* max_triangles= */ 8, 0.f, 2.f);
	meshopt_buildMeshletsFlex(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 8, /* min_triangles= */ 8, /* max_triangles= */ 16, 0.f, 2.f);
	meshopt_buildMeshletsFlex(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 16, /* min_triangles= */ 8, /* max_triangles= */ 32, 0.f, 2.f);
	meshopt_buildMeshletsFlex(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 16, /* min_triangles= */ 16, /* max_triangles= */ 32, 0.f, 2.f);

	meshopt_buildMeshletsSpatial(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 4, /* min_triangles= */ 4, /* max_triangles= */ 8, 0.f);
	meshopt_buildMeshletsSpatial(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 8, /* min_triangles= */ 4, /* max_triangles= */ 32, 0.f);
	meshopt_buildMeshletsSpatial(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 8, /* min_triangles= */ 8, /* max_triangles= */ 32, 0.f);
	meshopt_buildMeshletsSpatial(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 8, /* min_triangles= */ 12, /* max_triangles= */ 32, 0.f);
	meshopt_buildMeshletsSpatial(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 16, /* min_triangles= */ 16, /* max_triangles= */ 32, 0.f);
	meshopt_buildMeshletsSpatial(ml, mv, mt, ib, indices, vb[0], 100, sizeof(float) * 4, /* max_vertices= */ 16, /* min_triangles= */ 32, /* max_triangles= */ 32, 0.f);

	return 0;
}
