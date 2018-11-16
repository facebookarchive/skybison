// obmalloc.c implementation

#include "Python.h"

#include "runtime/runtime.h"

namespace python {

extern "C" void* PyObject_Malloc(size_t size) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return TrackedAllocation::malloc(runtime->trackedAllocations(), size);
}

extern "C" void PyObject_Free(void* ptr) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  TrackedAllocation::freePtr(runtime->trackedAllocations(), ptr);
}

} // namespace python
