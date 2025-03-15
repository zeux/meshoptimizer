"""
Tests for the meshoptimizer Python wrapper.

This file contains tests to verify that the encoding/decoding process
preserves the mesh geometry correctly.
"""
import numpy as np
import unittest
from meshoptimizer import (
    encode_vertex_buffer, decode_vertex_buffer,
    encode_index_buffer, decode_index_buffer,
    encode_index_sequence, decode_index_sequence
)

class TestEncoding(unittest.TestCase):
    """Test encoding and decoding functionality."""
    
    def setUp(self):
        """Set up test data."""
        # Create a simple mesh (a cube)
        self.vertices = np.array([
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

        self.indices = np.array([
            0, 1, 2, 2, 3, 0,  # front
            1, 5, 6, 6, 2, 1,  # right
            5, 4, 7, 7, 6, 5,  # back
            4, 0, 3, 3, 7, 4,  # left
            3, 2, 6, 6, 7, 3,  # top
            4, 5, 1, 1, 0, 4   # bottom
        ], dtype=np.uint32)
        
    
    def get_triangles_set(self, vertices, indices):
        """
        Get a set of triangles from vertices and indices.
        Each triangle is represented as a frozenset of tuples of vertex coordinates.
        This makes the comparison invariant to vertex order within triangles.
        """
        triangles = set()
        for i in range(0, len(indices), 3):
            # Get the three vertices of the triangle
            v1 = tuple(vertices[indices[i]])
            v2 = tuple(vertices[indices[i+1]])
            v3 = tuple(vertices[indices[i+2]])
            # Create a frozenset of the vertices (order-invariant)
            triangle = frozenset([v1, v2, v3])
            triangles.add(triangle)
        return triangles
    
    def test_encode_decode_vertices(self):
        """Test that encoding and decoding vertices preserves the data."""
        # Encode vertices
        encoded_vertices = encode_vertex_buffer(
            self.vertices, 
            len(self.vertices), 
            self.vertices.itemsize * self.vertices.shape[1]
        )
        
        # Decode vertices using the new function that returns a numpy array
        decoded_vertices = decode_vertex_buffer(
            len(self.vertices), 
            self.vertices.itemsize * self.vertices.shape[1], 
            encoded_vertices
        )
        
        # Check that the decoded vertices match the original
        np.testing.assert_array_almost_equal(self.vertices, decoded_vertices)
    
    def test_encode_decode_index_buffer(self):
        """Test that encoding and decoding indices preserves the data."""
        # Encode indices
        encoded_indices = encode_index_buffer(
            self.indices,
            len(self.indices),
            len(self.vertices)
        )
        
        # Decode indices
        decoded_indices = decode_index_buffer(
            len(self.indices),
            4,  # 4 bytes for uint32
            encoded_indices
        )
        
        # The encoding/decoding process may reorder indices for optimization
        # So we don't check that the indices match exactly, but that they represent the same triangles
        original_triangles = self.get_triangles_set(self.vertices, self.indices)
        decoded_triangles = self.get_triangles_set(self.vertices, decoded_indices)
        self.assertEqual(original_triangles, decoded_triangles)
    
    def test_encode_decode_index_sequence(self):
        """Test that encoding and decoding index sequence preserves the data."""
        # Encode index sequence
        encoded_sequence = encode_index_sequence(
            self.indices,
            len(self.indices),
            len(self.vertices)
        )
        
        # Decode index sequence
        decoded_sequence = decode_index_sequence(
            len(self.indices),
            4,  # 4 bytes for uint32
            encoded_sequence
        )
        
        # The encoding/decoding process may reorder indices for optimization
        # So we don't check that the indices match exactly, but that they represent the same triangles
        original_triangles = self.get_triangles_set(self.vertices, self.indices)
        decoded_triangles = self.get_triangles_set(self.vertices, decoded_sequence)
        self.assertEqual(original_triangles, decoded_triangles)


if __name__ == '__main__':
    unittest.main()
