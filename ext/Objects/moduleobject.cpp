// moduleobject.c implementation

#include "cpython-data.h"
#include "cpython-func.h"

#include "builtins-module.h"
#include "capi-handles.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "handles.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"

namespace py {

PY_EXPORT int PyModule_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isModule();
}

PY_EXPORT int PyModule_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfModule(
      ApiHandle::fromPyObject(obj)->asObject());
}

static void moduleDefInit(PyModuleDef* def) {
  if (def->m_base.m_index != 0) return;
  def->m_base.m_index = Runtime::nextModuleIndex();
}

PY_EXPORT PyObject* PyModule_Create2(PyModuleDef* def, int) {
  moduleDefInit(def);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, Runtime::internStrFromCStr(thread, def->m_name));
  Module module(&scope, runtime->newModule(module_name));
  module.setDef(runtime->newIntFromCPtr(def));

  if (def->m_methods != nullptr) {
    Object function_name(&scope, NoneType::object());
    Object function(&scope, NoneType::object());
    for (PyMethodDef* method = def->m_methods; method->ml_name != nullptr;
         method++) {
      if (method->ml_flags & METH_CLASS || method->ml_flags & METH_STATIC) {
        thread->raiseWithFmt(
            LayoutId::kValueError,
            "module functions cannot set METH_CLASS or METH_STATIC");
        return nullptr;
      }
      function_name = Runtime::internStrFromCStr(thread, method->ml_name);
      function =
          newCFunction(thread, method, function_name, module, module_name);
      moduleAtPut(thread, module, function_name, function);
    }
  }

  if (def->m_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->m_doc));
    moduleAtPutById(thread, module, ID(__doc__), doc);
  }

  Py_ssize_t m_size = def->m_size;
  void* state = nullptr;
  if (m_size > 0) {
    state = std::calloc(1, m_size);
    if (state == nullptr) {
      PyErr_NoMemory();
      return nullptr;
    }
  }
  module.setState(runtime->newIntFromCPtr(state));

  // TODO(eelizondo): Check m_slots
  // TODO(eelizondo): Set md_state
  return ApiHandle::newReference(thread, *module);
}

PY_EXPORT PyModuleDef* PyModule_GetDef(PyObject* pymodule) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Int def(&scope, module.def());
  return static_cast<PyModuleDef*>(def.asCPtr());
}

PY_EXPORT PyObject* PyModule_GetDict(PyObject* pymodule) {
  // Return the module_proxy object. Note that this is not a `PyDict` instance
  // so contrary to cpython it will not work with `PyDict_xxx` functions. It
  // does work with PyEval_EvalCode.
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  return ApiHandle::borrowedReference(thread, module.moduleProxy());
}

PY_EXPORT PyObject* PyModule_GetNameObject(PyObject* mod) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(mod)->asObject());
  if (!runtime->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Object name(&scope, moduleAtById(thread, module, ID(__name__)));
  if (!runtime->isInstanceOfStr(*name)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "nameless module");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *name);
}

PY_EXPORT void* PyModule_GetState(PyObject* mod) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ApiHandle* handle = ApiHandle::fromPyObject(mod);
  Object module_obj(&scope, handle->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  return Int::cast(module.state()).asCPtr();
}

PY_EXPORT PyObject* PyModuleDef_Init(PyModuleDef* def) {
  moduleDefInit(def);
  return reinterpret_cast<PyObject*>(def);
}

PY_EXPORT int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

PY_EXPORT int PyModule_ExecDef(PyObject* pymodule, PyModuleDef* def) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    return -1;
  }
  Module module(&scope, *module_obj);
  return execDef(thread, module, def);
}

PY_EXPORT PyObject* PyModule_FromDefAndSpec2(PyModuleDef* /* f */,
                                             PyObject* /* c */, int /* n */) {
  UNIMPLEMENTED("PyModule_FromDefAndSpec2");
}

PY_EXPORT const char* PyModule_GetFilename(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilename");
}

PY_EXPORT PyObject* PyModule_GetFilenameObject(PyObject* pymodule) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!runtime->isInstanceOfModule(*module_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Object filename(&scope, moduleAtById(thread, module, ID(__file__)));
  if (!runtime->isInstanceOfStr(*filename)) {
    thread->raiseWithFmt(LayoutId::kSystemError, "module filename missing");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *filename);
}

PY_EXPORT const char* PyModule_GetName(PyObject* pymodule) {
  PyObject* name = PyModule_GetNameObject(pymodule);
  if (name == nullptr) return nullptr;
  Py_DECREF(name);
  return PyUnicode_AsUTF8(name);
}

PY_EXPORT PyObject* PyModule_New(const char* c_name) {
  DCHECK(c_name != nullptr, "PyModule_New takes a valid string");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr(c_name));
  return ApiHandle::newReference(thread, runtime->newModule(name));
}

PY_EXPORT PyObject* PyModule_NewObject(PyObject* name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object module_obj(&scope, thread->runtime()->newModule(name_obj));
  return ApiHandle::newReference(thread, *module_obj);
}

PY_EXPORT int PyModule_SetDocString(PyObject* m, const char* doc) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object module(&scope, ApiHandle::fromPyObject(m)->asObject());
  Object uni(&scope, runtime->newStrFromCStr(doc));
  if (!uni.isStr()) {
    return -1;
  }
  Object name(&scope, runtime->symbols()->at(ID(__doc__)));
  if (thread->invokeMethod3(module, ID(__setattr__), name, uni)
          .isErrorException()) {
    return -1;
  }
  return 0;
}

PY_EXPORT PyTypeObject* PyModule_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kModule)));
}

}  // namespace py
