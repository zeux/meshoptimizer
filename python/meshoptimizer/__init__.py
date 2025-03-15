"""
Python wrapper for the meshoptimizer library.

This package provides Python bindings for the meshoptimizer C++ library,
which offers various algorithms for optimizing 3D meshes for GPU rendering.
It also provides utilities for compressing and storing numpy arrays.

High-level functionality is available in the 'export' submodule.
"""

from .encoder import (
    encode_vertex_buffer,
    encode_index_buffer,
    encode_index_sequence,
    encode_vertex_version,
    encode_index_version,
)

from .decoder import (
    decode_vertex_buffer,
    decode_index_buffer,
    decode_index_sequence,
    decode_vertex_version,
    decode_index_version,
    decode_filter_oct,
    decode_filter_quat,
    decode_filter_exp,
)

from .optimizer import (
    optimize_vertex_cache,
    optimize_vertex_cache_strip,
    optimize_vertex_cache_fifo,
    optimize_overdraw,
    optimize_vertex_fetch,
    optimize_vertex_fetch_remap,
)

from .simplifier import (
    simplify,
    simplify_with_attributes,
    simplify_sloppy,
    simplify_points,
    simplify_scale,
    SIMPLIFY_LOCK_BORDER,
    SIMPLIFY_SPARSE,
    SIMPLIFY_ERROR_ABSOLUTE,
    SIMPLIFY_PRUNE,
)

from .utils import (
    generate_vertex_remap,
    remap_vertex_buffer,
    remap_index_buffer,
)
