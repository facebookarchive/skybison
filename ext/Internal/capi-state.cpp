#include "capi-state.h"

#include <new>

#include "cpython-func.h"
#include "cpython-types.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "globals.h"
#include "handles-decl.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"
#include "visitor.h"

namespace py {

static const word kInitialCachesCapacity = 128;
static const word kInitialHandlesCapacity = 256;

void capiStateVisit(CAPIState* state, PointerVisitor* visitor) {
  state->handles_.visit(visitor);
  ApiHandle::visitReferences(&state->handles_, visitor);
  state->caches_.visit(visitor);
}

void finalizeCAPIState(Runtime* runtime) { runtime->capiState()->~CAPIState(); }

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
  CAPIState* state = runtime->capiState();
  new (state) CAPIState;
  state->caches_.initialize(runtime, kInitialCachesCapacity);
  state->handles_.initialize(runtime, kInitialHandlesCapacity);
}

word numTrackedApiHandles(Runtime* runtime) {
  return capiHandles(runtime)->numItems();
}

}  // namespace py
