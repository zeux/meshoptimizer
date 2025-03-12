# Meshoptimizer Python

Python bindings for the [meshoptimizer](https://github.com/zeux/meshoptimizer) library, which provides algorithms for optimizing 3D meshes for GPU rendering.

## Installation

```bash
pip install meshoptimizer
```

Or install from source:

```bash
git clone https://github.com/zeux/meshoptimizer.git
cd meshoptimizer/python
pip install -e .
```

## Features

- Vertex cache optimization
- Overdraw optimization
- Vertex fetch optimization
- Mesh simplification
- Vertex/index buffer compression and decompression
- Zip file storage for encoded meshes
- Numpy array compression and storage
- And more...

## Usage

### Low-level API

The meshoptimizer Python bindings provide a low-level API that directly maps to the C++ functions. All functions accept numpy arrays and have optional parameters that are automatically derived when not provided.

```python
import numpy as np
from meshoptimizer import (
    optimize_vertex_cache,
    optimize_overdraw,
    optimize_vertex_fetch,
    simplify,
    encode_vertex_buffer,
    decode_vertex_buffer,
    encode_index_buffer,
    decode_index_buffer
)

# Create a simple mesh (a cube)
vertices = np.array([
    # positions          
    [-0.5, -0.5, -0.5],
    [0.5, -0.5, -0.5],
    [0.5, 0.5, -0.5],
    [-0.5, 0.5, -0.5],
    [-0.5, -0.5, 0.5],
    [0.5, -0.5, 0.5],
    [0.5, 0.5, 0.5],
    [-0.5, 0.5, 0.5]
], dtype=np.float32)

indices = np.array([
    0, 1, 2, 2, 3, 0,  # front
    1, 5, 6, 6, 2, 1,  # right
    5, 4, 7, 7, 6, 5,  # back
    4, 0, 3, 3, 7, 4,  # left
    3, 2, 6, 6, 7, 3,  # top
    4, 5, 1, 1, 0, 4   # bottom
], dtype=np.uint32)

# Optimize vertex cache
optimized_indices = np.zeros_like(indices)
optimize_vertex_cache(optimized_indices, indices)  # vertex_count is automatically derived

# Optimize overdraw
optimized_indices2 = np.zeros_like(indices)
optimize_overdraw(
    optimized_indices2,
    optimized_indices,
    vertices
)  # index_count, vertex_count, and vertex_positions_stride are automatically derived

# Optimize vertex fetch
optimized_vertices = np.zeros_like(vertices)
unique_vertex_count = optimize_vertex_fetch(
    optimized_vertices,
    optimized_indices2,
    vertices
)  # index_count, vertex_count, and vertex_size are automatically derived

print(f"Optimized mesh has {unique_vertex_count} unique vertices")

# Simplify the mesh
simplified_indices = np.zeros(len(indices), dtype=np.uint32)
target_index_count = len(indices) // 2  # Keep 50% of triangles

simplified_index_count = simplify(
    simplified_indices,
    optimized_indices2,
    vertices,
    target_index_count=target_index_count
)  # index_count, vertex_count, and vertex_positions_stride are automatically derived

print(f"Simplified mesh has {simplified_index_count} indices")

# Encode the mesh for efficient transmission
encoded_vertices = encode_vertex_buffer(optimized_vertices[:unique_vertex_count])
encoded_indices = encode_index_buffer(simplified_indices[:simplified_index_count])

print(f"Encoded vertex buffer size: {len(encoded_vertices)} bytes")
print(f"Encoded index buffer size: {len(encoded_indices)} bytes")

# Decode the mesh
decoded_vertices = decode_vertex_buffer(
    unique_vertex_count,
    vertices.itemsize * vertices.shape[1],
    encoded_vertices
)

decoded_indices = decode_index_buffer(
    simplified_index_count,
    4,  # 4 bytes per index (uint32)
    encoded_indices
)

# Verify the decoded data
print(f"Decoded vertices shape: {decoded_vertices.shape}")
print(f"Decoded indices shape: {decoded_indices.shape}")
```

## Notes on Index Encoding/Decoding

When using the index buffer encoding and decoding functions, note that the decoded indices may not exactly match the original indices, even though the mesh geometry remains the same. This is due to how the meshoptimizer library internally encodes and decodes the indices. The triangles may be in a different order, but the resulting mesh is still valid and represents the same geometry.

## API Reference

### Vertex Remapping

- `generate_vertex_remap(destination, indices=None, index_count=None, vertices=None, vertex_count=None, vertex_size=None)`: Generate vertex remap table.
- `remap_vertex_buffer(destination, vertices, vertex_count=None, vertex_size=None, remap=None)`: Remap vertex buffer.
- `remap_index_buffer(destination, indices, index_count=None, remap=None)`: Remap index buffer.

### Optimization

- `optimize_vertex_cache(destination, indices, index_count=None, vertex_count=None)`: Optimize vertex cache.
- `optimize_vertex_cache_strip(destination, indices, index_count=None, vertex_count=None)`: Optimize vertex cache for strip-like caches.
- `optimize_vertex_cache_fifo(destination, indices, index_count=None, vertex_count=None, cache_size=16)`: Optimize vertex cache for FIFO caches.
- `optimize_overdraw(destination, indices, vertex_positions, index_count=None, vertex_count=None, vertex_positions_stride=None, threshold=1.05)`: Optimize overdraw.
- `optimize_vertex_fetch(destination_vertices, indices, source_vertices, index_count=None, vertex_count=None, vertex_size=None)`: Optimize vertex fetch.
- `optimize_vertex_fetch_remap(destination, indices, index_count=None, vertex_count=None)`: Generate vertex remap to optimize vertex fetch.

### Simplification

- `simplify(destination, indices, vertex_positions, index_count=None, vertex_count=None, vertex_positions_stride=None, target_index_count=None, target_error=0.01, options=0, result_error=None)`: Simplify mesh.
- `simplify_with_attributes(destination, indices, vertex_positions, vertex_attributes, attribute_weights, index_count=None, vertex_count=None, vertex_positions_stride=None, vertex_attributes_stride=None, attribute_count=None, vertex_lock=None, target_index_count=None, target_error=0.01, options=0, result_error=None)`: Simplify mesh with attribute metric.
- `simplify_sloppy(destination, indices, vertex_positions, index_count=None, vertex_count=None, vertex_positions_stride=None, target_index_count=None, target_error=0.01, result_error=None)`: Simplify mesh (sloppy).
- `simplify_points(destination, vertex_positions, vertex_colors=None, vertex_count=None, vertex_positions_stride=None, vertex_colors_stride=None, color_weight=1.0, target_vertex_count=None)`: Simplify point cloud.
- `simplify_scale(vertex_positions, vertex_count=None, vertex_positions_stride=None)`: Get the scale factor for simplification error.

### Encoding/Decoding

- `encode_vertex_buffer(vertices, vertex_count=None, vertex_size=None)`: Encode vertex buffer.
- `encode_index_buffer(indices, index_count=None, vertex_count=None)`: Encode index buffer.
- `encode_vertex_version(version)`: Set vertex encoder format version.
- `encode_index_version(version)`: Set index encoder format version.
- `decode_vertex_buffer(vertex_count, vertex_size, buffer)`: Decode vertex buffer.
- `decode_index_buffer(index_count, index_size, buffer)`: Decode index buffer.
- `decode_vertex_version(buffer)`: Get encoded vertex format version.
- `decode_index_version(buffer)`: Get encoded index format version.
- `decode_filter_oct(buffer, count, stride)`: Apply octahedral filter to decoded data.
- `decode_filter_quat(buffer, count, stride)`: Apply quaternion filter to decoded data.
- `decode_filter_exp(buffer, count, stride)`: Apply exponential filter to decoded data.

## License

MIT License