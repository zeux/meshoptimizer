"""
Optimization functions for meshoptimizer.
"""
import ctypes
from typing import Optional, Union, Tuple
import numpy as np
from ._loader import lib

def optimize_vertex_cache(destination: np.ndarray, indices: np.ndarray, 
                         index_count: Optional[int] = None, 
                         vertex_count: Optional[int] = None) -> None:
    """
    Optimize vertex cache.
    
    Args:
        destination: numpy array to store the optimized indices
        indices: numpy array of index data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from indices if not provided)
        
    Returns:
        None (destination is modified in-place)
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = np.max(indices) + 1
    
    # Call C function
    lib.meshopt_optimizeVertexCache(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_count
    )

def optimize_vertex_cache_strip(destination: np.ndarray, indices: np.ndarray, 
                               index_count: Optional[int] = None, 
                               vertex_count: Optional[int] = None) -> None:
    """
    Optimize vertex cache for strip-like caches.
    
    Args:
        destination: numpy array to store the optimized indices
        indices: numpy array of index data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from indices if not provided)
        
    Returns:
        None (destination is modified in-place)
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = np.max(indices) + 1
    
    # Call C function
    lib.meshopt_optimizeVertexCacheStrip(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_count
    )

def optimize_vertex_cache_fifo(destination: np.ndarray, indices: np.ndarray, 
                              index_count: Optional[int] = None, 
                              vertex_count: Optional[int] = None, 
                              cache_size: int = 16) -> None:
    """
    Optimize vertex cache for FIFO caches.
    
    Args:
        destination: numpy array to store the optimized indices
        indices: numpy array of index data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from indices if not provided)
        cache_size: size of the cache (default: 16)
        
    Returns:
        None (destination is modified in-place)
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = np.max(indices) + 1
    
    # Call C function
    lib.meshopt_optimizeVertexCacheFifo(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_count,
        cache_size
    )

def optimize_overdraw(destination: np.ndarray, indices: np.ndarray, 
                     vertex_positions: np.ndarray, 
                     index_count: Optional[int] = None, 
                     vertex_count: Optional[int] = None, 
                     vertex_positions_stride: Optional[int] = None, 
                     threshold: float = 1.05) -> None:
    """
    Optimize overdraw.
    
    Args:
        destination: numpy array to store the optimized indices
        indices: numpy array of index data
        vertex_positions: numpy array of vertex position data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from vertex_positions if not provided)
        vertex_positions_stride: stride of vertex positions in bytes (optional, derived from vertex_positions if not provided)
        threshold: threshold for optimization (default: 1.05)
        
    Returns:
        None (destination is modified in-place)
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Convert vertex_positions to numpy array if it's not already
    vertex_positions = np.asarray(vertex_positions, dtype=np.float32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = len(vertex_positions)
    
    # Derive vertex_positions_stride if not provided
    if vertex_positions_stride is None:
        vertex_positions_stride = vertex_positions.itemsize * vertex_positions.shape[1] if len(vertex_positions.shape) > 1 else vertex_positions.itemsize
    
    # Call C function
    lib.meshopt_optimizeOverdraw(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_positions.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        vertex_count,
        vertex_positions_stride,
        threshold
    )

def optimize_vertex_fetch(destination_vertices: np.ndarray, indices: np.ndarray, 
                         source_vertices: np.ndarray, 
                         index_count: Optional[int] = None, 
                         vertex_count: Optional[int] = None, 
                         vertex_size: Optional[int] = None) -> int:
    """
    Optimize vertex fetch.
    
    Args:
        destination_vertices: numpy array to store the optimized vertices
        indices: numpy array of index data
        source_vertices: numpy array of vertex data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from source_vertices if not provided)
        vertex_size: size of each vertex in bytes (optional, derived from source_vertices if not provided)
        
    Returns:
        Number of unique vertices
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Convert source_vertices to numpy array if it's not already
    source_vertices = np.asarray(source_vertices)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = len(source_vertices)
    
    # Derive vertex_size if not provided
    if vertex_size is None:
        vertex_size = source_vertices.itemsize * source_vertices.shape[1] if len(source_vertices.shape) > 1 else source_vertices.itemsize
    
    # Call C function
    result = lib.meshopt_optimizeVertexFetch(
        destination_vertices.ctypes.data_as(ctypes.c_void_p),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        source_vertices.ctypes.data_as(ctypes.c_void_p),
        vertex_count,
        vertex_size
    )
    
    return result

def optimize_vertex_fetch_remap(destination: np.ndarray, indices: np.ndarray, 
                               index_count: Optional[int] = None, 
                               vertex_count: Optional[int] = None) -> int:
    """
    Generate vertex remap to optimize vertex fetch.
    
    Args:
        destination: numpy array to store the remap table
        indices: numpy array of index data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from indices if not provided)
        
    Returns:
        Number of unique vertices
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = np.max(indices) + 1
    
    # Call C function
    result = lib.meshopt_optimizeVertexFetchRemap(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_count
    )
    
    return result