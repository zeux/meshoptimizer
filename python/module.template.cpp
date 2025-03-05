#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/ndarraytypes.h>

// Define implementation before including the header
#define MESHOPTIMIZER_NO_RESET_OVERRIDE
#define MESHOPTIMIZER_IMPLEMENTATION
#include "meshoptimizer.h"

// Include all implementation files directly
{{SOURCE_IMPORTS}}

// Prevent namespace pollution
namespace {

void* fallback_allocate(size_t size) {
    return PyMem_Malloc(size);
}

void fallback_deallocate(void* ptr) {
    PyMem_Free(ptr);
}

void* (*allocate_fun)(size_t) = fallback_allocate;
void (*deallocate_fun)(void*) = fallback_deallocate;

PyObject* meshopt_set_allocator(PyObject* self, PyObject* args) {
    meshopt_setAllocator(allocate_fun, deallocate_fun);
    Py_RETURN_NONE;
}

PyMethodDef MeshoptMethods[] = {
    {"set_allocator", meshopt_set_allocator, METH_NOARGS,
     "Set the default memory allocator"},
    {NULL, NULL, 0, NULL}
};

struct PyModuleDef meshopt_module = {
    PyModuleDef_HEAD_INIT,
    "_meshoptimizer",
    "Python binding for meshoptimizer library",
    -1,
    MeshoptMethods
};

} // anonymous namespace

PyMODINIT_FUNC PyInit__meshoptimizer(void) {
    import_array();
    
    PyObject* m = PyModule_Create(&meshopt_module);
    if (m == NULL)
        return NULL;

    meshopt_setAllocator(allocate_fun, deallocate_fun);
        
    return m;
}
