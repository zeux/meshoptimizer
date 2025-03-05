#include <Python.h>

// Define MESHOPTIMIZER_IMPLEMENTATION to include the implementation
#define MESHOPTIMIZER_IMPLEMENTATION
// Include the meshoptimizer header
#include "../../src/meshoptimizer.h"

// Include all the implementation files
{{SOURCE_IMPORTS}}

// Define the Python module

// Export all the C functions that are used by the Python wrapper
// This ensures they are available when loaded via ctypes

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
    PyObject* m = PyModule_Create(&meshoptimizermodule);
    if (m == NULL)
        return NULL;
    
    // We don't need to add any methods to the module since we're using ctypes
    // to access the C functions directly. The important part is that by including
    // meshoptimizer.h and linking against the C++ code, the functions will be
    // exported in the shared library.
    
    return m;
}
