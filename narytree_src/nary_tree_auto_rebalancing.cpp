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

template <typename T>
class NaryTreeAutoRebalancing {
public:
    class Node {
        friend class NaryTreeAutoRebalancing<T>;
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
        
        // Child management with automatic rebalancing
        Node& add_child(T child_data) {
            auto child = std::make_unique<Node>(std::move(child_data), this);
            Node* child_ptr = child.get();
            children_.push_back(std::move(child));
            
            // Trigger rebalancing at tree level after modification
            // This will be handled by the tree's add_child method
            
            return *child_ptr;
        }
        
        void add_child(std::unique_ptr<Node> child) {
            if (child) {
                child->parent_ = this;
                children_.push_back(std::move(child));
                // Rebalancing will be triggered at tree level
            }
        }
        
        bool remove_child(const Node* child) {
            auto it = std::find_if(children_.begin(), children_.end(),
                [child](const std::unique_ptr<Node>& ptr) { return ptr.get() == child; });
            
            if (it != children_.end()) {
                children_.erase(it);
                // Rebalancing will be triggered at tree level
                return true;
            }
            return false;
        }
        
        void remove_all_children() { 
            children_.clear(); 
            // Rebalancing will be triggered at tree level
        }
        
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
            size_t count = 1;
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
    size_t max_children_per_node_;
    size_t rebalance_operations_count_;
    bool auto_rebalancing_enabled_;
    
    // Rebalancing threshold - rebalance after every N operations
    static constexpr size_t REBALANCE_OPERATION_THRESHOLD = 10;

public:
    // Constructors
    explicit NaryTreeAutoRebalancing(T root_data, size_t max_children = 3) 
        : root_(std::make_unique<Node>(std::move(root_data))), 
          size_(1), 
          max_children_per_node_(max_children),
          rebalance_operations_count_(0),
          auto_rebalancing_enabled_(true) {}
    
    NaryTreeAutoRebalancing(size_t max_children = 3) 
        : root_(nullptr), 
          size_(0), 
          max_children_per_node_(max_children),
          rebalance_operations_count_(0),
          auto_rebalancing_enabled_(true) {}
    
    // Move semantics
    NaryTreeAutoRebalancing(NaryTreeAutoRebalancing&&) = default;
    NaryTreeAutoRebalancing& operator=(NaryTreeAutoRebalancing&&) = default;
    
    // Delete copy semantics
    NaryTreeAutoRebalancing(const NaryTreeAutoRebalancing&) = delete;
    NaryTreeAutoRebalancing& operator=(const NaryTreeAutoRebalancing&) = delete;
    
    // Auto-rebalancing control
    void enable_auto_rebalancing() { auto_rebalancing_enabled_ = true; }
    void disable_auto_rebalancing() { auto_rebalancing_enabled_ = false; }
    bool is_auto_rebalancing_enabled() const { return auto_rebalancing_enabled_; }
    
    void set_max_children(size_t max_children) { max_children_per_node_ = max_children; }
    size_t get_max_children() const { return max_children_per_node_; }
    
    size_t get_rebalance_operations_count() const { return rebalance_operations_count_; }
    
    // Tree operations with automatic rebalancing
    bool empty() const noexcept { return root_ == nullptr; }
    size_t size() const noexcept { return size_; }
    
    void set_root(T root_data) {
        root_ = std::make_unique<Node>(std::move(root_data));
        size_ = 1;
        trigger_rebalancing_check();
    }
    
    Node* root() noexcept { return root_.get(); }
    const Node* root() const noexcept { return root_.get(); }
    
    void clear() {
        root_.reset();
        size_ = 0;
        rebalance_operations_count_ = 0;
    }
    
    size_t depth() const {
        return root_ ? root_->depth() : 0;
    }
    
    // Enhanced add_child with automatic rebalancing
    Node* add_child_to_node(Node* parent_node, T child_data) {
        if (!parent_node) return nullptr;
        
        auto& new_child = parent_node->add_child(std::move(child_data));
        ++size_;
        
        trigger_rebalancing_check();
        
        // Return pointer may be invalid after rebalancing, so find it again
        return find_node_by_data(new_child.data());
    }
    
    // Enhanced remove operations with automatic rebalancing
    bool remove_child_from_node(Node* parent_node, const Node* child_to_remove) {
        if (!parent_node || !child_to_remove) return false;
        
        bool removed = parent_node->remove_child(child_to_remove);
        if (removed) {
            --size_;
            trigger_rebalancing_check();
        }
        
        return removed;
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
    
    // Helper to find node by data value
    Node* find_node_by_data(const T& data) {
        return find([&data](const Node& node) { return node.data() == data; });
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
        size_t total_rebalance_operations = 0;
    };
    
    TreeStats get_statistics() const {
        TreeStats stats;
        stats.total_rebalance_operations = rebalance_operations_count_;
        
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
            stats.min_children = 0;
        }
        
        return stats;
    }
    
    // Self-balancing functionality (from original implementation)
    std::vector<T> collect_all_data() const {
        std::vector<T> data;
        if (!root_) return data;
        
        data.reserve(size_);
        
        std::queue<const Node*> queue;
        queue.push(root_.get());
        
        while (!queue.empty()) {
            const Node* current = queue.front();
            queue.pop();
            
            data.push_back(current->data());
            
            for (size_t i = 0; i < current->child_count(); ++i) {
                queue.push(&current->child(i));
            }
        }
        
        return data;
    }
    
    std::unique_ptr<Node> build_balanced_subtree(
        const std::vector<T>& data, 
        size_t start, 
        size_t end, 
        size_t max_children_per_node) const {
        
        if (start >= end) return nullptr;
        
        auto node = std::make_unique<Node>(data[start]);
        
        if (end - start == 1) return node;
        
        size_t remaining = end - start - 1;
        size_t children_count = std::min(remaining, max_children_per_node);
        
        if (children_count == 0) return node;
        
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
    
    void balance_tree() {
        if (!root_ || size_ <= 1) return;
        
        auto data = collect_all_data();
        root_ = build_balanced_subtree(data, 0, data.size(), max_children_per_node_);
        ++rebalance_operations_count_;
    }
    
    bool needs_rebalancing() const {
        if (!root_ || size_ <= 3) return false;
        
        auto stats = get_statistics();
        
        // More aggressive rebalancing for auto-rebalancing version
        size_t optimal_depth = static_cast<size_t>(std::log(size_) / std::log(max_children_per_node_)) + 1;
        
        // Rebalance if tree is 1.5x deeper than optimal (more aggressive than original 2x)
        return stats.max_depth > optimal_depth * 3 / 2;
    }
    
    // Automatic rebalancing trigger
    void trigger_rebalancing_check() {
        if (!auto_rebalancing_enabled_) return;
        
        // Strategy 1: Check depth every REBALANCE_OPERATION_THRESHOLD operations
        if (size_ > 3 && (size_ % REBALANCE_OPERATION_THRESHOLD == 0)) {
            if (needs_rebalancing()) {
                balance_tree();
            }
        }
        
        // Strategy 2: Force rebalancing for very unbalanced trees
        if (size_ > 10) {
            auto stats = get_statistics();
            size_t optimal_depth = static_cast<size_t>(std::log(size_) / std::log(max_children_per_node_)) + 1;
            
            // Force rebalancing if tree is extremely unbalanced (2x optimal depth)
            if (stats.max_depth > optimal_depth * 2) {
                balance_tree();
            }
        }
    }
    
    // Memory usage estimation with rebalancing overhead tracking
    struct MemoryStats {
        size_t node_memory_bytes;
        size_t data_memory_estimate;
        size_t rebalancing_overhead_bytes;
        size_t total_estimated_bytes;
        double memory_per_node;
        size_t rebalance_operations;
    };
    
    MemoryStats get_memory_stats() const {
        MemoryStats stats;
        
        stats.node_memory_bytes = size_ * sizeof(Node);
        stats.data_memory_estimate = size_ * sizeof(T);
        
        // Additional overhead for auto-rebalancing metadata
        stats.rebalancing_overhead_bytes = size_ * sizeof(size_t) + // operation counters
                                          sizeof(max_children_per_node_) +
                                          sizeof(rebalance_operations_count_) +
                                          sizeof(auto_rebalancing_enabled_);
        
        stats.total_estimated_bytes = stats.node_memory_bytes + 
                                     stats.data_memory_estimate + 
                                     stats.rebalancing_overhead_bytes;
        
        stats.memory_per_node = size_ > 0 ? static_cast<double>(stats.total_estimated_bytes) / size_ : 0.0;
        stats.rebalance_operations = rebalance_operations_count_;
        
        return stats;
    }
};