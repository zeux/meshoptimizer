"""
Simplification functions for meshoptimizer.
"""
import ctypes
from typing import Optional
import numpy as np
from ._loader import lib

# Simplification options
SIMPLIFY_LOCK_BORDER = 1 << 0
SIMPLIFY_SPARSE = 1 << 1
SIMPLIFY_ERROR_ABSOLUTE = 1 << 2
SIMPLIFY_PRUNE = 1 << 3

def simplify(destination: np.ndarray, indices: np.ndarray, vertex_positions: np.ndarray, 
             index_count: Optional[int] = None, vertex_count: Optional[int] = None, 
             vertex_positions_stride: Optional[int] = None, target_index_count: Optional[int] = None, 
             target_error: float = 0.01, options: int = 0, 
             result_error: Optional[np.ndarray] = None) -> int:
    """
    Simplify mesh.
    
    Args:
        destination: numpy array to store the simplified indices
        indices: numpy array of index data
        vertex_positions: numpy array of vertex position data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from vertex_positions if not provided)
        vertex_positions_stride: stride of vertex positions in bytes (optional, derived from vertex_positions if not provided)
        target_index_count: target number of indices (optional, defaults to 25% of original)
        target_error: target error (default: 0.01)
        options: simplification options (default: 0)
        result_error: optional float to store the resulting error
        
    Returns:
        Number of indices in the simplified mesh
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
    
    # Derive target_index_count if not provided
    if target_index_count is None:
        target_index_count = index_count // 4  # 25% of original
    
    # Create result_error_ptr if result_error is provided
    if result_error is not None:
        result_error_ptr = ctypes.pointer(ctypes.c_float(0.0))
    else:
        result_error_ptr = ctypes.POINTER(ctypes.c_float)()
    
    # Call C function
    result = lib.meshopt_simplify(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_positions.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        vertex_count,
        vertex_positions_stride,
        target_index_count,
        target_error,
        options,
        result_error_ptr
    )
    
    # Update result_error if provided
    if result_error is not None:
        result_error[0] = result_error_ptr.contents.value
    
    return result

def simplify_with_attributes(destination: np.ndarray, indices: np.ndarray, vertex_positions: np.ndarray, 
                             vertex_attributes: np.ndarray, attribute_weights: np.ndarray, 
                             index_count: Optional[int] = None, vertex_count: Optional[int] = None, 
                             vertex_positions_stride: Optional[int] = None, 
                             vertex_attributes_stride: Optional[int] = None, 
                             attribute_count: Optional[int] = None, 
                             vertex_lock: Optional[np.ndarray] = None, 
                             target_index_count: Optional[int] = None, 
                             target_error: float = 0.01, options: int = 0, 
                             result_error: Optional[np.ndarray] = None) -> int:
    """
    Simplify mesh with attribute metric.
    
    Args:
        destination: numpy array to store the simplified indices
        indices: numpy array of index data
        vertex_positions: numpy array of vertex position data
        vertex_attributes: numpy array of vertex attribute data
        attribute_weights: numpy array of attribute weights
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from vertex_positions if not provided)
        vertex_positions_stride: stride of vertex positions in bytes (optional, derived from vertex_positions if not provided)
        vertex_attributes_stride: stride of vertex attributes in bytes (optional, derived from vertex_attributes if not provided)
        attribute_count: number of attributes (optional, derived from attribute_weights if not provided)
        vertex_lock: optional numpy array of vertex lock flags
        target_index_count: target number of indices (optional, defaults to 25% of original)
        target_error: target error (default: 0.01)
        options: simplification options (default: 0)
        result_error: optional float to store the resulting error
        
    Returns:
        Number of indices in the simplified mesh
    """
    # Convert indices to numpy array if it's not already
    indices = np.asarray(indices, dtype=np.uint32)
    
    # Convert vertex_positions to numpy array if it's not already
    vertex_positions = np.asarray(vertex_positions, dtype=np.float32)
    
    # Convert vertex_attributes to numpy array if it's not already
    vertex_attributes = np.asarray(vertex_attributes, dtype=np.float32)
    
    # Convert attribute_weights to numpy array if it's not already
    attribute_weights = np.asarray(attribute_weights, dtype=np.float32)
    
    # Derive index_count if not provided
    if index_count is None:
        index_count = len(indices)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = len(vertex_positions)
    
    # Derive vertex_positions_stride if not provided
    if vertex_positions_stride is None:
        vertex_positions_stride = vertex_positions.itemsize * vertex_positions.shape[1] if len(vertex_positions.shape) > 1 else vertex_positions.itemsize
    
    # Derive vertex_attributes_stride if not provided
    if vertex_attributes_stride is None:
        vertex_attributes_stride = vertex_attributes.itemsize * vertex_attributes.shape[1] if len(vertex_attributes.shape) > 1 else vertex_attributes.itemsize
    
    # Derive attribute_count if not provided
    if attribute_count is None:
        attribute_count = len(attribute_weights)
    
    # Derive target_index_count if not provided
    if target_index_count is None:
        target_index_count = index_count // 4  # 25% of original
    
    # Create result_error_ptr if result_error is provided
    if result_error is not None:
        result_error_ptr = ctypes.pointer(ctypes.c_float(0.0))
    else:
        result_error_ptr = ctypes.POINTER(ctypes.c_float)()
    
    # Create vertex_lock_ptr if vertex_lock is provided
    if vertex_lock is not None:
        vertex_lock = np.asarray(vertex_lock, dtype=np.uint8)
        vertex_lock_ptr = vertex_lock.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    else:
        vertex_lock_ptr = ctypes.POINTER(ctypes.c_ubyte)()
    
    # Call C function
    result = lib.meshopt_simplifyWithAttributes(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_positions.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        vertex_count,
        vertex_positions_stride,
        vertex_attributes.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        vertex_attributes_stride,
        attribute_weights.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        attribute_count,
        vertex_lock_ptr,
        target_index_count,
        target_error,
        options,
        result_error_ptr
    )
    
    # Update result_error if provided
    if result_error is not None:
        result_error[0] = result_error_ptr.contents.value
    
    return result

def simplify_sloppy(destination: np.ndarray, indices: np.ndarray, vertex_positions: np.ndarray, 
                   index_count: Optional[int] = None, vertex_count: Optional[int] = None, 
                   vertex_positions_stride: Optional[int] = None, 
                   target_index_count: Optional[int] = None, target_error: float = 0.01, 
                   result_error: Optional[np.ndarray] = None) -> int:
    """
    Simplify mesh (sloppy).
    
    Args:
        destination: numpy array to store the simplified indices
        indices: numpy array of index data
        vertex_positions: numpy array of vertex position data
        index_count: number of indices (optional, derived from indices if not provided)
        vertex_count: number of vertices (optional, derived from vertex_positions if not provided)
        vertex_positions_stride: stride of vertex positions in bytes (optional, derived from vertex_positions if not provided)
        target_index_count: target number of indices (optional, defaults to 25% of original)
        target_error: target error (default: 0.01)
        result_error: optional float to store the resulting error
        
    Returns:
        Number of indices in the simplified mesh
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
    
    # Derive target_index_count if not provided
    if target_index_count is None:
        target_index_count = index_count // 4  # 25% of original
    
    # Create result_error_ptr if result_error is provided
    if result_error is not None:
        result_error_ptr = ctypes.pointer(ctypes.c_float(0.0))
    else:
        result_error_ptr = ctypes.POINTER(ctypes.c_float)()
    
    # Call C function
    result = lib.meshopt_simplifySloppy(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        index_count,
        vertex_positions.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        vertex_count,
        vertex_positions_stride,
        target_index_count,
        ctypes.c_float(target_error),  # Explicitly convert to c_float
        result_error_ptr
    )
    
    # Update result_error if provided
    if result_error is not None:
        result_error[0] = result_error_ptr.contents.value
    
    return result

def simplify_points(destination: np.ndarray, vertex_positions: np.ndarray, 
                   vertex_colors: Optional[np.ndarray] = None, 
                   vertex_count: Optional[int] = None, 
                   vertex_positions_stride: Optional[int] = None, 
                   vertex_colors_stride: Optional[int] = None, 
                   color_weight: float = 1.0, 
                   target_vertex_count: Optional[int] = None) -> int:
    """
    Simplify point cloud.
    
    Args:
        destination: numpy array to store the simplified point indices
        vertex_positions: numpy array of vertex position data
        vertex_colors: numpy array of vertex color data (optional)
        vertex_count: number of vertices (optional, derived from vertex_positions if not provided)
        vertex_positions_stride: stride of vertex positions in bytes (optional, derived from vertex_positions if not provided)
        vertex_colors_stride: stride of vertex colors in bytes (optional, derived from vertex_colors if not provided)
        color_weight: weight of color in simplification (default: 1.0)
        target_vertex_count: target number of vertices (optional, defaults to 25% of original)
        
    Returns:
        Number of vertices in the simplified point cloud
    """
    # Convert vertex_positions to numpy array if it's not already
    vertex_positions = np.asarray(vertex_positions, dtype=np.float32)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = len(vertex_positions)
    
    # Derive vertex_positions_stride if not provided
    if vertex_positions_stride is None:
        vertex_positions_stride = vertex_positions.itemsize * vertex_positions.shape[1] if len(vertex_positions.shape) > 1 else vertex_positions.itemsize
    
    # Derive target_vertex_count if not provided
    if target_vertex_count is None:
        target_vertex_count = vertex_count // 4  # 25% of original
    
    # Handle vertex_colors
    if vertex_colors is not None:
        vertex_colors = np.asarray(vertex_colors, dtype=np.float32)
        
        # Derive vertex_colors_stride if not provided
        if vertex_colors_stride is None:
            vertex_colors_stride = vertex_colors.itemsize * vertex_colors.shape[1] if len(vertex_colors.shape) > 1 else vertex_colors.itemsize
        
        vertex_colors_ptr = vertex_colors.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    else:
        vertex_colors_ptr = ctypes.POINTER(ctypes.c_float)()
        vertex_colors_stride = 0
    
    # Call C function
    result = lib.meshopt_simplifyPoints(
        destination.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
        vertex_positions.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        vertex_count,
        vertex_positions_stride,
        vertex_colors_ptr,
        vertex_colors_stride,
        ctypes.c_float(color_weight),  # Explicitly convert to c_float
        target_vertex_count
    )
    
    return result

def simplify_scale(vertex_positions: np.ndarray, 
                  vertex_count: Optional[int] = None, 
                  vertex_positions_stride: Optional[int] = None) -> float:
    """
    Get the scale factor for simplification error.
    
    Args:
        vertex_positions: numpy array of vertex position data
        vertex_count: number of vertices (optional, derived from vertex_positions if not provided)
        vertex_positions_stride: stride of vertex positions in bytes (optional, derived from vertex_positions if not provided)
        
    Returns:
        Scale factor for simplification error
    """
    # Convert vertex_positions to numpy array if it's not already
    vertex_positions = np.asarray(vertex_positions, dtype=np.float32)
    
    # Derive vertex_count if not provided
    if vertex_count is None:
        vertex_count = len(vertex_positions)
    
    # Derive vertex_positions_stride if not provided
    if vertex_positions_stride is None:
        vertex_positions_stride = vertex_positions.itemsize * vertex_positions.shape[1] if len(vertex_positions.shape) > 1 else vertex_positions.itemsize
    
    # Call C function
    result = lib.meshopt_simplifyScale(
        vertex_positions.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
        vertex_count,
        vertex_positions_stride
    )
    
    return result