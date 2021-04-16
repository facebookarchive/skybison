#include "cpython-func.h"
#include "cpython-types.h"

#include "api-handle.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyAsyncGen_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kAsyncGenerator)));
}

PY_EXPORT PyTypeObject* PyCoro_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kCoroutine)));
}

PY_EXPORT PyTypeObject* PyGen_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kGenerator)));
}

}  // namespace py
