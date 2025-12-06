# 🐇 meshoptimizer [![Actions Status](https://github.com/zeux/meshoptimizer/workflows/build/badge.svg)](https://github.com/zeux/meshoptimizer/actions) [![codecov.io](https://codecov.io/github/zeux/meshoptimizer/coverage.svg?branch=master)](https://codecov.io/github/zeux/meshoptimizer?branch=master) [![MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md) [![GitHub](https://img.shields.io/badge/repo-github-green.svg)](https://github.com/zeux/meshoptimizer)

## Purpose

When a GPU renders triangle meshes, various stages of the GPU pipeline have to process vertex and index data. The efficiency of these stages depends on the data you feed to them; this library provides algorithms to help optimize meshes for these stages, as well as algorithms to reduce the mesh complexity and storage overhead.

The library provides a C and C++ interface for all algorithms; you can use it from C/C++ or from other languages via FFI (such as P/Invoke). If you want to use this library from Rust, you should use [meshopt crate](https://crates.io/crates/meshopt). JavaScript interface for some algorithms is available through [meshoptimizer.js](https://www.npmjs.com/package/meshoptimizer).

Two companion projects are developed and distributed alongside the library: [gltfpack](./gltf/README.md), a command-line tool that automatically optimizes glTF files, and [clusterlod.h](./demo/clusterlod.h), a single-header C/C++ library for continuous level of detail using clustered simplification.

## Installing

meshoptimizer is hosted on GitHub; you can download the latest release using git:

```
git clone -b v1.0 https://github.com/zeux/meshoptimizer.git
```

Alternatively you can [download the .zip archive from GitHub](https://github.com/zeux/meshoptimizer/archive/v1.0.zip).

The library is also available as a Linux package in several distributions ([ArchLinux](https://aur.archlinux.org/packages/meshoptimizer/), [Debian](https://packages.debian.org/libmeshoptimizer), [FreeBSD](https://www.freshports.org/misc/meshoptimizer/), [Nix](https://mynixos.com/nixpkgs/package/meshoptimizer), [Ubuntu](https://packages.ubuntu.com/libmeshoptimizer)), as well as a [Vcpkg port](https://github.com/microsoft/vcpkg/tree/master/ports/meshoptimizer) (see [installation instructions](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started)) and a [Conan package](https://conan.io/center/recipes/meshoptimizer).

[gltfpack](./gltf/README.md) is available as a pre-built binary on [Releases page](https://github.com/zeux/meshoptimizer/releases) or via [npm package](https://www.npmjs.com/package/gltfpack). Native binaries are recommended since they are more efficient and support texture compression.

## Building

meshoptimizer is distributed as a C/C++ header (`src/meshoptimizer.h`) and a set of C++ source files (`src/*.cpp`). To include it in your project, you can use one of two options:

* Use CMake to build the library (either as a standalone project or as part of your project)
* Add source files to your project's build system

The source files are organized in such a way that you don't need to change your build-system settings, and you only need to add the source files for the algorithms you use. They should build without warnings or special compilation options on all major compilers. If you prefer amalgamated builds, you can also concatenate the source files into a single `.cpp` file and build that instead.

To use meshoptimizer functions, simply `#include` the header `meshoptimizer.h`; the library source is C++, but the header is C-compatible.

## Core pipeline

When optimizing a mesh, to maximize rendering efficiency you should typically feed it through a set of optimizations (the order is important!):

1. Indexing
2. Vertex cache optimization
3. (optional) Overdraw optimization
4. Vertex fetch optimization
5. Vertex quantization
6. (optional) Shadow indexing

### Indexing

Most algorithms in this library assume that a mesh has a vertex buffer and an index buffer. For algorithms to work well and also for GPU to render your mesh efficiently, the vertex buffer has to have no redundant vertices; you can generate an index buffer from an unindexed vertex buffer or reindex an existing (potentially redundant) index buffer as follows:

> Note: meshoptimizer generally works with 32-bit (`unsigned int`) indices, however when using C++ APIs you can use any integer type for index data by using the provided template overloads. By convention, remap tables always use `unsigned int`.

First, generate a remap table from your existing vertex (and, optionally, index) data:

```c++
size_t index_count = face_count * 3;
size_t unindexed_vertex_count = face_count * 3;
std::vector<unsigned int> remap(unindexed_vertex_count); // temporary remap table
size_t vertex_count = meshopt_generateVertexRemap(&remap[0], NULL, index_count,
    &unindexed_vertices[0], unindexed_vertex_count, sizeof(Vertex));
```

Note that in this case we only have an unindexed vertex buffer; when input mesh has an index buffer, it will need to be passed to `meshopt_generateVertexRemap` instead of `NULL`, along with the correct source vertex count. In either case, the remap table is generated based on binary equivalence of the input vertices, so the resulting mesh will render the same way. Binary equivalence considers all input bytes, including padding which should be zero-initialized if the vertex structure has gaps.

After generating the remap table, you can allocate space for the target vertex buffer (`vertex_count` elements) and index buffer (`index_count` elements) and generate them:

```c++
meshopt_remapIndexBuffer(indices, NULL, index_count, &remap[0]);
meshopt_remapVertexBuffer(vertices, &unindexed_vertices[0], unindexed_vertex_count, sizeof(Vertex), &remap[0]);
```

You can then further optimize the resulting buffers by calling the other functions on them in-place.

`meshopt_generateVertexRemap` uses binary equivalence of vertex data, which is generally a reasonable default; however, in some cases some attributes may have floating point drift causing extra vertices to be generated. For such cases, it may be necessary to quantize some attributes (most importantly, normals and tangents) before generating the remap, or use `meshopt_generateVertexRemapCustom` algorithm that allows comparing individual attributes with tolerance by providing a custom comparison function:

```c++
size_t vertex_count = meshopt_generateVertexRemapCustom(&remap[0], NULL, index_count,
    &unindexed_vertices[0].px, unindexed_vertex_count, sizeof(Vertex),
    [&](unsigned int lhs, unsigned int rhs) -> bool {
        const Vertex& lv = unindexed_vertices[lhs];
        const Vertex& rv = unindexed_vertices[rhs];

        return fabsf(lv.tx - rv.tx) < 1e-3f && fabsf(lv.ty - rv.ty) < 1e-3f;
    });
```

### Vertex cache optimization

When the GPU renders the mesh, it has to run the vertex shader for each vertex; usually GPUs have a built-in fixed size cache that stores the transformed vertices (the result of running the vertex shader), and uses this cache to reduce the number of vertex shader invocations. This cache is usually small, 16-32 vertices, and can have different replacement policies; to use this cache efficiently, you have to reorder your triangles to maximize the locality of reused vertex references like so:

```c++
meshopt_optimizeVertexCache(indices, indices, index_count, vertex_count);
```

The details of vertex reuse vary between different GPU architectures, so vertex cache optimization uses an adaptive algorithm that produces a triangle sequence with good locality that works well across different GPUs. Alternatively, you can use an algorithm that optimizes specifically for fixed-size FIFO caches: `meshopt_optimizeVertexCacheFifo` (with a recommended cache size of 16). While it generally produces less performant results on most GPUs, it runs ~2x faster, which may benefit rapid content iteration.

### Overdraw optimization

After transforming the vertices, GPU sends the triangles for rasterization which results in generating pixels that are usually first run through the depth test, and pixels that pass it get the pixel shader executed to generate the final color. As pixel shaders get more expensive, it becomes more and more important to reduce overdraw. While in general improving overdraw requires view-dependent operations, this library provides an algorithm to reorder triangles to minimize the overdraw from all directions, which you can run after vertex cache optimization like this:

```c++
meshopt_optimizeOverdraw(indices, indices, index_count, &vertices[0].x, vertex_count, sizeof(Vertex), 1.05f);
```

The overdraw optimizer needs to read vertex positions as a float3 from the vertex; the code snippet above assumes that the vertex stores position as `float x, y, z`.

When performing the overdraw optimization you have to specify a floating-point threshold parameter. The algorithm tries to maintain a balance between vertex cache efficiency and overdraw; the threshold determines how much the algorithm can compromise the vertex cache hit ratio, with 1.05 meaning that the resulting ratio should be at most 5% worse than before the optimization.

Note that depending on the renderer structure and target hardware, the optimization may or may not be beneficial; for example, mobile GPUs with tiled deferred rendering (PowerVR, Apple) would not benefit from this optimization. For vertex heavy scenes it's recommended to measure the performance impact to ensure that the reduced vertex cache efficiency is outweighed by the reduced overdraw.

### Vertex fetch optimization

After the final triangle order has been established, we still can optimize the vertex buffer for memory efficiency. Before running the vertex shader GPU has to fetch the vertex attributes from the vertex buffer; the fetch is usually backed by a memory cache, and as such optimizing the data for the locality of memory access is important. You can do this by running this code:

```c++
meshopt_optimizeVertexFetch(vertices, indices, index_count, vertices, vertex_count, sizeof(Vertex));
```

This will reorder the vertices in the vertex buffer to try to improve the locality of reference, and rewrite the indices in place to match; if the vertex data is stored using multiple streams, you should use `meshopt_optimizeVertexFetchRemap` instead. This optimization has to be performed on the final index buffer since the optimal vertex order depends on the triangle order.

Note that the algorithm does not try to model cache replacement precisely and instead just orders vertices in the order of use, which generally produces results that are close to optimal.

### Vertex quantization

To optimize memory bandwidth when fetching the vertex data even further, and to reduce the amount of memory required to store the mesh, it is often beneficial to quantize the vertex attributes to smaller types. While this optimization can technically run at any part of the pipeline (and sometimes doing quantization as the first step can improve indexing by merging almost identical vertices), it generally is easier to run this after all other optimizations since some of them require access to float3 positions.

Quantization is usually domain specific; it's common to quantize normals using 3 8-bit integers but you can use higher-precision quantization (for example using 10 bits per component in a 10_10_10_2 format), or a different encoding to use just 2 components. For positions and texture coordinate data the two most common storage formats are half precision floats, and 16-bit normalized integers that encode the position relative to the AABB of the mesh or the UV bounding rectangle.

The number of possible combinations here is very large but this library does provide the building blocks, specifically functions to quantize floating point values to normalized integers, as well as half-precision floats. For example, here's how you can quantize a normal using 10-10-10 SNORM encoding:

```c++
unsigned int normal =
    ((meshopt_quantizeSnorm(v.nx, 10) & 1023) << 20) |
    ((meshopt_quantizeSnorm(v.ny, 10) & 1023) << 10) |
     (meshopt_quantizeSnorm(v.nz, 10) & 1023);
```

and here's how you can quantize a position using half precision floats:

```c++
unsigned short px = meshopt_quantizeHalf(v.x);
unsigned short py = meshopt_quantizeHalf(v.y);
unsigned short pz = meshopt_quantizeHalf(v.z);
```

Since quantized vertex attributes often need to remain in their compact representations for efficient transfer and storage, they are usually dequantized during vertex processing by configuring the GPU vertex input correctly to expect normalized integers or half precision floats, which often needs no or minimal changes to the shader code. When CPU dequantization is required instead, `meshopt_dequantizeHalf` can be used to convert half precision values back to single precision; for normalized integer formats, the dequantization just requires dividing by 2^N-1 for unorm and 2^(N-1)-1 for snorm variants. For example, manually reversing `meshopt_quantizeUnorm(v, 10)` can be done by dividing by 1023.

### Shadow indexing

Many rendering pipelines require meshes to be rendered to depth-only targets, such as shadow maps or during a depth pre-pass, in addition to color/G-buffer targets. While using the same geometry data for both cases is possible, reducing the number of unique vertices for depth-only rendering can be beneficial, especially when the source geometry has many attribute seams due to faceted shading or lightmap texture seams.

To achieve this, this library provides the `meshopt_generateShadowIndexBuffer` algorithm, which generates a second (shadow) index buffer that can be used with the original vertex data:

```c++
std::vector<unsigned int> shadow_indices(index_count);
// note: this assumes Vertex starts with float3 positions and should be adjusted accordingly for quantized positions
meshopt_generateShadowIndexBuffer(&shadow_indices[0], indices, index_count, &vertices[0].x, vertex_count, sizeof(float) * 3, sizeof(Vertex));
```

Because the vertex data is shared, shadow indexing should be done after other optimizations of the vertex/index data. However, it's possible (and recommended) to optimize the resulting shadow index buffer for vertex cache:

```c++
meshopt_optimizeVertexCache(&shadow_indices[0], &shadow_indices[0], index_count, vertex_count);
```

In some cases, it may be beneficial to split the vertex positions into a separate buffer to maximize efficiency for depth-only rendering. Note that the example above assumes only positions are relevant for shadow rendering, but more complex materials may require adding texture coordinates (for alpha testing) or skinning data to the vertex portion used as a key. `meshopt_generateShadowIndexBufferMulti` can be useful for these cases if the relevant data is not contiguous.

Note that for meshes with optimal indexing and few attribute seams, the shadow index buffer will be very similar to the original index buffer, so it may not be always worth generating a separate shadow index buffer even if the rendering pipeline relies on depth-only passes.

## Clusterization

While traditionally meshes have served as a unit of rendering, new approaches to rendering and raytracing are starting to use a smaller unit of work, such as clusters or meshlets. This allows more freedom in how the geometry is processed, and can lead to better performance and more efficient use of GPU hardware. This section describes algorithms designed to work with meshes as sets of clusters.

### Mesh shading

Modern GPUs are beginning to deviate from the traditional rasterization model. NVidia GPUs starting from Turing and AMD GPUs starting from RDNA2 provide a new programmable geometry pipeline that, instead of being built around index buffers and vertex shaders, is built around mesh shaders - a new shader type that allows to provide a batch of work to the rasterizer.

Using mesh shaders in context of traditional mesh rendering provides an opportunity to use a variety of optimization techniques, starting from more efficient vertex reuse, using various forms of culling (e.g. cluster frustum or occlusion culling) and in-memory compression to maximize the utilization of GPU hardware. Beyond traditional rendering mesh shaders provide a richer programming model that can synthesize new geometry more efficiently than common alternatives such as geometry shaders. Mesh shading can be accessed via Vulkan or Direct3D 12 APIs; please refer to [Introduction to Turing Mesh Shaders](https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/) and [Mesh Shaders and Amplification Shaders: Reinventing the Geometry Pipeline](https://devblogs.microsoft.com/directx/coming-to-directx-12-mesh-shaders-and-amplification-shaders-reinventing-the-geometry-pipeline/) for additional information.

To use mesh shaders for conventional rendering efficiently, geometry needs to be converted into a series of meshlets; each meshlet represents a small subset of the original mesh and comes with a small set of vertices and a separate micro-index buffer that references vertices in the meshlet. This information can be directly fed to the rasterizer from the mesh shader. This library provides algorithms to create meshlet data for a mesh, and - assuming geometry is static - can compute bounding information that can be used to perform cluster culling, rejecting meshlets that are invisible on screen.

To generate meshlet data, this library provides `meshopt_buildMeshlets` algorithm, which tries to balance topological efficiency (by maximizing vertex reuse inside meshlets) with culling efficiency (by minimizing meshlet radius and triangle direction divergence) and produces GPU-friendly data. As an alternative (that can be useful for load-time processing), `meshopt_buildMeshletsScan` can create the meshlet data using a vertex cache-optimized index buffer as a starting point by greedily aggregating consecutive triangles until they go over the meshlet limits. `meshopt_buildMeshlets` is recommended for offline data processing even if cone culling is not used.

```c++
const size_t max_vertices = 64;
const size_t max_triangles = 126; // note: in v0.25 or prior, max_triangles needs to be divisible by 4
const float cone_weight = 0.0f;

size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);
std::vector<meshopt_Meshlet> meshlets(max_meshlets);
std::vector<unsigned int> meshlet_vertices(indices.size());
std::vector<unsigned char> meshlet_triangles(indices.size()); // note: in v0.25 or prior, use indices.size() + max_meshlets * 3

size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices.data(),
    indices.size(), &vertices[0].x, vertices.size(), sizeof(Vertex), max_vertices, max_triangles, cone_weight);
```

To generate the meshlet data, `max_vertices` and `max_triangles` need to be set within limits supported by the hardware; for NVidia the values of 64 and 126 are recommended. `cone_weight` should be left as 0 if cluster cone culling is not used, and set to a value between 0 and 1 to balance cone culling efficiency with other forms of culling like frustum or occlusion culling (`0.25` is a reasonable default).

> Note that for earlier AMD GPUs, the best configurations tend to use the same limits for `max_vertices` and `max_triangles`, such as 64 and 64, or 128 and 128. Additionally, while NVidia recommends 64/126 as a good configuration, consider using a different configuration like `max_vertices 64, max_triangles 96`, to provide more realistic limits that are achievable on real-world meshes, and to reduce the overhead on other GPUs.

Each resulting meshlet refers to a portion of `meshlet_vertices` and `meshlet_triangles` arrays; the arrays are overallocated for the worst case so it's recommended to trim them before saving them as an asset / uploading them to the GPU:

```c++
const meshopt_Meshlet& last = meshlets[meshlet_count - 1];

meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
meshlet_triangles.resize(last.triangle_offset + last.triangle_count * 3);
meshlets.resize(meshlet_count);
```

Depending on the application, other strategies of storing the data can be useful; for example, `meshlet_vertices` serves as indices into the original vertex buffer but it might be worthwhile to generate a mini vertex buffer for each meshlet to remove the extra indirection when accessing vertex data, or it might be desirable to compress vertex data as vertices in each meshlet are likely to be very spatially coherent.

For optimal performance, it is recommended to further optimize each meshlet in isolation for better triangle and vertex locality by calling `meshopt_optimizeMeshlet` on vertex and index data like so:

```c++
meshopt_optimizeMeshlet(&meshlet_vertices[m.vertex_offset], &meshlet_triangles[m.triangle_offset], m.triangle_count, m.vertex_count);
```

Different applications will choose different strategies for rendering meshlets; on a GPU capable of mesh shading, meshlets can be rendered directly; for example, a basic GLSL shader for `VK_EXT_mesh_shader` extension could look like this (parts omitted for brevity):

```glsl
layout(binding = 0) readonly buffer Meshlets { Meshlet meshlets[]; };
layout(binding = 1) readonly buffer MeshletVertices { uint meshlet_vertices[]; };
layout(binding = 2) readonly buffer MeshletTriangles { uint8_t meshlet_triangles[]; };

void main() {
    Meshlet meshlet = meshlets[gl_WorkGroupID.x];
    SetMeshOutputsEXT(meshlet.vertex_count, meshlet.triangle_count);

    for (uint i = gl_LocalInvocationIndex; i < meshlet.vertex_count; i += gl_WorkGroupSize.x) {
        uint index = meshlet_vertices[meshlet.vertex_offset + i];
        gl_MeshVerticesEXT[i].gl_Position = world_view_projection * vec4(vertex_positions[index], 1);
    }

    for (uint i = gl_LocalInvocationIndex; i < meshlet.triangle_count; i += gl_WorkGroupSize.x) {
        uint offset = meshlet.triangle_offset + i * 3;
        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(
            meshlet_triangles[offset], meshlet_triangles[offset + 1], meshlet_triangles[offset + 2]);
    }
}
```

> Note that DirectX 12 mesh shaders cannot index raw buffers using arbitrary byte offsets. Use a typed SRV buffer (`Buffer<uint>`) with `DXGI_FORMAT_R8_UINT` format, repack each triangle to 32 bits to be able to use aligned 32-bit loads with `ByteAddressBuffer`, or consider utilizing [16-bit scalar types](https://github.com/microsoft/DirectXShaderCompiler/wiki/16-Bit-Scalar-Types) to load a 3-byte triangle using two aligned 16-bit loads like so: `Buffer.Load<uint16_t2>(triangle_offset & ~1)`, then extracting indices with bitwise operations based on `triangle_offset & 1`.

After generating the meshlet data, it's possible to generate extra data for each meshlet that can be saved and used at runtime to perform cluster culling, where each meshlet can be discarded if it's guaranteed to be invisible. To generate the data, `meshopt_computeMeshletBounds` can be used:

```c++
meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[m.vertex_offset], &meshlet_triangles[m.triangle_offset],
    m.triangle_count, &vertices[0].x, vertices.size(), sizeof(Vertex));
```

The resulting `bounds` values can be used to perform frustum or occlusion culling using the bounding sphere, or cone culling using the cone axis/angle (which will reject the entire meshlet if all triangles are guaranteed to be back-facing from the camera point of view):

```c++
if (dot(normalize(cone_apex - camera_position), cone_axis) >= cone_cutoff) reject();
```

Cluster culling should ideally run at a lower frequency than mesh shading, either using amplification/task shaders, or using a separate compute dispatch.

By default, the meshlet builder tries to form complete meshlets even if that requires merging disconnected regions of the mesh into a single meshlet. In some cases, such as hierarchical level of detail, or when advanced culling is used, it may be beneficial to prioritize spatial locality of triangles in a meshlet even if that results in partially filled meshlets. To that end, `meshopt_buildMeshletsFlex` function can be used instead of `meshopt_buildMeshlets`; it provides two triangle limits, `min_triangles` and `max_triangles`, and uses an additional configuration parameter, `split_factor` (recommended value is 2.0), to decide whether increasing the meshlet radius is worth it to fit more triangles in the meshlet. When using this function, the worst case bound for the number of meshlets has to be computed using `meshopt_buildMeshletsBound` with `min_triangles` parameter instead of `max_triangles`.

### Clustered raytracing

In addition to rasterization, meshlets can also be used for ray tracing. NVidia GPUs starting from Turing with recent drivers provide support for cluster acceleration structures (via `VK_NV_cluster_acceleration_structure` extension / NVAPI); instead of building a traditional BLAS, a cluster acceleration structure can be built for each meshlet and combined into a single clustered BLAS. While this currently results in reduced ray tracing performance for static geometry (for which a traditional BLAS may be more suitable), it allows updating the individual clusters without having to rebuild or refit the entire BLAS, which can be useful for mesh deformation or hierarchical level of detail.

When using meshlets for raytracing, the performance characteristics that matter differ from when rendering meshes with rasterization. For raytracing, clusters with optimal spatial division that minimize ray-triangle intersection tests are preferred, while for rasterization, clusters with maximum triangle count within vertex limits are ideal.

To generate meshlets optimized for raytracing, this library provides `meshopt_buildMeshletsSpatial` algorithm, which builds clusters using surface area heuristic (SAH) to produce raytracing-friendly cluster distributions:

```c++
const size_t max_vertices = 64;
const size_t min_triangles = 16;
const size_t max_triangles = 64;
const float fill_weight = 0.5f;

size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, min_triangles); // note: use min_triangles to compute worst case bound
std::vector<meshopt_Meshlet> meshlets(max_meshlets);
std::vector<unsigned int> meshlet_vertices(indices.size());
std::vector<unsigned char> meshlet_triangles(indices.size()); // note: in v0.25 or prior, use indices.size() + max_meshlets * 3

size_t meshlet_count = meshopt_buildMeshletsSpatial(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices.data(),
    indices.size(), &vertices[0].x, vertices.size(), sizeof(Vertex), max_vertices, min_triangles, max_triangles, fill_weight);
```

The algorithm recursively subdivides the triangles into a BVH-like hierarchy using SAH for optimal spatial partitioning while balancing cluster size; this results in clusters that are significantly more efficient to raytrace compared to clusters generated by `meshopt_buildMeshlets`, but can still be used for rasterization (for example, to build visibility buffers or G-buffers).

The `min_triangles` and `max_triangles` parameters control the allowed range of triangles per cluster. For optimal raytracing performance, `min_triangles` should be at most `max_triangles/2` (or, ideally, `max_triangles/4`) to give the algorithm enough freedom to produce high-quality spatial partitioning. For meshes with few seams due to normal or UV discontinuities, using `max_vertices` equal to `max_triangles` is recommended when rasterization performance is a concern; for meshes with many seams or for renderers that primarily use meshlets for ray tracing, a higher `max_vertices` value should be used as it ensures that more clusters can fully utilize the triangle limit.

The `fill_weight` parameter (typically between 0 and 1, although values higher than 1 could be used to prioritize cluster fill even more) controls the trade-off between pure SAH optimization and triangle utilization. A value of 0 will optimize purely for SAH, resulting in best raytracing performance but potentially smaller clusters. Values between 0.25 and 0.75 typically provide a good balance of SAH quality vs triangle count.

When the resulting meshlets are used to generate hardware-specific acceleration structures, using fast trace (e.g. `VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR`) builds results in maximum performance; if build performance is important, using `meshopt_optimizeMeshlet` can help improve ray tracing performance when using fast build (e.g. `VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR`), although the tracing performance will still be lower than with fast trace builds.

### Point cloud clusterization

Both of the meshlet algorithms are designed to work with triangle meshes. In some cases, splitting a point cloud into fixed size clusters can be useful; the resulting point clusters could be rendered via mesh or compute shaders, or the resulting subdivision can be used to parallelize point processing while maintaining locality of points. To that end, this library provides `meshopt_spatialClusterPoints` algorithm:

```c++
const size_t cluster_size = 256;

std::vector<unsigned int> index(mesh.vertices.size());
meshopt_spatialClusterPoints(&index[0], &mesh.vertices[0].px, mesh.vertices.size(), sizeof(Vertex), cluster_size);
```

The resulting index buffer could be used to process the points directly, or reorganize the point data into flat contiguous arrays. Every consecutive chunk of `cluster_size` points in the index buffer refers to a single cluster, with just the last cluster containing fewer points if the total number of points is not a multiple of `cluster_size`. Note that the index buffer is not a remap table, so `meshopt_remapVertexBuffer` can't be used to flatten the point data.

### Cluster partitioning

When working with clustered geometry, it can be beneficial to organize clusters into larger groups (partitions) for more efficient processing or workload distribution. This library provides an algorithm to partition clusters into groups of similar size while prioritizing locality:

```c++
const size_t partition_size = 24;

std::vector<unsigned int> cluster_partitions(cluster_count);
size_t partition_count = meshopt_partitionClusters(&cluster_partitions[0], &cluster_indices[0], total_index_count,
    &cluster_index_counts[0], cluster_count, &vertices[0].x, vertex_count, sizeof(Vertex), partition_size);
```

The algorithm assigns each cluster to a partition, aiming for a target partition size while prioritizing topological locality (sharing vertices) and spatial locality. The resulting partitions can be used for more efficient batched processing of clusters, or for hierarchical simplification schemes similar to Nanite.

Two clusters are considered topologically adjacent if they reference the same indices. In some cases, it can be helpful to process the indices using `meshopt_generateShadowIndexBuffer` (or remap them manually using the remap table generated by `meshopt_generatePositionRemap`), which allows clusters to be considered adjacent even when boundary vertices have different indices due to attribute discontinuities.

If vertex positions are specified (not `NULL`), spatial locality will influence priority of merging clusters; otherwise, the algorithm will rely solely on topological connections and will not merge disconnected clusters into the same partition, which may result in smaller partitions for some inputs.

After partitioning, each element in the destination array contains the partition ID (ranging from 0 to the returned partition count minus 1) for the corresponding cluster. Note that the partitions may be both smaller and larger than the target size; given a target size, the maximum partition size returned currently is `target + target / 3`.

## Mesh compression

In case storage size or transmission bandwidth is of importance, you might want to additionally compress vertex and index data. While several mesh compression libraries, like Google Draco, are available, they typically are designed to maximize the compression ratio at the cost of disturbing the vertex/index order (which makes the meshes inefficient to render on GPU) or decompression performance. They also frequently don't support custom game-ready quantized vertex formats and thus require to re-quantize the data after loading it, introducing extra quantization errors and making decoding slower.

Alternatively you can use general purpose compression libraries like zstd or Oodle to compress vertex/index data - however these compressors aren't designed to exploit redundancies in vertex/index data and as such compression rates can be unsatisfactory.

To that end, this library provides algorithms to "encode" vertex and index data. The result of the encoding is generally significantly smaller than initial data, and remains compressible with general purpose compressors - so you can either store encoded data directly (for modest compression ratios and maximum decoding performance), or further compress it with LZ4/zstd/Oodle to maximize compression ratio.

> Note: this compression scheme is available as a glTF extension [EXT_meshopt_compression](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_meshopt_compression/README.md) as well as [KHR_meshopt_compression](https://github.com/KhronosGroup/glTF/pull/2517).

### Vertex compression

This library provides a lossless algorithm to encode/decode vertex data. To encode vertices, you need to allocate a target buffer (using the worst case bound) and call the encoding function:

```c++
std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(vertex_count, sizeof(Vertex)));
vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), vertices, vertex_count, sizeof(Vertex)));
```

To decode the data at runtime, call the decoding function:

```c++
int res = meshopt_decodeVertexBuffer(vertices, vertex_count, sizeof(Vertex), &vbuf[0], vbuf.size());
assert(res == 0);
```

Note that vertex encoding assumes that vertex buffer was optimized for vertex fetch, and that vertices are quantized. Feeding unoptimized data into the encoder may produce poor compression ratios. The codec is lossless by itself - the only lossy step is quantization/reordering or filters that you may apply before encoding. Additionally, if the vertex data contains padding bytes, they should be zero-initialized to ensure that the encoder does not need to store uninitialized data.

Decoder is heavily optimized and can directly target write-combined memory; you can expect it to run at 3-6 GB/s on modern desktop CPUs. Compression ratio depends on the data; vertex data compression ratio is typically around 2-4x (compared to already quantized and optimally packed data). General purpose lossless compressors can further improve the compression ratio at some cost to decoding performance.

The vertex codec tries to take advantage of the inherent locality of sequential vertices and identify bit patterns that repeat in consecutive vertices. Typically, vertex cache + vertex fetch provides a reasonably local vertex traversal order; without an index buffer, it is recommended to sort vertices spatially (via `meshopt_spatialSortRemap`) to improve the compression ratio.

It is crucial to correctly specify the stride when encoding vertex data; however, for compression ratio it does not matter whether the vertices are interleaved or deinterleaved, as the codecs perform full byte deinterleaving internally. The stride of each stream must be a multiple of 4 bytes.

For optimal compression results, the values should be quantized to small integers. It can be valuable to use bit counts that are not multiples of 8. For example, instead of using 16 bits to represent texture coordinates, use 12-bit integers and divide by 4095 in the shader. Alternatively, using half-precision floats can often achieve good results.
For single-precision floating-point data, it's recommended to use `meshopt_quantizeFloat` to remove entropy from the lower bits of the mantissa; for best results, consider using 15 bits or 7 bits for extreme compression.
For normal or tangent vectors, using octahedral encoding is recommended over three components as it reduces redundancy; similarly, consider using 10-12 bits per component instead of 16.

When data is bit packed, specifying compression level 3 (via `meshopt_encodeVertexBufferLevel`) can improve the compression further by redistributing bits between components.

### Index compression

This library also provides algorithms to encode/decode index data. To encode triangle indices, you need to allocate a target buffer (using the worst case bound) and call the encoding function:

```c++
std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(index_count, vertex_count));
ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), indices, index_count));
```

To decode the data at runtime, call the decoding function:

```c++
int res = meshopt_decodeIndexBuffer(indices, index_count, &ibuf[0], ibuf.size());
assert(res == 0);
```

Note that index encoding assumes that the index buffer was optimized for vertex cache and vertex fetch. Feeding unoptimized data into the encoder will produce poor compression ratios. Codec preserves the order of triangles, however it can rotate each triangle to improve compression ratio (which means the provoking vertex may change).

Decoder is heavily optimized and can directly target write-combined memory; you can expect it to run at 3-6 GB/s on modern desktop CPUs.

The index codec targets 1 byte per triangle as a best case (6x smaller than raw 16-bit index data); on real-world meshes, it's typical to achieve 1-1.2 bytes per triangle. To reach this, the index data needs to be optimized for vertex cache and vertex fetch. Optimizations that do not disrupt triangle locality (such as overdraw) are safe to use in between.
To reduce the data size further, it's possible to use `meshopt_optimizeVertexCacheStrip` instead of `meshopt_optimizeVertexCache` when optimizing for vertex cache. This trades off some efficiency in vertex transform for smaller index (and sometimes vertex) data.

When referenced vertex indices are not sequential, the index codec will use around 2 bytes per index. This can happen when the referenced vertices are a sparse subset of the vertex buffer, such as when encoding LODs. General-purpose compression can be especially helpful in this case.

Index buffer codec only supports triangle list topology; when encoding triangle strips or line lists, use `meshopt_encodeIndexSequence`/`meshopt_decodeIndexSequence` instead. This codec typically encodes indices into ~1 byte per index, but compressing the results further with a general purpose compressor can improve the results to 1-3 bits per index.

### Point cloud compression

The vertex encoding algorithms can be used to compress arbitrary streams of attribute data; one other use case besides triangle meshes is point cloud data. Typically point clouds come with position, color and possibly other attributes but don't have an implied point order.

To compress point clouds efficiently, it's recommended to first preprocess the points by sorting them using the spatial sort algorithm:

```c++
std::vector<unsigned int> remap(point_count);
meshopt_spatialSortRemap(&remap[0], positions, point_count, sizeof(vec3));

// for each attribute stream
meshopt_remapVertexBuffer(positions, positions, point_count, sizeof(vec3), &remap[0]);
```

After this the resulting arrays should be quantized (e.g. using 16-bit fixed point numbers for positions and 8-bit color components), and the result can be compressed using `meshopt_encodeVertexBuffer` as described in the previous section. To decompress, `meshopt_decodeVertexBuffer` will recover the quantized data that can be used directly or converted back to original floating-point data. The compression ratio depends on the nature of source data, for colored points it's typical to get 35-40 bits per point.

### Vertex filters

To further leverage the inherent structure of some vertex data, it's possible to use filters that encode and decode the data in a lossy manner. This is similar to quantization but can be used without having to change the shader code. After decoding, the filter transformation needs to be reversed. For native game engine pipelines, it is usually more optimal to carefully prequantize and pretransform the vertex data, but sometimes (for example when serializing data in glTF format) this is not a practical option and filters are more convenient. This library provides four filters:

- Octahedral filter (`meshopt_encodeFilterOct`/`meshopt_decodeFilterOct`) encodes quantized (snorm) normal or tangent vectors using octahedral encoding. Any number of bits between 2 and 16 can be used with 4 bytes or 8 bytes per vector.
- Quaternion filter (`meshopt_encodeFilterQuat`/`meshopt_decodeFilterQuat`) encodes quantized (snorm) quaternion vectors; this can be used to encode rotations or tangent frames. Any number of bits between 4 and 16 can be used with 8 bytes per vector.
- Exponential filter (`meshopt_encodeFilterExp`/`meshopt_decodeFilterExp`) encodes single-precision floating-point vectors; this can be used to encode arbitrary floating-point data more efficiently. In addition to an arbitrary bit count (<= 24), the filter takes a "mode" parameter that allows specifying how the exponent sharing is performed to trade off compression ratio and quality:
    - `meshopt_EncodeExpSeparate` does not share exponents and results in the largest output
    - `meshopt_EncodeExpSharedVector` shares exponents between different components of the same vector
    - `meshopt_EncodeExpSharedComponent` shares exponents between the same component in different vectors
    - `meshopt_EncodeExpClamped` does not share exponents but clamps the exponent range to reduce exponent entropy
- Color filter (`meshopt_encodeFilterColor`/`meshopt_decodeFilterColor`) encodes quantized (unorm) RGBA colors using YCoCg encoding. Any number of bits between 2 and 16 can be used with 4 bytes or 8 bytes per vector.

Note that all filters are lossy and require the data to be deinterleaved with one attribute per stream; this facilitates efficient SIMD implementation of filter decoders, which decodes at 5-10 GB/s on modern desktop CPUs, allowing the overall decompression speed to be closer to that of the raw vertex codec.

### Versioning and compatibility

The following guarantees on data compatibility are provided for point releases (*no* guarantees are given for development branch):

- Data encoded with older versions of the library can always be decoded with newer versions;
- Data encoded with newer versions of the library can be decoded with older versions, provided that encoding versions are set correctly; if binary stability of encoded data is important, use `meshopt_encodeVertexVersion` and `meshopt_encodeIndexVersion` to 'pin' the data versions (or `version` argument of `meshopt_encodeVertexBufferLevel`).

By default, vertex data is encoded for format version 1 (compatible with meshoptimizer v0.23+), and index data is encoded for format version 1 (compatible with meshoptimizer v0.14+). When decoding the data, the decoder will automatically detect the version from the data header.

## Simplification

All algorithms presented so far don't affect visual appearance at all, with the exception of quantization that has minimal controlled impact. However, fundamentally the most effective way to reduce the rendering or transmission cost of a mesh is to reduce the number of triangles in the mesh.

### Basic simplification

This library provides a simplification algorithm, `meshopt_simplify`, that reduces the number of triangles in the mesh. Given a vertex and an index buffer, it generates a second index buffer that uses existing vertices in the vertex buffer. This index buffer can be used directly for rendering with the original vertex buffer (preferably after vertex cache optimization using `meshopt_optimizeVertexCache`), or a new compact vertex/index buffer can be generated using `meshopt_optimizeVertexFetch` that uses the optimal number and order of vertices.

```c++
float threshold = 0.2f;
size_t target_index_count = size_t(index_count * threshold);
float target_error = 1e-2f;

std::vector<unsigned int> lod(index_count);
float lod_error = 0.f;
lod.resize(meshopt_simplify(&lod[0], indices, index_count, &vertices[0].x, vertex_count, sizeof(Vertex),
    target_index_count, target_error, /* options= */ 0, &lod_error));
```

Target error is an approximate measure of the deviation from the original mesh using distance normalized to `[0..1]` range (e.g. `1e-2f` means that simplifier will try to maintain the error to be below 1% of the mesh extents). Note that the simplifier attempts to produce the requested number of indices at minimal error, but because of topological restrictions and error limit it is not guaranteed to reach the target index count and can stop earlier.

To disable the error limit, `target_error` can be set to `FLT_MAX`. This makes it more likely that the simplifier will reach the target index count, but it may produce a mesh that looks significantly different from the original, so using the resulting error to control viewing distance would be required. Conversely, setting `target_index_count` to 0 will simplify the input mesh as much as possible within the specified error limit; this can be useful for generating LODs that should look good at a given viewing distance.

The algorithm follows the topology of the original mesh in an attempt to preserve attribute seams, borders and overall appearance. For meshes with inconsistent topology or many seams, such as faceted meshes, it can result in simplifier getting "stuck" and not being able to simplify the mesh fully. Therefore it's critical that identical vertices are "welded" together, that is, the input vertex buffer does not contain duplicates. Additionally, it may be worthwhile to weld the vertices without taking into account vertex attributes that aren't critical and can be rebuilt later, or use "permissive" mode described below.

Alternatively, the library provides another simplification algorithm, `meshopt_simplifySloppy`, which doesn't follow the topology of the original mesh. This means that it doesn't preserve attribute seams or borders, but it can collapse internal details that are too small to matter because it can merge mesh features that are topologically disjoint but spatially close. In general, this algorithm produces meshes with worse geometric quality and poor attribute quality compared to `meshopt_simplify`.

The algorithm can also return the resulting normalized deviation that can be used to choose the correct level of detail based on screen size or solid angle; the error can be converted to object space by multiplying by the scaling factor returned by `meshopt_simplifyScale`. For example, given a mesh with a precomputed LOD and a prescaled error, the screen-space normalized error can be computed and used for LOD selection:

```c++
// lod_factor can be 1 or can be adjusted for more or less aggressive LOD selection
float d = max(0, distance(camera_position, mesh_center) - mesh_radius);
float e = d * (tan(camera_fovy / 2) * 2 / screen_height); // 1px in mesh space
bool lod_ok = e * lod_factor >= lod_error;
```

When a sequence of LOD meshes is generated that all use the original vertex buffer, care must be taken to order vertices optimally to not penalize mobile GPU architectures that are only capable of transforming a sequential vertex buffer range. It's recommended in this case to first optimize each LOD for vertex cache, then assemble all LODs in one large index buffer starting from the coarsest LOD (the one with fewest triangles), and call `meshopt_optimizeVertexFetch` on the final large index buffer. This will make sure that coarser LODs require a smaller vertex range and are efficient with respect to vertex fetch and transform.

### Attribute-aware simplification

While `meshopt_simplify` is aware of attribute discontinuities by default (and infers them through the supplied index buffer) and tries to preserve them, it can be useful to provide information about attribute values. This allows the simplifier to take attribute error into account which can improve shading (by using vertex normals), texture deformation (by using texture coordinates), and may be necessary to preserve vertex colors when textures are not used in the first place. This can be done by using a variant of the simplification function that takes attribute values and weight factors, `meshopt_simplifyWithAttributes`:

```c++
const float nrm_weight = 0.5f;
const float attr_weights[3] = {nrm_weight, nrm_weight, nrm_weight};

std::vector<unsigned int> lod(index_count);
float lod_error = 0.f;
lod.resize(meshopt_simplifyWithAttributes(&lod[0], indices, index_count, &vertices[0].x, vertex_count, sizeof(Vertex),
    &vertices[0].nx, sizeof(Vertex), attr_weights, 3, /* vertex_lock= */ NULL,
    target_index_count, target_error, /* options= */ 0, &lod_error));
```

The attributes are passed as a separate buffer (in the example above it's a subset of the same vertex buffer) and should be stored as consecutive floats; attribute weights are used to control the importance of each attribute in the simplification process. For normalized attributes like normals and vertex colors, a weight around 1.0 is usually appropriate; internally, a change of `1/weight` in attribute value over a distance `d` is approximately equivalent to a change of `d` in position. Using higher weights may be appropriate to preserve attribute quality at the cost of position quality. If the attribute has a different scale (e.g. unnormalized vertex colors in [0..255] range), the weight should be divided by the scaling factor (1/255 in this example).

Including texture coordinates in the attribute set is optional, as simplification generally preserves texture quality reasonably well by default; if included, a weight of around 10-100 is usually appropriate depending on the UV density. It's also possible to compute the weight automatically by setting it to the reciprocal average density of UVs, which can be computed as `1/sqrt(average UV area)` = `1/sqrt(sum(abs(uv area)) / triangle count)` over all triangles in the mesh, possibly scaled by a constant factor if necessary.

Both the target error and the resulting error combine positional error and attribute error, so the error can be used to control the LOD while taking attribute quality into account, assuming carefully chosen weights.

### Permissive simplification

By default, `meshopt_simplify` preserves attribute discontinuities inferred from the supplied index buffer. For meshes with many seams, the simplifier can get "stuck" and fail to fully simplify the mesh, as it cannot collapse vertices across attribute seams. This is especially problematic for meshes with faceted normals (flat shading), as the simplifier may not be able to reduce the triangle count at all. The `meshopt_SimplifyPermissive` option relaxes these restrictions, allowing the simplifier to collapse vertices across attribute discontinuities when the resulting error is acceptable:

```c++
std::vector<unsigned int> lod(index_count);
float lod_error = 0.f;
lod.resize(meshopt_simplifyWithAttributes(&lod[0], indices, index_count, &vertices[0].x, vertex_count, sizeof(Vertex),
    &vertices[0].nx, sizeof(Vertex), attr_weights, 3, /* vertex_lock= */ NULL,
    target_index_count, target_error, /* options= */ meshopt_SimplifyPermissive, &lod_error));
```

To maintain appearance, it's highly recommended to use this option together with attribute-aware simplification, as shown above, as it allows the simplifier to maintain attribute quality. In this mode, it is often desirable to selectively preserve certain attribute seams, such as UV seams or sharp creases. This can be achieved by using the `vertex_lock` array with flag `meshopt_SimplifyVertex_Protect` set for individual vertices to protect specific discontinuities. To fill this array, use `meshopt_generatePositionRemap` to create a mapping table for vertices with identical positions, and then compare each vertex to the remapped vertex to determine which attributes are different:

```c++
std::vector<unsigned int> remap(vertices.size());
meshopt_generatePositionRemap(&remap[0], &vertices[0].px, vertices.size(), sizeof(Vertex));

std::vector<unsigned char> locks(vertices.size());
for (size_t i = 0; i < vertices.size(); ++i) {
    unsigned int r = remap[i];

    if (r != i && (vertices[r].tx != vertices[i].tx || vertices[r].ty != vertices[i].ty))
        locks[i] |= meshopt_SimplifyVertex_Protect; // protect UV seams
}
```

This approach provides fine-grained control over which discontinuities to preserve. The permissive mode combined with selective locking provides a balance between simplification quality and attribute preservation, and usually results in higher quality LODs for the same target triangle count (and dramatically higher quality compared to `meshopt_simplifySloppy`).

> Note: this functionality is currently experimental and is subject to future improvements. Certain collapses are restricted to protect the overall topology, and attribute quality may occasionally regress.

### Simplification with vertex update

All simplification functions described so far reuse the original vertex buffer and only produce a new index buffer. This means that the resulting mesh will have the same vertex positions and attributes as the original mesh; this is optimal for minimizing the memory consumption and for highly detailed meshes often provides good quality. However, for more aggressive simplification to retain visual quality, it may be necessary to adjust vertex data for optimal appearance. This can be done by using a variant of the simplification function that updates vertex positions and attributes, `meshopt_simplifyWithUpdate`:

```c++
indices.resize(meshopt_simplifyWithUpdate(&indices[0], indices.size(), &vertices[0].px, vertices.size(), sizeof(Vertex),
    &vertices[0].nx, sizeof(Vertex), attr_weights, 3, /* vertex_lock= */ NULL,
    target_index_count, target_error, /* options= */ 0, &result_error));
```

Unlike `meshopt_simplify`/`meshopt_simplifyWithAttributes`, this function updates the index buffer as well as vertex positions and attributes in place. The resulting indices still refer to the original vertex buffer; any attributes that are not passed to the simplifier can be left unchanged. However, since the original contents of `vertices` is no longer valid for rendering the original mesh, a new compact vertex/index buffer should be generated using `meshopt_optimizeVertexFetch` (after optimizing the index data with `meshopt_optimizeVertexCache`). If the original data was important, it should be copied before calling this function.

Since the vertex positions are updated, this may require updating some attributes that could previously be left as-is when using the original vertex buffer. Notably, texture coordinates need to be updated to avoid texture distortion; thus it's highly recommended to include texture coordinates in the attribute data passed to the simplifier. For attributes to be updated, the corresponding attribute weight must not be zero; for texture coordinates, a weight of 1.0 is usually sufficient in this case (although a higher or mesh dependent weight could be used with this function or other functions to reduce UV stretching).

Attributes that have specific constraints like normals and colors should be renormalized or clamped after the function returns new data. Attributes like bone indices/weights don't have to be updated for reasonable results (but regularization via `meshopt_SimplifyRegularize` may still be helpful to maintain deformation quality). If bone weights *are* provided as attributes, they will need to be clamped and renormalized after the update to ensure they continue to add up to 1.

Using unique vertex data for each LOD in a chain can improve visual quality, but it comes at a cost of ~doubling vertex memory used (if each LOD is using half the triangles of the previous LOD). To reduce the memory footprint, it is possible to use shared vertices with `meshopt_simplifyWithAttributes` for the first one or two LODs in the chain, and only switch to `meshopt_simplifyWithUpdate` for the remainder. In that case, similarly to the use of `meshopt_simplify` described earlier, care must be taken to optimally arrange the vertices in the original vertex buffer.

### Advanced simplification

`meshopt_simplify*` functions expose additional options and parameters that can be used to control the simplification process in more detail.

For basic customization, a number of options can be passed via `options` bitmask that adjust the behavior of the simplifier:

- `meshopt_SimplifyLockBorder` restricts the simplifier from collapsing edges that are on the border of the mesh. This can be useful for simplifying mesh subsets independently, so that the LODs can be combined without introducing cracks.
- `meshopt_SimplifyErrorAbsolute` changes the error metric from relative to absolute both for the input error limit as well as for the resulting error. This can be used instead of `meshopt_simplifyScale`.
- `meshopt_SimplifySparse` improves simplification performance assuming input indices are a sparse subset of the mesh. This can be useful when simplifying small mesh subsets independently, and is intended to be used for meshlet simplification. For consistency, it is recommended to use absolute errors when sparse simplification is desired, as this flag changes the meaning of the relative errors.
- `meshopt_SimplifyPrune` allows the simplifier to remove isolated components regardless of the topological restrictions inside the component. This is generally recommended for full-mesh simplification as it can improve quality and reduce triangle count; note that with this option, triangles connected to locked vertices may be removed as part of their component.
- `meshopt_SimplifyRegularize` produces more regular triangle sizes and shapes during simplification, at some cost to geometric quality. This can improve geometric quality under deformation such as skinning.
- `meshopt_SimplifyPermissive` allows collapses across attribute discontinuities, except for vertices that are tagged with `meshopt_SimplifyVertex_Protect` via `vertex_lock`.

When using `meshopt_simplifyWithAttributes`, it is also possible to lock certain vertices by providing a `vertex_lock` array that contains a value for each vertex in the mesh, with `meshopt_SimplifyVertex_Lock` set for vertices that should not be collapsed. This can be useful to preserve certain vertices, such as the boundary of the mesh, with more control than `meshopt_SimplifyLockBorder` option provides. When using `meshopt_simplifyWithUpdate`, locking vertices (whether via `vertex_lock` or `meshopt_SimplifyLockBorder`) will also prevent the simplifier from updating their positions and attributes; this can be useful together with `meshopt_SimplifySparse` for meshlet simplification, as meshlets at one level of hierarchy can be simplified together without excessive data copying.

In addition to the `meshopt_SimplifyPrune` flag, you can explicitly prune isolated components by calling the `meshopt_simplifyPrune` function. This can be done before regular simplification or as the only step, which is useful for scenarios like isosurface cleanup. Similar to other simplification functions, the `target_error` argument controls the cutoff of component radius and is specified in relative units (e.g., `1e-2f` will remove components under 1%). If an absolute cutoff is desired, divide the parameter by the factor returned by `meshopt_simplifyScale`.

Simplification currently assumes that the input mesh is using the same material for all triangles. If the mesh uses multiple materials, it is possible to split the mesh into subsets based on the material and simplify each subset independently, using `meshopt_SimplifyLockBorder` or `vertex_lock` to preserve material boundaries; however, this limits the collapses and may reduce the resulting quality. An alternative approach is to encode information about the material into the vertex buffer, ensuring that all three vertices referencing the same triangle have the same material ID; this may require duplicating vertices on the boundary between materials. After this, simplification can be performed as usual, and after simplification per-triangle material information can be computed from the vertex material IDs. There is no need to inform the simplifier of the value of the material ID: the implicit boundaries created by duplicating vertices with conflicting material IDs will be preserved automatically (unless permissive simplification is used, in which case material boundaries should be protected via `vertex_lock`). If the source mesh is already split into subsets with non-overlapping vertex indices, and permissive simplification is not used, it should be sufficient to concatenate the subsets into a single vertex/index buffer and simplify the entire mesh at once; the result can be split back into subsets after simplification.

When generating a LOD chain, you can either re-simplify each LOD from the original mesh or use the previous LOD as the starting point for the next level. The latter approach is more efficient and produces smoother visual transitions between LOD levels while preserving mesh attributes better. With this method, resulting error values from previous levels should be accumulated for LOD selection. Additionally, consider using `meshopt_SimplifySparse` to improve performance when generating deep LOD chains.

### Point cloud simplification

In addition to triangle mesh simplification, this library provides a function to simplify point clouds. The algorithm reduces the point cloud to a specified number of points while preserving the overall appearance, and can optionally take per-point colors into account:

```c++
const float color_weight = 1;
std::vector<unsigned int> indices(target_count);
indices.resize(meshopt_simplifyPoints(&indices[0], &points[0].x, points.size(), sizeof(Point),
    &points[0].r, sizeof(Point), color_weight, target_count));
```

The resulting indices can be used to render the simplified point cloud; to reduce the memory footprint, the point cloud can be reindexed to create an array of points from the indices.

## Efficiency analyzers

While the only way to get precise performance data is to measure performance on the target GPU, it can be valuable to measure the impact of these optimization in a GPU-independent manner. To this end, the library provides analyzers for all three major optimization routines. For each optimization there is a corresponding analyze function, like `meshopt_analyzeOverdraw`, that returns a struct with statistics.

`meshopt_analyzeVertexCache` returns vertex cache statistics. The common metric to use is ACMR - average cache miss ratio, which is the ratio of the total number of vertex invocations to the triangle count. The worst-case ACMR is 3 (GPU has to process 3 vertices for each triangle); on regular grids the optimal ACMR approaches 0.5. On real meshes it usually is in [0.5..1.5] range depending on the amount of vertex splits. One other useful metric is ATVR - average transformed vertex ratio - which represents the ratio of vertex shader invocations to the total vertices, and has the best case of 1.0 regardless of mesh topology (each vertex is transformed once).

`meshopt_analyzeVertexFetch` returns vertex fetch statistics. The main metric it uses is overfetch - the ratio between the number of bytes read from the vertex buffer to the total number of bytes in the vertex buffer. Assuming non-redundant vertex buffers, the best case is 1.0 - each byte is fetched once.

`meshopt_analyzeOverdraw` returns overdraw statistics. The main metric it uses is overdraw - the ratio between the number of pixel shader invocations to the total number of covered pixels, as measured from several different orthographic cameras. The best case for overdraw is 1.0 - each pixel is shaded once.

`meshopt_analyzeCoverage` returns coverage statistics: the ratio of covered pixels to the viewport extent from each cardinal axis. This is not an efficiency measure per se, but it can be used to measure silhouette change after simplification as well as more precise distance based culling, where the amount of view dependent coverage can be estimated by computing a dot product between the view direction and the coverage vector.

Note that all analyzers use approximate models for the relevant GPU units, so the numbers you will get as the result are only a rough approximation of the actual performance.

## Deinterleaved geometry

All of the examples above assume that geometry is represented as a single vertex buffer and a single index buffer. This requires storing all vertex attributes - position, normal, texture coordinate, skinning weights etc. - in a single contiguous struct. However, in some cases using multiple vertex streams may be preferable. In particular, if some passes require only positional data - such as depth pre-pass or shadow map - then it may be beneficial to split it from the rest of the vertex attributes to make sure the bandwidth use during these passes is optimal. On some mobile GPUs a position-only attribute stream also improves efficiency of tiling algorithms.

Most of the functions in this library either only need the index buffer (such as vertex cache optimization) or only need positional information (such as overdraw optimization). However, several tasks require knowledge about all vertex attributes.

For indexing, `meshopt_generateVertexRemap` assumes that there's just one vertex stream; when multiple vertex streams are used, it's necessary to use `meshopt_generateVertexRemapMulti` as follows:

```c++
meshopt_Stream streams[] = {
    {&unindexed_pos[0], sizeof(float) * 3, sizeof(float) * 3},
    {&unindexed_nrm[0], sizeof(float) * 3, sizeof(float) * 3},
    {&unindexed_uv[0], sizeof(float) * 2, sizeof(float) * 2},
};

std::vector<unsigned int> remap(index_count);
size_t vertex_count = meshopt_generateVertexRemapMulti(&remap[0], NULL, index_count, index_count, streams, sizeof(streams) / sizeof(streams[0]));
```

After this `meshopt_remapVertexBuffer` needs to be called once for each vertex stream to produce the correctly reindexed stream. For shadow indexing, similarly `meshopt_generateShadowIndexBufferMulti` is available as a replacement.

Instead of calling `meshopt_optimizeVertexFetch` for reordering vertices in a single vertex buffer for efficiency, calling `meshopt_optimizeVertexFetchRemap` and then calling `meshopt_remapVertexBuffer` for each stream again is recommended.

Finally, when compressing vertex data, `meshopt_encodeVertexBuffer` should be used on each vertex stream separately - this allows the encoder to best utilize correlation between attribute values for different vertices.

## Specialized processing

In addition to the core optimization techniques, the library provides several specialized algorithms for specific rendering techniques and pipeline optimizations that require a particular configuration of vertex and index data.

### Triangle strip conversion

On most hardware, indexed triangle lists are the most efficient way to drive the GPU. However, in some cases triangle strips might prove beneficial:

- On some older GPUs, triangle strips may be a bit more efficient to render
- On extremely memory constrained systems, index buffers for triangle strips could save a bit of memory

This library provides an algorithm for converting a vertex cache optimized triangle list to a triangle strip:

```c++
std::vector<unsigned int> strip(meshopt_stripifyBound(index_count));
unsigned int restart_index = ~0u;
size_t strip_size = meshopt_stripify(&strip[0], indices, index_count, vertex_count, restart_index);
```

Typically you should expect triangle strips to have ~50-60% of indices compared to triangle lists (~1.5-1.8 indices per triangle) and have ~5% worse ACMR.
Note that triangle strips can be stitched with or without restart index support. Using restart indices can result in ~10% smaller index buffers, but on some GPUs restart indices may result in decreased performance.

To reduce the triangle strip size further, it's recommended to use `meshopt_optimizeVertexCacheStrip` instead of `meshopt_optimizeVertexCache` when optimizing for vertex cache. This trades off some efficiency in vertex transform for smaller index buffers.

### Geometry shader adjacency

For algorithms that use geometry shaders and require adjacency information, this library can generate an index buffer with adjacency data:

```c++
std::vector<unsigned int> adjacency(indices.size() * 2);
meshopt_generateAdjacencyIndexBuffer(&adjacency[0], &indices[0], indices.size(), &vertices[0].x, vertices.size(), sizeof(Vertex));
```

This creates an index buffer suitable for rendering with triangle-with-adjacency topology, providing 3 extra vertices per triangle that represent vertices opposite to each triangle's edge. This data can be used to compute silhouettes and perform other types of local geometric processing in geometry shaders. To render the mesh with adjacency data, the index buffer should be used with `D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ`/`VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY`/`GL_TRIANGLES_ADJACENCY` topology.

Note that the use of geometry shaders may have a performance impact on some GPUs; in some cases alternative implementation strategies may be more efficient.

### Tessellation with displacement mapping

For hardware tessellation with crack-free displacement mapping, this library can generate a special index buffer that supports PN-AEN tessellation:

```c++
std::vector<unsigned int> tess(indices.size() * 4);
meshopt_generateTessellationIndexBuffer(&tess[0], &indices[0], indices.size(), &vertices[0].x, vertices.size(), sizeof(Vertex));
```

This generates a 12-vertex patch for each input triangle with the following layout:

- 0, 1, 2: original triangle vertices
- 3, 4: opposing edge for edge 0, 1
- 5, 6: opposing edge for edge 1, 2
- 7, 8: opposing edge for edge 2, 0
- 9, 10, 11: dominant vertices for corners 0, 1, 2

This allows the use of hardware tessellation to implement PN-AEN and/or displacement mapping without cracks along UV seams or normal discontinuities. To render the mesh, the index buffer should be used with `D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST`/`VK_PRIMITIVE_TOPOLOGY_PATCH_LIST` (`patchControlPoints=12`) topology. For more details please refer to the following papers: [Crack-Free Point-Normal Triangles using Adjacent Edge Normals](https://developer.download.nvidia.com/whitepapers/2010/PN-AEN-Triangles-Whitepaper.pdf), [Tessellation on Any Budget](https://www.nvidia.com/content/pdf/gdc2011/john_mcdonald.pdf) and [My Tessellation Has Cracks!](https://developer.download.nvidia.com/assets/gamedev/files/gdc12/GDC12_DUDASH_MyTessellationHasCracks.pdf).

### Visibility buffers

To render geometry into visibility buffers, access to primitive index in fragment shader is required. While it is possible to use `SV_PrimitiveID`/`gl_PrimitiveID` in the fragment shader, this can result in suboptimal performance on some GPUs (notably, AMD RDNA1 and all NVidia GPUs), and may not be supported on mobile or console hardware. Using mesh shaders to generate primitive IDs is efficient but requires hardware support that is not universally available. To work around these limitations, this library provides a way to generate a special index buffer that uses provoking vertex to encode primitive IDs:

```c++
std::vector<unsigned int> provoke(indices.size());
std::vector<unsigned int> reorder(vertices.size() + indices.size() / 3);
reorder.resize(meshopt_generateProvokingIndexBuffer(&provoke[0], &reorder[0], &indices[0], indices.size(), vertices.size()));
```

This generates a special index buffer along with a reorder table that satisfies two constraints:

- `provoke[3 * tri] == tri`
- `reorder[provoke[x]]` refers to the original triangle vertices

To render the mesh with provoking vertex data, the application should use `provoke` as an index buffer and a vertex shader that passes vertex index (`SV_VertexID`/`gl_VertexIndex`) via a `flat`/`nointerpolation` attribute to the fragment shader as a primitive index, and loads vertex data manually by computing the real vertex index based on `reorder` table (`reorder[gl_VertexIndex]`). For more details please refer to [Variable Rate Shading with Visibility Buffer Rendering](https://advances.realtimerendering.com/s2024/content/Hable/Advances_SIGGRAPH_2024_VisibilityVRS-SIGGRAPH_Advances_2024.pptx); naturally, this technique does not require VRS.

> Note: This assumes the provoking vertex is the first vertex of a triangle, which is true for all graphics APIs except OpenGL/WebGL. For OpenGL/WebGL, you may need to rotate each triangle (abc -> bca) in the resulting index buffer, or use the `glProvokingVertex` function (OpenGL 3.2+) or `WEBGL_provoking_vertex` extension (WebGL2) to change the provoking vertex convention. For WebGL2, this is highly recommended to avoid a variety of emulation slowdowns that happen by default if `flat` attributes are used, such as an implicit use of geometry shaders.

Because the order of indices in the resulting index buffer must be preserved exactly for the technique to work, all optimizations that reorder indices (such as vertex cache optimization) must be applied before generating the provoking index buffer. Additionally, if index compression is used, `meshopt_encodeIndexSequence` should be used instead of `meshopt_encodeIndexBuffer` to ensure that the triangles are not rotated during encoding.

## Memory management

Many algorithms allocate temporary memory to store intermediate results or accelerate processing. The amount of memory allocated is a function of various input parameters such as vertex count and index count. By default memory is allocated using `operator new` and `operator delete`; if these operators are overloaded by the application, the overloads will be used instead. Alternatively it's possible to specify custom allocation/deallocation functions using `meshopt_setAllocator`, e.g.

```c++
meshopt_setAllocator(malloc, free);
```

> Note that the library expects the allocation function to either throw in case of out-of-memory (in which case the exception will propagate to the caller) or abort, so technically the use of `malloc` above isn't safe. If you want to handle out-of-memory errors without using C++ exceptions, you can use `setjmp`/`longjmp` instead.

Vertex and index decoders (`meshopt_decodeVertexBuffer`, `meshopt_decodeIndexBuffer`, `meshopt_decodeIndexSequence`) do not allocate memory and work completely within the buffer space provided via arguments.

All functions have bounded stack usage that does not exceed 32 KB for any algorithms.

## Experimental APIs

Several algorithms provided by this library are marked as "experimental"; this status is reflected in the comments as well as the annotation `MESHOPTIMIZER_EXPERIMENTAL` for each function.

APIs that are not experimental (annotated with `MESHOPTIMIZER_API`) are considered stable, which means that library updates will not break compatibility: existing calls should compile (API compatibility), existing binaries should link (ABI compatibility), and existing behavior should not change significantly (for example, floating point parameters will have similar behavior). This does not mean that the output of the algorithms will be identical: future versions may improve the algorithms and produce different results.

APIs that *are* experimental may have their interface change, both in ways that will cause existing calls to not compile, and in ways that may compile but have significantly different behavior (e.g., changes in parameter order, meaning, valid ranges). Experimental APIs may also, in rare cases, be removed from future library versions. It is recommended to carefully read release notes when updating the library if experimental APIs are in use. Some experimental APIs may also lack documentation in this README.

Applications may configure the library to change the attributes of experimental APIs, for example defining `MESHOPTIMIZER_EXPERIMENTAL` as `__attribute__((deprecated))` will emit compiler warnings when experimental APIs are used. When building a shared library with CMake, `MESHOPT_STABLE_EXPORTS` option can be set to only export stable APIs; this produces an ABI-stable shared library that can be updated without recompiling the application code.

Currently, the following APIs are experimental:

- `meshopt_SimplifyPermissive` mode for `meshopt_simplify*` functions (and associated `meshopt_SimplifyVertex_*` flags)

## License

This library is available to anybody free of charge, under the terms of [MIT License](LICENSE.md).

To honor the license agreement, please include attribution into the user-facing product documentation and/or credits, for example using this or similar text:

> Uses meshoptimizer. Copyright (c) 2016-2025, Arseny Kapoulkine
