#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

// Include our enhanced auto-rebalancing C++ implementation
#include "nary_tree_auto_rebalancing.cpp"

// C++ class wrapper
class NaryTreeAutoWrapper {
public:
    NaryTreeAutoRebalancing<PyObject*> tree;
    
    NaryTreeAutoWrapper(size_t max_children = 3) : tree(max_children) {}
    explicit NaryTreeAutoWrapper(PyObject* root_data, size_t max_children = 3) : tree(root_data, max_children) {
        Py_INCREF(root_data);
    }
    
    ~NaryTreeAutoWrapper() {
        // Clean up all PyObject references
        if (!tree.empty()) {
            tree.for_each([](const auto& node) {
                Py_DECREF(node.data());
            });
        }
    }
};

extern "C" {

// Python object structure for Auto-rebalancing NaryTree
typedef struct {
    PyObject_HEAD
    NaryTreeAutoWrapper* tree;
} NaryTreeAutoObject;

// Python object structure for Node (same as before)
typedef struct {
    PyObject_HEAD
    void* node_ptr;
    NaryTreeAutoObject* tree_obj;
} NodeAutoObject;

// Forward declarations
static PyTypeObject NodeAutoType;

// NaryTreeAuto methods
static PyObject* narytree_auto_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NaryTreeAutoObject* self = (NaryTreeAutoObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->tree = NULL;
    }
    return (PyObject*)self;
}

static int narytree_auto_init(NaryTreeAutoObject* self, PyObject* args, PyObject* kwds) {
    PyObject* root_data = NULL;
    size_t max_children = 3;
    
    static char* kwlist[] = {"root_data", "max_children", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|On", kwlist, &root_data, &max_children)) {
        return -1;
    }
    
    try {
        if (root_data) {
            self->tree = new NaryTreeAutoWrapper(root_data, max_children);
        } else {
            self->tree = new NaryTreeAutoWrapper(max_children);
        }
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    }
    
    return 0;
}

static void narytree_auto_dealloc(NaryTreeAutoObject* self) {
    delete self->tree;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* narytree_auto_set_root(NaryTreeAutoObject* self, PyObject* args) {
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

static PyObject* narytree_auto_empty(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyBool_FromLong(self->tree->tree.empty());
}

static PyObject* narytree_auto_size(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyLong_FromSize_t(self->tree->tree.size());
}

static PyObject* narytree_auto_depth(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyLong_FromSize_t(self->tree->tree.depth());
}

static PyObject* narytree_auto_clear(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    // Clean up PyObject references before clearing
    if (!self->tree->tree.empty()) {
        self->tree->tree.for_each([](const auto& node) {
            Py_DECREF(node.data());
        });
    }
    self->tree->tree.clear();
    Py_RETURN_NONE;
}

static PyObject* narytree_auto_root(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    auto* root_node = self->tree->tree.root();
    if (!root_node) {
        Py_RETURN_NONE;
    }
    
    NodeAutoObject* node_obj = (NodeAutoObject*)NodeAutoType.tp_alloc(&NodeAutoType, 0);
    if (!node_obj) {
        return NULL;
    }
    
    node_obj->node_ptr = root_node;
    node_obj->tree_obj = self;
    Py_INCREF(self);
    
    return (PyObject*)node_obj;
}

// Enhanced add_child method with auto-rebalancing
static PyObject* narytree_auto_add_child_to_node(NaryTreeAutoObject* self, PyObject* args) {
    PyObject* parent_node_obj;
    PyObject* child_data;
    
    if (!PyArg_ParseTuple(args, "OO", &parent_node_obj, &child_data)) {
        return NULL;
    }
    
    // Cast to NodeAutoObject to get the node pointer
    NodeAutoObject* parent_obj = (NodeAutoObject*)parent_node_obj;
    auto* parent_node = static_cast<NaryTreeAutoRebalancing<PyObject*>::Node*>(parent_obj->node_ptr);
    
    if (!parent_node) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid parent node");
        return NULL;
    }
    
    try {
        Py_INCREF(child_data);
        auto* new_child = self->tree->tree.add_child_to_node(parent_node, child_data);
        
        if (!new_child) {
            Py_DECREF(child_data);
            PyErr_SetString(PyExc_RuntimeError, "Failed to add child");
            return NULL;
        }
        
        NodeAutoObject* child_obj = (NodeAutoObject*)NodeAutoType.tp_alloc(&NodeAutoType, 0);
        if (!child_obj) {
            return NULL;
        }
        
        child_obj->node_ptr = new_child;
        child_obj->tree_obj = self;
        Py_INCREF(self);
        
        return (PyObject*)child_obj;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* narytree_auto_statistics(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
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
    PyDict_SetItemString(dict, "total_rebalance_operations", PyLong_FromSize_t(stats.total_rebalance_operations));
    
    return dict;
}

// Auto-rebalancing control methods
static PyObject* narytree_auto_enable_auto_rebalancing(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    self->tree->tree.enable_auto_rebalancing();
    Py_RETURN_NONE;
}

static PyObject* narytree_auto_disable_auto_rebalancing(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    self->tree->tree.disable_auto_rebalancing();
    Py_RETURN_NONE;
}

static PyObject* narytree_auto_is_auto_rebalancing_enabled(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyBool_FromLong(self->tree->tree.is_auto_rebalancing_enabled());
}

static PyObject* narytree_auto_get_rebalance_operations_count(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyLong_FromSize_t(self->tree->tree.get_rebalance_operations_count());
}

static PyObject* narytree_auto_set_max_children(NaryTreeAutoObject* self, PyObject* args) {
    size_t max_children;
    if (!PyArg_ParseTuple(args, "n", &max_children)) {
        return NULL;
    }
    
    self->tree->tree.set_max_children(max_children);
    Py_RETURN_NONE;
}

static PyObject* narytree_auto_get_max_children(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyLong_FromSize_t(self->tree->tree.get_max_children());
}

// Manual balancing methods (still available)
static PyObject* narytree_auto_balance_tree(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    try {
        self->tree->tree.balance_tree();
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject* narytree_auto_needs_rebalancing(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    return PyBool_FromLong(self->tree->tree.needs_rebalancing());
}

static PyObject* narytree_auto_get_memory_stats(NaryTreeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    auto mem_stats = self->tree->tree.get_memory_stats();
    
    PyObject* dict = PyDict_New();
    if (!dict) return NULL;
    
    PyDict_SetItemString(dict, "node_memory_bytes", PyLong_FromSize_t(mem_stats.node_memory_bytes));
    PyDict_SetItemString(dict, "data_memory_estimate", PyLong_FromSize_t(mem_stats.data_memory_estimate));
    PyDict_SetItemString(dict, "rebalancing_overhead_bytes", PyLong_FromSize_t(mem_stats.rebalancing_overhead_bytes));
    PyDict_SetItemString(dict, "total_estimated_bytes", PyLong_FromSize_t(mem_stats.total_estimated_bytes));
    PyDict_SetItemString(dict, "memory_per_node", PyFloat_FromDouble(mem_stats.memory_per_node));
    PyDict_SetItemString(dict, "rebalance_operations", PyLong_FromSize_t(mem_stats.rebalance_operations));
    
    return dict;
}

// Node methods (similar to original but adapted for auto-rebalancing)
static PyObject* node_auto_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    NodeAutoObject* self = (NodeAutoObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->node_ptr = NULL;
        self->tree_obj = NULL;
    }
    return (PyObject*)self;
}

static void node_auto_dealloc(NodeAutoObject* self) {
    Py_XDECREF(self->tree_obj);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* node_auto_data(NodeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTreeAutoRebalancing<PyObject*>::Node*>(self->node_ptr);
    PyObject* data = node->data();
    Py_INCREF(data);
    return data;
}

static PyObject* node_auto_add_child(NodeAutoObject* self, PyObject* args) {
    PyObject* child_data;
    if (!PyArg_ParseTuple(args, "O", &child_data)) {
        return NULL;
    }
    
    // Delegate to the tree's add_child_to_node method for auto-rebalancing
    PyObject* tree_add_args = PyTuple_Pack(2, (PyObject*)self, child_data);
    PyObject* result = narytree_auto_add_child_to_node(self->tree_obj, tree_add_args);
    Py_DECREF(tree_add_args);
    
    return result;
}

static PyObject* node_auto_child_count(NodeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTreeAutoRebalancing<PyObject*>::Node*>(self->node_ptr);
    return PyLong_FromSize_t(node->child_count());
}

static PyObject* node_auto_is_leaf(NodeAutoObject* self, PyObject* Py_UNUSED(ignored)) {
    if (!self->node_ptr) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    auto* node = static_cast<NaryTreeAutoRebalancing<PyObject*>::Node*>(self->node_ptr);
    return PyBool_FromLong(node->is_leaf());
}

// Method definitions
static PyMethodDef narytree_auto_methods[] = {
    {"set_root", (PyCFunction)narytree_auto_set_root, METH_VARARGS, "Set the root node data"},
    {"empty", (PyCFunction)narytree_auto_empty, METH_NOARGS, "Check if tree is empty"},
    {"size", (PyCFunction)narytree_auto_size, METH_NOARGS, "Get tree size"},
    {"depth", (PyCFunction)narytree_auto_depth, METH_NOARGS, "Get tree depth"},
    {"clear", (PyCFunction)narytree_auto_clear, METH_NOARGS, "Clear the tree"},
    {"root", (PyCFunction)narytree_auto_root, METH_NOARGS, "Get root node"},
    {"add_child_to_node", (PyCFunction)narytree_auto_add_child_to_node, METH_VARARGS, "Add child to specified node with auto-rebalancing"},
    {"statistics", (PyCFunction)narytree_auto_statistics, METH_NOARGS, "Get tree statistics including rebalancing info"},
    {"enable_auto_rebalancing", (PyCFunction)narytree_auto_enable_auto_rebalancing, METH_NOARGS, "Enable automatic rebalancing"},
    {"disable_auto_rebalancing", (PyCFunction)narytree_auto_disable_auto_rebalancing, METH_NOARGS, "Disable automatic rebalancing"},
    {"is_auto_rebalancing_enabled", (PyCFunction)narytree_auto_is_auto_rebalancing_enabled, METH_NOARGS, "Check if auto-rebalancing is enabled"},
    {"get_rebalance_operations_count", (PyCFunction)narytree_auto_get_rebalance_operations_count, METH_NOARGS, "Get number of rebalancing operations performed"},
    {"set_max_children", (PyCFunction)narytree_auto_set_max_children, METH_VARARGS, "Set maximum children per node"},
    {"get_max_children", (PyCFunction)narytree_auto_get_max_children, METH_NOARGS, "Get maximum children per node"},
    {"balance_tree", (PyCFunction)narytree_auto_balance_tree, METH_NOARGS, "Manually balance the tree"},
    {"needs_rebalancing", (PyCFunction)narytree_auto_needs_rebalancing, METH_NOARGS, "Check if tree needs rebalancing"},
    {"get_memory_stats", (PyCFunction)narytree_auto_get_memory_stats, METH_NOARGS, "Get memory usage statistics"},
    {NULL}
};

static PyMethodDef node_auto_methods[] = {
    {"data", (PyCFunction)node_auto_data, METH_NOARGS, "Get node data"},
    {"add_child", (PyCFunction)node_auto_add_child, METH_VARARGS, "Add child node with auto-rebalancing"},
    {"child_count", (PyCFunction)node_auto_child_count, METH_NOARGS, "Get number of children"},
    {"is_leaf", (PyCFunction)node_auto_is_leaf, METH_NOARGS, "Check if node is leaf"},
    {NULL}
};

// Type definitions
static PyTypeObject NaryTreeAutoType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree_auto.NaryTreeAuto",
    .tp_basicsize = sizeof(NaryTreeAutoObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)narytree_auto_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "N-ary tree data structure with automatic rebalancing",
    .tp_methods = narytree_auto_methods,
    .tp_init = (initproc)narytree_auto_init,
    .tp_new = narytree_auto_new,
};

static PyTypeObject NodeAutoType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree_auto.NodeAuto",
    .tp_basicsize = sizeof(NodeAutoObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)node_auto_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "N-ary tree node with auto-rebalancing support",
    .tp_methods = node_auto_methods,
    .tp_new = node_auto_new,
};

// Module definition
static PyModuleDef narytree_auto_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "narytree_auto",
    .m_doc = "N-ary tree data structure module with automatic rebalancing",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit_narytree_auto(void) {
    PyObject* m;
    
    if (PyType_Ready(&NaryTreeAutoType) < 0) {
        return NULL;
    }
    
    if (PyType_Ready(&NodeAutoType) < 0) {
        return NULL;
    }
    
    m = PyModule_Create(&narytree_auto_module);
    if (m == NULL) {
        return NULL;
    }
    
    Py_INCREF(&NaryTreeAutoType);
    if (PyModule_AddObject(m, "NaryTreeAuto", (PyObject*)&NaryTreeAutoType) < 0) {
        Py_DECREF(&NaryTreeAutoType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&NodeAutoType);
    if (PyModule_AddObject(m, "NodeAuto", (PyObject*)&NodeAutoType) < 0) {
        Py_DECREF(&NodeAutoType);
        Py_DECREF(&NaryTreeAutoType);
        Py_DECREF(m);
        return NULL;
    }
    
    return m;
}

} // extern "C"