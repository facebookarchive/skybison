#include "cpython-data.h"
#include "cpython-func.h"

#include "capi-handles.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyDictProxy_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kMappingProxy)));
}

PY_EXPORT PyObject* PyDescr_NAME_Func(PyObject*) {
  UNIMPLEMENTED("PyDescr_NAME");
}

PY_EXPORT PyObject* PyDescr_NewClassMethod(PyTypeObject* type,
                                           PyMethodDef* method) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, method->ml_name));
  Object type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  return ApiHandle::newReference(
      thread->runtime(), newClassMethod(thread, method, name, type_obj));
}

PY_EXPORT PyObject* PyDictProxy_New(PyObject* mapping) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object mapping_obj(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  Object result(&scope, thread->invokeFunction1(ID(builtins), ID(mappingproxy),
                                                mapping_obj));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread->runtime(), *result);
}

PY_EXPORT PyObject* PyDescr_NewGetSet(PyTypeObject* /* e */,
                                      PyGetSetDef* /* t */) {
  UNIMPLEMENTED("PyDescr_NewGetSet");
}

PY_EXPORT PyObject* PyDescr_NewMember(PyTypeObject* /* e */,
                                      PyMemberDef* /* r */) {
  UNIMPLEMENTED("PyDescr_NewMember");
}

PY_EXPORT PyObject* PyDescr_NewMethod(PyTypeObject* type, PyMethodDef* method) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, method->ml_name));
  Object type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  return ApiHandle::newReference(thread->runtime(),
                                 newMethod(thread, method, name, type_obj));
}

PY_EXPORT PyTypeObject* PyProperty_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kProperty)));
}

PY_EXPORT PyObject* PyWrapper_New(PyObject* /* d */, PyObject* /* f */) {
  UNIMPLEMENTED("PyWrapper_New");
}

}  // namespace py
