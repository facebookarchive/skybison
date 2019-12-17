#include "cpython-func.h"

#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT const char* Py_GetVersion() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str version(&scope, runtime->lookupNameInModule(thread, SymbolId::kSys,
                                                  SymbolId::kVersion));
  return PyUnicode_AsUTF8(ApiHandle::borrowedReference(thread, *version));
}

}  // namespace py
