#include "../src/meshoptimizer.h"

#include <vector>

#include <time.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
double timestamp()
{
	return double(clock()) / double(CLOCKS_PER_SEC);
}
#else
double timestamp()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
}
#endif

struct Vertex
{
	uint16_t data[16];
};

uint32_t murmur3(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6bu;
	h ^= h >> 13;
	h *= 0xc2b2ae35u;
	h ^= h >> 16;

	return h;
}

void bench(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	std::vector<Vertex> vb(vertices.size());
	std::vector<unsigned int> ib(indices.size());

	std::vector<unsigned char> vc(meshopt_encodeVertexBufferBound(vertices.size(), sizeof(Vertex)));
	std::vector<unsigned char> ic(meshopt_encodeIndexBufferBound(indices.size(), vertices.size()));

	printf("source: vertex data %d bytes, index data %d bytes\n", int(vertices.size() * sizeof(Vertex)), int(indices.size() * 4));

	for (int pass = 0; pass < 2; ++pass)
	{
		if (pass == 1)
			meshopt_optimizeVertexCacheStrip(&ib[0], &indices[0], indices.size(), vertices.size());
		else
			meshopt_optimizeVertexCache(&ib[0], &indices[0], indices.size(), vertices.size());

		meshopt_optimizeVertexFetch(&vb[0], &ib[0], indices.size(), &vertices[0], vertices.size(), sizeof(Vertex));

		vc.resize(vc.capacity());
		vc.resize(meshopt_encodeVertexBuffer(&vc[0], vc.size(), &vb[0], vertices.size(), sizeof(Vertex)));

		ic.resize(ic.capacity());
		ic.resize(meshopt_encodeIndexBuffer(&ic[0], ic.size(), &ib[0], indices.size()));

		printf("pass %d: vertex data %d bytes, index data %d bytes\n", pass, int(vc.size()), int(ic.size()));

		for (int attempt = 0; attempt < 10; ++attempt)
		{
			double t0 = timestamp();

			int rv = meshopt_decodeVertexBuffer(&vb[0], vertices.size(), sizeof(Vertex), &vc[0], vc.size());
			assert(rv == 0);
			(void)rv;

			double t1 = timestamp();

			int ri = meshopt_decodeIndexBuffer(&ib[0], indices.size(), 4, &ic[0], ic.size());
			assert(ri == 0);
			(void)ri;

			double t2 = timestamp();

			double GB = 1024 * 1024 * 1024;

			printf("decode: vertex %.2f ms (%.2f GB/sec), index %.2f ms (%.2f GB/sec)\n",
				(t1 - t0) * 1000, double(vertices.size() * sizeof(Vertex)) / GB / (t1 - t0),
				(t2 - t1) * 1000, double(indices.size() * 4) / GB / (t2 - t1));
		}
	}
}

int main()
{
	meshopt_encodeIndexVersion(1);

	const int N = 1000;

	std::vector<Vertex> vertices;
	vertices.reserve((N + 1) * (N + 1));

	for (int x = 0; x <= N; ++x)
	{
		for (int y = 0; y <= N; ++y)
		{
			Vertex v;

			for (int k = 0; k < 16; ++k)
			{
				uint32_t h = murmur3((x * (N + 1) + y) * 16 + k);

				// use random k-bit sequence for each word to test all encoding types
				// note: this doesn't stress the sentinel logic too much but it's all branchless so it's probably fine?
				v.data[k] = h & ((1 << k) - 1);
			}

			vertices.push_back(v);
		}
	}

	std::vector<unsigned int> indices;
	indices.reserve(N * N * 6);

	for (int x = 0; x < N; ++x)
	{
		for (int y = 0; y < N; ++y)
		{
			indices.push_back((x + 0) * N + (y + 0));
			indices.push_back((x + 1) * N + (y + 0));
			indices.push_back((x + 0) * N + (y + 1));

			indices.push_back((x + 0) * N + (y + 1));
			indices.push_back((x + 1) * N + (y + 0));
			indices.push_back((x + 1) * N + (y + 1));
		}
	}

	bench(vertices, indices);
}
