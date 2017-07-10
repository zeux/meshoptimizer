# meshoptimizer [![Build Status](https://travis-ci.org/zeux/meshoptimizer.svg?branch=master)](https://travis-ci.org/zeux/meshoptimizer) [![Build status](https://ci.appveyor.com/api/projects/status/ptx6p8wmqchivawq?svg=true)](https://ci.appveyor.com/project/zeux/meshoptimizer)

## Purpose

To make sure GPU can render indexed meshes efficiently, you should optimize them for vertex fetch locality and for vertex cache (to reduce the number of vertex shader invocations). This library provides algorithms for both.

Vertex cache optimization involves reordering triangle indices to make sure that the vertices referenced by the triangles are hitting the built-in fixed size GPU cache (which is usually small, 16-32 vertices, and can have different replacement policies) as much as possible. All algorithms take index data as an input and produce a new index buffer.

Vertex fetch optimization involves reordering vertices (and remapping triangle indices) to make sure that the memory cache that GPU uses to fetch vertex data is utilized efficiently.

You should first optimize your mesh for vertex cache (to get the optimal triangle order), and then optimize the result for vertex fetch (which keeps the triangle order but changes the vertex data) for maximum efficiency.

The vertex cache optimization algorithm is capable of optimizing for overdraw by splitting the mesh into clusters and reordering them using a heuristic that tries to reduce overdraw. The heuristic needs access to vertex positions.

## Vertex cache optimizer

Vertex cache optimizer models the cache as a fixed-size FIFO buffer (which defaults to 16 vertices but can be customized by changing the function arguments). To invoke it, call:

    meshopt::optimizeVertexCache(index_data, index_data, index_count, vertex_count);
    
index_data will be filled with an optimized index sequence. Note that you can run this algorithm in place, or specify a different destination index buffer.

To perform both vertex cache and overdraw optimization, you have to invoke the function with additional output arguments, which you then have to pass to another function:

    float threshold = 1.05f;
  
    std::vector<unsigned int> clusters(index_count / 3);
	size_t cluster_count = 0;
    meshopt::optimizeVertexCache(index_data, index_data, index_count, vertex_count, 16, &clusters[0], &cluster_count);
    meshopt::optimizeOverdraw(index_data, index_data, index_count, vertex_positions, vertex_stride, vertex_count, &clusters[0], cluster_count, 16, threshold);

The first call generates a cache-optimized index sequence as well as a set of clusters that the second call then reorders to get better overdraw results. The overdraw optimizer also needs to read vertex positions, which you have to provide as a pointer to a float3 vector and a stride, similar to glVertexPointer.

You can also provide a threshold that will determine how much the algorithm can compromise the vertex cache hit ratio in favor of overdraw; 1.05 means that the resulting vertex cache hit ratio should be at most 5% worse than a non-overdraw optimized order.

## Vertex fetch optimizer

This algorithm is pretty straightforward - it does not try to model cache replacement and instead just orders vertices in the order of their use, which generally produces results that are close to optimal on real meshes. To invoke it, call:

    meshopt::optimizeVertexFetch(new_vertices, vertices, indices, index_count, vertex_count, vertex_size);
    
In a similar fashion to other functions, you have to provide a pointer to the resulting vertex buffer which will be filled with vertices from the source vertex buffer.

## Efficiency analyzers

The library provides analyzers for all three major optimization routines to understand the impact of these optimizations. For each optimization there is a corresponding analyze function, like analyzeOverdraw, that returns a struct with statistics.

analyzeVertexCache returns vertex cache statistics. The common metric to use is ACMR - average cache miss ratio, which is the ratio of the total number of vertex invocations to the triangle count. The worst-case ACMR is 3 (GPU has to process 3 vertices for each triangle); on regular grids the optimal ACMR approaches 0.5. On real meshes it usually is in [0.5..1.5] ratio depending on the amount of vertex splits. One other useful metric is ATVR - average transformed vertex ratio - which represents the ratio of vertex shader invocations to the total vertices, and has the best case of 1.0 regardless of mesh topology (each vertex is transformed once).

analyzeVertexFetch returns vertex fetch statistics. The main metric it uses is overfetch - the ratio between the number of bytes read from the vertex buffer to the total number of bytes in the vertex buffer. Assuming non-redundant vertex buffers, the best case is 1.0 - each byte is fetched once.

analyzeOverdraw returns overdraw statistics. The main metric it uses is overdraw - the ratio between the number of pixel shader invocations to the total number of covered pixels, as measured from several different orthographic cameras. The best case for overdraw is 1.0 - each pixel is shaded once.

Note that all analyzers use approximate models for the relevant GPU units, so the numbers you will get as the result are only a rough approximation of the actual performance.

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE.md).
