
#include <Python.h>

// Add your Python module function definitions here
static PyMethodDef MeshoptimizerMethods[] = {
    {NULL, NULL, 0, NULL}  // Sentinel
};

static struct PyModuleDef meshoptimizermodule = {
    PyModuleDef_HEAD_INIT,
    "_meshoptimizer",
    "Python bindings for meshoptimizer library.",
    -1,
    MeshoptimizerMethods
};

PyMODINIT_FUNC PyInit__meshoptimizer(void) {
    return PyModule_Create(&meshoptimizermodule);
}
