# Succinct N-ary Tree - Clean Python Package

A high-performance C++17 N-ary tree implementation with Python bindings.

## Backstory

The development of this data structure was inspired by challenges in filesystem design and optimization. For more context, see the backstory at: https://nicoliberato.substack.com/p/from-ext4-to-zfs-the-ultimate-filesystem

## Installation from Source

```bash
# Clone the repository
git clone https://github.com/ncandio/nary_tree_improved.git
cd nary_tree_improved

# Install locally with pip
pip install .

# Or install in development mode
pip install -e .
```

## Quick Example

```python
import narytree

# Create a tree and build a hierarchy
tree = narytree.SuccinctNaryTree("Company")
root = tree.root()

# Add departments
engineering = root.add_child("Engineering")
sales = root.add_child("Sales")

# Add teams
backend = engineering.add_child("Backend Team")
backend.add_child("Alice (Senior Dev)")
backend.add_child("Bob (Junior Dev)")

print(f"Total size: {tree.size()} people")

# Check performance
stats = tree.get_locality_statistics()
print(f"Memory: {stats['memory_usage_bytes']} bytes")
print(f"Compression: {stats['compression_ratio']:.2f}")
```

This implementation features succinct data structures, locality optimization, lazy balancing, and significant memory reduction compared to traditional tree implementations.
