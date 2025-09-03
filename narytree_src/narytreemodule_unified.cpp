#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "succinct_narytree_unified.cpp"

// Python wrapper for SuccinctNaryTree
struct PyNaryTreeObject {
    PyObject_HEAD
    SuccinctNaryTree<PyObject*>* tree;
};

// Python wrapper for NodeView
struct PyNodeViewObject {
    PyObject_HEAD
    SuccinctNaryTree<PyObject*>::NodeView* node_view;
    PyNaryTreeObject* tree_obj; // Keep reference to parent tree
};

// Helper functions
static void ensure_python_object_refs(PyObject* obj) {
    if (obj) Py_INCREF(obj);
}

static void release_python_object_refs(PyObject* obj) {
    if (obj) Py_DECREF(obj);
}

// Forward declarations
static void PyNaryTree_dealloc(PyNaryTreeObject* self);
static PyObject* PyNaryTree_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int PyNaryTree_init(PyNaryTreeObject* self, PyObject* args, PyObject* kwds);
static PyObject* PyNaryTree_empty(PyNaryTreeObject* self);
static PyObject* PyNaryTree_size(PyNaryTreeObject* self);
static PyObject* PyNaryTree_set_root(PyNaryTreeObject* self, PyObject* args);
static PyObject* PyNaryTree_root(PyNaryTreeObject* self);
static PyObject* PyNaryTree_clear(PyNaryTreeObject* self);
static PyObject* PyNaryTree_get_locality_statistics(PyNaryTreeObject* self);
static PyObject* PyNaryTree_rebalance_for_locality(PyNaryTreeObject* self);

static void PyNodeView_dealloc(PyNodeViewObject* self);
static PyObject* PyNodeView_data(PyNodeViewObject* self);
static PyObject* PyNodeView_set_data(PyNodeViewObject* self, PyObject* args);
static PyObject* PyNodeView_add_child(PyNodeViewObject* self, PyObject* args);
static PyObject* PyNodeView_child_count(PyNodeViewObject* self);
static PyObject* PyNodeView_is_leaf(PyNodeViewObject* self);

// Method definitions
static PyMethodDef PyNodeView_methods[] = {
    {"data", (PyCFunction)PyNodeView_data, METH_NOARGS, "Get node data"},
    {"set_data", (PyCFunction)PyNodeView_set_data, METH_VARARGS, "Set node data"},
    {"add_child", (PyCFunction)PyNodeView_add_child, METH_VARARGS, "Add child node"},
    {"child_count", (PyCFunction)PyNodeView_child_count, METH_NOARGS, "Get child count"},
    {"is_leaf", (PyCFunction)PyNodeView_is_leaf, METH_NOARGS, "Check if leaf node"},
    {NULL, NULL, 0, NULL}
};

// Type definitions (must come before methods that use them)
static PyTypeObject PyNodeViewType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree.NodeView",
    .tp_basicsize = sizeof(PyNodeViewObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)PyNodeView_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Node view for succinct N-ary tree",
    .tp_methods = PyNodeView_methods,
};

static PyMethodDef PyNaryTree_methods[] = {
    {"empty", (PyCFunction)PyNaryTree_empty, METH_NOARGS, "Check if tree is empty"},
    {"size", (PyCFunction)PyNaryTree_size, METH_NOARGS, "Get tree size"},
    {"set_root", (PyCFunction)PyNaryTree_set_root, METH_VARARGS, "Set root data"},
    {"root", (PyCFunction)PyNaryTree_root, METH_NOARGS, "Get root node"},
    {"clear", (PyCFunction)PyNaryTree_clear, METH_NOARGS, "Clear tree"},
    {"get_locality_statistics", (PyCFunction)PyNaryTree_get_locality_statistics, METH_NOARGS, "Get locality statistics"},
    {"rebalance_for_locality", (PyCFunction)PyNaryTree_rebalance_for_locality, METH_NOARGS, "Rebalance for locality"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject PyNaryTreeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree.SuccinctNaryTree",
    .tp_basicsize = sizeof(PyNaryTreeObject),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor)PyNaryTree_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = "Succinct N-ary Tree with locality optimization",
    .tp_methods = PyNaryTree_methods,
    .tp_init = (initproc)PyNaryTree_init,
    .tp_new = PyNaryTree_new,
};

// PyNaryTree methods implementation
static void PyNaryTree_dealloc(PyNaryTreeObject* self) {
    if (self->tree) {
        // Release all Python object references stored in the tree
        if (!self->tree->empty()) {
            self->tree->for_each_preorder_locality_optimized([](auto& node) {
                release_python_object_refs(node.data());
            });
        }
        delete self->tree;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* PyNaryTree_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/) {
    PyNaryTreeObject* self = (PyNaryTreeObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->tree = nullptr;
    }
    return (PyObject*)self;
}

static int PyNaryTree_init(PyNaryTreeObject* self, PyObject* args, PyObject* kwds) {
    PyObject* root_data = nullptr;
    
    static char* kwlist[] = {(char*)"root_data", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &root_data)) {
        return -1;
    }
    
    try {
        if (root_data) {
            ensure_python_object_refs(root_data);
            self->tree = new SuccinctNaryTree<PyObject*>(root_data);
        } else {
            self->tree = new SuccinctNaryTree<PyObject*>();
        }
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    }
    
    return 0;
}

static PyObject* PyNaryTree_empty(PyNaryTreeObject* self) {
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    return PyBool_FromLong(self->tree->empty());
}

static PyObject* PyNaryTree_size(PyNaryTreeObject* self) {
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    return PyLong_FromSize_t(self->tree->size());
}

static PyObject* PyNaryTree_set_root(PyNaryTreeObject* self, PyObject* args) {
    PyObject* root_data;
    
    if (!PyArg_ParseTuple(args, "O", &root_data)) {
        return NULL;
    }
    
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    try {
        ensure_python_object_refs(root_data);
        self->tree->set_root(root_data);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject* PyNaryTree_root(PyNaryTreeObject* self) {
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    if (self->tree->empty()) {
        PyErr_SetString(PyExc_RuntimeError, "Tree is empty");
        return NULL;
    }
    
    try {
        // Create NodeView wrapper
        PyNodeViewObject* node_view = (PyNodeViewObject*)PyNodeViewType.tp_alloc(&PyNodeViewType, 0);
        if (!node_view) return NULL;
        
        auto root_view = self->tree->root();
        node_view->node_view = new SuccinctNaryTree<PyObject*>::NodeView(root_view);
        node_view->tree_obj = self;
        Py_INCREF(self); // Keep tree alive
        
        return (PyObject*)node_view;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* PyNaryTree_clear(PyNaryTreeObject* self) {
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    // Release all Python object references
    if (!self->tree->empty()) {
        self->tree->for_each_preorder_locality_optimized([](auto& node) {
            release_python_object_refs(node.data());
        });
    }
    
    self->tree->clear();
    Py_RETURN_NONE;
}

static PyObject* PyNaryTree_get_locality_statistics(PyNaryTreeObject* self) {
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    try {
        auto stats = self->tree->get_locality_statistics();
        
        PyObject* dict = PyDict_New();
        if (!dict) return NULL;
        
        PyDict_SetItemString(dict, "total_nodes", PyLong_FromSize_t(stats.total_nodes));
        PyDict_SetItemString(dict, "max_depth", PyLong_FromSize_t(stats.max_depth));
        PyDict_SetItemString(dict, "locality_score", PyFloat_FromDouble(stats.locality_score));
        PyDict_SetItemString(dict, "cache_line_efficiency", PyLong_FromSize_t(stats.cache_line_efficiency));
        PyDict_SetItemString(dict, "compression_ratio", PyFloat_FromDouble(stats.compression_ratio));
        PyDict_SetItemString(dict, "memory_usage_bytes", PyLong_FromSize_t(stats.memory_usage_bytes));
        
        return dict;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* PyNaryTree_rebalance_for_locality(PyNaryTreeObject* self) {
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    try {
        self->tree->rebalance_for_locality();
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

// PyNodeView methods implementation
static void PyNodeView_dealloc(PyNodeViewObject* self) {
    if (self->node_view) {
        delete self->node_view;
    }
    if (self->tree_obj) {
        Py_DECREF(self->tree_obj);
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* PyNodeView_data(PyNodeViewObject* self) {
    if (!self->node_view) {
        PyErr_SetString(PyExc_RuntimeError, "NodeView not initialized");
        return NULL;
    }
    
    try {
        PyObject* data = self->node_view->data();
        Py_INCREF(data);
        return data;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* PyNodeView_set_data(PyNodeViewObject* self, PyObject* args) {
    PyObject* new_data;
    
    if (!PyArg_ParseTuple(args, "O", &new_data)) {
        return NULL;
    }
    
    if (!self->node_view) {
        PyErr_SetString(PyExc_RuntimeError, "NodeView not initialized");
        return NULL;
    }
    
    try {
        // Release old reference, acquire new one
        release_python_object_refs(self->node_view->data());
        ensure_python_object_refs(new_data);
        self->node_view->set_data(new_data);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject* PyNodeView_add_child(PyNodeViewObject* self, PyObject* args) {
    PyObject* child_data;
    
    if (!PyArg_ParseTuple(args, "O", &child_data)) {
        return NULL;
    }
    
    if (!self->node_view) {
        PyErr_SetString(PyExc_RuntimeError, "NodeView not initialized");
        return NULL;
    }
    
    try {
        ensure_python_object_refs(child_data);
        auto child_view = self->node_view->add_child(child_data);
        
        // Create new NodeView wrapper for child
        PyNodeViewObject* child_node = (PyNodeViewObject*)PyNodeViewType.tp_alloc(&PyNodeViewType, 0);
        if (!child_node) return NULL;
        
        child_node->node_view = new SuccinctNaryTree<PyObject*>::NodeView(child_view);
        child_node->tree_obj = self->tree_obj;
        Py_INCREF(self->tree_obj);
        
        return (PyObject*)child_node;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject* PyNodeView_child_count(PyNodeViewObject* self) {
    if (!self->node_view) {
        PyErr_SetString(PyExc_RuntimeError, "NodeView not initialized");
        return NULL;
    }
    
    return PyLong_FromSize_t(self->node_view->child_count());
}

static PyObject* PyNodeView_is_leaf(PyNodeViewObject* self) {
    if (!self->node_view) {
        PyErr_SetString(PyExc_RuntimeError, "NodeView not initialized");
        return NULL;
    }
    
    return PyBool_FromLong(self->node_view->is_leaf());
}

// Module methods
static PyObject* narytree_create_tree(PyObject* /*self*/, PyObject* args) {
    PyObject* root_data = nullptr;
    
    if (!PyArg_ParseTuple(args, "|O", &root_data)) {
        return NULL;
    }
    
    PyObject* tree_args = root_data ? PyTuple_Pack(1, root_data) : PyTuple_New(0);
    if (!tree_args) return NULL;
    
    PyObject* tree = PyObject_CallObject((PyObject*)&PyNaryTreeType, tree_args);
    Py_DECREF(tree_args);
    
    return tree;
}

static PyObject* narytree_benchmark_locality(PyObject* /*self*/, PyObject* args) {
    int node_count = 1000;
    
    if (!PyArg_ParseTuple(args, "|i", &node_count)) {
        return NULL;
    }
    
    try {
        // Create test tree with specified node count
        auto test_tree = SuccinctNaryTree<int>(0);
        
        // Add nodes in a way that tests locality
        auto root = test_tree.root();
        for (int i = 1; i < node_count; ++i) {
            root.add_child(i);
        }
        
        // Get locality statistics
        auto stats = test_tree.get_locality_statistics();
        
        PyObject* result = PyDict_New();
        if (!result) return NULL;
        
        PyDict_SetItemString(result, "node_count", PyLong_FromLong(node_count));
        PyDict_SetItemString(result, "locality_score", PyFloat_FromDouble(stats.locality_score));
        PyDict_SetItemString(result, "compression_ratio", PyFloat_FromDouble(stats.compression_ratio));
        PyDict_SetItemString(result, "memory_usage_bytes", PyLong_FromSize_t(stats.memory_usage_bytes));
        
        return result;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyMethodDef narytree_methods[] = {
    {"create_tree", narytree_create_tree, METH_VARARGS, "Create a new succinct N-ary tree"},
    {"benchmark_locality", narytree_benchmark_locality, METH_VARARGS, "Benchmark locality performance"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef narytreemodule = {
    PyModuleDef_HEAD_INIT,
    "narytree",
    "Succinct N-ary Tree with locality optimization - unified representation",
    -1,
    narytree_methods,
    NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_narytree(void) {
    PyObject* m;
    
    if (PyType_Ready(&PyNaryTreeType) < 0) return NULL;
    if (PyType_Ready(&PyNodeViewType) < 0) return NULL;
    
    m = PyModule_Create(&narytreemodule);
    if (m == NULL) return NULL;
    
    Py_INCREF(&PyNaryTreeType);
    if (PyModule_AddObject(m, "SuccinctNaryTree", (PyObject*)&PyNaryTreeType) < 0) {
        Py_DECREF(&PyNaryTreeType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&PyNodeViewType);
    if (PyModule_AddObject(m, "NodeView", (PyObject*)&PyNodeViewType) < 0) {
        Py_DECREF(&PyNodeViewType);
        Py_DECREF(m);
        return NULL;
    }
    
    // Add module constants
    PyModule_AddIntConstant(m, "LAZY_BALANCE_THRESHOLD", 100);
    PyModule_AddStringConstant(m, "__version__", "1.0.0");
    PyModule_AddStringConstant(m, "__author__", "Nico Liberato");
    
    return m;
}