// obmalloc.c implementation

#include "runtime.h"

namespace python {

extern "C" void* PyObject_Malloc(size_t size) { return std::malloc(size); }

extern "C" void PyObject_Free(void* ptr) { return std::free(ptr); }

}  // namespace python
