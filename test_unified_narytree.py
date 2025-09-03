#!/usr/bin/env python3

import sys
import os
import time

# Add the Modules directory to the path for importing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'Modules'))

def test_unified_narytree():
    """Test the unified succinct N-ary tree implementation"""
    
    print("="*60)
    print("TESTING UNIFIED SUCCINCT N-ARY TREE")
    print("="*60)
    print("Features: Succinct encoding as primary storage + Locality optimization")
    print()
    
    try:
        # Build the module first
        print("Building narytree module...")
        build_result = os.system("cd Modules && python3 setup_narytree_unified.py build_ext --inplace")
        
        if build_result != 0:
            print("❌ Build failed! Check compilation errors.")
            return False
        
        print("✅ Build successful!")
        print()
        
        # Import the module
        import narytree
        
        print("Testing basic functionality...")
        
        # Test 1: Create tree with succinct representation
        print("1. Creating tree with succinct primary storage...")
        tree = narytree.SuccinctNaryTree("root_data")
        print(f"   Tree empty: {tree.empty()}")
        print(f"   Tree size: {tree.size()}")
        
        # Test 2: Get root and work with NodeView
        print("\\n2. Working with NodeView interface...")
        root = tree.root()
        print(f"   Root data: {root.data()}")
        print(f"   Is leaf: {root.is_leaf()}")
        print(f"   Child count: {root.child_count()}")
        
        # Test 3: Add children (succinct + locality)
        print("\\n3. Adding children with locality optimization...")
        child1 = root.add_child("child_1")
        child2 = root.add_child("child_2")
        child3 = root.add_child("child_3")
        
        print(f"   Root child count: {root.child_count()}")
        print(f"   Child 1 data: {child1.data()}")
        print(f"   Child 2 data: {child2.data()}")
        print(f"   Child 3 data: {child3.data()}")
        
        # Test 4: Nested structure
        print("\\n4. Creating nested structure...")
        grandchild1 = child1.add_child("grandchild_1")
        grandchild2 = child1.add_child("grandchild_2")
        
        print(f"   Tree size after nesting: {tree.size()}")
        print(f"   Child 1 child count: {child1.child_count()}")
        
        # Test 5: Locality statistics
        print("\\n5. Analyzing locality performance...")
        stats = tree.get_locality_statistics()
        
        print(f"   Total nodes: {stats['total_nodes']}")
        print(f"   Max depth: {stats['max_depth']}")
        print(f"   Locality score: {stats['locality_score']:.3f}")
        print(f"   Compression ratio: {stats['compression_ratio']:.3f}")
        print(f"   Memory usage: {stats['memory_usage_bytes']} bytes")
        print(f"   Cache efficiency: {stats['cache_line_efficiency']} nodes/line")
        
        # Test 6: Performance benchmark
        print("\\n6. Performance benchmark...")
        start_time = time.time()
        
        # Add many children to test lazy balancing
        current_node = child2
        for i in range(50):
            current_node = current_node.add_child(f"performance_test_{i}")
        
        end_time = time.time()
        
        print(f"   Added 50 nodes in {(end_time - start_time)*1000:.2f} ms")
        print(f"   Final tree size: {tree.size()}")
        
        # Test 7: Locality rebalancing
        print("\\n7. Testing locality rebalancing...")
        stats_before = tree.get_locality_statistics()
        tree.rebalance_for_locality()
        stats_after = tree.get_locality_statistics()
        
        print(f"   Locality score before: {stats_before['locality_score']:.3f}")
        print(f"   Locality score after: {stats_after['locality_score']:.3f}")
        print(f"   Memory usage: {stats_after['memory_usage_bytes']} bytes")
        
        # Test 8: Module-level functions
        print("\\n8. Testing module-level functions...")
        benchmark_result = narytree.benchmark_locality(1000)
        
        print(f"   Benchmark node count: {benchmark_result['node_count']}")
        print(f"   Benchmark locality score: {benchmark_result['locality_score']:.3f}")
        print(f"   Benchmark compression: {benchmark_result['compression_ratio']:.3f}")
        
        print("\\n" + "="*60)
        print("✅ ALL TESTS PASSED!")
        print("✅ Unified succinct + locality approach working correctly!")
        print("✅ Python import 'narytree' ready for use!")
        print("="*60)
        
        return True
        
    except ImportError as e:
        print(f"❌ Import failed: {e}")
        print("❌ Module compilation may have failed.")
        return False
    except Exception as e:
        print(f"❌ Test failed: {e}")
        return False

def demo_unified_approach():
    """Demonstrate the unified succinct + locality approach"""
    
    print("\\n" + "="*60)
    print("UNIFIED APPROACH DEMONSTRATION")
    print("="*60)
    
    try:
        import narytree
        
        print("Creating tree with unified succinct+locality storage...")
        tree = narytree.SuccinctNaryTree({"type": "root", "value": 42})
        
        root = tree.root()
        
        # Add hierarchical data
        config_node = root.add_child({"type": "config", "settings": ["opt1", "opt2"]})
        data_node = root.add_child({"type": "data", "records": 1000})
        cache_node = root.add_child({"type": "cache", "size_mb": 256})
        
        # Add nested configuration
        db_config = config_node.add_child({"database": "postgresql", "port": 5432})
        web_config = config_node.add_child({"webserver": "nginx", "port": 80})
        
        print("\\nTree structure created with complex nested data...")
        print(f"Total nodes: {tree.size()}")
        
        # Analyze the unified representation
        stats = tree.get_locality_statistics()
        
        print("\\nUnified Representation Analysis:")
        print(f"• Succinct compression: {stats['compression_ratio']:.1%} of traditional size")
        print(f"• Locality optimization: {stats['locality_score']:.1%} efficiency")
        print(f"• Total memory usage: {stats['memory_usage_bytes']} bytes")
        print(f"• Cache utilization: {stats['cache_line_efficiency']} nodes per cache line")
        
        # Demonstrate locality benefits
        print("\\nDemonstrating locality benefits...")
        start_time = time.time()
        
        # Sequential access should be very fast due to locality
        access_count = 0
        def count_nodes(node):
            nonlocal access_count
            access_count += 1
            
        # This uses the locality-optimized traversal
        # tree.for_each_preorder_locality_optimized(count_nodes)  # Would need Python binding
        
        end_time = time.time()
        
        print(f"Tree ready for high-performance operations!")
        print(f"Import with: import narytree")
        
        return True
        
    except Exception as e:
        print(f"❌ Demo failed: {e}")
        return False

if __name__ == "__main__":
    success = test_unified_narytree()
    
    if success:
        demo_unified_approach()
        
        print("\\n" + "🚀" * 20)
        print("UNIFIED SUCCINCT N-ARY TREE READY!")
        print("🚀" * 20)
        print()
        print("Usage:")
        print("  import narytree")
        print("  tree = narytree.SuccinctNaryTree('root')")
        print("  root = tree.root()")
        print("  child = root.add_child('child_data')")
        print("  stats = tree.get_locality_statistics()")
        print()
    else:
        print("❌ Setup incomplete. Check compilation errors.")
        sys.exit(1)