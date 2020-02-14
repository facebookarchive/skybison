#include "cpython-func.h"
#include "cpython-types.h"

#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyAsyncGen_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kAsyncGenerator)));
}

PY_EXPORT PyTypeObject* PyCoro_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kCoroutine)));
}

PY_EXPORT PyTypeObject* PyGen_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kGenerator)));
}

}  // namespace py
