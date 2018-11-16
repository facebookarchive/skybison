// obmalloc.c implementation

#include "Python.h"

#include "runtime/runtime.h"

namespace python {

extern "C" void* PyObject_Malloc(size_t size) {
  Thread* thread = Thread::currentThread();
  return TrackedAllocation::malloc(thread->runtime(), size);
}

extern "C" void PyObject_Free(void* ptr) {
  Thread* thread = Thread::currentThread();
  TrackedAllocation::freePtr(thread->runtime(), ptr);
}

} // namespace python
