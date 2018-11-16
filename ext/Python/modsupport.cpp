#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyModule_AddObject(PyObject* pymodule, const char* name,
                                 PyObject* obj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  if (name == nullptr) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  Object key(&scope, runtime->newStrFromCStr(name));
  if (!key->isStr()) {
    // TODO(cshapiro): throw a MemoryError
    return -1;
  }
  Module module(&scope, *module_obj);
  Object value(&scope, ApiHandle::fromPyObject(obj)->asObject());
  runtime->moduleAtPut(module, key, value);
  return 0;
}

PY_EXPORT int PyModule_AddStringConstant(PyObject* /* m */, const char* /* e */,
                                         const char* /* e */) {
  UNIMPLEMENTED("PyModule_AddStringConstant");
}

PY_EXPORT PyObject* Py_BuildValue(const char* /* t */, ...) {
  UNIMPLEMENTED("Py_BuildValue");
}

PY_EXPORT PyObject* Py_VaBuildValue(const char* /* t */, va_list /* a */) {
  UNIMPLEMENTED("Py_VaBuildValue");
}

PY_EXPORT PyObject* _Py_BuildValue_SizeT(const char* /* t */, ...) {
  UNIMPLEMENTED("_Py_BuildValue_SizeT");
}

PY_EXPORT PyObject* _Py_VaBuildValue_SizeT(const char* /* t */,
                                           va_list /* a */) {
  UNIMPLEMENTED("_Py_VaBuildValue_SizeT");
}

PY_EXPORT PyObject* PyEval_CallFunction(PyObject* /* e */, const char* /* t */,
                                        ...) {
  UNIMPLEMENTED("PyEval_CallFunction");
}

PY_EXPORT PyObject* PyEval_CallMethod(PyObject* /* j */, const char* /* e */,
                                      const char* /* t */, ...) {
  UNIMPLEMENTED("PyEval_CallMethod");
}

}  // namespace python
