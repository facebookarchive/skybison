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
  module->setDef(runtime->newIntFromCPointer(def));
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
  return static_cast<PyModuleDef*>(def->asCPointer());
}

extern "C" PyObject* PyModule_GetDict(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Handle<Module> module(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  return ApiHandle::fromObject(module->dict())->asPyObject();
}

}  // namespace python
