// moduleobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyModule_Create2(struct PyModuleDef* def, int) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  const char* c_name = def->m_name;
  Handle<Object> name(&scope, runtime->newStrFromCStr(c_name));
  Handle<Module> module(&scope, runtime->newModule(name));
  module->setDef(runtime->newIntFromCPtr(def));
  runtime->addModule(module);

  // TODO: Check m_slots
  // TODO: Set md_state
  // TODO: Validate m_methods
  // TODO: Add methods
  // TODO: Add m_doc

  return ApiHandle::fromObject(*module)->asPyObject();
}

extern "C" PyModuleDef* PyModule_GetDef(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Object> module_obj(&scope,
                            ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj->isModule()) {
    // TODO(cshapiro): throw a TypeError
    return nullptr;
  }
  Handle<Module> module(&scope, *module_obj);
  if (!module->def()->isInt()) {
    return nullptr;
  }
  Handle<Int> def(&scope, module->def());
  return static_cast<PyModuleDef*>(def->asCPtr());
}

extern "C" PyObject* PyModule_GetDict(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Module> module(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  return ApiHandle::fromObject(module->dict())->asPyObject();
}

extern "C" PyObject* PyModule_GetNameObject(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetNameObject");
}

extern "C" void* PyModule_GetState(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetState");
}

extern "C" PyObject* PyModuleDef_Init(struct PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModuleDef_Init");
}

extern "C" int PyModule_AddFunctions(PyObject* /* m */, PyMethodDef* /* s */) {
  UNIMPLEMENTED("PyModule_AddFunctions");
}

extern "C" int PyModule_ExecDef(PyObject* /* e */, PyModuleDef* /* f */) {
  UNIMPLEMENTED("PyModule_ExecDef");
}

extern "C" PyObject* PyModule_FromDefAndSpec2(struct PyModuleDef* /* f */,
                                              PyObject* /* c */, int /* n */) {
  UNIMPLEMENTED("PyModule_FromDefAndSpec2");
}

extern "C" const char* PyModule_GetFilename(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilename");
}

extern "C" PyObject* PyModule_GetFilenameObject(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetFilenameObject");
}

extern "C" const char* PyModule_GetName(PyObject* /* m */) {
  UNIMPLEMENTED("PyModule_GetName");
}

extern "C" PyObject* PyModule_New(const char* /* e */) {
  UNIMPLEMENTED("PyModule_New");
}

extern "C" PyObject* PyModule_NewObject(PyObject* /* e */) {
  UNIMPLEMENTED("PyModule_NewObject");
}

extern "C" int PyModule_SetDocString(PyObject* /* m */, const char* /* c */) {
  UNIMPLEMENTED("PyModule_SetDocString");
}

}  // namespace python
