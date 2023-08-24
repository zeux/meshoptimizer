// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

unsigned short meshopt_quantizeHalf(float v)
{
	union { float f; unsigned int ui; } u = {v};
	unsigned int ui = u.ui;

	int s = (ui >> 16) & 0x8000;
	int em = ui & 0x7fffffff;

	/* bias exponent and round to nearest; 112 is relative exponent bias (127-15) */
	int h = (em - (112 << 23) + (1 << 12)) >> 13;

	/* underflow: flush to zero; 113 encodes exponent -14 */
	h = (em < (113 << 23)) ? 0 : h;

	/* overflow: infinity; 143 encodes exponent 16 */
	h = (em >= (143 << 23)) ? 0x7c00 : h;

	/* NaN; note that we convert all types of NaN to qNaN */
	h = (em > (255 << 23)) ? 0x7e00 : h;

	return (unsigned short)(s | h);
}

float meshopt_quantizeFloat(float v, int N)
{
	union { float f; unsigned int ui; } u = {v};
	unsigned int ui = u.ui;

	const int mask = (1 << (23 - N)) - 1;
	const int round = (1 << (23 - N)) >> 1;

	int e = ui & 0x7f800000;
	unsigned int rui = (ui + round) & ~mask;

	/* round all numbers except inf/nan; this is important to make sure nan doesn't overflow into -0 */
	ui = e == 0x7f800000 ? ui : rui;

	/* flush denormals to zero */
	ui = e == 0 ? 0 : ui;

	u.ui = ui;
	return u.f;
}
