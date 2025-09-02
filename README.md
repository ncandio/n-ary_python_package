# Succinct N-ary Tree - Clean Python Package

A high-performance C++17 N-ary tree implementation with Python bindings.

## Backstory

The development of this data structure was inspired by challenges in filesystem design and optimization. For more context, see the backstory at: https://nicoliberato.substack.com/p/from-ext4-to-zfs-the-ultimate-filesystem

## Installation from Source

After cloning the repository, you can install the `narytree` package using `pip`. This will compile the C++ extension and install the package in your Python environment.

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

## N-ary Tree API Documentation

This document outlines the API for the `NaryTree::Node` class and the advanced features of the `NaryTree` implementation.

---

### `NaryTree::Node` API

This is the primary interface for interacting with individual nodes in the tree.

#### Data & Parent Access
*   `data()` / `set_data(T new_data)`: Gets or sets the data stored in the node.
*   `parent()`: Returns a pointer to the parent node.
*   `is_root()`: Checks if the node is the root of the tree.

#### Child Management
*   `add_child(T child_data)`: Adds a new child to the node and returns a reference to the newly created child `Node`.
*   `add_child(std::unique_ptr<Node> child)`: Adds an existing node (and its entire subtree) as a child.
*   `remove_child(const Node* child)`: Removes a specific child node.
*   `child(size_t index)`: Accesses a child by its index.
*   `child_count()`: Returns the number of immediate children.
*   `is_leaf()`: Checks if the node has any children.

#### Node-Level Traversal & Search
*   `for_each_preorder(Func&& func)`: Traverses the subtree from this node downwards in pre-order.
*   `for_each_postorder(Func&& func)`: Traverses the subtree in post-order.
*   `for_each_levelorder(Func&& func)`: Traverses the subtree in level-order (breadth-first).
*   `find(Predicate&& pred)`: Searches the subtree from this node downwards.

---

### Advanced Features

The `NaryTree` implementation includes several advanced, performance-oriented features.

#### 1. Self-Balancing
The tree can be explicitly rebalanced to prevent it from becoming too deep, which can degrade performance.
*   `balance_tree(size_t max_children_per_node = 3)`: Reconstructs the tree to be more balanced by collecting all node data and rebuilding the structure in a breadth-first manner.
*   `needs_rebalancing()`: A heuristic to determine if the tree's depth is significantly greater than the optimal depth.
*   `auto_balance_if_needed()`: A convenience method that calls `balance_tree()` if `needs_rebalancing()` is true.

#### 2. Succinct Encoding
This allows you to serialize the tree into a highly compressed, memory-efficient representation.
*   `encode_succinct()`: Returns a `SuccinctEncoding` struct containing the tree structure as a bit vector and the data in a flat array.
*   `static NaryTree decode_succinct(const SuccinctEncoding& encoding)`: A static factory method to reconstruct a tree from its succinct representation.

#### 3. Array-Based Storage & Locality Optimization
This feature is for high-performance scenarios where cache-friendliness is critical.
*   `enable_array_storage()`: Converts the internal representation from pointer-based `Node` objects to an index-based `ArrayNode` vector.
*   `rebalance_for_locality()`: Reorders the internal array so that children are physically located next to their parents in memory, significantly improving cache performance during traversals.
*   `calculate_locality_score()`: A utility method to measure how effective the current memory layout is.