#include "../src/meshoptimizer.h"

#include <vector>

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifndef TRACE
#define TRACE 0
#endif

const unsigned char kVertexHeader = 0xa0;

static int gEncodeVertexVersion = 1;

const size_t kVertexBlockSizeBytes = 8192;
const size_t kVertexBlockMaxSize = 256;
const size_t kByteGroupSize = 16;
const size_t kByteGroupDecodeLimit = 24;
const size_t kTailMaxSize = 32;

static size_t getVertexBlockSize(size_t vertex_size)
{
	// make sure the entire block fits into the scratch buffer
	size_t result = kVertexBlockSizeBytes / vertex_size;

	// align to byte group size; we encode each byte as a byte group
	// if vertex block is misaligned, it results in wasted bytes, so just truncate the block size
	result &= ~(kByteGroupSize - 1);

	return (result < kVertexBlockMaxSize) ? result : kVertexBlockMaxSize;
}

inline unsigned char zigzag8(unsigned char v)
{
	return ((signed char)(v) >> 7) ^ (v << 1);
}

#if TRACE
struct Stats
{
	size_t size;
	size_t header;  // bytes for header
	size_t bitg[4]; // bytes for bit groups
	size_t bitc[8]; // bit consistency: how many bits are shared between all bytes in a group
};

static Stats* bytestats = NULL;
static Stats vertexstats[256];
#endif

static bool encodeBytesGroupZero(const unsigned char* buffer)
{
	for (size_t i = 0; i < kByteGroupSize; ++i)
		if (buffer[i])
			return false;

	return true;
}

static unsigned char* encodeBytesGroupTry(unsigned char* data, const unsigned char* buffer, int bits)
{
	assert(bits >= 0 && bits <= 8);

	if (bits == 0)
		return encodeBytesGroupZero(buffer) ? data : NULL;

	if (bits == 8)
	{
		memcpy(data, buffer, kByteGroupSize);
		return data + kByteGroupSize;
	}

	// fixed portion: bits bits for each value
	// variable portion: full byte for each out-of-range value (using 1...1 as sentinel)
	unsigned char sentinel = (1 << bits) - 1;

	unsigned int accum = 0;
	unsigned int accum_bits = 0;

	for (size_t i = 0; i < kByteGroupSize; ++i)
	{
		unsigned char enc = (buffer[i] >= sentinel) ? sentinel : buffer[i];

		accum <<= bits;
		accum |= enc;
		accum_bits += bits;

		if (accum_bits >= 8)
		{
			accum_bits -= 8;
			*data++ = accum >> accum_bits;
		}
	}

	assert(accum_bits == 0);

	for (size_t i = 0; i < kByteGroupSize; ++i)
	{
		if (buffer[i] >= sentinel)
		{
			*data++ = buffer[i];
		}
	}

	return data;
}

static unsigned char* encodeBytes(unsigned char* data, unsigned char* data_end, const unsigned char* buffer, size_t buffer_size,
    int bits0, int bits1, int bits2, int bits3)
{
	assert(buffer_size % kByteGroupSize == 0);

	unsigned char* header = data;

	// round number of groups to 4 to get number of header bytes
	size_t header_size = (buffer_size / kByteGroupSize + 3) / 4;

	if (size_t(data_end - data) < header_size)
		return NULL;

	data += header_size;

	memset(header, 0, header_size);

	int last_bits = -1;

	for (size_t i = 0; i < buffer_size; i += kByteGroupSize)
	{
		if (size_t(data_end - data) < kByteGroupDecodeLimit)
			return NULL;

		int best_bits = -1;
		size_t best_size = SIZE_MAX;

		for (int bits = 0; bits <= 8; bits++)
		{
			if (bits != bits0 && bits != bits1 && bits != bits2 && bits != bits3)
				continue;

			unsigned char* next = encodeBytesGroupTry(data, buffer + i, bits);
			if (!next)
				continue;

			if (size_t(next - data) < best_size || ((bits == last_bits || bits == 8) && size_t(next - data) == best_size))
			{
				best_bits = bits;
				best_size = next - data;
			}
		}

		last_bits = best_bits;

		int bitsenc = (best_bits == bits0) ? 0 : (best_bits == bits1 ? 1 : (best_bits == bits2 ? 2 : 3));

		size_t header_offset = i / kByteGroupSize;

		header[header_offset / 4] |= bitsenc << ((header_offset % 4) * 2);

		unsigned char* next = encodeBytesGroupTry(data, buffer + i, best_bits);

		assert(next && data + best_size == next);
		data = next;

#if TRACE
		bytestats->bitg[bitsenc] += best_size;
#endif
	}

#if TRACE
	bytestats->header += header_size;
#endif

	return data;
}

template <typename T, typename ST>
static void makedelta(unsigned char* buffer, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, unsigned char last_vertex[256], size_t vertex_offset, int rot)
{
	const size_t tmask = ~(sizeof(T) - 1);

	T p = *(T*)&last_vertex[vertex_offset & tmask];

	for (size_t i = 0; i < vertex_count; ++i)
	{
		T e = *(T*)&vertex_data[vertex_offset & tmask];
		if (rot)
			e = (e << rot) | (e >> (sizeof(T) * 8 - rot));

		T d = e - p;
		buffer[i] = ((ST(d) >> (8 * sizeof(T) - 1)) ^ (d << 1)) >> (8 * (vertex_offset & (sizeof(T) - 1)));
		p = e;

		vertex_offset += vertex_size;
	}
}

bool tune_bits = true;
bool tune_lit = true;
bool tune_width = true;
bool tune_rot = false;

static unsigned char* encodeVertexBlock4(unsigned char* data, unsigned char* data_end, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, unsigned char last_vertex[256], size_t vertex_offset, int width, const int rot[4])
{
	assert(vertex_count > 0 && vertex_count <= kVertexBlockMaxSize);

	unsigned char buffer[kVertexBlockMaxSize];
	assert(sizeof(buffer) % kByteGroupSize == 0);

	// we sometimes encode elements we didn't fill when rounding to kByteGroupSize
	memset(buffer, 0, sizeof(buffer));

	unsigned char* header = data;

	// 2-bit selector per byte per block
	*data++ = 0;

	for (size_t k = vertex_offset; k < vertex_offset + 4; ++k)
	{
		switch (width)
		{
		case 1:
			makedelta<unsigned char, signed char>(buffer, vertex_data, vertex_count, vertex_size, last_vertex, k, rot[k - vertex_offset]);
			break;
		case 2:
			makedelta<unsigned short, signed short>(buffer, vertex_data, vertex_count, vertex_size, last_vertex, k, rot[k - vertex_offset]);
			break;
		case 4:
			makedelta<unsigned int, signed int>(buffer, vertex_data, vertex_count, vertex_size, last_vertex, k, rot[k - vertex_offset]);
			break;
		}

		size_t vertex_count_aligned = (vertex_count + kByteGroupSize - 1) & ~(kByteGroupSize - 1);

		const int encs[3][4] =
		    {
		        {0, 2, 4, 8},
		        {0, 1, 2, 8},
		        {2, 4, 6, 8},
		    };

		int best_enc = -1;
		size_t best_size = SIZE_MAX;

		for (int enc = 0; enc < (tune_bits ? 3 : 1); ++enc)
		{
			unsigned char* encp = encodeBytes(data, data_end, buffer, vertex_count_aligned, encs[enc][0], encs[enc][1], encs[enc][2], encs[enc][3]);
			assert(encp);

			if (size_t(encp - data) < best_size)
			{
				best_enc = enc;
				best_size = size_t(encp - data);
			}
		}

		// literal block
		if (best_size >= vertex_count && tune_lit)
		{
			header[k / 4] |= 3 << ((k % 4) * 2);

			memcpy(data, buffer, vertex_count);
			data += vertex_count;
		}
		else
		{
			assert(best_enc < 3);
			header[k / 4] |= best_enc << ((k % 4) * 2);

			data = encodeBytes(data, data_end, buffer, vertex_count_aligned, encs[best_enc][0], encs[best_enc][1], encs[best_enc][2], encs[best_enc][3]);
			if (!data)
				return NULL;
		}
	}

	return data;
}

static unsigned char* encodeVertexBlock(unsigned char* data, unsigned char* data_end, const unsigned char* vertex_data, size_t vertex_count, size_t vertex_size, unsigned char last_vertex[256])
{
	assert(vertex_count > 0 && vertex_count <= kVertexBlockMaxSize);

	for (size_t k = 0; k < vertex_size; k += 4)
	{
		int best_width = 1;
		int best_rot[4] = {};
		size_t best_size = SIZE_MAX;

		for (int width = 1; width <= (tune_width ? 4 : 1); width *= 2)
		{
			if (tune_rot)
			{
				if (width == 1)
				{
					for (int rot = 0; rot < 8; ++rot)
					{
						int try_rot[4] = {rot, best_rot[1], best_rot[2], best_rot[3]};
						unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, try_rot);
						assert(enc);

						if (size_t(enc - data) < best_size)
						{
							best_width = width;
							best_rot[0] = rot;
							best_size = enc - data;
						}
					}
					for (int rot = 0; rot < 8; ++rot)
					{
						int try_rot[4] = {best_rot[0], rot, best_rot[2], best_rot[3]};
						unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, try_rot);
						assert(enc);

						if (size_t(enc - data) < best_size)
						{
							best_width = width;
							best_rot[1] = rot;
							best_size = enc - data;
						}
					}
					for (int rot = 0; rot < 8; ++rot)
					{
						int try_rot[4] = {best_rot[0], best_rot[1], rot, best_rot[3]};
						unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, try_rot);
						assert(enc);

						if (size_t(enc - data) < best_size)
						{
							best_width = width;
							best_rot[2] = rot;
							best_size = enc - data;
						}
					}
					for (int rot = 0; rot < 8; ++rot)
					{
						int try_rot[4] = {best_rot[0], best_rot[1], best_rot[2], rot};
						unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, try_rot);
						assert(enc);

						if (size_t(enc - data) < best_size)
						{
							best_width = width;
							best_rot[3] = rot;
							best_size = enc - data;
						}
					}
				}
				else if (width == 2)
				{
					for (int rot = 0; rot < 16; ++rot)
					{
						int try_rot[4] = {rot, rot, best_rot[2], best_rot[3]};
						unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, try_rot);
						assert(enc);

						if (size_t(enc - data) < best_size)
						{
							best_width = width;
							best_rot[0] = best_rot[1] = rot;
							best_size = enc - data;
						}
					}

					for (int rot = 0; rot < 16; ++rot)
					{
						int try_rot[4] = {best_rot[0], best_rot[1], rot, rot};
						unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, try_rot);
						assert(enc);

						if (size_t(enc - data) < best_size)
						{
							best_width = width;
							best_rot[2] = best_rot[3] = rot;
							best_size = enc - data;
						}
					}
				}
				else
				{
					for (int rot = 0; rot < 32; ++rot)
					{
						int try_rot[4] = {rot, rot, rot, rot};
						unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, try_rot);
						assert(enc);

						if (size_t(enc - data) < best_size)
						{
							best_width = width;
							best_rot[0] = best_rot[1] = best_rot[2] = best_rot[3] = rot;
							best_size = enc - data;
						}
					}
				}
			}
			else
			{
				unsigned char* enc = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, width, best_rot);
				assert(enc);

				if (size_t(enc - data) < best_size)
				{
					best_width = width;
					best_size = enc - data;
				}
			}
		}

		*data++ = (best_rot[0] << 2) | (best_width - 1); // TODO: best_rot encoding doesn't fit for width 1/2

		// fprintf(stderr, "%d: width %d rot %d\n", int(k), best_width, best_rot);

		data = encodeVertexBlock4(data, data_end, vertex_data, vertex_count, vertex_size, last_vertex, k, best_width, best_rot);
	}

	memcpy(last_vertex, &vertex_data[vertex_size * (vertex_count - 1)], vertex_size);

	return data;
}

size_t encodeV1(unsigned char* buffer, size_t buffer_size, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	assert(vertex_size > 0 && vertex_size <= 256);
	assert(vertex_size % 4 == 0);

#if TRACE
	memset(vertexstats, 0, sizeof(vertexstats));
#endif

	const unsigned char* vertex_data = static_cast<const unsigned char*>(vertices);

	unsigned char* data = buffer;
	unsigned char* data_end = buffer + buffer_size;

	if (size_t(data_end - data) < 1 + vertex_size)
		return 0;

	int version = gEncodeVertexVersion;

	*data++ = (unsigned char)(kVertexHeader | version);

	unsigned char first_vertex[256] = {};
	if (vertex_count > 0)
		memcpy(first_vertex, vertex_data, vertex_size);

	unsigned char last_vertex[256] = {};
	memcpy(last_vertex, first_vertex, vertex_size);

	size_t vertex_block_size = getVertexBlockSize(vertex_size);

	size_t vertex_offset = 0;

	while (vertex_offset < vertex_count)
	{
		size_t block_size = (vertex_offset + vertex_block_size < vertex_count) ? vertex_block_size : vertex_count - vertex_offset;

		data = encodeVertexBlock(data, data_end, vertex_data + vertex_offset * vertex_size, block_size, vertex_size, last_vertex);
		if (!data)
			return 0;

		vertex_offset += block_size;
	}

	size_t tail_size = vertex_size < kTailMaxSize ? kTailMaxSize : vertex_size;

	if (size_t(data_end - data) < tail_size)
		return 0;

	// write first vertex to the end of the stream and pad it to 32 bytes; this is important to simplify bounds checks in decoder
	if (vertex_size < kTailMaxSize)
	{
		memset(data, 0, kTailMaxSize - vertex_size);
		data += kTailMaxSize - vertex_size;
	}

	memcpy(data, first_vertex, vertex_size);
	data += vertex_size;

	assert(data >= buffer + tail_size);
	assert(data <= buffer + buffer_size);

#if TRACE
	size_t total_size = data - buffer;

	for (size_t k = 0; k < vertex_size; ++k)
	{
		const Stats& vsk = vertexstats[k];

		printf("%2d: %7d bytes [%4.1f%%] %.1f bpv", int(k), int(vsk.size), double(vsk.size) / double(total_size) * 100, double(vsk.size) / double(vertex_count) * 8);

		size_t total_k = vsk.header + vsk.bitg[0] + vsk.bitg[1] + vsk.bitg[2] + vsk.bitg[3];

		printf(" |\thdr [%5.1f%%] bitg 1-3 [%4.1f%% %4.1f%% %4.1f%%]",
		    double(vsk.header) / double(total_k) * 100, double(vsk.bitg[1]) / double(total_k) * 100,
		    double(vsk.bitg[2]) / double(total_k) * 100, double(vsk.bitg[3]) / double(total_k) * 100);

		printf(" |\tbitc [%3.0f%% %3.0f%% %3.0f%% %3.0f%% %3.0f%% %3.0f%% %3.0f%% %3.0f%%]",
		    double(vsk.bitc[0]) / double(vertex_count) * 100, double(vsk.bitc[1]) / double(vertex_count) * 100,
		    double(vsk.bitc[2]) / double(vertex_count) * 100, double(vsk.bitc[3]) / double(vertex_count) * 100,
		    double(vsk.bitc[4]) / double(vertex_count) * 100, double(vsk.bitc[5]) / double(vertex_count) * 100,
		    double(vsk.bitc[6]) / double(vertex_count) * 100, double(vsk.bitc[7]) / double(vertex_count) * 100);
		printf("\n");
	}
#endif

	return data - buffer;
}

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
	double v10_raw;
	double v10_lz4;
	double v10_zstd;
	double count;
};

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
		input.resize(input.size() * 4);
		input.resize(meshopt_encodeVertexBuffer(input.data(), input.size(), decoded.data(), count, stride));
	}

	std::vector<unsigned char> output(decoded.size() * 4); // todo
	output.resize(encodeV1(output.data(), output.size(), decoded.data(), count, stride));

	size_t decoded_lz4 = measure_lz4(decoded);
	size_t input_lz4 = measure_lz4(input);
	size_t output_lz4 = measure_lz4(output);
	size_t decoded_zstd = measure_zstd(decoded);
	size_t input_zstd = measure_zstd(input);
	size_t output_zstd = measure_zstd(output);

	printf(" raw %zu KB\t", decoded.size() / 1024);
	printf(" v0 %.2f", double(input.size()) / double(decoded.size()));
	printf(" v1 %.2f", double(output.size()) / double(decoded.size()));
	printf(" v1/v0 %+.1f%%", double(output.size()) / double(input.size()) * 100 - 100);

	printf("\tlz4 %.2f:", double(decoded_lz4) / double(decoded.size()));
	printf(" v0 %.2f", double(input_lz4) / double(decoded.size()));
	printf(" v1 %.2f", double(output_lz4) / double(decoded.size()));
	printf(" v1/v0 %+.1f%%", double(output_lz4) / double(input_lz4) * 100 - 100);

	printf("\tzstd %.2f:", double(decoded_zstd) / double(decoded.size()));
	printf(" v0 %.2f", double(input_zstd) / double(decoded.size()));
	printf(" v1 %.2f", double(output_zstd) / double(decoded.size()));
	printf(" v1/v0 %+.1f%%", double(output_zstd) / double(input_zstd) * 100 - 100);

	if (stats)
	{
		stats->v10_raw += double(output.size()) / double(input.size()) - 1;
		stats->v10_lz4 += double(output_lz4) / double(input_lz4) - 1;
		stats->v10_zstd += double(output_zstd) / double(input_zstd) - 1;
		stats->count++;
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

	// printf("%s\n", path);
	printf("%25.*s:", int(namel), name0);
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

	if (argc > 1 && strcmp(argv[1], "-test") == 0)
	{
		Stats stats = {};
		for (int i = 2; i < argc; ++i)
			testFile(argv[i], &stats);
		printf("---\n");
		printf("%d files: raw v1/v0 %+.1f%%, lz4 v1/v0 %+.1f%%, zstd v1/v0 %+.1f%%\n",
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
	else if (getenv("V") && atoi(getenv("V")))
	{
		size_t vertex_count = input.size() / stride;
		std::vector<unsigned char> output(input.size() * 4); // todo
		size_t output_size = encodeV1(output.data(), output.size(), input.data(), vertex_count, stride);

		fwrite(output.data(), 1, output_size, stdout);
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
