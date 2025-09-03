#!/usr/bin/env python3
"""
Test runner for narytree package.
"""

import sys
import os
import subprocess

def main():
    """Run the test suite."""
    print("NaryTree Test Runner")
    print("=" * 40)
    
    # Check if narytree is installed
    try:
        import narytree
        print("✓ narytree package found")
    except ImportError:
        print("✗ narytree package not found!")
        print("Please install it first: python setup.py install --user")
        return 1
    
    # Run tests
    test_file = os.path.join("tests", "test_narytree.py")
    if not os.path.exists(test_file):
        print(f"✗ Test file not found: {test_file}")
        return 1
    
    print(f"Running tests from: {test_file}")
    print("-" * 40)
    
    try:
        result = subprocess.run([sys.executable, test_file], 
                              capture_output=False, 
                              text=True)
        return result.returncode
    except Exception as e:
        print(f"Error running tests: {e}")
        return 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)