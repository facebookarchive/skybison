#include "runtime.h"
#include "tuple-builtins.h"

namespace python {

struct PyCodeObject;

PY_EXPORT int PyCode_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isCode();
}

PY_EXPORT PyCodeObject* PyCode_New(int argcount, int kwonlyargcount,
                                   int nlocals, int stacksize, int flags,
                                   PyObject* code, PyObject* consts,
                                   PyObject* names, PyObject* varnames,
                                   PyObject* freevars, PyObject* cellvars,
                                   PyObject* filename, PyObject* name,
                                   int firstlineno, PyObject* lnotab) {
  Thread* thread = Thread::current();
  if (argcount < 0 || kwonlyargcount < 0 || nlocals < 0 || code == nullptr ||
      consts == nullptr || names == nullptr || varnames == nullptr ||
      freevars == nullptr || cellvars == nullptr || name == nullptr ||
      filename == nullptr || lnotab == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object consts_obj(&scope, ApiHandle::fromPyObject(consts)->asObject());
  Object names_obj(&scope, ApiHandle::fromPyObject(names)->asObject());
  Object varnames_obj(&scope, ApiHandle::fromPyObject(varnames)->asObject());
  Object freevars_obj(&scope, ApiHandle::fromPyObject(freevars)->asObject());
  Object cellvars_obj(&scope, ApiHandle::fromPyObject(cellvars)->asObject());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object filename_obj(&scope, ApiHandle::fromPyObject(filename)->asObject());
  Object lnotab_obj(&scope, ApiHandle::fromPyObject(lnotab)->asObject());
  Object code_obj(&scope, ApiHandle::fromPyObject(code)->asObject());
  Runtime* runtime = thread->runtime();
  // Check argument types
  // TODO(emacs): Call equivalent of PyObject_CheckReadBuffer(code) instead of
  // isInstanceOfBytes
  if (!runtime->isInstanceOfBytes(*code_obj) ||
      !runtime->isInstanceOfTuple(*consts_obj) ||
      !runtime->isInstanceOfTuple(*names_obj) ||
      !runtime->isInstanceOfTuple(*varnames_obj) ||
      !runtime->isInstanceOfTuple(*freevars_obj) ||
      !runtime->isInstanceOfTuple(*cellvars_obj) ||
      !runtime->isInstanceOfStr(*name_obj) ||
      !runtime->isInstanceOfStr(*filename_obj) ||
      !runtime->isInstanceOfBytes(*lnotab_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  return reinterpret_cast<PyCodeObject*>(ApiHandle::newReference(
      thread,
      runtime->newCode(argcount, kwonlyargcount, nlocals, stacksize, flags,
                       code_obj, consts_obj, names_obj, varnames_obj,
                       freevars_obj, cellvars_obj, filename_obj, name_obj,
                       firstlineno, lnotab_obj)));
}

PY_EXPORT Py_ssize_t PyCode_GetNumFree_Func(PyObject*) {
  UNIMPLEMENTED("PyCode_GetNumFree_Func");
}

PY_EXPORT PyObject* PyCode_GetName_Func(PyObject*) {
  UNIMPLEMENTED("PyCode_GetName_Func");
}

PY_EXPORT PyObject* PyCode_GetFreevars_Func(PyObject*) {
  UNIMPLEMENTED("PyCode_GetVars_Func");
}

PY_EXPORT PyObject* _PyCode_ConstantKey(PyObject*) {
  UNIMPLEMENTED("_PyCode_ConstantKey");
}

}  // namespace python
