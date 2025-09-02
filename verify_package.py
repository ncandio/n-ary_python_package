#!/usr/bin/env python3
"""
Verification script for the narytree package.
This script checks that all essential files are present and have the expected content.
"""

import os
import sys

def check_files():
    """Check that all required files are present."""
    required_files = [
        "MANIFEST.md",
        "narytreemodule_unified.cpp",
        "README.md",
        "requirements.txt",
        "setup.py",
        "succinct_narytree_unified.cpp",
        "test_package.py"
    ]
    
    missing_files = []
    for file in required_files:
        if not os.path.exists(file):
            missing_files.append(file)
    
    if missing_files:
        print(f"✗ Missing files: {missing_files}")
        return False
    else:
        print("✓ All required files are present")
        return True

def check_setup_py():
    """Check that setup.py has the correct configuration."""
    try:
        with open("setup.py", "r") as f:
            content = f.read()
        
        # Check for key elements
        if "narytree" not in content:
            print("✗ 'narytree' not found in setup.py")
            return False
            
        if "narytreemodule_unified.cpp" not in content:
            print("✗ Source file not found in setup.py")
            return False
            
        if "extra_compile_args=['-std=c++17'" not in content:
            print("✗ C++17 compile flag not found in setup.py")
            return False
            
        print("✓ setup.py has correct configuration")
        return True
    except Exception as e:
        print(f"✗ Error reading setup.py: {e}")
        return False

def check_cpp_files():
    """Check that C++ files have the expected content."""
    try:
        # Check main module file
        with open("narytreemodule_unified.cpp", "r") as f:
            content = f.read()
        
        if "#include <Python.h>" not in content:
            print("✗ Python.h not included in narytreemodule_unified.cpp")
            return False
            
        if "#include \"succinct_narytree_unified.cpp\"" not in content:
            print("✗ succinct_narytree_unified.cpp not included")
            return False
            
        # Check tree implementation file
        with open("succinct_narytree_unified.cpp", "r") as f:
            content = f.read()
        
        if "class SuccinctNaryTree" not in content:
            print("✗ SuccinctNaryTree class not found")
            return False
            
        print("✓ C++ files have expected content")
        return True
    except Exception as e:
        print(f"✗ Error reading C++ files: {e}")
        return False

def main():
    """Run all verification checks."""
    print("Verifying narytree package setup...")
    print("=" * 50)
    
    # Change to package directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    
    # Run checks
    files_ok = check_files()
    setup_ok = check_setup_py() if files_ok else False
    cpp_ok = check_cpp_files() if files_ok else False
    
    print("\n" + "=" * 50)
    if files_ok and setup_ok and cpp_ok:
        print("✓ All verification checks passed!")
        print("\nThis package is ready to be built with:")
        print("  pip install .")
        print("\nAfter building, test with:")
        print("  python test_package.py")
        return 0
    else:
        print("✗ Some verification checks failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())
