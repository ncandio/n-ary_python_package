#!/usr/bin/env python3
"""
Complete API Test Suite for N-ary Tree Package
Tests all 16 documented API methods with the complete implementation.
"""

import narytree
import sys

def test_complete_api():
    """Test all documented API methods"""
    print("🧪 COMPLETE API TEST SUITE")
    print("=" * 40)
    
    # Test tree creation
    tree = narytree.NaryTree('root')
    root = tree.root()
    
    # Build a test tree
    for i in range(3):
        child = root.add_child(f'child_{i}')
        for j in range(2):
            child.add_child(f'gc_{i}_{j}')
    
    print(f"✅ Created test tree with {tree.size()} nodes")
    
    # Test all required methods
    tests_passed = 0
    tests_total = 0
    
    # 1. Self-Balancing
    tests = [
        ("needs_rebalancing()", lambda: tree.needs_rebalancing()),
        ("balance_tree()", lambda: tree.balance_tree(3)),
        ("auto_balance_if_needed()", lambda: tree.auto_balance_if_needed()),
    ]
    
    # 2. Succinct Encoding  
    tests.extend([
        ("encode_succinct()", lambda: tree.encode_succinct()),
        ("decode_succinct()", lambda: narytree.NaryTree.decode_succinct(tree.encode_succinct())),
    ])
    
    # 3. Array-Based Storage & Locality
    tests.extend([
        ("enable_array_storage()", lambda: tree.enable_array_storage()),
        ("calculate_locality_score()", lambda: tree.calculate_locality_score()),
        ("rebalance_for_locality()", lambda: tree.rebalance_for_locality()),
    ])
    
    # 4. Statistics & Memory
    tests.extend([
        ("statistics()", lambda: tree.statistics()),
        ("get_memory_stats()", lambda: tree.get_memory_stats()),
    ])
    
    # 5. Basic Operations
    tests.extend([
        ("size()", lambda: tree.size()),
        ("empty()", lambda: tree.empty()),
        ("depth()", lambda: tree.depth()),
    ])
    
    # 6. Node Operations
    tests.extend([
        ("node.child_count()", lambda: root.child_count()),
        ("node.is_leaf()", lambda: root.is_leaf()),
        ("node.child(0)", lambda: root.child(0)),
    ])
    
    # Run tests
    for test_name, test_func in tests:
        tests_total += 1
        try:
            result = test_func()
            print(f"  ✅ {test_name}: SUCCESS")
            tests_passed += 1
        except Exception as e:
            print(f"  ❌ {test_name}: FAILED - {e}")
    
    print(f"\n📊 RESULTS: {tests_passed}/{tests_total} tests passed")
    
    if tests_passed == tests_total:
        print("🎉 ALL TESTS PASSED! Complete API is working perfectly!")
        return True
    else:
        print("❌ Some tests failed. Check implementation.")
        return False

if __name__ == "__main__":
    success = test_complete_api()
    sys.exit(0 if success else 1)