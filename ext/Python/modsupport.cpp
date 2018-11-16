#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" int PyModule_AddObject(PyObject* pymodule, const char* name,
                                  PyObject* obj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Handle<Object> module_obj(&scope,
                            ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  if (name == nullptr) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  Handle<Object> key(&scope, runtime->newStrFromCStr(name));
  if (!key->isStr()) {
    // TODO(cshapiro): throw a MemoryError
    return -1;
  }
  Handle<Module> module(&scope, *module_obj);
  Handle<Object> value(&scope, ApiHandle::fromPyObject(obj)->asObject());
  runtime->moduleAtPut(module, key, value);
  return 0;
}

extern "C" int PyModule_AddStringConstant(PyObject* /* m */,
                                          const char* /* e */,
                                          const char* /* e */) {
  UNIMPLEMENTED("PyModule_AddStringConstant");
}

extern "C" PyObject* Py_BuildValue(const char* /* t */, ...) {
  UNIMPLEMENTED("Py_BuildValue");
}

extern "C" PyObject* Py_VaBuildValue(const char* /* t */, va_list /* a */) {
  UNIMPLEMENTED("Py_VaBuildValue");
}

extern "C" PyObject* _Py_BuildValue_SizeT(const char* /* t */, ...) {
  UNIMPLEMENTED("_Py_BuildValue_SizeT");
}

extern "C" PyObject* _Py_VaBuildValue_SizeT(const char* /* t */,
                                            va_list /* a */) {
  UNIMPLEMENTED("_Py_VaBuildValue_SizeT");
}

extern "C" PyObject* PyEval_CallFunction(PyObject* /* e */, const char* /* t */,
                                         ...) {
  UNIMPLEMENTED("PyEval_CallFunction");
}

extern "C" PyObject* PyEval_CallMethod(PyObject* /* j */, const char* /* e */,
                                       const char* /* t */, ...) {
  UNIMPLEMENTED("PyEval_CallMethod");
}

}  // namespace python
