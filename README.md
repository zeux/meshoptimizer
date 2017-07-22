# meshoptimizer [![Build Status](https://travis-ci.org/zeux/meshoptimizer.svg?branch=master)](https://travis-ci.org/zeux/meshoptimizer) [![Build status](https://ci.appveyor.com/api/projects/status/ptx6p8wmqchivawq?svg=true)](https://ci.appveyor.com/project/zeux/meshoptimizer) [![codecov.io](http://codecov.io/github/zeux/meshoptimizer/coverage.svg?branch=master)](http://codecov.io/github/zeux/meshoptimizer?branch=master)

## Purpose

To make sure GPU can render indexed meshes efficiently, you should optimize them for vertex fetch locality and for vertex cache (to reduce the number of vertex shader invocations). This library provides algorithms for both.

Vertex cache optimization involves reordering triangle indices to make sure that the vertices referenced by the triangles are hitting the built-in fixed size GPU cache (which is usually small, 16-32 vertices, and can have different replacement policies) as much as possible. All algorithms take index data as an input and produce a new index buffer.

Vertex fetch optimization involves reordering vertices (and remapping triangle indices) to make sure that the memory cache that GPU uses to fetch vertex data is utilized efficiently.

You should first optimize your mesh for vertex cache (to get the optimal triangle order), and then optimize the result for vertex fetch (which keeps the triangle order but changes the vertex data) for maximum efficiency.

Additionally the library provides a way to optimize the mesh for overdraw by splitting the mesh into clusters and reordering them using a heuristic that tries to reduce overdraw. The heuristic needs access to vertex positions.

## Vertex cache optimizer

To optimize the index buffer for vertex cache, call:

```c++
meshopt::optimizeVertexCache(index_data, index_data, index_count, vertex_count);
```

The given example optimizes index_data in place; you can also specify a different destination index buffer.

To perform both vertex cache and overdraw optimization, you have to invoke another function on the index buffer produced by `optimizeVertexCache`:

```c++
meshopt::optimizeVertexCache(index_data, index_data, index_count, vertex_count, 16);
meshopt::optimizeOverdraw(index_data, index_data, index_count, vertex_positions, vertex_stride, vertex_count, 16, 1.05f);
```

The first call generates a cache-optimized index sequence that the second call then reorders to get better overdraw results. The overdraw optimizer also needs to read vertex positions, which you have to provide as a pointer to a float3 vector and a stride, similar to glVertexPointer.

You can also provide a threshold that will determine how much the algorithm can compromise the vertex cache hit ratio in favor of overdraw; 1.05 means that the resulting vertex cache hit ratio should be at most 5% worse than a non-overdraw optimized order.

Note that the vertex cache optimization algorithm models the cache as a fixed-size FIFO buffer, which may or may not match hardware. Additionally, overdraw optimization is performed using a view-independent heuristic and as such does not guarantee that the overdraw of the resulting mesh is optimal.

## Vertex fetch optimizer

To optimize the index/vertex buffers for vertex fetch efficiency, call:

```c++
meshopt::optimizeVertexFetch(vertices, vertices, indices, index_count, vertex_count, vertex_size);
```

In a similar fashion to other functions, you have to provide a pointer to the resulting vertex buffer which will be filled with vertices from the source vertex buffer. The given example optimizes vertex and index buffers in place.

Note that the algorithm does not try to model cache replacement precisely and instead just orders vertices in the order of use, which generally produces results that are close to optimal.

## Efficiency analyzers

The library provides analyzers for all three major optimization routines to understand the impact of these optimizations. For each optimization there is a corresponding analyze function, like analyzeOverdraw, that returns a struct with statistics.

`analyzeVertexCache` returns vertex cache statistics. The common metric to use is ACMR - average cache miss ratio, which is the ratio of the total number of vertex invocations to the triangle count. The worst-case ACMR is 3 (GPU has to process 3 vertices for each triangle); on regular grids the optimal ACMR approaches 0.5. On real meshes it usually is in [0.5..1.5] ratio depending on the amount of vertex splits. One other useful metric is ATVR - average transformed vertex ratio - which represents the ratio of vertex shader invocations to the total vertices, and has the best case of 1.0 regardless of mesh topology (each vertex is transformed once).

`analyzeVertexFetch` returns vertex fetch statistics. The main metric it uses is overfetch - the ratio between the number of bytes read from the vertex buffer to the total number of bytes in the vertex buffer. Assuming non-redundant vertex buffers, the best case is 1.0 - each byte is fetched once.

`analyzeOverdraw` returns overdraw statistics. The main metric it uses is overdraw - the ratio between the number of pixel shader invocations to the total number of covered pixels, as measured from several different orthographic cameras. The best case for overdraw is 1.0 - each pixel is shaded once.

Note that all analyzers use approximate models for the relevant GPU units, so the numbers you will get as the result are only a rough approximation of the actual performance.

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE.md).
