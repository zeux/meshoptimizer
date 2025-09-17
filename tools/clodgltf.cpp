#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../demo/clusterlod.h"
#include "../extern/cgltf.h"
#include "../src/meshoptimizer.h"

const size_t kNumThreads = 16;
const size_t kBytesPerVertex = 32;
const size_t kBytesPerTriangle = 64;
const size_t kMemoryLimit = size_t(32) << 30; // 32 GB

const bool kUseThreadArena = true;
const size_t kThreadArenaSize = size_t(1) << 30; // 1 GB per thread

struct Stream
{
	const float* data;
	size_t count;
	size_t stride;
};

struct Vertex
{
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;
};

static cgltf_result mmap_file_read(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, const char* path, cgltf_size* size, void** data)
{
	(void)memory_options;
	(void)file_options;

	int fd = open(path, O_RDONLY);
	if (fd == -1)
		return cgltf_result_file_not_found;

	struct stat sb;
	if (fstat(fd, &sb) == -1)
	{
		close(fd);
		return cgltf_result_io_error;
	}

	if (sb.st_size == 0)
	{
		close(fd);
		*size = 0;
		*data = NULL;
		return cgltf_result_success;
	}

	void* mapped = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mapped == MAP_FAILED)
	{
		close(fd);
		return cgltf_result_io_error;
	}

	close(fd);

	*size = sb.st_size;
	*data = mapped;

	return cgltf_result_success;
}

static void mmap_file_release(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, void* data, cgltf_size size)
{
	(void)memory_options;
	(void)file_options;

	if (data)
	{
		int rc = munmap(data, size);
		assert(rc == 0);
		(void)rc;
	}
}

struct Stats
{
	size_t clusters;
	size_t total_triangles;
	size_t lowest_triangles;
};

struct Request
{
	const cgltf_data* data;
	const cgltf_mesh* mesh;
	const cgltf_primitive* primitive;

	size_t primitive_index;
	size_t primitive_count;

	size_t time_estimate;
	size_t memory_requirements;
};

struct ThreadArena
{
	void* data;
	size_t size;
	size_t offset;
};

thread_local ThreadArena* gThreadArena;

static void processPrimitive(const Request& request, Stats& stats)
{
	const cgltf_mesh& mesh = *request.mesh;
	const cgltf_primitive& primitive = *request.primitive;

	const char* mesh_name = mesh.name ? mesh.name : "";

	printf("Processing primitive %d/%d (mesh %s/%d)\n",
	    int(request.primitive_index), int(request.primitive_count),
	    mesh_name, int(&primitive - mesh.primitives));

	if (primitive.type != cgltf_primitive_type_triangles)
	{
		printf("Skipping primitive %d/%d (mesh %s/%d): not triangles\n",
		    int(request.primitive_index), int(request.primitive_count),
		    mesh_name, int(&primitive - mesh.primitives));
		return;
	}

	if (!primitive.indices)
	{
		printf("Skipping primitive %d/%d (mesh %s/%d): no indices\n",
		    int(request.primitive_index), int(request.primitive_count),
		    mesh_name, int(&primitive - mesh.primitives));
		return;
	}

	// Find position and normal attributes
	const cgltf_accessor* position_accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
	const cgltf_accessor* normal_accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0);
	const cgltf_accessor* texcoord_accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0);

	if (!position_accessor)
	{
		printf("Skipping primitive %d/%d (mesh %s/%d): no position data\n",
		    int(request.primitive_index), int(request.primitive_count),
		    mesh_name, int(&primitive - mesh.primitives));
		return;
	}

	// Load indices
	std::vector<unsigned int> indices(primitive.indices->count);
	cgltf_accessor_unpack_indices(primitive.indices, &indices[0], sizeof(unsigned int), indices.size());

	unsigned int max_index = 0;
	for (size_t i = 0; i < indices.size(); ++i)
		max_index = std::max(max_index, indices[i]);

	if (max_index >= position_accessor->count)
	{
		printf("Skipping primitive %d/%d (mesh %s/%d): invalid indices\n",
		    int(request.primitive_index), int(request.primitive_count),
		    mesh_name, int(&primitive - mesh.primitives));
		return;
	}

	// Load vertices
	size_t vertex_count = position_accessor->count;

	// Load positions
	std::vector<float> positions(vertex_count * 3);
	cgltf_accessor_unpack_floats(position_accessor, &positions[0], positions.size());

	// Load normals and texcoords into a single attribute buffer
	const size_t attribute_count = 5;
	float attribute_weights[attribute_count] = {};

	std::vector<float> attributes;

	if (normal_accessor || texcoord_accessor)
	{
		std::vector<float> normals;
		if (normal_accessor)
		{
			normals.resize(vertex_count * 3);
			cgltf_accessor_unpack_floats(normal_accessor, &normals[0], normals.size());
		}

		std::vector<float> texcoords;
		if (texcoord_accessor)
		{
			texcoords.resize(vertex_count * 2);
			cgltf_accessor_unpack_floats(texcoord_accessor, &texcoords[0], texcoords.size());
		}

		attributes.resize(vertex_count * attribute_count);

		for (size_t i = 0; i < vertex_count; ++i)
		{
			if (normal_accessor)
			{
				attributes[i * attribute_count + 0] = normals[i * 3 + 0];
				attributes[i * attribute_count + 1] = normals[i * 3 + 1];
				attributes[i * attribute_count + 2] = normals[i * 3 + 2];
			}

			if (texcoord_accessor)
			{
				attributes[i * attribute_count + 3] = texcoords[i * 2 + 0];
				attributes[i * attribute_count + 4] = texcoords[i * 2 + 1];
			}
		}

		if (normal_accessor)
			attribute_weights[0] = attribute_weights[1] = attribute_weights[2] = 0.5f;

		if (texcoord_accessor)
			attribute_weights[3] = attribute_weights[4] = 0.01f;

		normals.clear();
		normals.shrink_to_fit();

		texcoords.clear();
		texcoords.shrink_to_fit();
	}

	if (vertex_count >= indices.size())
	{
		std::vector<unsigned int> remap(vertex_count);

		meshopt_Stream streams[2] = {
		    {positions.data(), sizeof(float) * 3, sizeof(float) * 3},
		    {attributes.data(), sizeof(float) * attribute_count, sizeof(float) * attribute_count},
		};
		size_t stream_count = attributes.empty() ? 1 : 2;

		size_t unique_vertices = meshopt_generateVertexRemapMulti(remap.data(), indices.data(), indices.size(), vertex_count, streams, stream_count);

		meshopt_remapVertexBuffer(positions.data(), positions.data(), vertex_count, sizeof(float) * 3, remap.data());
		if (!attributes.empty())
			meshopt_remapVertexBuffer(attributes.data(), attributes.data(), vertex_count, sizeof(float) * attribute_count, remap.data());

		meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());

		vertex_count = unique_vertices;
	}

	// improves locality, doesn't affect results
	meshopt_spatialSortTriangles(indices.data(), indices.data(), indices.size(), positions.data(), vertex_count, sizeof(float) * 3);

	clodConfig config = clodDefaultConfigRT(128);

	clodMesh cmesh = {};
	cmesh.indices = &indices[0];
	cmesh.index_count = indices.size();
	cmesh.vertex_count = vertex_count;
	cmesh.vertex_positions = positions.data();
	cmesh.vertex_positions_stride = sizeof(float) * 3;
	cmesh.vertex_attributes = attributes.data();
	cmesh.vertex_attributes_stride = sizeof(float) * attribute_count;

	if (normal_accessor)
	{
		cmesh.attribute_weights = attribute_weights;
		cmesh.attribute_count = sizeof(attribute_weights) / sizeof(attribute_weights[0]);
	}

	if (texcoord_accessor)
		cmesh.attribute_protect_mask = (1 << 3) | (1 << 4); // protect UV seams

	size_t lowest = clodBuild(config, cmesh, &stats,
	    [](void* context, clodCluster cluster)
	    {
		    Stats& stats = *static_cast<Stats*>(context);

		    stats.clusters++;
		    stats.total_triangles += cluster.index_count / 3;
	    });

	printf("Done with primitive %d/%d (mesh %s/%d): %zu vertices (%zu unique), %zu triangles => %zu triangles\n",
	    int(request.primitive_index), int(request.primitive_count),
	    mesh_name, int(&primitive - mesh.primitives),
	    position_accessor->count, vertex_count, indices.size() / 3,
	    lowest);

	stats.lowest_triangles = lowest;
}

static void* threadArenaAlloc(size_t size)
{
	ThreadArena* arena = gThreadArena;

	if (arena && arena->size - arena->offset >= size)
	{
		void* result = static_cast<char*>(arena->data) + arena->offset;
		arena->offset += (size + 15) & ~size_t(15);
		return result;
	}

	return ::operator new(size);
}

static void threadArenaFree(void* ptr)
{
	ThreadArena* arena = gThreadArena;

	if (arena && ptr >= arena->data && ptr < static_cast<char*>(arena->data) + arena->size)
	{
		assert(ptr <= static_cast<char*>(arena.data) + arena.offset);
		arena->offset = static_cast<char*>(ptr) - static_cast<char*>(arena->data);
		return;
	}

	::operator delete(ptr);
}

struct RequestQueue
{
	std::vector<Request> items;
	std::vector<Stats> stats;

	std::atomic<size_t> next{0};
	std::atomic<size_t> memory{0};

	size_t limit = 0;
};

static void workerThread(RequestQueue& queue)
{
	ThreadArena arena = {};

	if (kUseThreadArena)
	{
		arena.data = ::operator new(kThreadArenaSize);
		arena.size = kThreadArenaSize;

		assert(gThreadArena == NULL);
		gThreadArena = &arena;
	}

	for (;;)
	{
		size_t index = queue.next++;
		if (index >= queue.items.size())
			break;

		Request request = queue.items[index];

		for (;;)
		{
			size_t memory = queue.memory.load();

			if (memory != 0 && memory + request.memory_requirements > queue.limit)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			if (queue.memory.compare_exchange_weak(memory, memory + request.memory_requirements))
				break;

			std::this_thread::yield();
		}

		Stats stats = {};
		processPrimitive(request, stats);

		if (!queue.stats.empty())
			queue.stats[index] = stats;

		queue.memory -= request.memory_requirements;
	}

	if (kUseThreadArena)
	{
		assert(gthreadArena == &arena);
		gThreadArena = NULL;

		assert(arena.offset == 0);
		::operator delete(arena.data);
	}
}

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		fprintf(stderr, "Usage: clodgltf <path>\n");
		return 1;
	}

	const char* path = argv[1];

	cgltf_options options = {};
	options.file.read = mmap_file_read;
	options.file.release = mmap_file_release;
	cgltf_data* data = NULL;

	cgltf_result result = cgltf_parse_file(&options, path, &data);
	if (result != cgltf_result_success)
	{
		fprintf(stderr, "Error parsing %s\n", path);
		return 1;
	}

	// Important: we validate before load_buffers to avoid the overhead of reading all index data; we'll check that in processPrimitive
	result = cgltf_validate(data);
	if (result != cgltf_result_success)
	{
		fprintf(stderr, "Invalid GLTF file %s\n", path);
		cgltf_free(data);
		return 1;
	}

	result = cgltf_load_buffers(&options, data, path);
	if (result != cgltf_result_success)
	{
		fprintf(stderr, "Error loading buffers for %s\n", path);
		cgltf_free(data);
		return 1;
	}

	RequestQueue queue;
	queue.limit = kMemoryLimit;

	size_t total_triangles = 0;

	for (size_t mi = 0; mi < data->meshes_count; ++mi)
	{
		const cgltf_mesh& mesh = data->meshes[mi];

		for (size_t pi = 0; pi < mesh.primitives_count; ++pi)
		{
			const cgltf_primitive& primitive = mesh.primitives[pi];
			total_triangles += (primitive.indices ? primitive.indices->count / 3 : 0);

			const cgltf_accessor* positions = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);

			Request request = {};
			request.data = data;
			request.mesh = &mesh;
			request.primitive = &primitive;
			request.time_estimate = (primitive.indices ? primitive.indices->count / 3 : 0);
			request.memory_requirements =
			    size_t(positions ? positions->count : 0) * kBytesPerVertex +
			    size_t(primitive.indices ? primitive.indices->count / 3 : 0) * kBytesPerTriangle;
			queue.items.push_back(request);
		}
	}

	// order more expensive requests first for better balancing
	std::sort(queue.items.begin(), queue.items.end(), [](const Request& a, const Request& b)
	    { return a.time_estimate > b.time_estimate; });

	size_t peak_memory_requirements = 0;
	size_t current_primitive = 0;

	for (Request& request : queue.items)
	{
		if (current_primitive < kNumThreads)
			peak_memory_requirements += request.memory_requirements;
		request.primitive_index = ++current_primitive;
		request.primitive_count = queue.items.size();
	}

	queue.stats.resize(queue.items.size());

	printf("Processing %d meshes from %s (peak memory required %.2f GB, limit %.2f GB)\n",
	    int(data->meshes_count), path,
	    double(peak_memory_requirements) / double(1 << 30), double(kMemoryLimit) / double(1 << 30));

	meshopt_setAllocator(threadArenaAlloc, threadArenaFree);

	std::vector<std::thread> threads;
	for (size_t i = 0; i < kNumThreads; ++i)
		threads.emplace_back(workerThread, std::ref(queue));

	for (auto& thread : threads)
		thread.join();

	meshopt_setAllocator(::operator new, ::operator delete);

	cgltf_free(data);

	Stats totals = {};
	for (const Stats& stats : queue.stats)
	{
		totals.clusters += stats.clusters;
		totals.total_triangles += stats.total_triangles;
		totals.lowest_triangles += stats.lowest_triangles;
	}

	printf("All done! %d primitives, %d meshes, %.2fB triangles => lowest %.3fM triangles, total %.2fB triangles in %.1fM clusters\n",
	    int(current_primitive), int(data->meshes_count), double(total_triangles) / 1e9,
	    double(totals.lowest_triangles) / 1e6, double(totals.total_triangles) / 1e9, double(totals.clusters) / 1e6);
	return 0;
}
