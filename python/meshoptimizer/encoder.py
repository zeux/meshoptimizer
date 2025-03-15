"""
Encoder functions for meshoptimizer.
"""
import ctypes
from typing import Optional
import numpy as np
from ._loader import lib

def encode_vertex_buffer(vertices: np.ndarray, 
                        vertex_count: Optional[int] = None, 
                        vertex_size: Optional[int] = None) -> bytes:
    """
    Encode vertex buffer data.
    
    Args:
        vertices: numpy array of vertex data
        vertex_count: number of vertices (optional, derived from vertices if not provided)
        vertex_size: size of each vertex in bytes (optional, derived from vertices if not provided)
        
    Returns:
        Encoded buffer as bytes
    """
    # Convert vertices to numpy array if it's not already
    vertices = np.asarray(vertices, dtype=np.float32)
    
    # Derive vertex_count and vertex_size if not provided
    if vertex_count is None:
        vertex_count = len(vertices)
    
    if vertex_size is None:
        vertex_size = vertices.itemsize * vertices.shape[1] if len(vertices.shape) > 1 else vertices.itemsize
    
    # Calculate buffer size
    bound = lib.meshopt_encodeVertexBufferBound(vertex_count, vertex_size)
    
    # Allocate buffer
    buffer = np.zeros(bound, dtype=np.uint8)
    
    # Call C function
    result_size = lib.meshopt_encodeVertexBuffer(
        buffer.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        bound,
        vertices.ctypes.data_as(ctypes.c_void_p),
        vertex_count,
        vertex_size
    )
    
    if result_size == 0:
        raise RuntimeError("Failed to encode vertex buffer")
    
    # Return only the used portion of the buffer
    return bytes(buffer[:result_size])

def encode_index_buffer(indices: np.ndarray, 
                       index_count: Optional[int] = None, 
                       vertex_count: Optional[int] = None) -> bytes:
    """
    Encode index buffer data.
    
    Args:
        indices: numpy array of index data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from indices if not provided)
        
    Returns:
        Encoded buffer as bytes
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = np.max(indices) + 1
    
    # Calculate buffer size
    bound = lib.meshopt_encodeIndexBufferBound(index_count, vertex_count)
    
    # Allocate buffer
    buffer = np.zeros(bound, dtype=np.uint8)
    
    # Call C function
    result_size = lib.meshopt_encodeIndexBuffer(
        buffer.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        bound,
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count
    )
    
    if result_size == 0:
        raise RuntimeError("Failed to encode index buffer")
    
    # Return only the used portion of the buffer
    return bytes(buffer[:result_size])

def encode_vertex_version(version: int) -> None:
    """
    Set vertex encoder format version.
    
    Args:
        version: version number (0 or 1)
    """
    if version not in (0, 1):
        raise ValueError("Version must be 0 or 1")
    
    lib.meshopt_encodeVertexVersion(version)

def encode_index_version(version: int) -> None:
    """
    Set index encoder format version.
    
    Args:
        version: version number (0 or 1)
    """
    if version not in (0, 1):
        raise ValueError("Version must be 0 or 1")
    
    lib.meshopt_encodeIndexVersion(version)

def encode_index_sequence(indices: np.ndarray,
                         index_count: Optional[int] = None,
                         vertex_count: Optional[int] = None) -> bytes:
    """
    Encode index sequence data.
    
    Args:
        indices: numpy array of index data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from indices if not provided)
        
    Returns:
        Encoded buffer as bytes
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = np.max(indices) + 1
    
    # Calculate buffer size
    bound = lib.meshopt_encodeIndexSequenceBound(index_count, vertex_count)
    
    # Allocate buffer
    buffer = np.zeros(bound, dtype=np.uint8)
    
    # Call C function
    result_size = lib.meshopt_encodeIndexSequence(
        buffer.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        bound,
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count
    )
    
    if result_size == 0:
        raise RuntimeError("Failed to encode index sequence")
    
    # Return only the used portion of the buffer
    return bytes(buffer[:result_size])