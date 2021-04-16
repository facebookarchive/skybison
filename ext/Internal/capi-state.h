#pragma once

#include "cpython-types.h"

#include "api-handle-dict.h"
#include "api-handle.h"
#include "capi.h"
#include "runtime.h"
#include "vector.h"

namespace py {

struct CAPIState {
  // Some API functions promise to cache their return value and return the same
  // value for repeated invocations on a specific PyObject. Those value are
  // cached here.
  ApiHandleDict caches;

  // A linked list of freed handles.
  // The last node is the frontier of allocated handles.
  FreeListNode* free_handles;

  // The raw memory used to allocated handles.
  byte* handle_buffer;
  word handle_buffer_size;

  // C-API object handles
  ApiHandleDict handles;

  Vector<PyObject*> modules;
};

static_assert(sizeof(CAPIState) < kCAPIStateSize, "kCAPIStateSize too small");

inline ApiHandleDict* capiCaches(Runtime* runtime) {
  return &runtime->capiState()->caches;
}

inline FreeListNode** capiFreeHandles(Runtime* runtime) {
  return &runtime->capiState()->free_handles;
}

inline ApiHandleDict* capiHandles(Runtime* runtime) {
  return &runtime->capiState()->handles;
}

inline Vector<PyObject*>* capiModules(Runtime* runtime) {
  return &runtime->capiState()->modules;
}

}  // namespace py
