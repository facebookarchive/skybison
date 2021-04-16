#include "capi-state.h"

#include <new>

#include "cpython-func.h"
#include "cpython-types.h"

#include "api-handle.h"
#include "dict-builtins.h"
#include "globals.h"
#include "handles-decl.h"
#include "handles.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"
#include "visitor.h"

namespace py {

static const word kHandleBlockSize = word{4} * kGiB;
static const word kInitialCachesCapacity = 128;
static const word kInitialHandlesCapacity = 256;

void finalizeCAPIState(Runtime* runtime) {
  CAPIState* state = capiState(runtime);
  OS::freeMemory(state->handle_buffer, state->handle_buffer_size);
  state->~CAPIState();
}

static void freeExtensionModule(PyObject* obj, const Module& module) {
  PyModuleDef* def =
      reinterpret_cast<PyModuleDef*>(Int::cast(module.def()).asCPtr());
  if (def->m_free != nullptr) {
    def->m_free(obj);
  }
  module.setDef(SmallInt::fromWord(0));
  if (module.hasState()) {
    std::free(Int::cast(module.state()).asCPtr());
    module.setState(SmallInt::fromWord(0));
  }
}

void freeExtensionModules(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object module_obj(&scope, NoneType::object());
  for (PyObject* obj : *capiModules(runtime)) {
    if (obj == nullptr) {
      continue;
    }
    module_obj = ApiHandle::fromPyObject(obj)->asObject();
    if (!runtime->isInstanceOfModule(*module_obj)) {
      continue;
    }
    Module module(&scope, *module_obj);
    if (module.hasDef()) {
      freeExtensionModule(obj, module);
    }
    Py_DECREF(obj);
  }
}

void initializeCAPIState(Runtime* runtime) {
  CAPIState* state = capiState(runtime);
  new (state) CAPIState;
  state->caches.initialize(kInitialCachesCapacity);
  state->handles.initialize(kInitialHandlesCapacity);

  state->handle_buffer =
      OS::allocateMemory(kHandleBlockSize, &state->handle_buffer_size);
  state->free_handles = reinterpret_cast<FreeListNode*>(state->handle_buffer);
}

word numTrackedApiHandles(Runtime* runtime) {
  return capiHandles(runtime)->numItems();
}

}  // namespace py
