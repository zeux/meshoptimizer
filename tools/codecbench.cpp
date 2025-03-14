#include "../src/meshoptimizer.h"

#include <vector>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

double timestamp()
{
	return emscripten_get_now() * 1e-3;
}
#elif defined(_WIN32)
struct LARGE_INTEGER
{
	__int64 QuadPart;
};
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

double timestamp()
{
	LARGE_INTEGER freq, counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	return double(counter.QuadPart) / double(freq.QuadPart);
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

void benchCodecs(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, double& bestvd, double& bestid, bool verbose)
{
	std::vector<Vertex> vb(vertices.size());
	std::vector<unsigned int> ib(indices.size());

	std::vector<unsigned char> vc(meshopt_encodeVertexBufferBound(vertices.size(), sizeof(Vertex)));
	std::vector<unsigned char> ic(meshopt_encodeIndexBufferBound(indices.size(), vertices.size()));

	if (verbose)
		printf("source: vertex data %d bytes, index data %d bytes\n", int(vertices.size() * sizeof(Vertex)), int(indices.size() * 4));

	for (int pass = 0; pass < (verbose ? 2 : 1); ++pass)
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

		if (verbose)
			printf("pass %d: vertex data %d bytes, index data %d bytes\n", pass, int(vc.size()), int(ic.size()));

		for (int attempt = 0; attempt < 50; ++attempt)
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

			if (verbose)
				printf("decode: vertex %.2f ms (%.2f GB/sec), index %.2f ms (%.2f GB/sec)\n",
				    (t1 - t0) * 1000, double(vertices.size() * sizeof(Vertex)) / GB / (t1 - t0),
				    (t2 - t1) * 1000, double(indices.size() * 4) / GB / (t2 - t1));

			if (pass == 0)
			{
				bestvd = std::max(bestvd, double(vertices.size() * sizeof(Vertex)) / GB / (t1 - t0));
				bestid = std::max(bestid, double(indices.size() * 4) / GB / (t2 - t1));
			}
		}
	}
}

void benchFilters(size_t count, double& besto8, double& besto12, double& bestq12, double& bestexp, bool verbose)
{
	// note: the filters are branchless so we just run them on runs of zeroes
	size_t count4 = (count + 3) & ~3;
	std::vector<unsigned char> d4(count4 * 4);
	std::vector<unsigned char> d8(count4 * 8);

	if (verbose)
		printf("filters: oct8 data %d bytes, oct12/quat12 data %d bytes\n", int(d4.size()), int(d8.size()));

	for (int attempt = 0; attempt < 50; ++attempt)
	{
		double t0 = timestamp();

		meshopt_decodeFilterOct(&d4[0], count4, 4);

		double t1 = timestamp();

		meshopt_decodeFilterOct(&d8[0], count4, 8);

		double t2 = timestamp();

		meshopt_decodeFilterQuat(&d8[0], count4, 8);

		double t3 = timestamp();

		meshopt_decodeFilterExp(&d8[0], count4, 8);

		double t4 = timestamp();

		double GB = 1024 * 1024 * 1024;

		if (verbose)
			printf("filter: oct8 %.2f ms (%.2f GB/sec), oct12 %.2f ms (%.2f GB/sec), quat12 %.2f ms (%.2f GB/sec), exp %.2f ms (%.2f GB/sec)\n",
			    (t1 - t0) * 1000, double(d4.size()) / GB / (t1 - t0),
			    (t2 - t1) * 1000, double(d8.size()) / GB / (t2 - t1),
			    (t3 - t2) * 1000, double(d8.size()) / GB / (t3 - t2),
			    (t4 - t3) * 1000, double(d8.size()) / GB / (t4 - t3));

		besto8 = std::max(besto8, double(d4.size()) / GB / (t1 - t0));
		besto12 = std::max(besto12, double(d8.size()) / GB / (t2 - t1));
		bestq12 = std::max(bestq12, double(d8.size()) / GB / (t3 - t2));
		bestexp = std::max(bestexp, double(d8.size()) / GB / (t4 - t3));
	}
}

struct File
{
	std::vector<unsigned char> v0;
	std::vector<unsigned char> v1;
	size_t stride;
	size_t count;
};

File readFile(const char* path)
{
	FILE* file = fopen(path, "rb");
	assert(file);

	const char* name = strrchr(path, '/');
	name = name ? name + 1 : path;

	int vcnt, vsz;
	int sr = sscanf(name, "v%d_s%d_", &vcnt, &vsz);
	assert(sr == 2);
	(void)sr;

	std::vector<unsigned char> input;
	unsigned char buffer[4096];
	size_t bytes_read;

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
		input.insert(input.end(), buffer, buffer + bytes_read);

	fclose(file);

	File result = {};
	result.count = vcnt;
	result.stride = vsz;

	std::vector<unsigned char> decoded(result.count * result.stride);
	int res = meshopt_decodeVertexBuffer(&decoded[0], result.count, result.stride, &input[0], input.size());
	if (res != 0 && input.size() == decoded.size())
	{
		// some files are not encoded
		memcpy(decoded.data(), input.data(), decoded.size());
	}

	meshopt_encodeVertexVersion(0);
	result.v0.resize(meshopt_encodeVertexBufferBound(result.count, result.stride));
	result.v0.resize(meshopt_encodeVertexBuffer(result.v0.data(), result.v0.size(), decoded.data(), result.count, result.stride));

	meshopt_encodeVertexVersion(1);
	result.v1.resize(meshopt_encodeVertexBufferBound(result.count, result.stride));
	result.v1.resize(meshopt_encodeVertexBuffer(result.v1.data(), result.v1.size(), decoded.data(), result.count, result.stride));

	return result;
}

int main(int argc, char** argv)
{
	meshopt_encodeIndexVersion(1);

	bool verbose = false;
	bool loop = false;
	bool inputs = false;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-v") == 0)
			verbose = true;
		if (strcmp(argv[i], "-l") == 0)
			loop = true;
		if (argv[i][0] != '-')
			inputs = true;
	}

	if (inputs)
	{
		std::vector<File> files;
		for (int i = 1; i < argc; ++i)
			if (argv[i][0] != '-')
				files.push_back(readFile(argv[i]));

		size_t max_size = 0;
		size_t total_size = 0;
		size_t total_v0 = 0, total_v1 = 0;
		for (size_t i = 0; i < files.size(); ++i)
		{
			max_size = std::max(max_size, files[i].count * files[i].stride);
			total_size += files[i].count * files[i].stride;
			total_v0 += files[i].v0.size();
			total_v1 += files[i].v1.size();
		}

		std::vector<unsigned char> buffer(max_size);

		printf("Algorithm   :\tvtx0\tvtx1\n");
		printf("Size (MB)   :\t%.2f\t%.2f\n", double(total_v0) / 1024 / 1024, double(total_v1) / 1024 / 1024);
		printf("Ratio       :\t%.2f\t%.2f\n", double(total_v0) / double(total_size), double(total_v1) / double(total_size));

		for (int l = 0; l < (loop ? 100 : 1); ++l)
		{
			double bestvd0 = 0, bestvd1 = 0;

			for (int attempt = 0; attempt < 50; ++attempt)
			{
				double t0 = timestamp();

				for (size_t i = 0; i < files.size(); ++i)
				{
					int rv = meshopt_decodeVertexBuffer(&buffer[0], files[i].count, files[i].stride, &files[i].v0[0], files[i].v0.size());
					assert(rv == 0);
					(void)rv;
				}

				double t1 = timestamp();

				for (size_t i = 0; i < files.size(); ++i)
				{
					int rv = meshopt_decodeVertexBuffer(&buffer[0], files[i].count, files[i].stride, &files[i].v1[0], files[i].v1.size());
					assert(rv == 0);
					(void)rv;
				}

				double t2 = timestamp();

				double GB = 1024 * 1024 * 1024;

				bestvd0 = std::max(bestvd0, double(total_size) / GB / (t1 - t0));
				bestvd1 = std::max(bestvd1, double(total_size) / GB / (t2 - t1));
			}

			printf("Score (GB/s):\t%.2f\t%.2f\n", bestvd0, bestvd1);
		}
		return 0;
	}

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
				v.data[k] = h & ((1 << (k + 1)) - 1);
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

	printf("Algorithm   :\tvtx0\tvtx1\tidx\toct8\toct12\tquat12\texp\n");

	for (int l = 0; l < (loop ? 100 : 1); ++l)
	{
		meshopt_encodeVertexVersion(0);

		double bestvd0 = 0, bestid = 0;
		benchCodecs(vertices, indices, bestvd0, bestid, verbose);

		meshopt_encodeVertexVersion(1);

		double bestvd1 = 0, bestidr = 0;
		benchCodecs(vertices, indices, bestvd1, bestidr, verbose);

		double besto8 = 0, besto12 = 0, bestq12 = 0, bestexp = 0;
		benchFilters(8 * N * N, besto8, besto12, bestq12, bestexp, verbose);

		printf("Score (GB/s):\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
		    bestvd0, bestvd1, bestid, besto8, besto12, bestq12, bestexp);
	}
}
