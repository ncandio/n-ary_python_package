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

// Enhanced N-ary Tree with array-based storage, locality optimization, and succinct encoding

template <typename T>
class NaryTree {
public:
    // Array-based node for better locality
    struct ArrayNode {
        T data;
        int parent_index;
        int first_child_index;
        int child_count;
        bool is_valid;
        
        ArrayNode() : parent_index(-1), first_child_index(-1), child_count(0), is_valid(false) {}
        ArrayNode(const T& d, int parent = -1) 
            : data(d), parent_index(parent), first_child_index(-1), child_count(0), is_valid(true) {}
    };

public:
    class Node {
        friend class NaryTree<T>;  // Allow NaryTree to access private members
    private:
        T data_;
        std::vector<std::unique_ptr<Node>> children_;
        Node* parent_;
        
    public:
        explicit Node(T data, Node* parent = nullptr) 
            : data_(std::move(data)), parent_(parent) {}
        
        ~Node() = default;
        
        // Move and copy semantics
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&) = default;
        Node& operator=(Node&&) = default;
        
        // Data access
        const T& data() const noexcept { return data_; }
        T& data() noexcept { return data_; }
        void set_data(T new_data) { data_ = std::move(new_data); }
        
        // Child management
        Node& add_child(T child_data) {
            auto child = std::make_unique<Node>(std::move(child_data), this);
            Node* child_ptr = child.get();
            children_.push_back(std::move(child));
            return *child_ptr;
        }
        
        void add_child(std::unique_ptr<Node> child) {
            if (child) {
                child->parent_ = this;
                children_.push_back(std::move(child));
            }
        }
        
        bool remove_child(const Node* child) {
            auto it = std::find_if(children_.begin(), children_.end(),
                [child](const std::unique_ptr<Node>& ptr) { return ptr.get() == child; });
            
            if (it != children_.end()) {
                children_.erase(it);
                return true;
            }
            return false;
        }
        
        void remove_all_children() { children_.clear(); }
        
        // Child access
        size_t child_count() const noexcept { return children_.size(); }
        bool is_leaf() const noexcept { return children_.empty(); }
        
        Node& child(size_t index) {
            if (index >= children_.size()) {
                throw std::out_of_range("Child index out of range");
            }
            return *children_[index];
        }
        
        const Node& child(size_t index) const {
            if (index >= children_.size()) {
                throw std::out_of_range("Child index out of range");
            }
            return *children_[index];
        }
        
        // Parent access
        Node* parent() noexcept { return parent_; }
        const Node* parent() const noexcept { return parent_; }
        bool is_root() const noexcept { return parent_ == nullptr; }
        
        // Tree navigation
        size_t depth() const {
            size_t max_depth = 0;
            for (const auto& child : children_) {
                max_depth = std::max(max_depth, child->depth());
            }
            return max_depth + 1;
        }
        
        size_t height_from_root() const {
            size_t height = 0;
            const Node* current = this;
            while (current->parent_ != nullptr) {
                current = current->parent_;
                ++height;
            }
            return height;
        }
        
        size_t total_nodes() const {
            size_t count = 1; // Count this node
            for (const auto& child : children_) {
                count += child->total_nodes();
            }
            return count;
        }
        
        // Tree traversal
        template<typename Func>
        void for_each_preorder(Func&& func) {
            func(*this);
            for (auto& child : children_) {
                child->for_each_preorder(std::forward<Func>(func));
            }
        }
        
        template<typename Func>
        void for_each_postorder(Func&& func) {
            for (auto& child : children_) {
                child->for_each_postorder(std::forward<Func>(func));
            }
            func(*this);
        }
        
        template<typename Func>
        void for_each_levelorder(Func&& func) {
            std::queue<Node*> queue;
            queue.push(this);
            
            while (!queue.empty()) {
                Node* current = queue.front();
                queue.pop();
                func(*current);
                
                for (auto& child : current->children_) {
                    queue.push(child.get());
                }
            }
        }
        
        // Search operations
        template<typename Predicate>
        Node* find(Predicate&& pred) {
            if (pred(*this)) return this;
            
            for (auto& child : children_) {
                if (auto result = child->find(std::forward<Predicate>(pred))) {
                    return result;
                }
            }
            return nullptr;
        }
        
        template<typename Predicate>
        const Node* find(Predicate&& pred) const {
            if (pred(*this)) return this;
            
            for (const auto& child : children_) {
                if (auto result = child->find(std::forward<Predicate>(pred))) {
                    return result;
                }
            }
            return nullptr;
        }
        
        // Utility methods
        std::vector<Node*> get_children() {
            std::vector<Node*> result;
            result.reserve(children_.size());
            std::transform(children_.begin(), children_.end(), std::back_inserter(result),
                [](const std::unique_ptr<Node>& ptr) { return ptr.get(); });
            return result;
        }
        
        std::vector<const Node*> get_children() const {
            std::vector<const Node*> result;
            result.reserve(children_.size());
            std::transform(children_.begin(), children_.end(), std::back_inserter(result),
                [](const std::unique_ptr<Node>& ptr) { return ptr.get(); });
            return result;
        }
    };

private:
    std::unique_ptr<Node> root_;
    size_t size_;

public:
    // Constructors
    explicit NaryTree(T root_data, bool enable_array = false) 
        : root_(std::make_unique<Node>(std::move(root_data))), size_(1),
          array_root_index_(-1), operations_since_balance_(0), use_array_storage_(enable_array) {
        if (enable_array) {
            enable_array_storage();
        }
    }
    
    NaryTree() : root_(nullptr), size_(0), array_root_index_(-1), 
                 operations_since_balance_(0), use_array_storage_(false) {}
    
    // Move semantics
    NaryTree(NaryTree&&) = default;
    NaryTree& operator=(NaryTree&&) = default;
    
    // Delete copy semantics for now (can be implemented later if needed)
    NaryTree(const NaryTree&) = delete;
    NaryTree& operator=(const NaryTree&) = delete;
    
    // Tree operations
    bool empty() const noexcept { return root_ == nullptr; }
    size_t size() const noexcept { return size_; }
    
    void set_root(T root_data) {
        root_ = std::make_unique<Node>(std::move(root_data));
        size_ = 1;
    }
    
    Node* root() noexcept { return root_.get(); }
    const Node* root() const noexcept { return root_.get(); }
    
    void clear() {
        root_.reset();
        size_ = 0;
    }
    
    size_t depth() const {
        return root_ ? root_->depth() : 0;
    }
    
    // Tree-wide operations
    template<typename Func>
    void for_each(Func&& func) {
        if (root_) {
            root_->for_each_preorder(std::forward<Func>(func));
        }
    }
    
    template<typename Predicate>
    Node* find(Predicate&& pred) {
        return root_ ? root_->find(std::forward<Predicate>(pred)) : nullptr;
    }
    
    template<typename Predicate>
    const Node* find(Predicate&& pred) const {
        return root_ ? root_->find(std::forward<Predicate>(pred)) : nullptr;
    }
    
    // Statistics
    struct TreeStats {
        size_t total_nodes = 0;
        size_t leaf_nodes = 0;
        size_t internal_nodes = 0;
        size_t max_depth = 0;
        double avg_children_per_node = 0.0;
        size_t max_children = 0;
        size_t min_children = std::numeric_limits<size_t>::max();
    };
    
    TreeStats get_statistics() const {
        TreeStats stats;
        if (!root_) return stats;
        
        size_t total_children = 0;
        
        root_->for_each_preorder([&stats, &total_children](const Node& node) {
            ++stats.total_nodes;
            
            if (node.is_leaf()) {
                ++stats.leaf_nodes;
                stats.min_children = std::min(stats.min_children, size_t(0));
            } else {
                ++stats.internal_nodes;
                size_t child_count = node.child_count();
                stats.max_children = std::max(stats.max_children, child_count);
                stats.min_children = std::min(stats.min_children, child_count);
                total_children += child_count;
            }
            
            stats.max_depth = std::max(stats.max_depth, node.height_from_root() + 1);
        });
        
        if (stats.internal_nodes > 0) {
            stats.avg_children_per_node = static_cast<double>(total_children) / stats.internal_nodes;
        }
        
        if (stats.leaf_nodes == stats.total_nodes) {
            stats.min_children = 0; // All nodes are leaves
        }
        
        return stats;
    }
    
    // Self-balancing functionality
    // Collects all nodes in level-order for memory-efficient rebalancing
    std::vector<T> collect_all_data() const {
        std::vector<T> data;
        if (!root_) return data;
        
        data.reserve(size_); // Memory optimization: pre-allocate
        
        std::queue<const Node*> queue;
        queue.push(root_.get());
        
        while (!queue.empty()) {
            const Node* current = queue.front();
            queue.pop();
            
            data.push_back(current->data()); // Copy data
            
            for (size_t i = 0; i < current->child_count(); ++i) {
                queue.push(&current->child(i));
            }
        }
        
        return data;
    }
    
    // Memory-efficient balanced tree reconstruction
    std::unique_ptr<Node> build_balanced_subtree(
        const std::vector<T>& data, 
        size_t start, 
        size_t end, 
        size_t max_children_per_node = 3) const {
        
        if (start >= end) return nullptr;
        
        // Create root of this subtree
        auto node = std::make_unique<Node>(data[start]);
        
        if (end - start == 1) return node; // Leaf node
        
        // Calculate optimal distribution of remaining nodes
        size_t remaining = end - start - 1;
        size_t children_count = std::min(remaining, max_children_per_node);
        
        if (children_count == 0) return node;
        
        // Distribute remaining nodes among children as evenly as possible
        size_t base_size = remaining / children_count;
        size_t extra = remaining % children_count;
        
        size_t current_start = start + 1;
        
        for (size_t i = 0; i < children_count && current_start < end; ++i) {
            size_t child_size = base_size + (i < extra ? 1 : 0);
            size_t child_end = current_start + child_size;
            
            if (child_end > end) child_end = end;
            
            auto child = build_balanced_subtree(data, current_start, child_end, max_children_per_node);
            if (child) {
                child->parent_ = node.get();
                node->children_.push_back(std::move(child));
            }
            
            current_start = child_end;
        }
        
        return node;
    }
    
    // Main self-balancing method
    void balance_tree(size_t max_children_per_node = 3) {
        if (!root_ || size_ <= 1) return; // Nothing to balance
        
        // Collect all data (memory-efficient approach)
        auto data = collect_all_data();
        
        // Rebuild tree with balanced structure
        root_ = build_balanced_subtree(data, 0, data.size(), max_children_per_node);
    }
    
    // Check if tree needs rebalancing (optimization heuristic)
    bool needs_rebalancing() const {
        if (!root_ || size_ <= 3) return false;
        
        auto stats = get_statistics();
        
        // Heuristic: rebalance if tree is too deep relative to optimal depth
        size_t optimal_depth = static_cast<size_t>(std::log(size_) / std::log(3)) + 1;
        
        return stats.max_depth > optimal_depth * 2;
    }
    
    // Automatic rebalancing with configurable threshold
    void auto_balance_if_needed(size_t max_children_per_node = 3) {
        if (needs_rebalancing()) {
            balance_tree(max_children_per_node);
        }
    }
    
    // Memory usage estimation
    struct MemoryStats {
        size_t node_memory_bytes;
        size_t data_memory_estimate;
        size_t total_estimated_bytes;
        double memory_per_node;
    };
    
    MemoryStats get_memory_stats() const {
        MemoryStats stats;
        
        stats.node_memory_bytes = size_ * sizeof(Node);
        stats.data_memory_estimate = size_ * sizeof(T); // Conservative estimate
        stats.total_estimated_bytes = stats.node_memory_bytes + stats.data_memory_estimate;
        stats.memory_per_node = size_ > 0 ? static_cast<double>(stats.total_estimated_bytes) / size_ : 0.0;
        
        return stats;
    }
    
    // Succinct data structure representation
    struct SuccinctEncoding {
        std::vector<bool> structure_bits;  // 2n+1 bits for tree structure
        std::vector<T> data_array;         // Linear array of data in preorder
        size_t node_count;
        
        size_t memory_usage() const {
            size_t bit_bytes = (structure_bits.size() + 7) / 8;  // Round up to bytes
            size_t data_bytes = data_array.size() * sizeof(T);
            size_t metadata_bytes = sizeof(size_t);
            return bit_bytes + data_bytes + metadata_bytes;
        }
        
        double compression_ratio() const {
            size_t traditional_size = node_count * (sizeof(void*) * 2 + sizeof(T) + 16);
            return traditional_size > 0 ? (double)memory_usage() / traditional_size : 1.0;
        }
    };
    
    // Convert N-ary tree to succinct representation using direct preorder encoding
    SuccinctEncoding encode_succinct() const {
        SuccinctEncoding encoding;
        if (!root_) return encoding;
        
        encode_succinct_preorder(root_.get(), encoding.structure_bits, encoding.data_array);
        encoding.node_count = size_;
        
        return encoding;
    }
    
private:
    // Direct preorder encoding: 1 for internal node, 0 for end of children
    void encode_succinct_preorder(const Node* node, std::vector<bool>& structure, std::vector<T>& data) const {
        if (!node) return;
        
        structure.push_back(true); // Internal node
        data.push_back(node->data());
        
        // Encode all children
        for (size_t i = 0; i < node->child_count(); ++i) {
            encode_succinct_preorder(&node->child(i), structure, data);
        }
        
        // Mark end of children for this node
        structure.push_back(false);
    }

public:
    // Decode succinct representation back to N-ary tree
    static NaryTree decode_succinct(const SuccinctEncoding& encoding) {
        NaryTree tree;
        if (encoding.structure_bits.empty() || encoding.data_array.empty()) {
            return tree;
        }
        
        size_t bit_index = 0;
        size_t data_index = 0;
        
        tree.root_ = decode_succinct_preorder(encoding.structure_bits, encoding.data_array, 
                                            bit_index, data_index);
        tree.size_ = encoding.node_count;
        
        return tree;
    }

    // Enhanced array-based storage for locality
private:
    std::vector<ArrayNode> array_nodes_;
    int array_root_index_;
    int operations_since_balance_;
    bool use_array_storage_;
    static const int LAZY_BALANCE_THRESHOLD = 100;

public:
    // Enable array-based storage for better locality
    void enable_array_storage() {
        if (!use_array_storage_) {
            convert_to_array_storage();
            use_array_storage_ = true;
        }
    }
    
    // Lazy rebalancing for locality optimization
    void rebalance_for_locality() {
        if (!use_array_storage_ || array_nodes_.empty()) return;
        
        std::vector<ArrayNode> new_nodes;
        std::vector<int> old_to_new(array_nodes_.size(), -1);
        std::queue<int> queue;
        
        // Breadth-first reordering for better cache locality
        queue.push(array_root_index_);
        old_to_new[array_root_index_] = 0;
        new_nodes.push_back(array_nodes_[array_root_index_]);
        new_nodes[0].parent_index = -1;
        new_nodes[0].first_child_index = -1;
        new_nodes[0].child_count = 0;
        
        int new_index = 1;
        
        while (!queue.empty()) {
            int current_old = queue.front();
            queue.pop();
            int current_new = old_to_new[current_old];
            
            // Find all children - preserves N-ary structure
            std::vector<int> children;
            for (int i = 0; i < array_nodes_.size(); ++i) {
                if (array_nodes_[i].is_valid && array_nodes_[i].parent_index == current_old) {
                    children.push_back(i);
                }
            }
            
            if (!children.empty()) {
                new_nodes[current_new].first_child_index = new_index;
                new_nodes[current_new].child_count = children.size();
                
                // Add all N children consecutively for locality
                for (int child_old : children) {
                    old_to_new[child_old] = new_index;
                    new_nodes.push_back(array_nodes_[child_old]);
                    new_nodes[new_index].parent_index = current_new;
                    queue.push(child_old);
                    new_index++;
                }
            }
        }
        
        array_nodes_ = std::move(new_nodes);
        array_root_index_ = 0;
        operations_since_balance_ = 0;
    }
    
    // Locality analysis
    double calculate_locality_score() const {
        if (!use_array_storage_ || array_nodes_.empty()) return 0.5;
        
        double score = 0.0;
        int comparisons = 0;
        
        for (int i = 0; i < array_nodes_.size(); ++i) {
            if (array_nodes_[i].is_valid && array_nodes_[i].child_count > 0) {
                int first_child = array_nodes_[i].first_child_index;
                
                // Better score when children are close to parent
                double distance = std::abs(first_child - i);
                score += 1.0 / (1.0 + distance / 10.0);
                comparisons++;
                
                // Better score when children are consecutive
                for (int j = 1; j < array_nodes_[i].child_count; ++j) {
                    if (first_child + j < array_nodes_.size() && array_nodes_[first_child + j].is_valid) {
                        score += 1.0; // Consecutive children
                    } else {
                        score += 0.5; // Gap in children
                    }
                    comparisons++;
                }
            }
        }
        
        return comparisons > 0 ? score / comparisons : 1.0;
    }

private:
    void convert_to_array_storage() {
        if (!root_) return;
        
        array_nodes_.clear();
        array_root_index_ = 0;
        operations_since_balance_ = 0;
        
        // Convert pointer-based tree to array-based
        std::queue<std::pair<const Node*, int>> queue;
        queue.push({root_.get(), -1});
        
        while (!queue.empty()) {
            auto [node, parent_idx] = queue.front();
            queue.pop();
            
            int current_idx = array_nodes_.size();
            array_nodes_.push_back(ArrayNode(node->data(), parent_idx));
            
            if (parent_idx == -1) {
                array_root_index_ = current_idx;
            } else {
                // Update parent's child info
                if (array_nodes_[parent_idx].child_count == 0) {
                    array_nodes_[parent_idx].first_child_index = current_idx;
                }
                array_nodes_[parent_idx].child_count++;
            }
            
            // Add children to queue
            for (size_t i = 0; i < node->child_count(); ++i) {
                queue.push({&node->child(i), current_idx});
            }
        }
        
        // Immediately rebalance for optimal locality
        rebalance_for_locality();
    }
    
private:
    // Helper for decoding direct preorder encoding
    static std::unique_ptr<Node> decode_succinct_preorder(
        const std::vector<bool>& structure, const std::vector<T>& data,
        size_t& bit_index, size_t& data_index) {
        
        if (bit_index >= structure.size() || !structure[bit_index]) {
            return nullptr; // End of children marker or end of data
        }
        
        // Skip the '1' bit for internal node
        bit_index++;
        
        if (data_index >= data.size()) return nullptr;
        
        auto node = std::make_unique<Node>(data[data_index++]);
        
        // Read children until we hit a '0' (end of children marker)
        while (bit_index < structure.size() && structure[bit_index]) {
            auto child = decode_succinct_preorder(structure, data, bit_index, data_index);
            if (child) {
                child->parent_ = node.get();
                node->children_.push_back(std::move(child));
            }
        }
        
        // Skip the '0' end-of-children marker
        if (bit_index < structure.size()) {
            bit_index++;
        }
        
        return node;
    }

public:
};