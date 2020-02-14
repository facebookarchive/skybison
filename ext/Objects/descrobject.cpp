#include "cpython-data.h"
#include "cpython-func.h"

#include "capi-handles.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyDictProxy_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kMappingProxy)));
}

PY_EXPORT PyObject* PyDescr_NewClassMethod(PyTypeObject* type,
                                           PyMethodDef* def) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object type_obj(&scope, ApiHandle::fromPyTypeObject(type)->asObject());
  Object function(&scope, functionFromMethodDef(
                              thread, def->ml_name,
                              bit_cast<void*>(def->ml_meth), def->ml_doc,
                              methodTypeFromMethodFlags(
                                  def->ml_flags & ~METH_CLASS & ~METH_STATIC)));
  DCHECK(!function.isError(), "should have ignored METH_CLASS and METH_STATIC");
  Object result(&scope,
                thread->invokeFunction2(ID(builtins), ID(_descrclassmethod),
                                        type_obj, function));
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyDictProxy_New(PyObject* mapping) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object mapping_obj(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  Object result(&scope, thread->invokeFunction1(ID(builtins), ID(mappingproxy),
                                                mapping_obj));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyDescr_NewGetSet(PyTypeObject* /* e */,
                                      PyGetSetDef* /* t */) {
  UNIMPLEMENTED("PyDescr_NewGetSet");
}

PY_EXPORT PyObject* PyDescr_NewMember(PyTypeObject* /* e */,
                                      PyMemberDef* /* r */) {
  UNIMPLEMENTED("PyDescr_NewMember");
}

PY_EXPORT PyObject* PyDescr_NewMethod(PyTypeObject* /* type */,
                                      PyMethodDef* def) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object function(&scope, functionFromMethodDef(
                              thread, def->ml_name,
                              bit_cast<void*>(def->ml_meth), def->ml_doc,
                              methodTypeFromMethodFlags(
                                  def->ml_flags & ~METH_CLASS & ~METH_STATIC)));
  DCHECK(!function.isError(), "should have ignored METH_CLASS and METH_STATIC");
  return ApiHandle::newReference(thread, *function);
}

PY_EXPORT PyTypeObject* PyProperty_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kProperty)));
}

PY_EXPORT PyObject* PyWrapper_New(PyObject* /* d */, PyObject* /* f */) {
  UNIMPLEMENTED("PyWrapper_New");
}

}  // namespace py
