"""
Utility functions for meshoptimizer.
"""
import ctypes
from typing import Optional
import numpy as np
from ._loader import lib

def generate_vertex_remap(destination: np.ndarray, 
                         indices: Optional[np.ndarray] = None, 
                         index_count: Optional[int] = None, 
                         vertices: Optional[np.ndarray] = None, 
                         vertex_count: Optional[int] = None, 
                         vertex_size: Optional[int] = None) -> int:
    """
    Generate vertex remap table.
    
    Args:
        destination: numpy array to store the remap table
        indices: numpy array of index data (can be None for unindexed geometry)
        index_count: number of indices (optional, derived from indices if not provided)
        vertices: numpy array of vertex data
        vertex_count: number of vertices (optional, derived from vertices if not provided)
        vertex_size: size of each vertex in bytes (optional, derived from vertices if not provided)
        
    Returns:
        Number of unique vertices
    """
    # Convert indices to numpy array if it's not already and not None
    if indices is not None:
        indices = np.asarray(indices, dtype=np.uint32)
        
        # Derive index_count if not provided
        if index_count is None:
            index_count = len(indices)
    else:
        # If indices is None, index_count must be 0
        index_count = 0
    
    # Convert vertices to numpy array if it's not already
    if vertices is not None:
        vertices = np.asarray(vertices)
        
        # Derive vertex_count if not provided
        if vertex_count is None:
            vertex_count = len(vertices)
        
        # Derive vertex_size if not provided
        if vertex_size is None:
            vertex_size = vertices.itemsize * vertices.shape[1] if len(vertices.shape) > 1 else vertices.itemsize
    
    # Call C function
    result = lib.meshopt_generateVertexRemap(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)) if indices is not None else None,
        index_count,
        vertices.ctypes.data_as(ctypes.c_void_p) if vertices is not None else None,
        vertex_count,
        vertex_size
    )
    
    return result

def remap_vertex_buffer(destination: np.ndarray, 
                       vertices: np.ndarray, 
                       vertex_count: Optional[int] = None, 
                       vertex_size: Optional[int] = None, 
                       remap: Optional[np.ndarray] = None) -> None:
    """
    Remap vertex buffer.
    
    Args:
        destination: numpy array to store the remapped vertices
        vertices: numpy array of vertex data
        vertex_count: number of vertices (optional, derived from vertices if not provided)
        vertex_size: size of each vertex in bytes (optional, derived from vertices if not provided)
        remap: numpy array of remap data
        
    Returns:
        None (destination is modified in-place)
    """
    # Convert vertices to numpy array if it's not already
    vertices = np.asarray(vertices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = len(vertices)
    
    # Derive vertex_size if not provided
    if vertex_size is None:
        vertex_size = vertices.itemsize * vertices.shape[1] if len(vertices.shape) > 1 else vertices.itemsize
    
    # Convert remap to numpy array if it's not already and not None
    if remap is not None:
        remap = np.asarray(remap, dtype=np.uint32)
    
    # Call C function
    lib.meshopt_remapVertexBuffer(
        destination.ctypes.data_as(ctypes.c_void_p),
        vertices.ctypes.data_as(ctypes.c_void_p),
        vertex_count,
        vertex_size,
        remap.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)) if remap is not None else None
    )

def remap_index_buffer(destination: np.ndarray, 
                      indices: np.ndarray, 
                      index_count: Optional[int] = None, 
                      remap: Optional[np.ndarray] = None) -> None:
    """
    Remap index buffer.
    
    Args:
        destination: numpy array to store the remapped indices
        indices: numpy array of index data
        index_count: number of indices (optional, derived from indices if not provided)
        remap: numpy array of remap data
        
    Returns:
        None (destination is modified in-place)
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Convert remap to numpy array if it's not already and not None
    if remap is not None:
        remap = np.asarray(remap, dtype=np.uint32)
    
    # Call C function
    lib.meshopt_remapIndexBuffer(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        remap.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)) if remap is not None else None
    )