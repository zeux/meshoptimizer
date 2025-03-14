#include "../src/meshoptimizer.h"

#include <vector>

#include <math.h>
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
	double count;
	double size;

	double ratio_v0;
	double ratio_v1;
	double ratio_v1_lz4;
	double ratio_v1_zstd;

	double total_v0;
	double total_v1;
	double total_v1_lz4;
	double total_v1_zstd;
};

void testFile(FILE* file, size_t count, size_t stride, int level, Stats* stats = 0)
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
		// some files are not encoded; encode them with v0 to let the rest of the flow proceed as is
		memcpy(decoded.data(), input.data(), decoded.size());
		input.resize(meshopt_encodeVertexBufferBound(count, stride));
		meshopt_encodeVertexVersion(0);
		input.resize(meshopt_encodeVertexBuffer(input.data(), input.size(), decoded.data(), count, stride));
	}
	else if (res != 0)
	{
		printf(" error decoding input: %d", res);
		return;
	}

	std::vector<unsigned char> output(meshopt_encodeVertexBufferBound(count, stride));
	meshopt_encodeVertexVersion(1);
	output.resize(meshopt_encodeVertexBufferLevel(output.data(), output.size(), decoded.data(), count, stride, level));

	printf(" raw %zu KB\t", decoded.size() / 1024);
	printf(" v0 %.3f", double(input.size()) / double(decoded.size()));
	printf(" v1 %.3f", double(output.size()) / double(decoded.size()));

	if (stats)
	{
		stats->count++;
		stats->size += double(decoded.size());

		stats->total_v0 += double(input.size());
		stats->total_v1 += double(output.size());

		stats->ratio_v0 += log(double(input.size()) / double(decoded.size()));
		stats->ratio_v1 += log(double(output.size()) / double(decoded.size()));
	}

	if (stats && stats->testz)
	{
		size_t decoded_lz4 = measure_lz4(decoded);
		size_t output_lz4 = measure_lz4(output);
		size_t decoded_zstd = measure_zstd(decoded);
		size_t output_zstd = measure_zstd(output);

		stats->total_v1_lz4 += output_lz4;
		stats->total_v1_zstd += output_zstd;

		stats->ratio_v1_lz4 += log(double(output_lz4) / double(decoded.size()));
		stats->ratio_v1_zstd += log(double(output_zstd) / double(decoded.size()));

		printf("\tlz4 %.3f", double(decoded_lz4) / double(decoded.size()));
		printf(" v1+lz4 %.3f", double(output_lz4) / double(decoded.size()));

		printf("\tzstd %.3f", double(decoded_zstd) / double(decoded.size()));
		printf(" v1+zstd %.3f", double(output_zstd) / double(decoded.size()));
	}
}

void testFile(const char* path, int level, Stats* stats = 0)
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

	testFile(file, vcnt, vsz, level, stats);
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
		int level = -1;
		if (argc > 2 && argv[2][0] == '-' && argv[2][1] >= '0' && argv[2][1] <= '9')
			level = atoi(argv[2] + 1);
		for (int i = level < 0 ? 2 : 3; i < argc; ++i)
			testFile(argv[i], level < 0 ? 2 : level, &stats);

		printf("---\n");
		printf("%d files: v0 %.3f, v1 %.3f",
		    int(stats.count),
		    exp(stats.ratio_v0 / stats.count), exp(stats.ratio_v1 / stats.count));
		if (stats.testz)
			printf(", v1+lz4 %.3f, v1+zstd %.3f\n", exp(stats.ratio_v1_lz4 / stats.count), exp(stats.ratio_v1_zstd / stats.count));
		else
			printf("\n");

		printf("total: %d files, raw %.2f MB, v0 %.2f MB, v1 %.2f MB",
		    int(stats.count),
		    stats.size / 1024 / 1024, stats.total_v0 / 1024 / 1024, stats.total_v1 / 1024 / 1024);
		if (stats.testz)
			printf(", v1+lz4 %.2f MB, v1+zstd %.2f MB\n", stats.total_v1_lz4 / 1024 / 1024, stats.total_v1_zstd / 1024 / 1024);
		else
			printf("\n");

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
		meshopt_encodeVertexVersion(1);
		size_t vertex_count = input.size() / stride;
		std::vector<unsigned char> output(meshopt_encodeVertexBufferBound(vertex_count, stride));
		size_t output_size = meshopt_encodeVertexBuffer(output.data(), output.size(), input.data(), vertex_count, stride);

		fwrite(output.data(), 1, output_size, stdout);
		return 0;
	}
}
