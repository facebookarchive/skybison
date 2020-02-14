#include "cpython-func.h"
#include "cpython-types.h"

#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyLongRangeIter_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kLongRangeIterator)));
}

PY_EXPORT PyTypeObject* PyRangeIter_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kRangeIterator)));
}

PY_EXPORT PyTypeObject* PyRange_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kRange)));
}

}  // namespace py
