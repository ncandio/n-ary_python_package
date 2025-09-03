#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

// Include our enhanced C++ implementation
#include "nary_tree.cpp"

// C++ class wrapper
class NaryTreeWrapper {
public:
    NaryTree<PyObject*> tree;
    
    NaryTreeWrapper() {}
    explicit NaryTreeWrapper(PyObject* root_data) : tree(root_data) {
        Py_INCREF(root_data);
    }
    
    ~NaryTreeWrapper() {
        // Clean up all PyObject references
        if (!tree.empty()) {
            tree.for_each([](const auto& node) {
                Py_DECREF(node.data());
            });
        }
    }
};

extern "C" {

// Forward declaration - will be defined later
extern PyTypeObject NodeType;

// Python object structure for NaryTree
typedef struct {
    PyObject_HEAD
    NaryTreeWrapper* tree;
} NaryTreeObject;

// Python object structure for Node
typedef struct {
    PyObject_HEAD
    void* node_ptr;  // Pointer to the actual C++ Node
    NaryTreeObject* tree_obj;  // Reference to parent tree to prevent deallocation
} NodeObject;

// NaryTree methods
static PyObject* narytree_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NaryTreeObject* self = (NaryTreeObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->tree = NULL;
    }
    return (PyObject*)self;
}

static int narytree_init(NaryTreeObject* self, PyObject* args, PyObject* kwds) {
    PyObject* root_data = NULL;
    
    if (!PyArg_ParseTuple(args, "|O", &root_data)) {
        return -1;
    }
    
    try {
        if (root_data) {
            self->tree = new NaryTreeWrapper(root_data);
        } else {
            self->tree = new NaryTreeWrapper();
        }
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    }
    
    return 0;
}

static void narytree_dealloc(NaryTreeObject* self) {
    delete self->tree;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* narytree_set_root(NaryTreeObject* self, PyObject* args) {
    PyObject* root_data;
    if (!PyArg_ParseTuple(args, "O", &root_data)) {
        return NULL;
    }
    
    try {
        Py_INCREF(root_data);
        self->tree->tree.set_root(root_data);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject* narytree_empty(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyBool_FromLong(self->tree->tree.empty());
}

static PyObject* narytree_size(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyLong_FromSize_t(self->tree->tree.size());
}

static PyObject* narytree_depth(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyLong_FromSize_t(self->tree->tree.depth());
}

static PyObject* narytree_clear(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    // Clean up PyObject references before clearing
    if (!self->tree->tree.empty()) {
        self->tree->tree.for_each([](const auto& node) {
            Py_DECREF(node.data());
        });
    }
    self->tree->tree.clear();
    Py_RETURN_NONE;
}

static PyObject* narytree_root(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    auto* root_node = self->tree->tree.root();
    if (!root_node) {
        Py_RETURN_NONE;
    }
    
    NodeObject* node_obj = (NodeObject*)NodeType.tp_alloc(&NodeType, 0);
    if (!node_obj) {
        return NULL;
    }
    
    node_obj->node_ptr = root_node;
    node_obj->tree_obj = self;
    Py_INCREF(self);
    
    return (PyObject*)node_obj;
}

static PyObject* narytree_statistics(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    auto stats = self->tree->tree.get_statistics();
    
    PyObject* dict = PyDict_New();
    if (!dict) return NULL;
    
    PyDict_SetItemString(dict, "total_nodes", PyLong_FromSize_t(stats.total_nodes));
    PyDict_SetItemString(dict, "leaf_nodes", PyLong_FromSize_t(stats.leaf_nodes));
    PyDict_SetItemString(dict, "internal_nodes", PyLong_FromSize_t(stats.internal_nodes));
    PyDict_SetItemString(dict, "max_depth", PyLong_FromSize_t(stats.max_depth));
    PyDict_SetItemString(dict, "avg_children_per_node", PyFloat_FromDouble(stats.avg_children_per_node));
    PyDict_SetItemString(dict, "max_children", PyLong_FromSize_t(stats.max_children));
    PyDict_SetItemString(dict, "min_children", PyLong_FromSize_t(stats.min_children));
    
    return dict;
}

// Balancing methods
static PyObject* narytree_balance_tree(NaryTreeObject* self, PyObject* args) {
    size_t max_children_per_node = 3;  // Default branching factor
    
    if (!PyArg_ParseTuple(args, "|n", &max_children_per_node)) {
        return NULL;
    }
    
    try {
        self->tree->tree.balance_tree(max_children_per_node);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject* narytree_needs_rebalancing(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyBool_FromLong(self->tree->tree.needs_rebalancing());
}

static PyObject* narytree_auto_balance_if_needed(NaryTreeObject* self, PyObject* args) {
    size_t max_children_per_node = 3;
    
    if (!PyArg_ParseTuple(args, "|n", &max_children_per_node)) {
        return NULL;
    }
    
    try {
        self->tree->tree.auto_balance_if_needed(max_children_per_node);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject* narytree_get_memory_stats(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    auto mem_stats = self->tree->tree.get_memory_stats();
    
    PyObject* dict = PyDict_New();
    if (!dict) return NULL;
    
    PyDict_SetItemString(dict, "node_memory_bytes", PyLong_FromSize_t(mem_stats.node_memory_bytes));
    PyDict_SetItemString(dict, "data_memory_estimate", PyLong_FromSize_t(mem_stats.data_memory_estimate));
    PyDict_SetItemString(dict, "total_estimated_bytes", PyLong_FromSize_t(mem_stats.total_estimated_bytes));
    PyDict_SetItemString(dict, "memory_per_node", PyFloat_FromDouble(mem_stats.memory_per_node));
    
    return dict;
}

static PyObject* narytree_encode_succinct(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    try {
        auto encoding = self->tree->tree.encode_succinct();
        
        PyObject* dict = PyDict_New();
        if (!dict) return NULL;
        
        // Convert bit vector to bytes
        PyObject* structure_bytes = PyBytes_FromStringAndSize(nullptr, (encoding.structure_bits.size() + 7) / 8);
        if (!structure_bytes) {
            Py_DECREF(dict);
            return NULL;
        }
        
        char* byte_data = PyBytes_AsString(structure_bytes);
        for (size_t i = 0; i < encoding.structure_bits.size(); ++i) {
            if (encoding.structure_bits[i]) {
                byte_data[i / 8] |= (1 << (i % 8));
            }
        }
        
        // Convert data array to Python list
        PyObject* data_list = PyList_New(encoding.data_array.size());
        if (!data_list) {
            Py_DECREF(structure_bytes);
            Py_DECREF(dict);
            return NULL;
        }
        
        for (size_t i = 0; i < encoding.data_array.size(); ++i) {
            PyObject* data_item = encoding.data_array[i];
            Py_INCREF(data_item);
            PyList_SetItem(data_list, i, data_item);
        }
        
        PyDict_SetItemString(dict, "structure_bits", structure_bytes);
        PyDict_SetItemString(dict, "data_array", data_list);
        PyDict_SetItemString(dict, "node_count", PyLong_FromSize_t(encoding.node_count));
        PyDict_SetItemString(dict, "memory_usage", PyLong_FromSize_t(encoding.memory_usage()));
        
        Py_DECREF(structure_bytes);
        Py_DECREF(data_list);
        
        return dict;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* narytree_decode_succinct(PyTypeObject* type, PyObject* args) {
    PyObject* encoding_dict;
    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &encoding_dict)) {
        return NULL;
    }
    
    try {
        // Extract structure bits
        PyObject* structure_bytes = PyDict_GetItemString(encoding_dict, "structure_bits");
        PyObject* data_list = PyDict_GetItemString(encoding_dict, "data_array");
        PyObject* node_count_obj = PyDict_GetItemString(encoding_dict, "node_count");
        
        if (!structure_bytes || !data_list || !node_count_obj) {
            PyErr_SetString(PyExc_ValueError, "Invalid succinct encoding format");
            return NULL;
        }
        
        // Convert bytes back to bit vector
        Py_ssize_t bytes_size = PyBytes_Size(structure_bytes);
        const char* byte_data = PyBytes_AsString(structure_bytes);
        std::vector<bool> structure_bits;
        
        for (Py_ssize_t i = 0; i < bytes_size * 8; ++i) {
            structure_bits.push_back((byte_data[i / 8] & (1 << (i % 8))) != 0);
        }
        
        // Convert Python list to data vector
        Py_ssize_t list_size = PyList_Size(data_list);
        std::vector<PyObject*> data_array;
        data_array.reserve(list_size);
        
        for (Py_ssize_t i = 0; i < list_size; ++i) {
            PyObject* item = PyList_GetItem(data_list, i);
            Py_INCREF(item);
            data_array.push_back(item);
        }
        
        size_t node_count = PyLong_AsSize_t(node_count_obj);
        
        // Create encoding struct
        NaryTree<PyObject*>::SuccinctEncoding encoding_struct;
        encoding_struct.structure_bits = std::move(structure_bits);
        encoding_struct.data_array = std::move(data_array);
        encoding_struct.node_count = node_count;
        
        // Decode tree
        auto decoded_tree = NaryTree<PyObject*>::decode_succinct(encoding_struct);
        
        // Create Python object
        NaryTreeObject* tree_obj = (NaryTreeObject*)type->tp_alloc(type, 0);
        if (!tree_obj) {
            return NULL;
        }
        
        tree_obj->tree = new NaryTreeWrapper();
        tree_obj->tree->tree = std::move(decoded_tree);
        
        return (PyObject*)tree_obj;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* narytree_enable_array_storage(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    try {
        self->tree->tree.enable_array_storage();
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* narytree_calculate_locality_score(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    try {
        double score = self->tree->tree.calculate_locality_score();
        return PyFloat_FromDouble(score);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* narytree_rebalance_for_locality(NaryTreeObject* self, PyObject* Py_UNUSED(ignored)) {
    try {
        self->tree->tree.rebalance_for_locality();
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    Py_RETURN_NONE;
}

// Node methods
static PyObject* node_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NodeObject* self = (NodeObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->node_ptr = NULL;
        self->tree_obj = NULL;
    }
    return (PyObject*)self;
}

static void node_dealloc(NodeObject* self) {
    Py_XDECREF(self->tree_obj);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* node_data(NodeObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    PyObject* data = node->data();
    Py_INCREF(data);
    return data;
}

static PyObject* node_set_data(NodeObject* self, PyObject* args) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    PyObject* new_data;
    if (!PyArg_ParseTuple(args, "O", &new_data)) {
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    PyObject* old_data = node->data();
    
    Py_INCREF(new_data);
    node->set_data(new_data);
    Py_DECREF(old_data);
    
    Py_RETURN_NONE;
}

static PyObject* node_add_child(NodeObject* self, PyObject* args) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    PyObject* child_data;
    if (!PyArg_ParseTuple(args, "O", &child_data)) {
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    
    try {
        Py_INCREF(child_data);
        auto& child_node = node->add_child(child_data);
        
        NodeObject* child_obj = (NodeObject*)NodeType.tp_alloc(&NodeType, 0);
        if (!child_obj) {
            return NULL;
        }
        
        child_obj->node_ptr = &child_node;
        child_obj->tree_obj = self->tree_obj;
        Py_INCREF(self->tree_obj);
        
        return (PyObject*)child_obj;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* node_child_count(NodeObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    return PyLong_FromSize_t(node->child_count());
}

static PyObject* node_is_leaf(NodeObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    return PyBool_FromLong(node->is_leaf());
}

static PyObject* node_child(NodeObject* self, PyObject* args) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "n", &index)) {
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    
    try {
        auto& child_node = node->child(static_cast<size_t>(index));
        
        NodeObject* child_obj = (NodeObject*)NodeType.tp_alloc(&NodeType, 0);
        if (!child_obj) {
            return NULL;
        }
        
        child_obj->node_ptr = &child_node;
        child_obj->tree_obj = self->tree_obj;
        Py_INCREF(self->tree_obj);
        
        return (PyObject*)child_obj;
    } catch (const std::out_of_range& e) {
        PyErr_SetString(PyExc_IndexError, e.what());
        return NULL;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* node_depth(NodeObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    return PyLong_FromSize_t(node->depth());
}

static PyObject* node_height_from_root(NodeObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTree<PyObject*>::Node*>(self->node_ptr);
    return PyLong_FromSize_t(node->height_from_root());
}

// Method definitions
static PyMethodDef narytree_methods[] = {
    {"set_root", (PyCFunction)narytree_set_root, METH_VARARGS, "Set the root node data"},
    {"empty", (PyCFunction)narytree_empty, METH_NOARGS, "Check if tree is empty"},
    {"size", (PyCFunction)narytree_size, METH_NOARGS, "Get tree size"},
    {"depth", (PyCFunction)narytree_depth, METH_NOARGS, "Get tree depth"},
    {"clear", (PyCFunction)narytree_clear, METH_NOARGS, "Clear the tree"},
    {"root", (PyCFunction)narytree_root, METH_NOARGS, "Get root node"},
    {"statistics", (PyCFunction)narytree_statistics, METH_NOARGS, "Get tree statistics"},
    {"balance_tree", (PyCFunction)narytree_balance_tree, METH_VARARGS, "Balance the tree with optional branching factor"},
    {"needs_rebalancing", (PyCFunction)narytree_needs_rebalancing, METH_NOARGS, "Check if tree needs rebalancing"},
    {"auto_balance_if_needed", (PyCFunction)narytree_auto_balance_if_needed, METH_VARARGS, "Automatically balance if needed"},
    {"get_memory_stats", (PyCFunction)narytree_get_memory_stats, METH_NOARGS, "Get memory usage statistics"},
    {"encode_succinct", (PyCFunction)narytree_encode_succinct, METH_NOARGS, "Encode tree as succinct data structure"},
    {"decode_succinct", (PyCFunction)narytree_decode_succinct, METH_VARARGS | METH_CLASS, "Decode succinct representation to tree"},
    {"enable_array_storage", (PyCFunction)narytree_enable_array_storage, METH_NOARGS, "Convert to array-based storage for better locality"},
    {"calculate_locality_score", (PyCFunction)narytree_calculate_locality_score, METH_NOARGS, "Calculate memory locality effectiveness score"},
    {"rebalance_for_locality", (PyCFunction)narytree_rebalance_for_locality, METH_NOARGS, "Rebalance array storage for optimal cache locality"},
    {NULL}
};

static PyMethodDef node_methods[] = {
    {"data", (PyCFunction)node_data, METH_NOARGS, "Get node data"},
    {"set_data", (PyCFunction)node_set_data, METH_VARARGS, "Set node data"},
    {"add_child", (PyCFunction)node_add_child, METH_VARARGS, "Add child node"},
    {"child_count", (PyCFunction)node_child_count, METH_NOARGS, "Get number of children"},
    {"is_leaf", (PyCFunction)node_is_leaf, METH_NOARGS, "Check if node is leaf"},
    {"child", (PyCFunction)node_child, METH_VARARGS, "Get child by index"},
    {"depth", (PyCFunction)node_depth, METH_NOARGS, "Get node depth"},
    {"height_from_root", (PyCFunction)node_height_from_root, METH_NOARGS, "Get height from root"},
    {NULL}
};

// Type definitions
static PyTypeObject NaryTreeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree.NaryTree",
    .tp_basicsize = sizeof(NaryTreeObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)narytree_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "N-ary tree data structure",
    .tp_methods = narytree_methods,
    .tp_init = (initproc)narytree_init,
    .tp_new = narytree_new,
};

// Define the NodeType (complete the forward declaration)
PyTypeObject NodeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree.Node",
    .tp_basicsize = sizeof(NodeObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)node_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "N-ary tree node",
    .tp_methods = node_methods,
    .tp_new = node_new,
};

// Module definition
static PyModuleDef narytreemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "narytree",
    .m_doc = "N-ary tree data structure module",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit_narytree(void) {
    PyObject* m;
    
    if (PyType_Ready(&NaryTreeType) < 0) {
        return NULL;
    }
    
    if (PyType_Ready(&NodeType) < 0) {
        return NULL;
    }
    
    m = PyModule_Create(&narytreemodule);
    if (m == NULL) {
        return NULL;
    }
    
    Py_INCREF(&NaryTreeType);
    if (PyModule_AddObject(m, "NaryTree", (PyObject*)&NaryTreeType) < 0) {
        Py_DECREF(&NaryTreeType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&NodeType);
    if (PyModule_AddObject(m, "Node", (PyObject*)&NodeType) < 0) {
        Py_DECREF(&NodeType);
        Py_DECREF(&NaryTreeType);
        Py_DECREF(m);
        return NULL;
    }
    
    return m;
}

} // extern "C"