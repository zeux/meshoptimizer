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
- And more...

## Usage

### Basic Usage

```python
import numpy as np
from meshoptimizer import Mesh

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

# Create a mesh
mesh = Mesh(vertices, indices)

# Optimize the mesh
mesh.optimize_vertex_cache()
mesh.optimize_overdraw()
mesh.optimize_vertex_fetch()

# Simplify the mesh
mesh.simplify(target_ratio=0.5)  # Keep 50% of triangles

# Encode the mesh for efficient transmission
encoded = mesh.encode()

# Decode the mesh
decoded = Mesh.decode(
    encoded,
    vertex_count=len(mesh.vertices),
    vertex_size=mesh.vertices.itemsize * mesh.vertices.shape[1],
    index_count=len(mesh.indices)
)
```

### Low-level API

If you need more control, you can use the low-level API directly:

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

# Optimize vertex cache
optimized_indices = np.zeros_like(indices)
optimize_vertex_cache(optimized_indices, indices, len(indices), len(vertices))

# Optimize overdraw
optimized_indices2 = np.zeros_like(indices)
optimize_overdraw(
    optimized_indices2,
    optimized_indices,
    vertices,
    len(indices),
    len(vertices),
    vertices.itemsize * vertices.shape[1],
    1.05  # threshold
)

# And so on...
```

## Notes on Index Encoding/Decoding

When using the index buffer encoding and decoding functions, note that the decoded indices may not exactly match the original indices, even though the mesh geometry remains the same. This is due to how the meshoptimizer library internally encodes and decodes the indices. The triangles may be in a different order, but the resulting mesh is still valid and represents the same geometry.

## API Reference

### High-level API

#### `Mesh` class

- `__init__(vertices, indices=None)`: Initialize a mesh with vertices and optional indices.
- `optimize_vertex_cache()`: Optimize the mesh for vertex cache efficiency.
- `optimize_overdraw(threshold=1.05)`: Optimize the mesh for overdraw.
- `optimize_vertex_fetch()`: Optimize the mesh for vertex fetch efficiency.
- `simplify(target_ratio=0.25, target_error=0.01, options=0)`: Simplify the mesh.
- `encode()`: Encode the mesh for efficient transmission.
- `decode(encoded_data, vertex_count, vertex_size, index_count=None, index_size=4)` (class method): Decode an encoded mesh.

### Low-level API

#### Vertex Remapping

- `generate_vertex_remap(destination, indices, index_count, vertices, vertex_count, vertex_size)`: Generate vertex remap table.
- `remap_vertex_buffer(destination, vertices, vertex_count, vertex_size, remap)`: Remap vertex buffer.
- `remap_index_buffer(destination, indices, index_count, remap)`: Remap index buffer.

#### Optimization

- `optimize_vertex_cache(destination, indices, index_count, vertex_count)`: Optimize vertex cache.
- `optimize_vertex_cache_strip(destination, indices, index_count, vertex_count)`: Optimize vertex cache for strip-like caches.
- `optimize_vertex_cache_fifo(destination, indices, index_count, vertex_count, cache_size)`: Optimize vertex cache for FIFO caches.
- `optimize_overdraw(destination, indices, vertex_positions, index_count, vertex_count, vertex_positions_stride, threshold)`: Optimize overdraw.
- `optimize_vertex_fetch(destination_vertices, indices, source_vertices, index_count, vertex_count, vertex_size)`: Optimize vertex fetch.
- `optimize_vertex_fetch_remap(destination, indices, index_count, vertex_count)`: Generate vertex remap to optimize vertex fetch.

#### Simplification

- `simplify(destination, indices, vertex_positions, index_count, vertex_count, vertex_positions_stride, target_index_count, target_error, options, result_error)`: Simplify mesh.
- `simplify_with_attributes(destination, indices, vertex_positions, vertex_attributes, attribute_weights, index_count, vertex_count, vertex_positions_stride, vertex_attributes_stride, attribute_count, vertex_lock, target_index_count, target_error, options, result_error)`: Simplify mesh with attribute metric.
- `simplify_sloppy(destination, indices, vertex_positions, index_count, vertex_count, vertex_positions_stride, target_index_count, target_error, result_error)`: Simplify mesh (sloppy).
- `simplify_points(destination, vertex_positions, vertex_colors, vertex_count, vertex_positions_stride, vertex_colors_stride, color_weight, target_vertex_count)`: Simplify point cloud.
- `simplify_scale(vertex_positions, vertex_count, vertex_positions_stride)`: Get the scale factor for simplification error.

#### Encoding/Decoding

- `encode_vertex_buffer(vertices, vertex_count, vertex_size)`: Encode vertex buffer.
- `encode_index_buffer(indices, index_count, vertex_count)`: Encode index buffer.
- `encode_vertex_version(version)`: Set vertex encoder format version.
- `encode_index_version(version)`: Set index encoder format version.
- `decode_vertex_buffer(destination, vertex_count, vertex_size, buffer)`: Decode vertex buffer.
- `decode_index_buffer(destination, index_count, index_size, buffer)`: Decode index buffer.
- `decode_vertex_version(buffer)`: Get encoded vertex format version.
- `decode_index_version(buffer)`: Get encoded index format version.
- `decode_filter_oct(buffer, count, stride)`: Apply octahedral filter to decoded data.
- `decode_filter_quat(buffer, count, stride)`: Apply quaternion filter to decoded data.
- `decode_filter_exp(buffer, count, stride)`: Apply exponential filter to decoded data.

## License

MIT License