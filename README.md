# meshoptimizer [![Build Status](https://travis-ci.org/zeux/meshoptimizer.svg?branch=master)](https://travis-ci.org/zeux/meshoptimizer)

## Purpose

To make sure GPU can render indexed meshes efficiently, you should optimize them for pre-transform cache (to improve vertex fetch locality) and for post-transform cache (to reduce the number of vertex shader invocations). This library provides algorithms for both.

Post-transform cache optimization involves reordering triangle indices to make sure that the vertices referenced by the triangles are hitting the built-in fixed size GPU cache (which is usually small, 16-32 vertices, and can have different replacement policies) as much as possible. All algorithms take index data as an input and produce a new index buffer.

Pre-transform cache optimization involves reordering vertices (and remapping triangle indices) to make sure that the memory cache that GPU uses to fetch vertex data is utilized efficiently.

You should first optimize your mesh for post-transform cache (to get the optimal triangle order), and then optimize the result for pre-transform cache (which keeps the triangle order but changes the vertex data) for maximum efficiency.

The post-transform optimization algorithm is capable of optimizing for overdraw by splitting the mesh into clusters and reordering them using a heuristic that tries to reduce overdraw. The heuristic needs access to vertex positions.

## Post-transform optimizer

The library implements an algorithm [Tipsify](http://gfx.cs.princeton.edu/pubs/Sander_2007_%3ETR/tipsy.pdf) for post-transform cache and overdraw optimization.

To use Tipsify for post-transform optimization, invoke this function:

    optimizePostTransform(new_index_data, index_data, index_count, vertex_count)
    
You have to pass a pointer to the resulting index array as new_index_data, which will be filled with an optimized index sequence.

To use Tipsify for post-transform and overdraw optimization, you have to invoke the function with an additional output argument, which you then have to pass to another function:

    float threshold = 1.05f;
  
    std::vector<unsigned int> temp_clusters;
    optimizePostTransform(temp_index_data, index_data, index_count, vertex_count, 16, &temp_clusters);
    optimizeOverdraw(new_index_data, index_data, index_count, vertex_positions, vertex_stride, vertex_count, clusters, 16, threshold);

The first call generates a cache-optimized index sequence to the temp location as well as a set of clusters that the second call then reorders to get better overdraw results. The overdraw optimizer also needs to read vertex positions, which you have to provide as a pointer to a float3 vector and a stride, similar to glVertexPointer.

You can also provide a threshold that will determine how much the algorithm can compromise the vertex cache hit ratio in favor of overdraw; 1.05 means that the resulting vertex cache hit ratio should be at most 5% worse than a non-overdraw optimized order.

## Pre-transform optimizer

This algorithm is pretty straightforward - it does not try to model cache replacement and instead just orders vertices in the order of their use, which generally produces results that are close to optimal on real meshes. To invoke it, call:

    optimizePreTransform(new_vertices, vertices, indices, index_count, vertex_count, vertex_size)
    
In a similar fashion to other functions, you have to provide a pointer to the resulting vertex buffer which will be filled with vertices from the source vertex buffer.

## Vertex cache analyzer

You can use the provided analyzer to compute approximate statistics for the given vertex data by calling analyzePostTransform function.

The common metric to use is ACMR - average cache miss ratio, which is the ratio of the total number of vertex invocations to the triangle count. The worst-case ACMR is 3 (GPU has to process 3 vertices for each triangle); on regular grids the optimal ACMR approaches 0.5. On real meshes it usually is in [0.5..1.5] ratio depending on the amount of vertex splits.

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE.md)
