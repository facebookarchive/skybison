// listobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyList_New(Py_ssize_t size) {
  if (size < 0) {
    return nullptr;
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<List> list(&scope, runtime->newList());
  Handle<ObjectArray> items(&scope, runtime->newObjectArray(size));
  list->setNumItems(size);
  list->setItems(*items);

  return ApiHandle::fromObject(*list);
}

PY_EXPORT int PyList_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isList();
}

PY_EXPORT int PyList_Check_Func(PyObject* obj) {
  if (PyList_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kList);
}

PY_EXPORT PyObject* PyList_AsTuple(PyObject* /* v */) {
  UNIMPLEMENTED("PyList_AsTuple");
}

PY_EXPORT PyObject* PyList_GetItem(PyObject* /* p */, Py_ssize_t /* i */) {
  UNIMPLEMENTED("PyList_GetItem");
}

PY_EXPORT int PyList_Reverse(PyObject* /* v */) {
  UNIMPLEMENTED("PyList_Reverse");
}

PY_EXPORT int PyList_SetItem(PyObject* /* p */, Py_ssize_t /* i */,
                             PyObject* /* m */) {
  UNIMPLEMENTED("PyList_SetItem");
}

PY_EXPORT int PyList_Append(PyObject* /* p */, PyObject* /* m */) {
  UNIMPLEMENTED("PyList_Append");
}

PY_EXPORT PyObject* PyList_GetSlice(PyObject* /* a */, Py_ssize_t /* w */,
                                    Py_ssize_t /* h */) {
  UNIMPLEMENTED("PyList_GetSlice");
}

PY_EXPORT int PyList_Insert(PyObject* /* p */, Py_ssize_t /* e */,
                            PyObject* /* m */) {
  UNIMPLEMENTED("PyList_Insert");
}

PY_EXPORT int PyList_SetSlice(PyObject* /* a */, Py_ssize_t /* w */,
                              Py_ssize_t /* h */, PyObject* /* v */) {
  UNIMPLEMENTED("PyList_SetSlice");
}

PY_EXPORT Py_ssize_t PyList_Size(PyObject* /* p */) {
  UNIMPLEMENTED("PyList_Size");
}

PY_EXPORT int PyList_Sort(PyObject* /* v */) { UNIMPLEMENTED("PyList_Sort"); }

}  // namespace python
