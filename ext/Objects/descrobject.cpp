#include "capi-handles.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "runtime.h"

namespace python {

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
                thread->invokeFunction2(SymbolId::kBuiltins,
                                        SymbolId::kUnderDescrClassMethod,
                                        type_obj, function));
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyDictProxy_New(PyObject* /* g */) {
  UNIMPLEMENTED("PyDictProxy_New");
}

PY_EXPORT PyObject* PyDescr_NewGetSet(PyTypeObject* /* e */,
                                      PyGetSetDef* /* t */) {
  UNIMPLEMENTED("PyDescr_NewGetSet");
}

PY_EXPORT PyObject* PyDescr_NewMember(PyTypeObject* /* e */,
                                      PyMemberDef* /* r */) {
  UNIMPLEMENTED("PyDescr_NewMember");
}

PY_EXPORT PyObject* PyDescr_NewMethod(PyTypeObject* /* e */,
                                      PyMethodDef* /* d */) {
  UNIMPLEMENTED("PyDescr_NewMethod");
}

PY_EXPORT PyObject* PyWrapper_New(PyObject* /* d */, PyObject* /* f */) {
  UNIMPLEMENTED("PyWrapper_New");
}

}  // namespace python
