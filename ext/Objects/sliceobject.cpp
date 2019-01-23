#include "runtime.h"

namespace python {

PY_EXPORT int PySlice_Check_Func(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->asObject()->isSlice();
}

PY_EXPORT int PySlice_GetIndices(PyObject* /* r */, Py_ssize_t /* h */,
                                 Py_ssize_t* /* t */, Py_ssize_t* /* p */,
                                 Py_ssize_t* /* p */) {
  UNIMPLEMENTED("PySlice_GetIndices");
}

PY_EXPORT PyObject* PySlice_New(PyObject* start, PyObject* stop,
                                PyObject* step) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object none(&scope, NoneType::object());
  Slice slice(&scope, thread->runtime()->newSlice(none, none, none));
  if (start != nullptr) {
    slice->setStart(ApiHandle::fromPyObject(start)->asObject());
  }
  if (stop != nullptr) {
    slice->setStop(ApiHandle::fromPyObject(stop)->asObject());
  }
  if (step != nullptr) {
    slice->setStep(ApiHandle::fromPyObject(step)->asObject());
  }
  return ApiHandle::newReference(thread, *slice);
}

PY_EXPORT Py_ssize_t PySlice_AdjustIndices(Py_ssize_t /* h */,
                                           Py_ssize_t* /* t */,
                                           Py_ssize_t* /* p */,
                                           Py_ssize_t /* p */) {
  UNIMPLEMENTED("PySlice_AdjustIndices");
}

PY_EXPORT int PySlice_GetIndicesEx(PyObject* /* r */, Py_ssize_t /* h */,
                                   Py_ssize_t* /* t */, Py_ssize_t* /* p */,
                                   Py_ssize_t* /* p */, Py_ssize_t* /* h */) {
  UNIMPLEMENTED("PySlice_GetIndicesEx");
}

PY_EXPORT int PySlice_Unpack(PyObject* /* r */, Py_ssize_t* /* t */,
                             Py_ssize_t* /* p */, Py_ssize_t* /* p */) {
  UNIMPLEMENTED("PySlice_Unpack");
}

}  // namespace python
