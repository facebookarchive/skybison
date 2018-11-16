/* moduleobject.c implementation */

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

#include "Python.h"

namespace python {

extern "C" PyObject* PyModule_Create2(struct PyModuleDef* pymodule, int) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  const char* c_name = pymodule->m_name;
  Handle<Object> name(&scope, runtime->newStringFromCString(c_name));
  Handle<Module> module(&scope, runtime->newModule(name));
  runtime->addModule(module);

  // TODO: Check m_slots
  // TODO: Set md_state
  // TODO: Validate m_methods
  // TODO: Add methods
  // TODO: Add m_doc

  return runtime->asApiHandle(*module)->asPyObject();
}

extern "C" PyObject* PyModule_GetDict(PyObject* pymodule) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<Module> module(&scope, runtime->asObject(pymodule));
  return runtime->asApiHandle(module->dictionary())->asPyObject();
}

} // namespace python
