# 🐇 meshoptimizer [![Actions Status](https://github.com/zeux/meshoptimizer/workflows/build/badge.svg)](https://github.com/zeux/meshoptimizer/actions) [![codecov.io](https://codecov.io/github/zeux/meshoptimizer/coverage.svg?branch=master)](https://codecov.io/github/zeux/meshoptimizer?branch=master) ![MIT](https://img.shields.io/badge/license-MIT-blue.svg) [![GitHub](https://img.shields.io/badge/repo-github-green.svg)](https://github.com/zeux/meshoptimizer)

## Purpose

When a GPU renders triangle meshes, various stages of the GPU pipeline have to process vertex and index data. The efficiency of these stages depends on the data you feed to them; this library provides algorithms to help optimize meshes for these stages, as well as algorithms to reduce the mesh complexity and storage overhead.

The library provides a C and C++ interface for all algorithms; you can use it from C/C++ or from other languages via FFI (such as P/Invoke). If you want to use this library from Rust, you should use [meshopt crate](https://crates.io/crates/meshopt). JavaScript interface for some algorithms is available through [meshoptimizer.js](https://www.npmjs.com/package/meshoptimizer).

[gltfpack](./gltf/README.md), which is a tool that can automatically optimize glTF files, is developed and distributed alongside the library.

## Installing

meshoptimizer is hosted on GitHub; you can download the latest release using git:

```
git clone -b v0.22 https://github.com/zeux/meshoptimizer.git
```

Alternatively you can [download the .zip archive from GitHub](https://github.com/zeux/meshoptimizer/archive/v0.22.zip).

The library is also available as a Linux package in several distributions ([ArchLinux](https://aur.archlinux.org/packages/meshoptimizer/), [Debian](https://packages.debian.org/libmeshoptimizer), [FreeBSD](https://www.freshports.org/misc/meshoptimizer/), [Nix](https://mynixos.com/nixpkgs/package/meshoptimizer), [Ubuntu](https://packages.ubuntu.com/libmeshoptimizer)), as well as a [Vcpkg port](https://github.com/microsoft/vcpkg/tree/master/ports/meshoptimizer) (see [installation instructions](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started)) and a [Conan package](https://conan.io/center/recipes/meshoptimizer).

[gltfpack](./gltf/README.md) is available as a pre-built binary on [Releases page](https://github.com/zeux/meshoptimizer/releases) or via [npm package](https://www.npmjs.com/package/gltfpack). Native binaries are recommended since they are more efficient and support texture compression.

## Building

meshoptimizer is distributed as a set of C++ source files. To include it into your project, you can use one of the two options:

* Use CMake to build the library (either as a standalone project or as part of your project)
* Add source files to your project's build system

The source files are organized in such a way that you don't need to change your build-system settings, and you only need to add the source files for the algorithms you use. They should build without warnings or special compilation options on all major compilers.

## Pipeline

When optimizing a mesh, you should typically feed it through a set of optimizations (the order is important!):

1. Indexing
2. (optional, discussed last) Simplification
3. Vertex cache optimization
4. Overdraw optimization
5. Vertex fetch optimization
6. Vertex quantization
7. Shadow indexing
8. (optional) Vertex/index buffer compression

## Indexing

Most algorithms in this library assume that a mesh has a vertex buffer and an index buffer. For algorithms to work well and also for GPU to render your mesh efficiently, the vertex buffer has to have no redundant vertices; you can generate an index buffer from an unindexed vertex buffer or reindex an existing (potentially redundant) index buffer as follows:

> Note: meshoptimizer generally works with 32-bit (`unsigned int`) indices, however when using C++ APIs you can use any integer type for index data by using the provided template overloads. By convention, remap tables always use `unsigned int`.

First, generate a remap table from your existing vertex (and, optionally, index) data:

```c++
size_t index_count = face_count * 3;
size_t unindexed_vertex_count = face_count * 3;
std::vector<unsigned int> remap(unindexed_vertex_count); // temporary remap table
size_t vertex_count = meshopt_generateVertexRemap(&remap[0], NULL, index_count, &unindexed_vertices[0], unindexed_vertex_count, sizeof(Vertex));
```

Note that in this case we only have an unindexed vertex buffer; when input mesh has an index buffer, it will need to be passed to `meshopt_generateVertexRemap` instead of `NULL`, along with the correct source vertex count. In either case, the remap table is generated based on binary equivalence of the input vertices, so the resulting mesh will render the same way. Binary equivalence considers all input bytes, including padding which should be zero-initialized if the vertex structure has gaps.

After generating the remap table, you can allocate space for the target vertex buffer (`vertex_count` elements) and index buffer (`index_count` elements) and generate them:

```c++
meshopt_remapIndexBuffer(indices, NULL, index_count, &remap[0]);
meshopt_remapVertexBuffer(vertices, &unindexed_vertices[0], unindexed_vertex_count, sizeof(Vertex), &remap[0]);
```

You can then further optimize the resulting buffers by calling the other functions on them in-place.

`meshopt_generateVertexRemap` uses binary equivalence of vertex data, which is generally a reasonable default; however, in some cases some attributes may have floating point drift causing extra vertices to be generated. For such cases, it may be necessary to quantize some attributes (most importantly, normals and tangents) before generating the remap, or use a custom weld algorithm that supports per-attribute tolerance instead.

## Vertex cache optimization

When the GPU renders the mesh, it has to run the vertex shader for each vertex; usually GPUs have a built-in fixed size cache that stores the transformed vertices (the result of running the vertex shader), and uses this cache to reduce the number of vertex shader invocations. This cache is usually small, 16-32 vertices, and can have different replacement policies; to use this cache efficiently, you have to reorder your triangles to maximize the locality of reused vertex references like so:

```c++
meshopt_optimizeVertexCache(indices, indices, index_count, vertex_count);
```

## Overdraw optimization

After transforming the vertices, GPU sends the triangles for rasterization which results in generating pixels that are usually first ran through the depth test, and pixels that pass it get the pixel shader executed to generate the final color. As pixel shaders get more expensive, it becomes more and more important to reduce overdraw. While in general improving overdraw requires view-dependent operations, this library provides an algorithm to reorder triangles to minimize the overdraw from all directions, which you should run after vertex cache optimization like this:

```c++
meshopt_optimizeOverdraw(indices, indices, index_count, &vertices[0].x, vertex_count, sizeof(Vertex), 1.05f);
```

The overdraw optimizer needs to read vertex positions as a float3 from the vertex; the code snippet above assumes that the vertex stores position as `float x, y, z`.

When performing the overdraw optimization you have to specify a floating-point threshold parameter. The algorithm tries to maintain a balance between vertex cache efficiency and overdraw; the threshold determines how much the algorithm can compromise the vertex cache hit ratio, with 1.05 meaning that the resulting ratio should be at most 5% worse than before the optimization.

## Vertex fetch optimization

After the final triangle order has been established, we still can optimize the vertex buffer for memory efficiency. Before running the vertex shader GPU has to fetch the vertex attributes from the vertex buffer; the fetch is usually backed by a memory cache, and as such optimizing the data for the locality of memory access is important. You can do this by running this code:

```c++
meshopt_optimizeVertexFetch(vertices, indices, index_count, vertices, vertex_count, sizeof(Vertex));
```

This will reorder the vertices in the vertex buffer to try to improve the locality of reference, and rewrite the indices in place to match; if the vertex data is stored using multiple streams, you should use `meshopt_optimizeVertexFetchRemap` instead. This optimization has to be performed on the final index buffer since the optimal vertex order depends on the triangle order.

Note that the algorithm does not try to model cache replacement precisely and instead just orders vertices in the order of use, which generally produces results that are close to optimal.

## Vertex quantization

To optimize memory bandwidth when fetching the vertex data even further, and to reduce the amount of memory required to store the mesh, it is often beneficial to quantize the vertex attributes to smaller types. While this optimization can technically run at any part of the pipeline (and sometimes doing quantization as the first step can improve indexing by merging almost identical vertices), it generally is easier to run this after all other optimizations since some of them require access to float3 positions.

Quantization is usually domain specific; it's common to quantize normals using 3 8-bit integers but you can use higher-precision quantization (for example using 10 bits per component in a 10_10_10_2 format), or a different encoding to use just 2 components. For positions and texture coordinate data the two most common storage formats are half precision floats, and 16-bit normalized integers that encode the position relative to the AABB of the mesh or the UV bounding rectangle.

The number of possible combinations here is very large but this library does provide the building blocks, specifically functions to quantize floating point values to normalized integers, as well as half-precision floats. For example, here's how you can quantize a normal:

```c++
unsigned int normal =
    (meshopt_quantizeUnorm(v.nx, 10) << 20) |
    (meshopt_quantizeUnorm(v.ny, 10) << 10) |
     meshopt_quantizeUnorm(v.nz, 10);
```

and here's how you can quantize a position:

```c++
unsigned short px = meshopt_quantizeHalf(v.x);
unsigned short py = meshopt_quantizeHalf(v.y);
unsigned short pz = meshopt_quantizeHalf(v.z);
```

Since quantized vertex attributes often need to remain in their compact representations for efficient transfer and storage, they are usually dequantized during vertex processing by configuring the GPU vertex input correctly to expect normalized integers or half precision floats, which often needs no or minimal changes to the shader code. When CPU dequantization is required instead, `meshopt_dequantizeHalf` can be used to convert half precision values back to single precision; for normalized integer formats, the dequantization just requires dividing by 2^N-1 for unorm and 2^(N-1)-1 for snorm variants, for example manually reversing `meshopt_quantizeUnorm(v, 10)` can be done by dividing by 1023.

## Shadow indexing

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

## Vertex/index buffer compression

In case storage size or transmission bandwidth is of importance, you might want to additionally compress vertex and index data. While several mesh compression libraries, like Google Draco, are available, they typically are designed to maximize the compression ratio at the cost of disturbing the vertex/index order (which makes the meshes inefficient to render on GPU) or decompression performance. They also frequently don't support custom game-ready quantized vertex formats and thus require to re-quantize the data after loading it, introducing extra quantization errors and making decoding slower.

Alternatively you can use general purpose compression libraries like zstd or Oodle to compress vertex/index data - however these compressors aren't designed to exploit redundancies in vertex/index data and as such compression rates can be unsatisfactory.

To that end, this library provides algorithms to "encode" vertex and index data. The result of the encoding is generally significantly smaller than initial data, and remains compressible with general purpose compressors - so you can either store encoded data directly (for modest compression ratios and maximum decoding performance), or further compress it with zstd/Oodle to maximize compression ratio.

> Note: this compression scheme is available as a glTF extension [EXT_meshopt_compression](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Vendor/EXT_meshopt_compression/README.md).

To encode, you need to allocate target buffers (preferably using the worst case bound) and call encoding functions:

```c++
std::vector<unsigned char> vbuf(meshopt_encodeVertexBufferBound(vertex_count, sizeof(Vertex)));
vbuf.resize(meshopt_encodeVertexBuffer(&vbuf[0], vbuf.size(), vertices, vertex_count, sizeof(Vertex)));

std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(index_count, vertex_count));
ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), indices, index_count));
```

You can then either serialize `vbuf`/`ibuf` as is, or compress them further. To decode the data at runtime, call decoding functions:

```c++
int resvb = meshopt_decodeVertexBuffer(vertices, vertex_count, sizeof(Vertex), &vbuf[0], vbuf.size());
int resib = meshopt_decodeIndexBuffer(indices, index_count, &ibuf[0], ibuf.size());
assert(resvb == 0 && resib == 0);
```

Note that vertex encoding assumes that vertex buffer was optimized for vertex fetch, and that vertices are quantized; index encoding assumes that the vertex/index buffers were optimized for vertex cache and vertex fetch. Feeding unoptimized data into the encoders will produce poor compression ratios. Both codecs are lossless - the only lossy step is quantization that happens before encoding.

Decoding functions are heavily optimized and can directly target write-combined memory; you can expect both decoders to run at 3-5 GB/s on modern desktop CPUs. Compression ratios depend on the data; vertex data compression ratio is typically around 2-4x (compared to already quantized data), index data compression ratio is around 5-6x (compared to raw 16-bit index data). General purpose lossless compressors can further improve on these results.

For additional improvements in compression ratio and decoding performance, it is recommended to switch to vertex codec v1 (via `meshopt_encodeVertexVersion(1)`). This will result in smaller outputs that decode faster, and provide additional control over compression level - `meshopt_encodeVertexBuffer` will use compression level 2 by default, but using `meshopt_encodeVertexBufferLevel` allows to improve compression in certain cases by using level 3, or to reduce compression ratio and improve encoding speed by using level 1. Note that v1 format requires meshoptimizer v0.23 or later for decoding.

Index buffer codec only supports triangle list topology; when encoding triangle strips or line lists, use `meshopt_encodeIndexSequence`/`meshopt_decodeIndexSequence` instead. This codec typically encodes indices into ~1 byte per index, but compressing the results further with a general purpose compressor can improve the results to 1-3 bits per index.

The following guarantees on data compatibility are provided for point releases (*no* guarantees are given for development branch):

- Data encoded with older versions of the library can always be decoded with newer versions;
- Data encoded with newer versions of the library can be decoded with older versions, provided that encoding versions are set correctly; if binary stability of encoded data is important, use `meshopt_encodeVertexVersion` and `meshopt_encodeIndexVersion` to 'pin' the data versions.

Due to a very high decoding performance and compatibility with general purpose lossless compressors, the compression is a good fit for the use on the web. To that end, meshoptimizer provides both vertex and index decoders compiled into WebAssembly and wrapped into a module with JavaScript-friendly interface, `js/meshopt_decoder.js`, that you can use to decode meshes that were encoded offline:

```js
// ready is a Promise that is resolved when (asynchronous) WebAssembly compilation finishes
await MeshoptDecoder.ready;

// decode from *Data (Uint8Array) into *Buffer (Uint8Array)
MeshoptDecoder.decodeVertexBuffer(vertexBuffer, vertexCount, vertexSize, vertexData);
MeshoptDecoder.decodeIndexBuffer(indexBuffer, indexCount, indexSize, indexData);
```

[Usage example](https://meshoptimizer.org/demo/) is available, with source in `demo/index.html`; this example uses .GLB files encoded using `gltfpack`.

## Point cloud compression

The vertex encoding algorithms can be used to compress arbitrary streams of attribute data; one other use case besides triangle meshes is point cloud data. Typically point clouds come with position, color and possibly other attributes but don't have an implied point order.

To compress point clouds efficiently, it's recommended to first preprocess the points by sorting them using the spatial sort algorithm:

```c++
std::vector<unsigned int> remap(point_count);
meshopt_spatialSortRemap(&remap[0], positions, point_count, sizeof(vec3));

// for each attribute stream
meshopt_remapVertexBuffer(positions, positions, point_count, sizeof(vec3), &remap[0]);
```

After this the resulting arrays should be quantized (e.g. using 16-bit fixed point numbers for positions and 8-bit color components), and the result can be compressed using `meshopt_encodeVertexBuffer` as described in the previous section. To decompress, `meshopt_decodeVertexBuffer` will recover the quantized data that can be used directly or converted back to original floating-point data. The compression ratio depends on the nature of source data, for colored points it's typical to get 35-40 bits per point as a result.

## Advanced compression

Both vertex and index codecs are designed to be used in a three-stage pipeline:

- Preparation (quantization, filtering, ordering)
- Encoding (`meshopt_encodeVertexBuffer`/`meshopt_encodeIndexBuffer`)
- Optional compression (LZ4/zlib/zstd/Oodle)

The preparation stage is crucial for achieving good compression ratios; this section will cover some techniques that can be used to improve the results.

The index codec targets 1 byte per triangle as a best case; on real-world data, it's typical to achieve 1-1.2 bytes per triangle. To reach this, the data needs to be optimized for vertex cache and vertex fetch. Optimizations that do not disrupt triangle locality (such as overdraw) are safe to use in between.
To reduce the data size further, it's possible to use `meshopt_optimizeVertexCacheStrip` instead of `meshopt_optimizeVertexCache` when optimizing for vertex cache. This trades off some efficiency in vertex transform for smaller vertex and index data.

When referenced vertex indices are not sequential, the index codec will use around 2 bytes per index. This can happen when the referenced vertices are a sparse subset of the vertex buffer, such as when encoding LODs. General-purpose compression can be especially helpful in this case.

The vertex codec tries to take advantage of the inherent locality of sequential vertices and identify bit patterns that repeat in consecutive vertices. Typically, vertex cache + vertex fetch provides a reasonably local vertex traversal order; without an index buffer, it is recommended to sort vertices spatially to improve the compression ratio.
It is crucial to correctly specify the stride when encoding vertex data; however, it does not matter whether the vertices are interleaved or deinterleaved, as the codecs perform full byte deinterleaving internally.

For optimal compression results, the values must be quantized to small integers. It can be valuable to use bit counts that are not multiples of 8. For example, instead of using 16 bits to represent texture coordinates, use 12-bit integers and divide by 4095 in the shader. Alternatively, using half-precision floats can often achieve good results.
For single-precision floating-point data, it's recommended to use `meshopt_quantizeFloat` to remove entropy from the lower bits of the mantissa. Due to current limitations of the codec, the bit count needs to be 15 (23-8) for good results (7 can be used for more extreme compression).
For normal or tangent vectors, using octahedral encoding is recommended over three components as it reduces redundancy. Similarly to other quantized values, consider using 10-12 bits per component instead of 16.

When data is bit packed, using v1 vertex codec (via `meshopt_encodeVertexVersion(1)`) and specifying compression level 3 (`meshopt_encodeVertexBufferLevel`) can improve the compression further by redistributing bits between components. Note that v1 vertex codec is recommended regardless, as it improves compression ratios and decoding performance even absent bit packing.

To further leverage the inherent structure of some data, the preparation stage can use filters that encode and decode the data in a lossy manner. This is similar to quantization but can be used without having to change the shader code. After decoding, the filter transformation needs to be reversed. For native game engine pipelines, it is usually more optimal to carefully prequantize and pretransform the vertex data, but sometimes (for example when serializing data in glTF format) this is not a practical option and filters are more practical. This library provides three filters:

- Octahedral filter (`meshopt_encodeFilterOct`/`meshopt_decodeFilterOct`) encodes quantized (snorm) normal or tangent vectors using octahedral encoding. Any number of bits <= 16 can be used with 4 bytes or 8 bytes per vector.
- Quaternion filter (`meshopt_encodeFilterQuat`/`meshopt_decodeFilterQuat`) encodes quantized (snorm) quaternion vectors; this can be used to encode rotations or tangent frames. Any number of bits between 4 and 16 can be used with 8 bytes per vector.
- Exponential filter (`meshopt_encodeFilterExp`/`meshopt_decodeFilterExp`) encodes single-precision floating-point vectors; this can be used to encode arbitrary floating-point data more efficiently. In addition to an arbitrary bit count (<= 24), the filter takes a "mode" parameter that allows specifying how the exponent sharing is performed to trade off compression ratio and quality:

    - `meshopt_EncodeExpSeparate` does not share exponents and results in the largest output
    - `meshopt_EncodeExpSharedVector` shares exponents between different components of the same vector
    - `meshopt_EncodeExpSharedComponent` shares exponents between the same component in different vectors
    - `meshopt_EncodeExpClamped` does not share exponents but clamps the exponent range to reduce exponent entropy

Note that all filters are lossy and require the data to be deinterleaved with one attribute per stream; this faciliates efficient SIMD implementation of filter decoders, allowing the overall decompression speed to be closer to that of the raw codec.

## Triangle strip conversion

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

Finally, when compressing vertex data, `meshopt_encodeVertexBuffer` should be used on each vertex stream separately - this allows the encoder to best utilize corellation between attribute values for different vertices.

## Simplification

All algorithms presented so far don't affect visual appearance at all, with the exception of quantization that has minimal controlled impact. However, fundamentally the most effective way at reducing the rendering or transmission cost of a mesh is to make the mesh simpler.

This library provides two simplification algorithms that reduce the number of triangles in the mesh. Given a vertex and an index buffer, they generate a second index buffer that uses existing vertices in the vertex buffer. This index buffer can be used directly for rendering with the original vertex buffer (preferably after vertex cache optimization using `meshopt_optimizeVertexCache`), or a new compact vertex/index buffer can be generated using `meshopt_optimizeVertexFetch` that uses the optimal number and order of vertices.

The first simplification algorithm, `meshopt_simplify`, follows the topology of the original mesh in an attempt to preserve attribute seams, borders and overall appearance. For meshes with inconsistent topology or many seams, such as faceted meshes, it can result in simplifier getting "stuck" and not being able to simplify the mesh fully. Therefore it's critical that identical vertices are "welded" together, that is, the input vertex buffer does not contain duplicates. Additionally, it may be worthwhile to weld the vertices without taking into account vertex attributes that aren't critical and can be rebuilt later.

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

The second simplification algorithm, `meshopt_simplifySloppy`, doesn't follow the topology of the original mesh. This means that it doesn't preserve attribute seams or borders, but it can collapse internal details that are too small to matter better because it can merge mesh features that are topologically disjoint but spatially close.

```c++
float threshold = 0.2f;
size_t target_index_count = size_t(index_count * threshold);
float target_error = 1e-1f;

std::vector<unsigned int> lod(index_count);
float lod_error = 0.f;
lod.resize(meshopt_simplifySloppy(&lod[0], indices, index_count, &vertices[0].x, vertex_count, sizeof(Vertex),
    target_index_count, target_error, &lod_error));
```

This algorithm will not stop early due to topology restrictions but can still do so if target index count can't be reached without introducing an error larger than target. It is 5-6x faster than `meshopt_simplify` when simplification ratio is large, and is able to reach ~20M triangles/sec on a desktop CPU (`meshopt_simplify` works at ~3M triangles/sec).

Both algorithms can also return the resulting normalized deviation that can be used to choose the correct level of detail based on screen size or solid angle; the error can be converted to object space by multiplying by the scaling factor returned by `meshopt_simplifyScale`. For example, given a mesh with a precomputed LOD and a prescaled error, the screen-space normalized error can be computed and used for LOD selection:

```c++
// lod_factor can be 1 or can be adjusted for more or less aggressive LOD selection
float d = max(0, distance(camera_position, mesh_center) - mesh_radius);
float e = d * (tan(camera_fovy / 2) * 2 / screen_height); // 1px in mesh space
bool lod_ok = e * lod_factor >= lod_error;
```

When a sequence of LOD meshes is generated that all use the original vertex buffer, care must be taken to order vertices optimally to not penalize mobile GPU architectures that are only capable of transforming a sequential vertex buffer range. It's recommended in this case to first optimize each LOD for vertex cache, then assemble all LODs in one large index buffer starting from the coarsest LOD (the one with fewest triangles), and call `meshopt_optimizeVertexFetch` on the final large index buffer. This will make sure that coarser LODs require a smaller vertex range and are efficient wrt vertex fetch and transform.

## Advanced simplification

The main simplification algorithm, `meshopt_simplify`, exposes additional options and functions that can be used to control the simplification process in more detail.

For basic customization, a number of options can be passed via `options` bitmask that adjust the behavior of the simplifier:

- `meshopt_SimplifyLockBorder` restricts the simplifier from collapsing edges that are on the border of the mesh. This can be useful for simplifying mesh subsets independently, so that the LODs can be combined without introducing cracks.
- `meshopt_SimplifyErrorAbsolute` changes the error metric from relative to absolute both for the input error limit as well as for the resulting error. This can be used instead of `meshopt_simplifyScale`.
- `meshopt_SimplifySparse` improves simplification performance assuming input indices are a sparse subset of the mesh. This can be useful when simplifying small mesh subsets independently, and is intended to be used for meshlet simplification. For consistency, it is recommended to use absolute errors when sparse simplification is desired, as this flag changes the meaning of the relative errors.
- `meshopt_SimplifyPrune` allows the simplifier to remove isolated components regardless of the topological restrictions inside the component. This is generally recommended for full-mesh simplification as it can improve quality and reduce triangle count; note that with this option, triangles connected to locked vertices may be removed as part of their component.

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

Both the target error and the resulting error combine positional error and attribute error, so the error can be used to control the LOD while taking attribute quality into account, assuming carefully chosen weights.

When using `meshopt_simplifyWithAttributes`, it is also possible to lock certain vertices by providing a `vertex_lock` array that contains a boolean value for each vertex in the mesh. This can be useful to preserve certain vertices, such as the boundary of the mesh, with more control than `meshopt_SimplifyLockBorder` option provides.

Simplification currently assumes that the input mesh is using the same material for all triangles. If the mesh uses multiple materials, it is possible to split the mesh into subsets based on the material and simplify each subset independently, using `meshopt_SimplifyLockBorder` or `vertex_lock` to preserve material boundaries; however, this limits the collapses and as a result may reduce the resulting quality. An alternative approach is to encode information about the material into the vertex buffer, ensuring that all three vertices referencing the same triangle have the same material ID; this may require duplicating vertices on the boundary between materials. After this, simplification can be performed as usual, and after simplification per-triangle material information can be computed from the vertex material IDs. There is no need to inform the simplifier of the value of the material ID: the implicit boundaries created by duplicating vertices with conflicting material IDs will be preserved automatically.

## Point cloud simplification

In addition to triangle mesh simplification, this library provides a function to simplify point clouds. The algorithm reduces the point cloud to a specified number of points while preserving the overall appearance, and can optionally take per-point colors into account:

```c++
const float color_weight = 1;
std::vector<unsigned int> indices(target_count);
indices.resize(meshopt_simplifyPoints(&indices[0], &points[0].x, points.size(), sizeof(Point),
    &points[0].r, sizeof(Point), color_weight, target_count));
```

The resulting indices can be used to render the simplified point cloud; to reduce the memory footprint, the point cloud can be reindexed to create an array of points from the indices.

## Mesh shading

Modern GPUs are beginning to deviate from the traditional rasterization model. NVidia GPUs starting from Turing and AMD GPUs starting from RDNA2 provide a new programmable geometry pipeline that, instead of being built around index buffers and vertex shaders, is built around mesh shaders - a new shader type that allows to provide a batch of work to the rasterizer.

Using mesh shaders in context of traditional mesh rendering provides an opportunity to use a variety of optimization techniques, starting from more efficient vertex reuse, using various forms of culling (e.g. cluster frustum or occlusion culling) and in-memory compression to maximize the utilization of GPU hardware. Beyond traditional rendering mesh shaders provide a richer programming model that can synthesize new geometry more efficiently than common alternatives such as geometry shaders. Mesh shading can be accessed via Vulkan or Direct3D 12 APIs; please refer to [Introduction to Turing Mesh Shaders](https://developer.nvidia.com/blog/introduction-turing-mesh-shaders/) and [Mesh Shaders and Amplification Shaders: Reinventing the Geometry Pipeline](https://devblogs.microsoft.com/directx/coming-to-directx-12-mesh-shaders-and-amplification-shaders-reinventing-the-geometry-pipeline/) for additional information.

To use mesh shaders for conventional rendering efficiently, geometry needs to be converted into a series of meshlets; each meshlet represents a small subset of the original mesh and comes with a small set of vertices and a separate micro-index buffer that references vertices in the meshlet. This information can be directly fed to the rasterizer from the mesh shader. This library provides algorithms to create meshlet data for a mesh, and - assuming geometry is static - can compute bounding information that can be used to perform cluster culling, a technique that can reject a meshlet if it's invisible on screen.

To generate meshlet data, this library provides `meshopt_buildMeshlets` algorithm, which tries to balance topological efficiency (by maximizing vertex reuse inside meshlets) with culling efficiency (by minimizing meshlet radius and triangle direction divergence) and produces GPU-friendly data. As an alternative (that can be useful for load-time processing), `meshopt_buildMeshletsScan` can create the meshlet data using a vertex cache-optimized index buffer as a starting point by greedily aggregating consecutive triangles until they go over the meshlet limits. `meshopt_buildMeshlets` is recommended for offline data processing even if cone culling is not used.

```c++
const size_t max_vertices = 64;
const size_t max_triangles = 124;
const float cone_weight = 0.0f;

size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);
std::vector<meshopt_Meshlet> meshlets(max_meshlets);
std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices.data(),
    indices.size(), &vertices[0].x, vertices.size(), sizeof(Vertex), max_vertices, max_triangles, cone_weight);
```

To generate the meshlet data, `max_vertices` and `max_triangles` need to be set within limits supported by the hardware; for NVidia the values of 64 and 124 are recommended (`max_triangles` must be divisible by 4 so 124 is the value closest to official NVidia's recommended 126). `cone_weight` should be left as 0 if cluster cone culling is not used, and set to a value between 0 and 1 to balance cone culling efficiency with other forms of culling like frustum or occlusion culling.

Each resulting meshlet refers to a portion of `meshlet_vertices` and `meshlet_triangles` arrays; the arrays are overallocated for the worst case so it's recommended to trim them before saving them as an asset / uploading them to the GPU:

```c++
const meshopt_Meshlet& last = meshlets[meshlet_count - 1];

meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
meshlets.resize(meshlet_count);
```

However depending on the application other strategies of storing the data can be useful; for example, `meshlet_vertices` serves as indices into the original vertex buffer but it might be worthwhile to generate a mini vertex buffer for each meshlet to remove the extra indirection when accessing vertex data, or it might be desirable to compress vertex data as vertices in each meshlet are likely to be very spatially coherent.

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

After generating the meshlet data, it's also possible to generate extra data for each meshlet that can be saved and used at runtime to perform cluster culling, where each meshlet can be discarded if it's guaranteed to be invisible. To generate the data, `meshlet_computeMeshletBounds` can be used:

```c++
meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[m.vertex_offset], &meshlet_triangles[m.triangle_offset],
    m.triangle_count, &vertices[0].x, vertices.size(), sizeof(Vertex));
```

The resulting `bounds` values can be used to perform frustum or occlusion culling using the bounding sphere, or cone culling using the cone axis/angle (which will reject the entire meshlet if all triangles are guaranteed to be back-facing from the camera point of view):

```c++
if (dot(normalize(cone_apex - camera_position), cone_axis) >= cone_cutoff) reject();
```

## Efficiency analyzers

While the only way to get precise performance data is to measure performance on the target GPU, it can be valuable to measure the impact of these optimization in a GPU-independent manner. To this end, the library provides analyzers for all three major optimization routines. For each optimization there is a corresponding analyze function, like `meshopt_analyzeOverdraw`, that returns a struct with statistics.

`meshopt_analyzeVertexCache` returns vertex cache statistics. The common metric to use is ACMR - average cache miss ratio, which is the ratio of the total number of vertex invocations to the triangle count. The worst-case ACMR is 3 (GPU has to process 3 vertices for each triangle); on regular grids the optimal ACMR approaches 0.5. On real meshes it usually is in [0.5..1.5] range depending on the amount of vertex splits. One other useful metric is ATVR - average transformed vertex ratio - which represents the ratio of vertex shader invocations to the total vertices, and has the best case of 1.0 regardless of mesh topology (each vertex is transformed once).

`meshopt_analyzeVertexFetch` returns vertex fetch statistics. The main metric it uses is overfetch - the ratio between the number of bytes read from the vertex buffer to the total number of bytes in the vertex buffer. Assuming non-redundant vertex buffers, the best case is 1.0 - each byte is fetched once.

`meshopt_analyzeOverdraw` returns overdraw statistics. The main metric it uses is overdraw - the ratio between the number of pixel shader invocations to the total number of covered pixels, as measured from several different orthographic cameras. The best case for overdraw is 1.0 - each pixel is shaded once.

Note that all analyzers use approximate models for the relevant GPU units, so the numbers you will get as the result are only a rough approximation of the actual performance.

## Specialized processing

In addition to the core optimization techniques, the library provides several specialized algorithms for specific rendering techniques and pipeline optimizations that require a particular configuration of vertex and index data.

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

Note: This assumes the provoking vertex is the first vertex of a triangle, which is true for all graphics APIs except OpenGL/WebGL. For OpenGL/WebGL, you may need to rotate each triangle (abc -> bca) in the resulting index buffer, or use the `glProvokingVertex` function (OpenGL 3.2+) or `WEBGL_provoking_vertex` extension (WebGL2) to change the provoking vertex convention. For WebGL2, this is highly recommended to avoid a variety of emulation slowdowns that happen by default if `flat` attributes are used, such as an implicit use of geometry shaders.

## Memory management

Many algorithms allocate temporary memory to store intermediate results or accelerate processing. The amount of memory allocated is a function of various input parameters such as vertex count and index count. By default memory is allocated using `operator new` and `operator delete`; if these operators are overloaded by the application, the overloads will be used instead. Alternatively it's possible to specify custom allocation/deallocation functions using `meshopt_setAllocator`, e.g.

```c++
meshopt_setAllocator(malloc, free);
```

> Note that the library expects the allocation function to either throw in case of out-of-memory (in which case the exception will propagate to the caller) or abort, so technically the use of `malloc` above isn't safe. If you want to handle out-of-memory errors without using C++ exceptions, you can use `setjmp`/`longjmp` instead.

Vertex and index decoders (`meshopt_decodeVertexBuffer`, `meshopt_decodeIndexBuffer`, `meshopt_decodeIndexSequence`) do not allocate memory and work completely within the buffer space provided via arguments.

All functions have bounded stack usage that does not exceed 32 KB for any algorithms.

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE.md).
