#include <Python.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

// Forward declarations
class SuccinctNaryTree;
class NodeView;

// Python object structures
typedef struct {
    PyObject_HEAD
    std::shared_ptr<SuccinctNaryTree> tree;
} NaryTreeObject;

typedef struct {
    PyObject_HEAD
    std::shared_ptr<SuccinctNaryTree> tree;
    size_t node_id;
} NodeViewObject;

// Include the actual tree implementation
#include "succinct_narytree_unified.cpp"

static PyTypeObject NaryTreeType;
static PyTypeObject NodeViewType;

// Forward declarations for Python methods
static PyObject* NodeView_add_child(NodeViewObject* self, PyObject* args);
static PyObject* NodeView_data(NodeViewObject* self, PyObject* Py_UNUSED(ignored));
static PyObject* NodeView_set_data(NodeViewObject* self, PyObject* args);
static PyObject* NodeView_child_count(NodeViewObject* self, PyObject* Py_UNUSED(ignored));
static PyObject* NodeView_is_leaf(NodeViewObject* self, PyObject* Py_UNUSED(ignored));

// NaryTree methods
static PyObject*
NaryTree_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    NaryTreeObject *self = (NaryTreeObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->tree = nullptr;
    }
    return (PyObject*)self;
}

static int
NaryTree_init(NaryTreeObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *root_data = nullptr;
    if (!PyArg_ParseTuple(args, "|O", &root_data)) {
        return -1;
    }
    
    try {
        self->tree = std::make_shared<SuccinctNaryTree>();
        if (root_data) {
            Py_INCREF(root_data);
            self->tree->set_root(root_data);
        }
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return -1;
    }
    
    return 0;
}

static void
NaryTree_dealloc(NaryTreeObject* self)
{
    self->tree.reset();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
NaryTree_size(NaryTreeObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    return PyLong_FromSize_t(self->tree->size());
}

static PyObject*
NaryTree_empty(NaryTreeObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    return PyBool_FromLong(self->tree->empty() ? 1 : 0);
}

static PyObject*
NaryTree_clear(NaryTreeObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    self->tree->clear();
    Py_RETURN_NONE;
}

static PyObject*
NaryTree_set_root(NaryTreeObject* self, PyObject* args)
{
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }
    
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    try {
        Py_INCREF(data);
        self->tree->set_root(data);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject*
NaryTree_root(NaryTreeObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree || self->tree->empty()) {
        PyErr_SetString(PyExc_RuntimeError, "Tree is empty");
        return NULL;
    }
    
    NodeViewObject* node = (NodeViewObject*)NodeViewType.tp_alloc(&NodeViewType, 0);
    if (node == NULL) {
        return NULL;
    }
    
    node->tree = self->tree;
    node->node_id = 0; // Root is always at index 0
    
    return (PyObject*)node;
}

static PyObject*
NaryTree_get_locality_statistics(NaryTreeObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Tree not initialized");
        return NULL;
    }
    
    try {
        auto stats = self->tree->get_locality_statistics();
        PyObject* dict = PyDict_New();
        if (!dict) return NULL;
        
        PyDict_SetItemString(dict, "locality_score", PyFloat_FromDouble(stats.locality_score));
        PyDict_SetItemString(dict, "compression_ratio", PyFloat_FromDouble(stats.compression_ratio));
        PyDict_SetItemString(dict, "memory_usage_bytes", PyLong_FromSize_t(stats.memory_usage_bytes));
        
        return dict;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject*
NaryTree_rebalance_for_locality(NaryTreeObject* self, PyObject* Py_UNUSED(ignored))
{
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

// NodeView methods
static PyObject*
NodeView_add_child(NodeViewObject* self, PyObject* args)
{
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }
    
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    try {
        Py_INCREF(data);
        size_t child_id = self->tree->add_child(self->node_id, data);
        
        NodeViewObject* child_node = (NodeViewObject*)NodeViewType.tp_alloc(&NodeViewType, 0);
        if (child_node == NULL) {
            return NULL;
        }
        
        child_node->tree = self->tree;
        child_node->node_id = child_id;
        
        return (PyObject*)child_node;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject*
NodeView_data(NodeViewObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    try {
        PyObject* data = self->tree->get_data(self->node_id);
        if (data) {
            Py_INCREF(data);
            return data;
        }
        Py_RETURN_NONE;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject*
NodeView_set_data(NodeViewObject* self, PyObject* args)
{
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) {
        return NULL;
    }
    
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    try {
        Py_INCREF(data);
        self->tree->set_data(self->node_id, data);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject*
NodeView_child_count(NodeViewObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    try {
        return PyLong_FromSize_t(self->tree->child_count(self->node_id));
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static PyObject*
NodeView_is_leaf(NodeViewObject* self, PyObject* Py_UNUSED(ignored))
{
    if (!self->tree) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid node");
        return NULL;
    }
    
    try {
        return PyBool_FromLong(self->tree->is_leaf(self->node_id) ? 1 : 0);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

static void
NodeView_dealloc(NodeViewObject* self)
{
    self->tree.reset();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Method definitions
static PyMethodDef NaryTree_methods[] = {
    {"size", (PyCFunction)NaryTree_size, METH_NOARGS, "Get tree size"},
    {"empty", (PyCFunction)NaryTree_empty, METH_NOARGS, "Check if tree is empty"},
    {"clear", (PyCFunction)NaryTree_clear, METH_NOARGS, "Clear the tree"},
    {"set_root", (PyCFunction)NaryTree_set_root, METH_VARARGS, "Set root data"},
    {"root", (PyCFunction)NaryTree_root, METH_NOARGS, "Get root node"},
    {"get_locality_statistics", (PyCFunction)NaryTree_get_locality_statistics, METH_NOARGS, "Get locality statistics"},
    {"rebalance_for_locality", (PyCFunction)NaryTree_rebalance_for_locality, METH_NOARGS, "Rebalance for locality"},
    {NULL}
};

static PyMethodDef NodeView_methods[] = {
    {"add_child", (PyCFunction)NodeView_add_child, METH_VARARGS, "Add child node"},
    {"data", (PyCFunction)NodeView_data, METH_NOARGS, "Get node data"},
    {"set_data", (PyCFunction)NodeView_set_data, METH_VARARGS, "Set node data"},
    {"child_count", (PyCFunction)NodeView_child_count, METH_NOARGS, "Get child count"},
    {"is_leaf", (PyCFunction)NodeView_is_leaf, METH_NOARGS, "Check if node is leaf"},
    {NULL}
};

// Type definitions
static PyTypeObject NaryTreeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree.SuccinctNaryTree",
    .tp_doc = "Succinct N-ary tree with locality optimization",
    .tp_basicsize = sizeof(NaryTreeObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NaryTree_new,
    .tp_init = (initproc)NaryTree_init,
    .tp_dealloc = (destructor)NaryTree_dealloc,
    .tp_methods = NaryTree_methods,
};

static PyTypeObject NodeViewType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "narytree.NodeView",
    .tp_doc = "Node view for succinct N-ary tree",
    .tp_basicsize = sizeof(NodeViewObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)NodeView_dealloc,
    .tp_methods = NodeView_methods,
};

// Utility functions
static PyObject*
narytree_create_tree(PyObject* self, PyObject* args)
{
    PyObject *root_data = nullptr;
    if (!PyArg_ParseTuple(args, "|O", &root_data)) {
        return NULL;
    }
    
    PyObject* tree_args = root_data ? PyTuple_Pack(1, root_data) : PyTuple_New(0);
    if (!tree_args) return NULL;
    
    PyObject* tree = PyObject_CallObject((PyObject*)&NaryTreeType, tree_args);
    Py_DECREF(tree_args);
    
    return tree;
}

static PyObject*
narytree_benchmark_locality(PyObject* self, PyObject* args)
{
    int node_count;
    if (!PyArg_ParseTuple(args, "i", &node_count)) {
        return NULL;
    }
    
    // Simple benchmark - create tree with specified node count
    try {
        SuccinctNaryTree tree;
        tree.set_root(Py_None);
        
        // Add nodes in a balanced way
        for (int i = 1; i < node_count; ++i) {
            size_t parent_id = (i - 1) / 4;  // Roughly 4-ary tree
            tree.add_child(parent_id, Py_None);
        }
        
        auto stats = tree.get_locality_statistics();
        
        PyObject* dict = PyDict_New();
        if (!dict) return NULL;
        
        PyDict_SetItemString(dict, "node_count", PyLong_FromLong(node_count));
        PyDict_SetItemString(dict, "locality_score", PyFloat_FromDouble(stats.locality_score));
        PyDict_SetItemString(dict, "compression_ratio", PyFloat_FromDouble(stats.compression_ratio));
        PyDict_SetItemString(dict, "memory_usage_bytes", PyLong_FromSize_t(stats.memory_usage_bytes));
        
        return dict;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
}

// Module methods
static PyMethodDef module_methods[] = {
    {"create_tree", narytree_create_tree, METH_VARARGS, "Create a new succinct N-ary tree"},
    {"benchmark_locality", narytree_benchmark_locality, METH_VARARGS, "Benchmark locality performance"},
    {NULL}
};

// Module definition
static struct PyModuleDef narytree_module = {
    PyModuleDef_HEAD_INIT,
    "narytree",
    "Succinct N-ary tree with locality optimization",
    -1,
    module_methods
};

// Module initialization
PyMODINIT_FUNC
PyInit_narytree(void)
{
    PyObject* m;
    
    if (PyType_Ready(&NaryTreeType) < 0)
        return NULL;
    
    if (PyType_Ready(&NodeViewType) < 0)
        return NULL;
    
    m = PyModule_Create(&narytree_module);
    if (m == NULL)
        return NULL;
    
    Py_INCREF(&NaryTreeType);
    if (PyModule_AddObject(m, "SuccinctNaryTree", (PyObject*)&NaryTreeType) < 0) {
        Py_DECREF(&NaryTreeType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&NodeViewType);
    if (PyModule_AddObject(m, "NodeView", (PyObject*)&NodeViewType) < 0) {
        Py_DECREF(&NodeViewType);
        Py_DECREF(m);
        return NULL;
    }
    
    if (PyModule_AddIntConstant(m, "LAZY_BALANCE_THRESHOLD", 100) < 0) {
        Py_DECREF(m);
        return NULL;
    }
    
    return m;
}