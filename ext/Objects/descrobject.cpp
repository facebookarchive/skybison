#include "cpython-data.h"
#include "cpython-func.h"
#include "function-builtins.h"
#include "runtime.h"

namespace python {

static Function::ExtensionType flagToFunctionType(int flag) {
  switch (flag) {
    case METH_NOARGS:
      return Function::ExtensionType::kMethNoArgs;
    case METH_O:
      return Function::ExtensionType::kMethO;
    case METH_VARARGS:
      return Function::ExtensionType::kMethVarArgs;
    case METH_VARARGS | METH_KEYWORDS:
      return Function::ExtensionType::kMethVarArgsAndKeywords;
    default:
      UNIMPLEMENTED("Unsupported function type");
  }
}

PY_EXPORT PyObject* PyDescr_NewClassMethod(PyTypeObject* type,
                                           PyMethodDef* def) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  Object function(
      &scope,
      functionFromMethodDef(thread, def->ml_name, bit_cast<void*>(def->ml_meth),
                            def->ml_doc, flagToFunctionType(def->ml_flags)));
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
