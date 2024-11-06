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

	unsigned int res[999];

	meshopt_simplify(res, ib, indices, vb[0], 100, sizeof(float) * 4, 0, 1e-1f, 0, NULL);
	meshopt_simplify(res, ib, indices, vb[0], 100, sizeof(float) * 4, 0, FLT_MAX, 0, NULL);
	meshopt_simplify(res, ib, indices, vb[0], 100, sizeof(float) * 4, 0, FLT_MAX, meshopt_SimplifyLockBorder, NULL);
	meshopt_simplify(res, ib, indices, vb[0], 100, sizeof(float) * 4, 0, FLT_MAX, meshopt_SimplifySparse, NULL);
	meshopt_simplify(res, ib, indices, vb[0], 100, sizeof(float) * 4, 0, FLT_MAX, meshopt_SimplifyPrune, NULL);

	float aw = 1;
	meshopt_simplifyWithAttributes(res, ib, indices, vb[0], 100, sizeof(float) * 4, vb[0] + 3, sizeof(float) * 4, &aw, 1, NULL, 0, FLT_MAX, 0, NULL);

	return 0;
}
