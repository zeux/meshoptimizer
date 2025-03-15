"""
Decoder functions for meshoptimizer.
"""
import ctypes
from typing import Union
import numpy as np
from ._loader import lib

def decode_vertex_buffer(vertex_count: int, 
                        vertex_size: int, 
                        buffer: Union[bytes, np.ndarray]) -> np.ndarray:
    """
    Decode vertex buffer data.
    
    Args:
        vertex_count: number of vertices
        vertex_size: size of each vertex in bytes
        buffer: encoded buffer as bytes
        
    Returns:
        Numpy array containing the decoded vertex data
    """
    # Convert buffer to numpy array if it's not already
    buffer_array = np.frombuffer(buffer, dtype=np.uint8)
    
    # Create destination array
    # Calculate the number of float32 elements needed
    float_count = vertex_count * vertex_size // 4
    destination = np.zeros(float_count, dtype=np.float32)
    
    # Call C function
    result = lib.meshopt_decodeVertexBuffer(
        destination.ctypes.data_as(ctypes.c_void_p),
        vertex_count,
        vertex_size,
        buffer_array.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        len(buffer_array)
    )
    
    if result != 0:
        raise RuntimeError(f"Failed to decode vertex buffer: error code {result}")
    
    # Reshape the array if vertex_size indicates multiple components per vertex
    components_per_vertex = vertex_size // 4  # Assuming float32 (4 bytes) components
    if components_per_vertex > 1:
        destination = destination.reshape(vertex_count, components_per_vertex)
    
    return destination

def decode_index_buffer(index_count: int, 
                       index_size: int, 
                       buffer: Union[bytes, np.ndarray]) -> np.ndarray:
    """
    Decode index buffer data.
    
    Args:
        index_count: number of indices
        index_size: size of each index in bytes (2 or 4)
        buffer: encoded buffer as bytes
        
    Returns:
        Numpy array containing the decoded index data
    """
    # Convert buffer to numpy array if it's not already
    buffer_array = np.frombuffer(buffer, dtype=np.uint8)
    
    # Create destination array
    destination = np.zeros(index_count, dtype=np.uint32)
    
    # Call C function
    result = lib.meshopt_decodeIndexBuffer(
        destination.ctypes.data_as(ctypes.c_void_p),
        index_count,
        index_size,
        buffer_array.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        len(buffer_array)
    )
    
    if result != 0:
        raise RuntimeError(f"Failed to decode index buffer: error code {result}")
    
    return destination

def decode_vertex_version(buffer: Union[bytes, np.ndarray]) -> int:
    """
    Get encoded vertex format version.
    
    Args:
        buffer: encoded buffer as bytes
        
    Returns:
        Format version of the encoded vertex buffer, or -1 if the buffer header is invalid
    """
    # Convert buffer to numpy array if it's not already
    buffer_array = np.frombuffer(buffer, dtype=np.uint8)
    
    return lib.meshopt_decodeVertexVersion(
        buffer_array.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        len(buffer_array)
    )

def decode_index_version(buffer: Union[bytes, np.ndarray]) -> int:
    """
    Get encoded index format version.
    
    Args:
        buffer: encoded buffer as bytes
        
    Returns:
        Format version of the encoded index buffer, or -1 if the buffer header is invalid
    """
    # Convert buffer to numpy array if it's not already
    buffer_array = np.frombuffer(buffer, dtype=np.uint8)
    
    return lib.meshopt_decodeIndexVersion(
        buffer_array.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        len(buffer_array)
    )

def decode_index_sequence(index_count: int,
                         index_size: int,
                         buffer: Union[bytes, np.ndarray]) -> np.ndarray:
    """
    Decode index sequence data.
    
    Args:
        index_count: number of indices
        index_size: size of each index in bytes (2 or 4)
        buffer: encoded buffer as bytes
        
    Returns:
        Numpy array containing the decoded index data
    """
    # Convert buffer to numpy array if it's not already
    buffer_array = np.frombuffer(buffer, dtype=np.uint8)
    
    # Create destination array
    destination = np.zeros(index_count, dtype=np.uint32)
    
    # Call C function
    result = lib.meshopt_decodeIndexSequence(
        destination.ctypes.data_as(ctypes.c_void_p),
        index_count,
        index_size,
        buffer_array.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte)),
        len(buffer_array)
    )
    
    if result != 0:
        raise RuntimeError(f"Failed to decode index sequence: error code {result}")
    
    return destination

def decode_filter_oct(buffer: np.ndarray, count: int, stride: int) -> np.ndarray:
    """
    Apply octahedral filter to decoded data.
    
    Args:
        buffer: numpy array of decoded data
        count: number of elements
        stride: stride between elements in bytes
        
    Returns:
        Numpy array with the filter applied (a copy of the input buffer)
    """
    # Create a copy of the buffer to avoid modifying the original
    result_buffer = buffer.copy()
    
    lib.meshopt_decodeFilterOct(
        result_buffer.ctypes.data_as(ctypes.c_void_p),
        count,
        stride
    )
    
    return result_buffer

def decode_filter_quat(buffer: np.ndarray, count: int, stride: int) -> np.ndarray:
    """
    Apply quaternion filter to decoded data.
    
    Args:
        buffer: numpy array of decoded data
        count: number of elements
        stride: stride between elements in bytes
        
    Returns:
        Numpy array with the filter applied (a copy of the input buffer)
    """
    # Create a copy of the buffer to avoid modifying the original
    result_buffer = buffer.copy()
    
    lib.meshopt_decodeFilterQuat(
        result_buffer.ctypes.data_as(ctypes.c_void_p),
        count,
        stride
    )
    
    return result_buffer

def decode_filter_exp(buffer: np.ndarray, count: int, stride: int) -> np.ndarray:
    """
    Apply exponential filter to decoded data.
    
    Args:
        buffer: numpy array of decoded data
        count: number of elements
        stride: stride between elements in bytes
        
    Returns:
        Numpy array with the filter applied (a copy of the input buffer)
    """
    # Create a copy of the buffer to avoid modifying the original
    result_buffer = buffer.copy()
    
    lib.meshopt_decodeFilterExp(
        result_buffer.ctypes.data_as(ctypes.c_void_p),
        count,
        stride
    )
    
    return result_buffer