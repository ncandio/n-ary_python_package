#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <queue>
#include <stack>
#include <type_traits>
#include <limits>
#include <cmath>
#include <cstring>
#include <iostream>

template <typename T>
class SuccinctNaryTree {
public:
    // Unified succinct representation as primary working storage
    struct SuccinctWorkingStorage {
        std::vector<bool> structure_bits;      // 2n+1 bits for tree structure
        std::vector<T> data_array;             // Linear array of data in preorder
        std::vector<uint32_t> parent_indices;  // Parent index for each node
        std::vector<uint32_t> first_child_pos; // First child position in structure_bits
        std::vector<uint16_t> child_counts;    // Number of children per node
        size_t node_count;
        size_t operations_since_balance;
        
        SuccinctWorkingStorage() : node_count(0), operations_since_balance(0) {}
        
        // Memory usage calculation
        size_t memory_usage() const {
            size_t bit_bytes = (structure_bits.size() + 7) / 8;
            size_t data_bytes = data_array.size() * sizeof(T);
            size_t index_bytes = parent_indices.size() * sizeof(uint32_t);
            size_t child_pos_bytes = first_child_pos.size() * sizeof(uint32_t);
            size_t child_count_bytes = child_counts.size() * sizeof(uint16_t);
            size_t metadata_bytes = sizeof(size_t) * 2;
            
            return bit_bytes + data_bytes + index_bytes + child_pos_bytes + 
                   child_count_bytes + metadata_bytes;
        }
        
        // Compression ratio vs traditional pointer-based tree
        double compression_ratio() const {
            if (node_count == 0) return 1.0;
            size_t traditional_size = node_count * (sizeof(void*) * 3 + sizeof(T) + 32);
            return (double)memory_usage() / traditional_size;
        }
    };

private:
    SuccinctWorkingStorage storage_;
    static const size_t LAZY_BALANCE_THRESHOLD = 100;
    
public:
    // Node interface that works directly with succinct representation
    class NodeView {
    private:
        SuccinctNaryTree* tree_;
        size_t node_index_;
        
    public:
        NodeView(SuccinctNaryTree* tree, size_t index) 
            : tree_(tree), node_index_(index) {}
        
        // Data access with locality optimization
        const T& data() const {
            if (node_index_ >= tree_->storage_.data_array.size()) {
                throw std::out_of_range("Invalid node index");
            }
            return tree_->storage_.data_array[node_index_];
        }
        
        T& data() {
            if (node_index_ >= tree_->storage_.data_array.size()) {
                throw std::out_of_range("Invalid node index");
            }
            return tree_->storage_.data_array[node_index_];
        }
        
        void set_data(const T& new_data) {
            if (node_index_ >= tree_->storage_.data_array.size()) {
                throw std::out_of_range("Invalid node index");
            }
            tree_->storage_.data_array[node_index_] = new_data;
        }
        
        // Child management with succinct structure updates
        NodeView add_child(const T& child_data) {
            return tree_->add_child_to_node(node_index_, child_data);
        }
        
        bool remove_child(size_t child_index) {
            return tree_->remove_child_from_node(node_index_, child_index);
        }
        
        // Locality-optimized child access
        size_t child_count() const {
            if (node_index_ >= tree_->storage_.child_counts.size()) return 0;
            return tree_->storage_.child_counts[node_index_];
        }
        
        NodeView child(size_t child_index) {
            size_t child_node_index = tree_->get_child_node_index(node_index_, child_index);
            return NodeView(tree_, child_node_index);
        }
        
        // Parent access
        NodeView parent() {
            if (node_index_ >= tree_->storage_.parent_indices.size()) {
                throw std::out_of_range("Invalid node index");
            }
            uint32_t parent_idx = tree_->storage_.parent_indices[node_index_];
            return NodeView(tree_, parent_idx);
        }
        
        bool is_root() const {
            return node_index_ == 0;
        }
        
        bool is_leaf() const {
            return child_count() == 0;
        }
        
        size_t index() const { return node_index_; }
    };
    
public:
    // Constructors
    explicit SuccinctNaryTree(const T& root_data) {
        initialize_with_root(root_data);
    }
    
    SuccinctNaryTree() {
        storage_.node_count = 0;
        storage_.operations_since_balance = 0;
    }
    
    // Tree operations working directly on succinct representation
    bool empty() const noexcept { 
        return storage_.node_count == 0; 
    }
    
    size_t size() const noexcept { 
        return storage_.node_count; 
    }
    
    void set_root(const T& root_data) {
        clear();
        initialize_with_root(root_data);
    }
    
    NodeView root() {
        if (empty()) throw std::runtime_error("Tree is empty");
        return NodeView(this, 0);
    }
    
    void clear() {
        storage_.structure_bits.clear();
        storage_.data_array.clear();
        storage_.parent_indices.clear();
        storage_.first_child_pos.clear();
        storage_.child_counts.clear();
        storage_.node_count = 0;
        storage_.operations_since_balance = 0;
    }
    
    // Locality-optimized operations on succinct representation
    NodeView add_child_to_node(size_t parent_index, const T& child_data) {
        if (parent_index >= storage_.node_count) {
            throw std::out_of_range("Invalid parent index");
        }
        
        // Add to succinct arrays with locality optimization
        size_t new_index = storage_.node_count;
        
        // Insert data maintaining preorder locality
        storage_.data_array.push_back(child_data);
        storage_.parent_indices.push_back(parent_index);
        storage_.child_counts.push_back(0);
        storage_.first_child_pos.push_back(0);
        
        // Update parent's child count
        if (parent_index < storage_.child_counts.size()) {
            storage_.child_counts[parent_index]++;
        }
        
        // Update structure bits to maintain succinct property
        update_structure_bits_for_new_child(parent_index, new_index);
        
        storage_.node_count++;
        storage_.operations_since_balance++;
        
        // Lazy balancing for locality optimization
        check_and_rebalance_for_locality();
        
        return NodeView(this, new_index);
    }
    
    bool remove_child_from_node(size_t parent_index, size_t child_index) {
        if (parent_index >= storage_.node_count) return false;
        
        size_t child_node_index = get_child_node_index(parent_index, child_index);
        if (child_node_index >= storage_.node_count) return false;
        
        // Remove from succinct arrays while maintaining locality
        remove_node_and_descendants(child_node_index);
        
        storage_.operations_since_balance++;
        check_and_rebalance_for_locality();
        
        return true;
    }
    
    // Locality-optimized traversal working directly on succinct arrays
    template<typename Func>
    void for_each_preorder_locality_optimized(Func&& func) {
        if (empty()) return;
        
        // Sequential access through data_array for optimal cache usage
        for (size_t i = 0; i < storage_.data_array.size(); ++i) {
            NodeView node(this, i);
            func(node);
        }
    }
    
    template<typename Func>
    void for_each_levelorder_locality_optimized(Func&& func) {
        if (empty()) return;
        
        std::queue<size_t> queue;
        queue.push(0); // Root index
        
        while (!queue.empty()) {
            size_t current_index = queue.front();
            queue.pop();
            
            NodeView node(this, current_index);
            func(node);
            
            // Add children with locality-aware ordering
            size_t child_count = storage_.child_counts[current_index];
            for (size_t i = 0; i < child_count; ++i) {
                size_t child_idx = get_child_node_index(current_index, i);
                queue.push(child_idx);
            }
        }
    }
    
    // Get current succinct representation (no conversion needed!)
    const SuccinctWorkingStorage& get_succinct_representation() const {
        return storage_;
    }
    
    // Load from succinct representation
    void load_from_succinct(const SuccinctWorkingStorage& succinct) {
        storage_ = succinct;
        // No conversion needed - succinct IS the working representation
    }
    
    // Lazy balancing for locality optimization on succinct representation
    void rebalance_for_locality() {
        if (storage_.node_count <= 3) return;
        
        // Collect data in breadth-first order for optimal locality
        std::vector<T> reordered_data;
        std::vector<uint32_t> new_parent_indices;
        std::vector<uint16_t> new_child_counts;
        std::vector<uint32_t> new_first_child_pos;
        std::vector<bool> new_structure_bits;
        
        reordered_data.reserve(storage_.node_count);
        new_parent_indices.reserve(storage_.node_count);
        new_child_counts.reserve(storage_.node_count);
        new_first_child_pos.reserve(storage_.node_count);
        new_structure_bits.reserve(storage_.structure_bits.size());
        
        // Breadth-first reordering for cache locality
        std::queue<size_t> queue;
        std::vector<size_t> old_to_new_mapping(storage_.node_count);
        
        queue.push(0); // Start with root
        size_t new_index = 0;
        
        while (!queue.empty()) {
            size_t old_index = queue.front();
            queue.pop();
            
            old_to_new_mapping[old_index] = new_index;
            
            // Copy node data in locality-optimized order
            reordered_data.push_back(storage_.data_array[old_index]);
            new_child_counts.push_back(storage_.child_counts[old_index]);
            
            // Update parent index
            if (old_index == 0) {
                new_parent_indices.push_back(0); // Root parent
            } else {
                uint32_t old_parent = storage_.parent_indices[old_index];
                new_parent_indices.push_back(old_to_new_mapping[old_parent]);
            }
            
            new_first_child_pos.push_back(0); // Will be updated later
            
            // Add children to queue for breadth-first processing
            size_t child_count = storage_.child_counts[old_index];
            for (size_t i = 0; i < child_count; ++i) {
                size_t child_idx = get_child_node_index(old_index, i);
                queue.push(child_idx);
            }
            
            new_index++;
        }
        
        // Update succinct representation with locality-optimized layout
        storage_.data_array = std::move(reordered_data);
        storage_.parent_indices = std::move(new_parent_indices);
        storage_.child_counts = std::move(new_child_counts);
        storage_.first_child_pos = std::move(new_first_child_pos);
        
        // Rebuild structure bits in locality-optimized order
        rebuild_structure_bits_locality_optimized();
        
        storage_.operations_since_balance = 0;
    }
    
    // Statistics with locality analysis
    struct LocalityStats {
        size_t total_nodes;
        size_t max_depth;
        double locality_score;        // 0.0-1.0, higher is better
        size_t cache_line_efficiency; // Nodes per cache line
        double compression_ratio;     // vs traditional tree
        size_t memory_usage_bytes;
    };
    
    LocalityStats get_locality_statistics() const {
        LocalityStats stats;
        stats.total_nodes = storage_.node_count;
        stats.memory_usage_bytes = storage_.memory_usage();
        stats.compression_ratio = storage_.compression_ratio();
        
        // Calculate locality score based on access patterns
        stats.locality_score = calculate_locality_score();
        
        // Cache line efficiency (assume 64-byte cache lines)
        size_t nodes_per_cache_line = 64 / sizeof(T);
        stats.cache_line_efficiency = nodes_per_cache_line;
        
        // Calculate max depth
        stats.max_depth = calculate_max_depth();
        
        return stats;
    }

private:
    void initialize_with_root(const T& root_data) {
        storage_.data_array.clear();
        storage_.parent_indices.clear();
        storage_.child_counts.clear();
        storage_.first_child_pos.clear();
        storage_.structure_bits.clear();
        
        // Initialize root in succinct representation
        storage_.data_array.push_back(root_data);
        storage_.parent_indices.push_back(0); // Root is its own parent
        storage_.child_counts.push_back(0);
        storage_.first_child_pos.push_back(0);
        storage_.structure_bits.push_back(true);  // Root marker
        storage_.structure_bits.push_back(false); // End of root children
        
        storage_.node_count = 1;
        storage_.operations_since_balance = 0;
    }
    
    void update_structure_bits_for_new_child(size_t /*parent_index*/, size_t /*child_index*/) {
        // Update structure bits to maintain succinct property
        // This is simplified - in practice would need careful bit manipulation
        storage_.structure_bits.push_back(true);  // Child node
        storage_.structure_bits.push_back(false); // End of child's children
    }
    
    size_t get_child_node_index(size_t parent_index, size_t child_index) const {
        if (parent_index >= storage_.child_counts.size()) return SIZE_MAX;
        if (child_index >= storage_.child_counts[parent_index]) return SIZE_MAX;
        
        // Find child using locality-optimized search
        size_t found_children = 0;
        for (size_t i = parent_index + 1; i < storage_.node_count; ++i) {
            if (storage_.parent_indices[i] == parent_index) {
                if (found_children == child_index) {
                    return i;
                }
                found_children++;
            }
        }
        return SIZE_MAX;
    }
    
    void remove_node_and_descendants(size_t node_index) {
        // Mark for removal and compact arrays
        // Simplified implementation - would need careful index management
        if (node_index >= storage_.node_count) return;
        
        // Find all descendants
        std::vector<size_t> to_remove;
        collect_descendants(node_index, to_remove);
        
        // Sort in reverse order for safe removal
        std::sort(to_remove.rbegin(), to_remove.rend());
        
        // Remove from all arrays maintaining consistency
        for (size_t idx : to_remove) {
            if (idx < storage_.data_array.size()) {
                storage_.data_array.erase(storage_.data_array.begin() + idx);
                storage_.parent_indices.erase(storage_.parent_indices.begin() + idx);
                storage_.child_counts.erase(storage_.child_counts.begin() + idx);
                storage_.first_child_pos.erase(storage_.first_child_pos.begin() + idx);
                storage_.node_count--;
            }
        }
        
        // Update parent indices after removal
        update_indices_after_removal(to_remove);
    }
    
    void collect_descendants(size_t node_index, std::vector<size_t>& descendants) {
        descendants.push_back(node_index);
        
        // Find direct children
        for (size_t i = 0; i < storage_.node_count; ++i) {
            if (storage_.parent_indices[i] == node_index) {
                collect_descendants(i, descendants);
            }
        }
    }
    
    void update_indices_after_removal(const std::vector<size_t>& removed_indices) {
        // Update all parent indices to account for removed nodes
        for (size_t i = 0; i < storage_.parent_indices.size(); ++i) {
            uint32_t& parent_idx = storage_.parent_indices[i];
            
            // Count how many removed indices are before this parent index
            size_t offset = 0;
            for (size_t removed_idx : removed_indices) {
                if (removed_idx < parent_idx) offset++;
            }
            
            if (offset > 0) {
                parent_idx -= offset;
            }
        }
    }
    
    void check_and_rebalance_for_locality() {
        if (storage_.operations_since_balance >= LAZY_BALANCE_THRESHOLD) {
            if (needs_locality_rebalancing()) {
                rebalance_for_locality();
            }
        }
    }
    
    bool needs_locality_rebalancing() const {
        if (storage_.node_count <= 3) return false;
        
        // Check if locality score is below threshold
        double locality_score = calculate_locality_score();
        return locality_score < 0.7; // Threshold for rebalancing
    }
    
    double calculate_locality_score() const {
        if (storage_.node_count <= 1) return 1.0;
        
        double score = 0.0;
        size_t comparisons = 0;
        
        // Analyze spatial locality in the arrays
        for (size_t i = 0; i < storage_.node_count; ++i) {
            size_t child_count = storage_.child_counts[i];
            
            if (child_count > 0) {
                // Check if children are stored close to parent (good locality)
                for (size_t j = 0; j < child_count; ++j) {
                    size_t child_idx = get_child_node_index(i, j);
                    if (child_idx != SIZE_MAX) {
                        // Closer children = better locality score
                        double distance = std::abs((int)child_idx - (int)i);
                        score += 1.0 / (1.0 + distance / 10.0);
                        comparisons++;
                    }
                }
            }
        }
        
        return comparisons > 0 ? score / comparisons : 1.0;
    }
    
    void rebuild_structure_bits_locality_optimized() {
        storage_.structure_bits.clear();
        storage_.structure_bits.reserve(2 * storage_.node_count + 1);
        
        // Rebuild structure bits in breadth-first order for locality
        std::queue<size_t> queue;
        queue.push(0);
        
        while (!queue.empty()) {
            size_t current = queue.front();
            queue.pop();
            
            storage_.structure_bits.push_back(true); // Node marker
            
            // Process children
            size_t child_count = storage_.child_counts[current];
            for (size_t i = 0; i < child_count; ++i) {
                size_t child_idx = get_child_node_index(current, i);
                if (child_idx != SIZE_MAX) {
                    queue.push(child_idx);
                }
            }
            
            storage_.structure_bits.push_back(false); // End of children
        }
    }
    
    size_t calculate_max_depth() const {
        if (empty()) return 0;
        
        size_t max_depth = 0;
        std::queue<std::pair<size_t, size_t>> queue; // {node_index, depth}
        queue.push({0, 1});
        
        while (!queue.empty()) {
            auto [node_idx, depth] = queue.front();
            queue.pop();
            
            max_depth = std::max(max_depth, depth);
            
            size_t child_count = storage_.child_counts[node_idx];
            for (size_t i = 0; i < child_count; ++i) {
                size_t child_idx = get_child_node_index(node_idx, i);
                if (child_idx != SIZE_MAX) {
                    queue.push({child_idx, depth + 1});
                }
            }
        }
        
        return max_depth;
    }
    
    // Search with locality optimization
    template<typename Predicate>
    NodeView find_locality_optimized(Predicate&& pred) {
        // Sequential search through data_array for cache efficiency
        for (size_t i = 0; i < storage_.data_array.size(); ++i) {
            if (pred(storage_.data_array[i])) {
                return NodeView(this, i);
            }
        }
        throw std::runtime_error("Node not found");
    }
    
    // Memory analysis
    void analyze_memory_layout() const {
        auto stats = get_locality_statistics();
        
        std::cout << "=== SUCCINCT N-ARY TREE MEMORY ANALYSIS ===" << std::endl;
        std::cout << "Total nodes: " << stats.total_nodes << std::endl;
        std::cout << "Memory usage: " << stats.memory_usage_bytes << " bytes" << std::endl;
        std::cout << "Compression ratio: " << stats.compression_ratio << std::endl;
        std::cout << "Locality score: " << stats.locality_score << std::endl;
        std::cout << "Cache efficiency: " << stats.cache_line_efficiency << " nodes/line" << std::endl;
        std::cout << "Max depth: " << stats.max_depth << std::endl;
    }
    
    friend class NodeView;
};