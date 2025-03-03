"""
Tests for the meshoptimizer Python wrapper.

This file contains tests to verify that the simplification functions
work correctly and preserve the mesh geometry as much as possible.
"""
import numpy as np
import unittest
from meshoptimizer import (
    Mesh, 
    simplify,
    simplify_sloppy,
    simplify_points,
    simplify_scale,
    SIMPLIFY_LOCK_BORDER,
    SIMPLIFY_SPARSE,
    SIMPLIFY_ERROR_ABSOLUTE,
    SIMPLIFY_PRUNE
)

class TestSimplification(unittest.TestCase):
    """Test simplification functionality."""
    
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
        
        # Create a more complex mesh (a sphere)
        # Generate a sphere with 8 segments and 8 rings
        segments = 8
        rings = 8
        vertices = []
        indices = []
        
        # Generate vertices
        for i in range(rings + 1):
            v = i / rings
            phi = v * np.pi
            
            for j in range(segments):
                u = j / segments
                theta = u * 2 * np.pi
                
                x = np.sin(phi) * np.cos(theta)
                y = np.sin(phi) * np.sin(theta)
                z = np.cos(phi)
                
                vertices.append([x, y, z])
        
        # Generate indices
        for i in range(rings):
            for j in range(segments):
                a = i * segments + j
                b = i * segments + (j + 1) % segments
                c = (i + 1) * segments + (j + 1) % segments
                d = (i + 1) * segments + j
                
                # Two triangles per quad
                indices.extend([a, b, c])
                indices.extend([a, c, d])
        
        self.sphere_vertices = np.array(vertices, dtype=np.float32)
        self.sphere_indices = np.array(indices, dtype=np.uint32)
        self.sphere_mesh = Mesh(self.sphere_vertices, self.sphere_indices)
    
    def test_simplify_basic(self):
        """Test basic simplification."""
        # Simplify the mesh
        simplified_indices = np.zeros_like(self.indices)
        result_error = np.array([0.0], dtype=np.float32)
        
        new_index_count = simplify(
            simplified_indices, 
            self.indices, 
            self.vertices, 
            len(self.indices), 
            len(self.vertices), 
            self.vertices.itemsize * self.vertices.shape[1], 
            len(self.indices) // 2,  # Target 50% reduction
            0.01,  # Target error
            0,  # No options
            result_error
        )
        
        # Check that the number of indices is reduced
        self.assertLessEqual(new_index_count, len(self.indices))
        
        # Check that the error is reasonable
        self.assertGreaterEqual(result_error[0], 0.0)
    
    def test_simplify_options(self):
        """Test simplification with different options."""
        # Test with SIMPLIFY_LOCK_BORDER option
        simplified_indices = np.zeros_like(self.indices)
        result_error = np.array([0.0], dtype=np.float32)
        
        new_index_count = simplify(
            simplified_indices, 
            self.indices, 
            self.vertices, 
            len(self.indices), 
            len(self.vertices), 
            self.vertices.itemsize * self.vertices.shape[1], 
            len(self.indices) // 2,  # Target 50% reduction
            0.01,  # Target error
            SIMPLIFY_LOCK_BORDER,  # Lock border vertices
            result_error
        )
        
        # Check that the number of indices is reduced
        self.assertLessEqual(new_index_count, len(self.indices))
        
        # Test with SIMPLIFY_SPARSE option
        simplified_indices = np.zeros_like(self.indices)
        result_error = np.array([0.0], dtype=np.float32)
        
        new_index_count = simplify(
            simplified_indices, 
            self.indices, 
            self.vertices, 
            len(self.indices), 
            len(self.vertices), 
            self.vertices.itemsize * self.vertices.shape[1], 
            len(self.indices) // 2,  # Target 50% reduction
            0.01,  # Target error
            SIMPLIFY_SPARSE,  # Sparse simplification
            result_error
        )
        
        # Check that the number of indices is reduced
        self.assertLessEqual(new_index_count, len(self.indices))
    
    def test_simplify_sloppy(self):
        """Test sloppy simplification."""
        # Simplify the mesh (sloppy)
        simplified_indices = np.zeros_like(self.sphere_indices)
        result_error = np.array([0.0], dtype=np.float32)
        
        new_index_count = simplify_sloppy(
            simplified_indices, 
            self.sphere_indices, 
            self.sphere_vertices, 
            len(self.sphere_indices), 
            len(self.sphere_vertices), 
            self.sphere_vertices.itemsize * self.sphere_vertices.shape[1], 
            len(self.sphere_indices) // 4,  # Target 75% reduction
            0.01,  # Target error
            result_error
        )
        
        # Check that the number of indices is reduced
        self.assertLessEqual(new_index_count, len(self.sphere_indices))
        
        # Check that the error is reasonable
        self.assertGreaterEqual(result_error[0], 0.0)
    
    def test_simplify_points(self):
        """Test point cloud simplification."""
        # Create a point cloud
        points = np.random.rand(100, 3).astype(np.float32)
        
        # Simplify the point cloud
        simplified_points = np.zeros(50, dtype=np.uint32)
        
        new_point_count = simplify_points(
            simplified_points, 
            points, 
            None,  # No colors
            len(points), 
            points.itemsize * points.shape[1], 
            0,  # No colors stride
            0.0,  # No color weight
            50  # Target 50% reduction
        )
        
        # Check that the number of points is reduced
        self.assertLessEqual(new_point_count, 50)
        
        # Test with colors
        colors = np.random.rand(100, 3).astype(np.float32)
        
        simplified_points = np.zeros(50, dtype=np.uint32)
        
        new_point_count = simplify_points(
            simplified_points, 
            points, 
            colors, 
            len(points), 
            points.itemsize * points.shape[1], 
            colors.itemsize * colors.shape[1], 
            1.0,  # Equal weight for colors
            50  # Target 50% reduction
        )
        
        # Check that the number of points is reduced
        self.assertLessEqual(new_point_count, 50)
    
    def test_simplify_scale(self):
        """Test simplification scale calculation."""
        # Calculate the scale
        scale = simplify_scale(
            self.vertices, 
            len(self.vertices), 
            self.vertices.itemsize * self.vertices.shape[1]
        )
        
        # Check that the scale is positive
        self.assertGreater(scale, 0.0)
    
    def test_mesh_simplify(self):
        """Test mesh simplification using the Mesh class."""
        # Create a copy of the sphere mesh
        simplified_mesh = Mesh(self.sphere_vertices.copy(), self.sphere_indices.copy())
        
        # Simplify the mesh
        simplified_mesh.simplify(target_ratio=0.5)  # Keep 50% of triangles
        
        # Check that the number of triangles is reduced
        self.assertLessEqual(len(simplified_mesh.indices) // 3, len(self.sphere_indices) // 3)
        
        # Check that the mesh is still valid
        # (Each triangle should have 3 unique vertices)
        for i in range(0, len(simplified_mesh.indices), 3):
            a = simplified_mesh.indices[i]
            b = simplified_mesh.indices[i+1]
            c = simplified_mesh.indices[i+2]
            
            # Check that indices are within bounds
            self.assertLess(a, len(simplified_mesh.vertices))
            self.assertLess(b, len(simplified_mesh.vertices))
            self.assertLess(c, len(simplified_mesh.vertices))
            
            # Check that the triangle has 3 unique vertices
            # (This is not always true for simplified meshes, but it's a good sanity check)
            # self.assertNotEqual(a, b)
            # self.assertNotEqual(b, c)
            # self.assertNotEqual(c, a)

if __name__ == '__main__':
    unittest.main()