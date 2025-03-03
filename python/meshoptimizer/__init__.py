"""
Python wrapper for the meshoptimizer library.

This package provides Python bindings for the meshoptimizer C++ library,
which offers various algorithms for optimizing 3D meshes for GPU rendering.
"""

from .encoder import (
    encode_vertex_buffer,
    encode_index_buffer,
    encode_vertex_version,
    encode_index_version,
)

from .decoder import (
    decode_vertex_buffer,
    decode_index_buffer,
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

import numpy as np
from typing import Optional, Union, Dict, Any, ClassVar, Type, TypeVar

T = TypeVar('T', bound='Mesh')

class Mesh:
    """
    A class representing a 3D mesh with optimization capabilities.
    """
    
    def __init__(self, vertices: np.ndarray, indices: Optional[np.ndarray] = None) -> None:
        """
        Initialize a mesh with vertices and optional indices.
        
        Args:
            vertices: numpy array of vertex data
            indices: numpy array of indices (optional)
        """
        self.vertices = np.asarray(vertices, dtype=np.float32)
        self.indices = np.asarray(indices, dtype=np.uint32) if indices is not None else None
        self.vertex_count = len(vertices)
        self.index_count = len(indices) if indices is not None else 0
    
    def optimize_vertex_cache(self) -> 'Mesh':
        """
        Optimize the mesh for vertex cache efficiency.
        
        Returns:
            self (for method chaining)
        """
        if self.indices is None:
            raise ValueError("Mesh has no indices to optimize")
        
        # Create output array
        optimized_indices = np.zeros_like(self.indices)
        
        # Call optimization function
        optimize_vertex_cache(
            optimized_indices, 
            self.indices, 
            self.index_count, 
            self.vertex_count
        )
        
        self.indices = optimized_indices
        return self
    
    def optimize_overdraw(self, threshold: float = 1.05) -> 'Mesh':
        """
        Optimize the mesh for overdraw.
        
        Args:
            threshold: threshold for optimization (default: 1.05)
            
        Returns:
            self (for method chaining)
        """
        if self.indices is None:
            raise ValueError("Mesh has no indices to optimize")
        
        # Create output array
        optimized_indices = np.zeros_like(self.indices)
        
        # Call optimization function
        optimize_overdraw(
            optimized_indices, 
            self.indices, 
            self.vertices, 
            self.index_count, 
            self.vertex_count, 
            self.vertices.itemsize * self.vertices.shape[1], 
            threshold
        )
        
        self.indices = optimized_indices
        return self
    
    def optimize_vertex_fetch(self) -> 'Mesh':
        """
        Optimize the mesh for vertex fetch efficiency.
        
        Returns:
            self (for method chaining)
        """
        if self.indices is None:
            raise ValueError("Mesh has no indices to optimize")
        
        # Create output array
        optimized_vertices = np.zeros_like(self.vertices)
        
        # Call optimization function
        unique_vertex_count = optimize_vertex_fetch(
            optimized_vertices, 
            self.indices, 
            self.vertices, 
            self.index_count, 
            self.vertex_count, 
            self.vertices.itemsize * self.vertices.shape[1]
        )
        
        self.vertices = optimized_vertices[:unique_vertex_count]
        self.vertex_count = unique_vertex_count
        return self
    
    def simplify(self, target_ratio: float = 0.25, target_error: float = 0.01, options: int = 0) -> 'Mesh':
        """
        Simplify the mesh.
        
        Args:
            target_ratio: ratio of triangles to keep (default: 0.25)
            target_error: target error (default: 0.01)
            options: simplification options (default: 0)
            
        Returns:
            self (for method chaining)
        """
        if self.indices is None:
            raise ValueError("Mesh has no indices to simplify")
        
        # Calculate target index count
        target_index_count = int(self.index_count * target_ratio)
        
        # Create output array
        simplified_indices = np.zeros(self.index_count, dtype=np.uint32)
        
        # Call simplification function
        result_error = np.array([0.0], dtype=np.float32)
        new_index_count = simplify(
            simplified_indices, 
            self.indices, 
            self.vertices, 
            self.index_count, 
            self.vertex_count, 
            self.vertices.itemsize * self.vertices.shape[1], 
            target_index_count, 
            target_error, 
            options, 
            result_error
        )
        
        self.indices = simplified_indices[:new_index_count]
        self.index_count = new_index_count
        return self
    
    def encode(self) -> Dict[str, bytes]:
        """
        Encode the mesh for efficient transmission.
        
        Returns:
            Dictionary with encoded vertices and indices
        """
        # Encode vertices
        encoded_vertices = encode_vertex_buffer(
            self.vertices, 
            self.vertex_count, 
            self.vertices.itemsize * self.vertices.shape[1]
        )
        
        # Encode indices if present
        encoded_indices = None
        if self.indices is not None:
            encoded_indices = encode_index_buffer(
                self.indices, 
                self.index_count, 
                self.vertex_count
            )
        
        return {
            'vertices': encoded_vertices,
            'indices': encoded_indices
        }
    
    @classmethod
    def decode(cls: Type[T], encoded_data: Dict[str, bytes], 
              vertex_count: int, vertex_size: int, 
              index_count: Optional[int] = None, 
              index_size: int = 4) -> T:
        """
        Decode an encoded mesh.
        
        Args:
            encoded_data: Dictionary with encoded vertices and indices
            vertex_count: Number of vertices
            vertex_size: Size of each vertex in bytes
            index_count: Number of indices (optional)
            index_size: Size of each index in bytes (default: 4)
            
        Returns:
            Decoded Mesh object
        """
        # Decode vertices using the new function that returns a numpy array
        vertices = decode_vertex_buffer(
            vertex_count, 
            vertex_size, 
            encoded_data['vertices']
        )
        
        # Decode indices if present using the new function that returns a numpy array
        indices = None
        if encoded_data['indices'] is not None and index_count is not None:
            indices = decode_index_buffer(
                index_count, 
                index_size, 
                encoded_data['indices']
            )
        
        return cls(vertices, indices)

# Version information
__version__ = '0.1.0'