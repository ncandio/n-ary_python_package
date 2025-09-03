#!/usr/bin/env python3
"""
Basic test suite for the narytree package.
"""

import unittest
import sys
import os

# Add parent directory to path for local testing
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    import narytree
except ImportError:
    print("ERROR: narytree package not found. Please install it first:")
    print("python setup.py install --user")
    sys.exit(1)


class TestSuccinctNaryTree(unittest.TestCase):
    """Test cases for SuccinctNaryTree class."""
    
    def setUp(self):
        """Set up test fixtures before each test method."""
        self.tree = narytree.SuccinctNaryTree()
    
    def test_empty_tree_creation(self):
        """Test creating an empty tree."""
        self.assertTrue(self.tree.empty())
        self.assertEqual(self.tree.size(), 0)
    
    def test_tree_with_root_data(self):
        """Test creating a tree with root data."""
        tree = narytree.SuccinctNaryTree("root_data")
        self.assertFalse(tree.empty())
        self.assertEqual(tree.size(), 1)
        
        root = tree.root()
        self.assertIsNotNone(root)
        self.assertEqual(root.data(), "root_data")
        self.assertTrue(root.is_leaf())
        self.assertEqual(root.child_count(), 0)
    
    def test_add_children(self):
        """Test adding children to a node."""
        self.tree.set_root("root")
        root = self.tree.root()
        
        child1 = root.add_child("child_1")
        child2 = root.add_child("child_2")
        
        self.assertEqual(root.child_count(), 2)
        self.assertEqual(self.tree.size(), 3)
        self.assertEqual(child1.data(), "child_1")
        self.assertEqual(child2.data(), "child_2")
    
    def test_locality_statistics(self):
        """Test locality statistics functionality."""
        self.tree.set_root("root")
        root = self.tree.root()
        
        # Add some structure
        for i in range(10):
            root.add_child(f"child_{i}")
        
        stats = self.tree.get_locality_statistics()
        
        # Verify stats structure
        self.assertIsInstance(stats, dict)
        self.assertIn('locality_score', stats)
        self.assertIn('compression_ratio', stats)
        self.assertIn('memory_usage_bytes', stats)
        
        # Verify stats values are reasonable
        self.assertGreaterEqual(stats['locality_score'], 0.0)
        self.assertGreater(stats['memory_usage_bytes'], 0)


class TestUtilityFunctions(unittest.TestCase):
    """Test cases for utility functions."""
    
    def test_create_tree_function(self):
        """Test the create_tree utility function."""
        tree = narytree.create_tree("test_root")
        self.assertIsInstance(tree, narytree.SuccinctNaryTree)
        self.assertFalse(tree.empty())
        self.assertEqual(tree.size(), 1)
    
    def test_benchmark_locality_function(self):
        """Test the benchmark_locality utility function."""
        result = narytree.benchmark_locality(50)
        
        # Verify result structure
        self.assertIsInstance(result, dict)
        self.assertIn('node_count', result)
        self.assertIn('locality_score', result)
        self.assertEqual(result['node_count'], 50)


if __name__ == "__main__":
    print("Running narytree test suite...")
    unittest.main(verbosity=2)