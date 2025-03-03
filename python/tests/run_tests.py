#!/usr/bin/env python3
"""
Test runner for the meshoptimizer Python wrapper.

This script runs all the tests for the meshoptimizer Python wrapper.
"""
import unittest
import sys
import os

# Add the parent directory to the path so we can import the meshoptimizer package
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

# Import the test modules
from test_encoding import TestEncoding
from test_optimization import TestOptimization
from test_simplification import TestSimplification
from test_mesh_integrity import TestMeshIntegrity

if __name__ == '__main__':
    # Create a test suite
    test_suite = unittest.TestSuite()
    
    # Create a test loader
    loader = unittest.TestLoader()
    
    # Add the test cases
    test_suite.addTest(loader.loadTestsFromTestCase(TestEncoding))
    test_suite.addTest(loader.loadTestsFromTestCase(TestOptimization))
    test_suite.addTest(loader.loadTestsFromTestCase(TestSimplification))
    test_suite.addTest(loader.loadTestsFromTestCase(TestMeshIntegrity))
    
    # Run the tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(test_suite)
    
    # Exit with non-zero code if tests failed
    sys.exit(not result.wasSuccessful())