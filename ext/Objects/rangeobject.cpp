#include "cpython-func.h"
#include "cpython-types.h"

#include "api-handle.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyLongRangeIter_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kLongRangeIterator)));
}

PY_EXPORT PyTypeObject* PyRangeIter_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kRangeIterator)));
}

PY_EXPORT PyTypeObject* PyRange_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::borrowedReference(runtime, runtime->typeAt(LayoutId::kRange)));
}

}  // namespace py
