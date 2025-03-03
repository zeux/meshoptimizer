"""
Tests for the meshoptimizer Python wrapper.

This file contains tests to verify that the encoding/decoding process
preserves the mesh geometry correctly.
"""
import numpy as np
import unittest
from meshoptimizer import Mesh, encode_vertex_buffer, encode_index_buffer, decode_vertex_buffer, decode_index_buffer

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
        
        self.mesh = Mesh(self.vertices, self.indices)
    
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
    
    def test_encode_decode_mesh(self):
        """Test that encoding and decoding a mesh preserves the geometry."""
        # Encode the mesh
        encoded = self.mesh.encode()
        
        # Decode the mesh
        decoded_mesh = Mesh.decode(
            encoded,
            vertex_count=len(self.mesh.vertices),
            vertex_size=self.mesh.vertices.itemsize * self.mesh.vertices.shape[1],
            index_count=len(self.mesh.indices)
        )
        
        # Check that the decoded vertices match the original
        np.testing.assert_array_almost_equal(self.mesh.vertices, decoded_mesh.vertices)
        
        # The indices might not match exactly due to how the encoding/decoding works,
        # but the geometry should be preserved. Let's check that by comparing
        # the triangles.
        
        # Get the triangles from the original and decoded meshes
        original_triangles = self.get_triangles_set(self.mesh.vertices, self.mesh.indices)
        decoded_triangles = self.get_triangles_set(decoded_mesh.vertices, decoded_mesh.indices)
        
        # Check that the triangles match
        self.assertEqual(original_triangles, decoded_triangles)
    
    def test_optimize_and_encode_decode(self):
        """Test that optimizing and then encoding/decoding preserves the geometry."""
        # Optimize the mesh
        optimized_mesh = Mesh(self.vertices.copy(), self.indices.copy())
        optimized_mesh.optimize_vertex_cache()
        optimized_mesh.optimize_overdraw()
        optimized_mesh.optimize_vertex_fetch()
        
        # Encode the optimized mesh
        encoded = optimized_mesh.encode()
        
        # Decode the mesh
        decoded_mesh = Mesh.decode(
            encoded,
            vertex_count=len(optimized_mesh.vertices),
            vertex_size=optimized_mesh.vertices.itemsize * optimized_mesh.vertices.shape[1],
            index_count=len(optimized_mesh.indices)
        )
        
        # Check that the decoded vertices match the optimized vertices
        np.testing.assert_array_almost_equal(optimized_mesh.vertices, decoded_mesh.vertices)
        
        # Get the triangles from the optimized and decoded meshes
        optimized_triangles = self.get_triangles_set(optimized_mesh.vertices, optimized_mesh.indices)
        decoded_triangles = self.get_triangles_set(decoded_mesh.vertices, decoded_mesh.indices)
        
        # Check that the triangles match
        self.assertEqual(optimized_triangles, decoded_triangles)
    
    def test_simplify_and_encode_decode(self):
        """Test that simplifying and then encoding/decoding preserves the geometry."""
        # Simplify the mesh
        simplified_mesh = Mesh(self.vertices.copy(), self.indices.copy())
        simplified_mesh.simplify(target_ratio=0.5)  # Keep 50% of triangles
        
        # Encode the simplified mesh
        encoded = simplified_mesh.encode()
        
        # Decode the mesh
        decoded_mesh = Mesh.decode(
            encoded,
            vertex_count=len(simplified_mesh.vertices),
            vertex_size=simplified_mesh.vertices.itemsize * simplified_mesh.vertices.shape[1],
            index_count=len(simplified_mesh.indices)
        )
        
        # Check that the decoded vertices match the simplified vertices
        np.testing.assert_array_almost_equal(simplified_mesh.vertices, decoded_mesh.vertices)
        
        # Get the triangles from the simplified and decoded meshes
        simplified_triangles = self.get_triangles_set(simplified_mesh.vertices, simplified_mesh.indices)
        decoded_triangles = self.get_triangles_set(decoded_mesh.vertices, decoded_mesh.indices)
        
        # Check that the triangles match
        self.assertEqual(simplified_triangles, decoded_triangles)

if __name__ == '__main__':
    unittest.main()
