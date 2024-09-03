#include "../src/meshoptimizer.h"

#include <vector>

#ifdef METIS
#include <metis.h>
#endif

struct Vertex
{
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;
};

double timestamp();

void nanite(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	(void)vertices;
	(void)indices;
}
