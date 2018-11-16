// obmalloc.c implementation

#include "cpython-func.h"
#include "runtime.h"

namespace python {

extern "C" void* PyObject_Malloc(size_t size) { return PyMem_RawMalloc(size); }

extern "C" void* PyObject_Calloc(size_t nelem, size_t size) {
  return PyMem_RawCalloc(nelem, size);
}

extern "C" void* PyObject_Realloc(void* ptr, size_t size) {
  return PyMem_RawRealloc(ptr, size);
}

extern "C" void PyObject_Free(void* ptr) { return PyMem_RawFree(ptr); }

extern "C" void* PyMem_Malloc(size_t size) { return PyMem_RawMalloc(size); }

extern "C" void* PyMem_Calloc(size_t nelem, size_t size) {
  return PyMem_RawCalloc(nelem, size);
}

extern "C" void* PyMem_Realloc(void* ptr, size_t size) {
  return PyMem_RawRealloc(ptr, size);
}

extern "C" void PyMem_Free(void* ptr) { return PyMem_RawFree(ptr); }

extern "C" void* PyMem_RawMalloc(size_t size) {
  if (size == 0) {
    size = 1;
  }
  return std::malloc(size);
}

extern "C" void* PyMem_RawCalloc(size_t nelem, size_t size) {
  if (size == 0 || nelem == 0) {
    nelem = 1;
    size = 1;
  }
  return std::calloc(nelem, size);
}

extern "C" void* PyMem_RawRealloc(void* ptr, size_t size) {
  if (size == 0) {
    size = 1;
  }
  return std::realloc(ptr, size);
}

extern "C" void PyMem_RawFree(void* ptr) { return std::free(ptr); }

}  // namespace python
