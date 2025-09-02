#!/usr/bin/env python3
"""
Simple test to verify the narytree package can be built and imported.
"""

def test_import():
    """Test that we can import the narytree module."""
    try:
        import narytree
        print("✓ Successfully imported narytree module")
        return True
    except ImportError as e:
        print(f"✗ Failed to import narytree module: {e}")
        return False

def test_basic_functionality():
    """Test basic functionality of the narytree module."""
    try:
        import narytree
        
        # Create a tree
        tree = narytree.SuccinctNaryTree("root")
        print("✓ Successfully created SuccinctNaryTree")
        
        # Get root node
        root = tree.root()
        print("✓ Successfully got root node")
        
        # Check root data
        assert root.data() == "root"
        print("✓ Root data is correct")
        
        # Add a child
        child = root.add_child("child")
        print("✓ Successfully added child node")
        
        # Check tree size
        assert tree.size() == 2
        print("✓ Tree size is correct")
        
        return True
    except Exception as e:
        print(f"✗ Error in basic functionality test: {e}")
        return False

if __name__ == "__main__":
    print("Testing narytree package...")
    print("=" * 40)
    
    import_success = test_import()
    if import_success:
        functionality_success = test_basic_functionality()
        if functionality_success:
            print("\n✓ All tests passed!")
        else:
            print("\n✗ Functionality tests failed!")
    else:
        print("\n✗ Import test failed!")
