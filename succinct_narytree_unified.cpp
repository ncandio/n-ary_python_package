#pragma once
#include <Python.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>

struct LocalityStatistics {
    double locality_score;
    double compression_ratio;
    size_t memory_usage_bytes;
};

class SuccinctNaryTree {
private:
    std::vector<PyObject*> node_data;
    std::vector<std::vector<size_t>> children;
    size_t next_id;
    static const size_t LAZY_BALANCE_THRESHOLD = 100;
    size_t operations_since_balance;
    
    void balance_if_needed() {
        operations_since_balance++;
        if (operations_since_balance >= LAZY_BALANCE_THRESHOLD) {
            rebalance_for_locality();
            operations_since_balance = 0;
        }
    }
    
    void release_python_objects() {
        for (auto* obj : node_data) {
            if (obj) {
                Py_DECREF(obj);
            }
        }
    }

public:
    SuccinctNaryTree() : next_id(0), operations_since_balance(0) {}
    
    ~SuccinctNaryTree() {
        clear();
    }
    
    // Copy constructor
    SuccinctNaryTree(const SuccinctNaryTree& other) = delete;
    SuccinctNaryTree& operator=(const SuccinctNaryTree& other) = delete;
    
    bool empty() const {
        return node_data.empty();
    }
    
    size_t size() const {
        return node_data.size();
    }
    
    void clear() {
        release_python_objects();
        node_data.clear();
        children.clear();
        next_id = 0;
        operations_since_balance = 0;
    }
    
    void set_root(PyObject* data) {
        if (empty()) {
            node_data.push_back(data);
            children.push_back(std::vector<size_t>());
            next_id = 1;
        } else {
            if (node_data[0]) {
                Py_DECREF(node_data[0]);
            }
            node_data[0] = data;
        }
        balance_if_needed();
    }
    
    size_t add_child(size_t parent_id, PyObject* data) {
        if (parent_id >= node_data.size()) {
            throw std::runtime_error("Invalid parent node ID");
        }
        
        size_t child_id = next_id++;
        node_data.push_back(data);
        children.push_back(std::vector<size_t>());
        children[parent_id].push_back(child_id);
        
        balance_if_needed();
        return child_id;
    }
    
    PyObject* get_data(size_t node_id) const {
        if (node_id >= node_data.size()) {
            throw std::runtime_error("Invalid node ID");
        }
        return node_data[node_id];
    }
    
    void set_data(size_t node_id, PyObject* data) {
        if (node_id >= node_data.size()) {
            throw std::runtime_error("Invalid node ID");
        }
        if (node_data[node_id]) {
            Py_DECREF(node_data[node_id]);
        }
        node_data[node_id] = data;
        balance_if_needed();
    }
    
    size_t child_count(size_t node_id) const {
        if (node_id >= children.size()) {
            throw std::runtime_error("Invalid node ID");
        }
        return children[node_id].size();
    }
    
    bool is_leaf(size_t node_id) const {
        return child_count(node_id) == 0;
    }
    
    LocalityStatistics get_locality_statistics() const {
        LocalityStatistics stats;
        
        // Calculate memory usage
        size_t total_memory = node_data.size() * sizeof(PyObject*);
        for (const auto& child_vec : children) {
            total_memory += child_vec.capacity() * sizeof(size_t);
        }
        stats.memory_usage_bytes = total_memory;
        
        // Calculate compression ratio (simplified)
        size_t uncompressed_size = node_data.size() * (sizeof(PyObject*) + sizeof(void*) * 4);
        stats.compression_ratio = static_cast<double>(total_memory) / uncompressed_size;
        
        // Calculate locality score (simplified - based on memory layout)
        stats.locality_score = std::max(0.0, 1.0 - stats.compression_ratio);
        
        return stats;
    }
    
    void rebalance_for_locality() {
        // Simple rebalancing - ensure data is laid out for cache efficiency
        // In a full implementation, this would reorganize the internal structure
        operations_since_balance = 0;
        
        // For now, just compact the vectors to remove excess capacity
        for (auto& child_vec : children) {
            child_vec.shrink_to_fit();
        }
    }
};