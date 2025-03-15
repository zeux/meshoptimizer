"""
Library loader for meshoptimizer.
"""
import ctypes
import os
import sys
import platform
import glob
from typing import Optional, List, Any
import numpy as np

def find_library() -> str:
    """Find the meshoptimizer shared library."""
    # Get the directory of this file
    this_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Look for any _meshoptimizer*.so or _meshoptimizer*.pyd file
    if platform.system() == 'Windows':
        pattern = os.path.join(this_dir, '_meshoptimizer*.pyd')
    else:
        pattern = os.path.join(this_dir, '_meshoptimizer*.so')
    
    lib_files = glob.glob(pattern)
    
    if lib_files:
        return lib_files[0]
    
    raise ImportError(f"Could not find meshoptimizer library in {this_dir}")

# Load the library
try:
    lib_path = find_library()
    lib = ctypes.CDLL(lib_path)
except ImportError as e:
    # If the library is not found, provide a helpful error message
    print(f"Error loading meshoptimizer library: {e}")
    print("Make sure the library is properly installed.")
    raise

# Define function signatures
def setup_function_signatures() -> None:
    """Set up the function signatures for the library."""
    # Vertex remap functions
    lib.meshopt_generateVertexRemap.argtypes = [
        ctypes.POINTER(ctypes.c_uint),  # destination
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t,                # index_count
        ctypes.c_void_p,                # vertices
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t                 # vertex_size
    ]
    lib.meshopt_generateVertexRemap.restype = ctypes.c_size_t
    
    lib.meshopt_remapVertexBuffer.argtypes = [
        ctypes.c_void_p,                # destination
        ctypes.c_void_p,                # vertices
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t,                # vertex_size
        ctypes.POINTER(ctypes.c_uint)   # remap
    ]
    lib.meshopt_remapVertexBuffer.restype = None
    
    lib.meshopt_remapIndexBuffer.argtypes = [
        ctypes.POINTER(ctypes.c_uint),  # destination
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t,                # index_count
        ctypes.POINTER(ctypes.c_uint)   # remap
    ]
    lib.meshopt_remapIndexBuffer.restype = None
    
    # Vertex cache optimization
    lib.meshopt_optimizeVertexCache.argtypes = [
        ctypes.POINTER(ctypes.c_uint),  # destination
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t,                # index_count
        ctypes.c_size_t                 # vertex_count
    ]
    lib.meshopt_optimizeVertexCache.restype = None
    
    # Overdraw optimization
    lib.meshopt_optimizeOverdraw.argtypes = [
        ctypes.POINTER(ctypes.c_uint),  # destination
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t,                # index_count
        ctypes.POINTER(ctypes.c_float), # vertex_positions
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t,                # vertex_positions_stride
        ctypes.c_float                  # threshold
    ]
    lib.meshopt_optimizeOverdraw.restype = None
    
    # Vertex fetch optimization
    lib.meshopt_optimizeVertexFetch.argtypes = [
        ctypes.c_void_p,                # destination
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t,                # index_count
        ctypes.c_void_p,                # vertices
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t                 # vertex_size
    ]
    lib.meshopt_optimizeVertexFetch.restype = ctypes.c_size_t
    
    # Simplification
    lib.meshopt_simplify.argtypes = [
        ctypes.POINTER(ctypes.c_uint),  # destination
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t,                # index_count
        ctypes.POINTER(ctypes.c_float), # vertex_positions
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t,                # vertex_positions_stride
        ctypes.c_size_t,                # target_index_count
        ctypes.c_float,                 # target_error
        ctypes.c_uint,                  # options
        ctypes.POINTER(ctypes.c_float)  # result_error
    ]
    lib.meshopt_simplify.restype = ctypes.c_size_t
    
    # Simplification scale
    lib.meshopt_simplifyScale.argtypes = [
        ctypes.POINTER(ctypes.c_float), # vertex_positions
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t                 # vertex_positions_stride
    ]
    lib.meshopt_simplifyScale.restype = ctypes.c_float  # Return type is float
    
    # Encoding
    lib.meshopt_encodeVertexBufferBound.argtypes = [
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t                 # vertex_size
    ]
    lib.meshopt_encodeVertexBufferBound.restype = ctypes.c_size_t
    
    lib.meshopt_encodeVertexBuffer.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t,                # buffer_size
        ctypes.c_void_p,                # vertices
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t                 # vertex_size
    ]
    lib.meshopt_encodeVertexBuffer.restype = ctypes.c_size_t
    
    lib.meshopt_encodeIndexBufferBound.argtypes = [
        ctypes.c_size_t,                # index_count
        ctypes.c_size_t                 # vertex_count
    ]
    lib.meshopt_encodeIndexBufferBound.restype = ctypes.c_size_t
    
    lib.meshopt_encodeIndexBuffer.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t,                # buffer_size
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t                 # index_count
    ]
    lib.meshopt_encodeIndexBuffer.restype = ctypes.c_size_t
    
    lib.meshopt_encodeIndexSequenceBound.argtypes = [
        ctypes.c_size_t,                # index_count
        ctypes.c_size_t                 # vertex_count
    ]
    lib.meshopt_encodeIndexSequenceBound.restype = ctypes.c_size_t
    
    lib.meshopt_encodeIndexSequence.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t,                # buffer_size
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t                 # index_count
    ]
    lib.meshopt_encodeIndexSequence.restype = ctypes.c_size_t
    
    # Decoding
    lib.meshopt_decodeVertexBuffer.argtypes = [
        ctypes.c_void_p,                # destination
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t,                # vertex_size
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t                 # buffer_size
    ]
    lib.meshopt_decodeVertexBuffer.restype = ctypes.c_int
    
    lib.meshopt_decodeIndexBuffer.argtypes = [
        ctypes.c_void_p,                # destination
        ctypes.c_size_t,                # index_count
        ctypes.c_size_t,                # index_size
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t                 # buffer_size
    ]
    lib.meshopt_decodeIndexBuffer.restype = ctypes.c_int
    
    lib.meshopt_decodeIndexSequence.argtypes = [
        ctypes.c_void_p,                # destination
        ctypes.c_size_t,                # index_count
        ctypes.c_size_t,                # index_size
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t                 # buffer_size
    ]
    lib.meshopt_decodeIndexSequence.restype = ctypes.c_int
    
    # Encoding/Decoding versions
    lib.meshopt_encodeVertexVersion.argtypes = [ctypes.c_int]
    lib.meshopt_encodeVertexVersion.restype = None
    
    lib.meshopt_encodeIndexVersion.argtypes = [ctypes.c_int]
    lib.meshopt_encodeIndexVersion.restype = None
    
    lib.meshopt_decodeVertexVersion.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t                 # buffer_size
    ]
    lib.meshopt_decodeVertexVersion.restype = ctypes.c_int
    
    lib.meshopt_decodeIndexVersion.argtypes = [
        ctypes.POINTER(ctypes.c_ubyte), # buffer
        ctypes.c_size_t                 # buffer_size
    ]
    lib.meshopt_decodeIndexVersion.restype = ctypes.c_int
    
    # Simplify sloppy
    lib.meshopt_simplifySloppy.argtypes = [
        ctypes.POINTER(ctypes.c_uint),  # destination
        ctypes.POINTER(ctypes.c_uint),  # indices
        ctypes.c_size_t,                # index_count
        ctypes.POINTER(ctypes.c_float), # vertex_positions
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t,                # vertex_positions_stride
        ctypes.c_size_t,                # target_index_count
        ctypes.c_float,                 # target_error
        ctypes.POINTER(ctypes.c_float)  # result_error
    ]
    lib.meshopt_simplifySloppy.restype = ctypes.c_size_t
    
    # Simplify points
    lib.meshopt_simplifyPoints.argtypes = [
        ctypes.POINTER(ctypes.c_uint),  # destination
        ctypes.POINTER(ctypes.c_float), # vertex_positions
        ctypes.c_size_t,                # vertex_count
        ctypes.c_size_t,                # vertex_positions_stride
        ctypes.POINTER(ctypes.c_float), # vertex_colors
        ctypes.c_size_t,                # vertex_colors_stride
        ctypes.c_float,                 # color_weight
        ctypes.c_size_t                 # target_vertex_count
    ]
    lib.meshopt_simplifyPoints.restype = ctypes.c_size_t

# Set up function signatures
try:
    setup_function_signatures()
except AttributeError as e:
    print(f"Error setting up function signatures: {e}")
    print("The library might be missing some expected functions.")
    raise