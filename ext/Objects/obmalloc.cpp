// obmalloc.c implementation

#include "cpython-func.h"
#include "runtime.h"

namespace python {

PY_EXPORT void* PyObject_Malloc(size_t size) { return PyMem_RawMalloc(size); }

PY_EXPORT void* PyObject_Calloc(size_t nelem, size_t size) {
  return PyMem_RawCalloc(nelem, size);
}

PY_EXPORT void* PyObject_Realloc(void* ptr, size_t size) {
  return PyMem_RawRealloc(ptr, size);
}

PY_EXPORT void PyObject_Free(void* ptr) { return PyMem_RawFree(ptr); }

PY_EXPORT void* PyMem_Malloc(size_t size) { return PyMem_RawMalloc(size); }

PY_EXPORT void* PyMem_Calloc(size_t nelem, size_t size) {
  return PyMem_RawCalloc(nelem, size);
}

PY_EXPORT void* PyMem_Realloc(void* ptr, size_t size) {
  return PyMem_RawRealloc(ptr, size);
}

PY_EXPORT void PyMem_Free(void* ptr) { return PyMem_RawFree(ptr); }

PY_EXPORT void* PyMem_RawMalloc(size_t size) {
  if (size == 0) {
    size = 1;
  }
  return std::malloc(size);
}

PY_EXPORT void* PyMem_RawCalloc(size_t nelem, size_t size) {
  if (size == 0 || nelem == 0) {
    nelem = 1;
    size = 1;
  }
  return std::calloc(nelem, size);
}

PY_EXPORT void* PyMem_RawRealloc(void* ptr, size_t size) {
  if (size == 0) {
    size = 1;
  }
  return std::realloc(ptr, size);
}

PY_EXPORT void PyMem_RawFree(void* ptr) { return std::free(ptr); }

PY_EXPORT void* PyMem_New_Func(size_t size, size_t n) {
  if (n > kMaxWord / size) return nullptr;
  return PyMem_Malloc(n * size);
}

PY_EXPORT void* PyMem_Resize_Func(void* p, size_t size, size_t n) {
  if (n > kMaxWord / size) return nullptr;
  return PyMem_Realloc(p, n * size);
}

PY_EXPORT char* _PyMem_RawStrdup(const char* str) {
  size_t size = std::strlen(str) + 1;
  char* result = static_cast<char*>(PyMem_RawMalloc(size));
  if (result != nullptr) {
    std::memcpy(result, str, size);
  }
  return result;
}

PY_EXPORT char* _PyMem_Strdup(const char* str) {
  size_t size = std::strlen(str) + 1;
  char* result = static_cast<char*>(PyMem_Malloc(size));
  if (result != nullptr) {
    std::memcpy(result, str, size);
  }
  return result;
}

}  // namespace python
