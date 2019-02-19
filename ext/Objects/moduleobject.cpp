// moduleobject.c implementation

#include "cpython-data.h"
#include "cpython-func.h"
#include "handles.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"

namespace python {

PY_EXPORT int PyModule_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isModule();
}

PY_EXPORT int PyModule_Check_Func(PyObject* obj) {
  return Thread::currentThread()->runtime()->isInstanceOfModule(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT PyObject* PyModule_Create2(struct PyModuleDef* def, int) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object name(&scope, runtime->newStrFromCStr(def->m_name));
  Module module(&scope, runtime->newModule(name));
  module.setDef(runtime->newIntFromCPtr(def));

  if (def->m_methods != nullptr) {
    for (PyMethodDef* fdef = def->m_methods; fdef->ml_name != nullptr; fdef++) {
      if (fdef->ml_flags & METH_CLASS || fdef->ml_flags & METH_STATIC) {
        thread->raiseValueErrorWithCStr(
            "module functions cannot set METH_CLASS or METH_STATIC");
        return nullptr;
      }
      Function function(&scope, runtime->newFunction());
      Str function_name(&scope, runtime->newStrFromCStr(fdef->ml_name));
      function.setName(*function_name);
      function.setCode(runtime->newIntFromCPtr(bit_cast<void*>(fdef->ml_meth)));
      switch (fdef->ml_flags) {
        case METH_NOARGS:
          function.setEntry(moduleTrampolineNoArgs);
          function.setEntryKw(moduleTrampolineNoArgsKw);
          function.setEntryEx(moduleTrampolineNoArgsEx);
          break;
        case METH_O:
          function.setEntry(moduleTrampolineOneArg);
          function.setEntryKw(moduleTrampolineOneArgKw);
          function.setEntryEx(moduleTrampolineOneArgEx);
          break;
        case METH_VARARGS:
          function.setEntry(moduleTrampolineVarArgs);
          function.setEntryKw(moduleTrampolineVarArgsKw);
          function.setEntryEx(moduleTrampolineVarArgsEx);
          break;
        case METH_VARARGS | METH_KEYWORDS:
          function.setEntry(moduleTrampolineKeywordArgs);
          function.setEntryKw(moduleTrampolineKeywordArgsKw);
          function.setEntryEx(moduleTrampolineKeywordArgsEx);
          break;
        case METH_FASTCALL:
          UNIMPLEMENTED("METH_FASTCALL");
        default:
          UNIMPLEMENTED(
              "Bad call flags in PyCFunction_Call. METH_OLDARGS is no longer "
              "supported!");
      }
      function.setModule(*module);
      runtime->attributeAtPut(thread, module, function_name, function);
    }
  }

  if (def->m_doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->m_doc));
    Object key(&scope, runtime->symbols()->DunderDoc());
    runtime->moduleAtPut(module, key, doc);
  }

  ApiHandle* result = ApiHandle::newReference(thread, *module);
  if (def->m_size > 0) {
    result->setCache(std::malloc(def->m_size));
  }

  // TODO(eelizondo): Check m_slots
  // TODO(eelizondo): Set md_state

  return result;
}

PY_EXPORT PyModuleDef* PyModule_GetDef(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj.isModule()) {
    // TODO(cshapiro): throw a TypeError
    return nullptr;
  }
  Module module(&scope, *module_obj);
  if (!module.def()->isInt()) {
    return nullptr;
  }
  Int def(&scope, module.def());
  return static_cast<PyModuleDef*>(def.asCPtr());
}

PY_EXPORT PyObject* PyModule_GetDict(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Module module(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  return ApiHandle::newReference(thread, module.dict());
}

PY_EXPORT PyObject* PyModule_GetNameObject(PyObject* mod) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(mod)->asObject());
  if (!module_obj.isModule()) {
    // TODO(atalaba): Allow for module subclassing
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Str key(&scope, runtime->symbols()->DunderName());
  Object name(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(*name)) {
    thread->raiseSystemErrorWithCStr("nameless module");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *name);
}

PY_EXPORT void* PyModule_GetState(PyObject* mod) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  ApiHandle* handle = ApiHandle::fromPyObject(mod);
  Object module_obj(&scope, handle->asObject());
  if (!module_obj.isModule()) {
    // TODO(atalaba): Support module subclassing
    thread->raiseBadArgument();
    return nullptr;
  }
  return handle->cache();
}

PY_EXPORT PyObject* PyModuleDef_Init(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModuleDef_Init");
}

PY_EXPORT int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

PY_EXPORT int PyModule_ExecDef(PyObject* pymodule, PyModuleDef* def) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!thread->runtime()->isInstanceOfModule(*module_obj)) {
    return -1;
  }
  Module module(&scope, *module_obj);
  return execDef(thread, module, def);
}

PY_EXPORT PyObject* PyModule_FromDefAndSpec2(struct PyModuleDef* /* f */,
                                             PyObject* /* c */, int /* n */) {
  UNIMPLEMENTED("PyModule_FromDefAndSpec2");
}

PY_EXPORT const char* PyModule_GetFilename(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilename");
}

PY_EXPORT PyObject* PyModule_GetFilenameObject(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj.isModule()) {
    // TODO(atalaba): Allow for module subclassing
    thread->raiseBadArgument();
    return nullptr;
  }
  Module module(&scope, *module_obj);
  Str key(&scope, runtime->symbols()->DunderFile());
  Object filename(&scope, runtime->moduleAt(module, key));
  if (!runtime->isInstanceOfStr(*filename)) {
    thread->raiseSystemErrorWithCStr("module filename missing");
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
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr(c_name));
  return ApiHandle::newReference(thread, runtime->newModule(name));
}

PY_EXPORT PyObject* PyModule_NewObject(PyObject* name) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object module_obj(&scope, thread->runtime()->newModule(name_obj));
  return ApiHandle::newReference(thread, *module_obj);
}

PY_EXPORT int PyModule_SetDocString(PyObject* m, const char* doc) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object module_obj(&scope, ApiHandle::fromPyObject(m)->asObject());
  Object uni(&scope, runtime->newStrFromCStr(doc));
  if (!uni.isStr() || !module_obj.isModule()) {
    return -1;
  }
  Module module(&scope, *module_obj);
  Object key(&scope, runtime->symbols()->DunderDoc());
  runtime->moduleAtPut(module, key, uni);
  return 0;
}

}  // namespace python
