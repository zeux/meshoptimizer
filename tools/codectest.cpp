#include "../src/meshoptimizer.h"

#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

size_t measure(const char* cmd, const std::vector<unsigned char>& data)
{
	FILE* file = fopen("/tmp/codectest.in", "wb");
	if (!file)
		return 0;
	fwrite(data.data(), data.size(), 1, file);
	fclose(file);

	char actualcmd[1024];
	snprintf(actualcmd, sizeof(actualcmd), cmd, "/tmp/codectest.in", "/tmp/codectest.out");

	int rc = system(actualcmd);
	if (rc)
		return 0;

	file = fopen("/tmp/codectest.out", "rb");
	if (!file)
		return 0;
	fseek(file, 0, SEEK_END);
	long result = ftell(file);
	fclose(file);

	return result;
}

size_t measure_lz4(const std::vector<unsigned char>& data)
{
	return measure("lz4 -f -q %s %s", data);
}

size_t measure_zstd(const std::vector<unsigned char>& data)
{
	return measure("zstd -f -q %s -o %s", data);
}

struct Stats
{
	bool testz;
	double v10_raw;
	double v10_lz4;
	double v10_zstd;
	double count;
};

inline unsigned char zigzag8(unsigned char v)
{
	return ((signed char)(v) >> 7) ^ (v << 1);
}

inline unsigned short zigzag16(unsigned short v)
{
	return ((signed short)(v) >> 15) ^ (v << 1);
}

inline unsigned int zigzag32(unsigned int v)
{
	return ((signed int)(v) >> 31) ^ (v << 1);
}

void makedeltas(unsigned char* deltas, size_t count, size_t stride, const unsigned char* vertices, const int modes[64])
{
	unsigned char last[256] = {};
	unsigned char lastd[256] = {};

	for (size_t i = 0; i < count; ++i)
	{
		for (size_t j = 0; j < stride; j += 4)
		{
			int mode = modes[j / 4];

			int mode0 = mode % 3;        // 1/2/4 bytes
			int mode1 = (mode / 3) % 3;  // zigzag 1/2/0 times
			int mode2 = (mode / 9) % 2;  // sub/xor
			int mode3 = (mode / 18) % 3; // first/second order sub/xor

			// hardcode some values for testing
			// mode0 = 0;
			// mode1 = 0;
			// mode2 = 0;
			// mode3 = 0;

			if (mode0 == 0)
			{
				for (size_t k = 0; k < 4; ++k)
				{
					unsigned char& d = deltas[i * stride + j + k];

					d = vertices[i * stride + j + k];
					if (mode2 == 0)
						d -= last[j + k];
					else
						d ^= last[j + k];

					if (mode3)
					{
						unsigned char nd = d;

						if (mode3 == 1)
							d -= lastd[j + k];
						else
							d ^= lastd[j + k];

						lastd[j + k] = nd;
					}

					if (mode1 != 2)
						d = zigzag8(d);
					if (mode1 == 1)
						d = zigzag8(d);

					last[j + k] = vertices[i * stride + j + k];
				}
			}
			else if (mode0 == 1)
			{
				for (size_t k = 0; k < 4; k += 2)
				{
					unsigned short& d = *(unsigned short*)&deltas[i * stride + j + k];

					d = *(unsigned short*)&vertices[i * stride + j + k];

					if (mode2 == 0)
						d -= *(unsigned short*)&last[j + k];
					else
						d ^= *(unsigned short*)&last[j + k];

					if (mode3)
					{
						unsigned short nd = d;

						if (mode3)
							d -= *(unsigned short*)&lastd[j + k];
						else
							d ^= *(unsigned short*)&lastd[j + k];

						*(unsigned short*)&lastd[j + k] = nd;
					}

					if (mode1 != 2)
						d = zigzag16(d);
					if (mode1 == 1)
						d = zigzag16(d);

					*(unsigned short*)&last[j + k] = *(unsigned short*)&vertices[i * stride + j + k];
				}
			}
			else
			{
				unsigned int& d = *(unsigned int*)&deltas[i * stride + j];

				d = *(unsigned int*)&vertices[i * stride + j];

				if (mode2 == 0)
					d -= *(unsigned int*)&last[j];
				else
					d ^= *(unsigned int*)&last[j];

				if (mode3)
				{
					unsigned int nd = d;

					if (mode3 == 1)
						d -= *(unsigned int*)&lastd[j];
					else
						d ^= *(unsigned int*)&lastd[j];

					*(unsigned int*)&lastd[j] = nd;
				}

				if (mode1 != 2)
					d = zigzag32(d);
				if (mode1 == 1)
					d = zigzag32(d);

				*(unsigned int*)&last[j] = *(unsigned int*)&vertices[i * stride + j];
			}
		}
	}
}

void tunedeltas(unsigned char* output, size_t output_size, unsigned char* deltas, size_t count, size_t stride, const unsigned char* data, int* modes)
{
	memset(modes, 0, sizeof(int) * (stride / 4));

	size_t best_size = size_t(-1);

	for (size_t j = 0; j < stride / 4; ++j)
	{
		for (int mode = 0; mode < 3 * 3 * 2 * 3; ++mode)
		{
			int old_mode = modes[j];
			modes[j] = mode;

			makedeltas(deltas, count, stride, data, modes);
			size_t size = meshopt_encodeVertexBuffer(output, output_size, deltas, count, stride);

			if (size >= best_size)
			{
				modes[j] = old_mode;
			}
			else
			{
				best_size = size;
			}
		}
	}
}

void testFile(FILE* file, size_t count, size_t stride, Stats* stats = 0)
{
	std::vector<unsigned char> input;
	unsigned char buffer[4096];
	size_t bytes_read;

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
		input.insert(input.end(), buffer, buffer + bytes_read);

	std::vector<unsigned char> decoded(count * stride);
	int res = meshopt_decodeVertexBuffer(&decoded[0], count, stride, &input[0], input.size());
	if (res != 0 && input.size() == decoded.size())
	{
		// some files are not encoded; encode them with v1 to let the rest of the flow proceed as is
		memcpy(decoded.data(), input.data(), decoded.size());
		input.resize(meshopt_encodeVertexBufferBound(count, stride));
		input.resize(meshopt_encodeVertexBuffer(input.data(), input.size(), decoded.data(), count, stride));
	}

	std::vector<unsigned char> output(meshopt_encodeVertexBufferBound(count, stride));

	if (1)
	{
		meshopt_encodeVertexVersion(0xf);
		std::vector<unsigned char> deltas(count * stride);
		int deltamodes[64] = {};
		if (1)
			tunedeltas(output.data(), output.size(), &deltas[0], count, stride, &decoded[0], deltamodes);
		makedeltas(&deltas[0], count, stride, &decoded[0], deltamodes);
		output.resize(meshopt_encodeVertexBuffer(output.data(), output.size(), deltas.data(), count, stride));
	}
	else
	{
		meshopt_encodeVertexVersion(0xe);
		output.resize(meshopt_encodeVertexBuffer(output.data(), output.size(), decoded.data(), count, stride));
	}

	meshopt_encodeVertexVersion(0);

	printf(" raw %zu KB\t", decoded.size() / 1024);
	printf(" v0 %.2f", double(input.size()) / double(decoded.size()));
	printf(" v1 %.2f", double(output.size()) / double(decoded.size()));
	printf(" v1/v0 %+.1f%%", double(output.size()) / double(input.size()) * 100 - 100);

	if (stats)
	{
		stats->v10_raw += double(output.size()) / double(input.size()) - 1;
		stats->count++;
	}

	if (stats && stats->testz)
	{
		size_t decoded_lz4 = measure_lz4(decoded);
		size_t input_lz4 = measure_lz4(input);
		size_t output_lz4 = measure_lz4(output);
		size_t decoded_zstd = measure_zstd(decoded);
		size_t input_zstd = measure_zstd(input);
		size_t output_zstd = measure_zstd(output);

		printf("\tlz4 %.2f:", double(decoded_lz4) / double(decoded.size()));
		printf(" v0 %.2f", double(input_lz4) / double(decoded.size()));
		printf(" v1 %.2f", double(output_lz4) / double(decoded.size()));
		printf(" v1/v0 %+.1f%%", double(output_lz4) / double(input_lz4) * 100 - 100);

		printf("\tzstd %.2f:", double(decoded_zstd) / double(decoded.size()));
		printf(" v0 %.2f", double(input_zstd) / double(decoded.size()));
		printf(" v1 %.2f", double(output_zstd) / double(decoded.size()));
		printf(" v1/v0 %+.1f%%", double(output_zstd) / double(input_zstd) * 100 - 100);

		stats->v10_lz4 += double(output_lz4) / double(input_lz4) - 1;
		stats->v10_zstd += double(output_zstd) / double(input_zstd) - 1;
	}
}

void testFile(const char* path, Stats* stats = 0)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return;

	const char* name = strrchr(path, '/');
	name = name ? name + 1 : path;

	int vcnt, vsz;
	int sr = sscanf(name, "v%d_s%d_", &vcnt, &vsz);
	assert(sr == 2);
	(void)sr;

	const char* name0 = strchr(strchr(name, '_') + 1, '_') + 1;
	const char* name1 = strstr(name0, "_R");
	size_t namel = name1 ? name1 - name0 : strlen(name0);
	namel = namel > 25 ? 25 : namel;

#if TRACE
	printf("%.*s: %s\n", int(namel), name0, path);
#else
	printf("%25.*s:", int(namel), name0);
#endif

	testFile(file, vcnt, vsz, stats);
	printf("\n");

	fclose(file);
}

int main(int argc, char** argv)
{
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif

	if (argc > 1 && (strcmp(argv[1], "-test") == 0 || strcmp(argv[1], "-testz") == 0))
	{
		Stats stats = {};
		stats.testz = strcmp(argv[1], "-testz") == 0;
		for (int i = 2; i < argc; ++i)
			testFile(argv[i], &stats);
		printf("---\n");
		printf("%d files: raw v1/v0 %+.2f%%, lz4 v1/v0 %+.2f%%, zstd v1/v0 %+.2f%%\n",
		    int(stats.count), stats.v10_raw / stats.count * 100, stats.v10_lz4 / stats.count * 100, stats.v10_zstd / stats.count * 100);
		return 0;
	}

	if (argc < 2 || argc > 3 || atoi(argv[1]) <= 0)
	{
		fprintf(stderr, "Usage: %s <stride> [<count>]\n", argv[0]);
		return 1;
	}

	size_t stride = atoi(argv[1]);

	std::vector<unsigned char> input;
	unsigned char buffer[4096];
	size_t bytes_read;

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), stdin)) > 0)
		input.insert(input.end(), buffer, buffer + bytes_read);

	if (argc == 3)
	{
		// if count is specified, we assume input is meshopt-encoded and decode it first
		size_t count = atoi(argv[2]);

		std::vector<unsigned char> decoded(count * stride);
		int res = meshopt_decodeVertexBuffer(&decoded[0], count, stride, &input[0], input.size());
		if (res != 0)
		{
			fprintf(stderr, "Error decoding input: %d\n", res);
			return 2;
		}

		fwrite(decoded.data(), 1, decoded.size(), stdout);
		return 0;
	}
	else
	{
		size_t vertex_count = input.size() / stride;
		std::vector<unsigned char> output(meshopt_encodeVertexBufferBound(vertex_count, stride));
		size_t output_size = meshopt_encodeVertexBuffer(output.data(), output.size(), input.data(), vertex_count, stride);

		fwrite(output.data(), 1, output_size, stdout);
		return 0;
	}
}
