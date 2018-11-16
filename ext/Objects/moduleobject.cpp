/* moduleobject.c implementation */

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

#include "Python.h"

namespace py = python;

PyObject* PyModule_Create2(struct PyModuleDef* module, int) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  const char* name = module->m_name;
  py::Handle<py::Object> name_str(&scope, runtime->newStringFromCString(name));
  py::Handle<py::Module> module_obj(&scope, runtime->newModule(name_str));
  runtime->addModule(module_obj);

  // TODO: Check m_slots
  // TODO: Set md_state
  // TODO: Validate m_methods
  // TODO: Add methods
  // TODO: Add m_doc

  return Py_AsPyObject(runtime->asApiHandle(*module_obj));
}

PyObject* PyModule_GetDict(PyObject* m) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Module> module(&scope, runtime->asObject(Py_AsApiHandle(m)));
  return Py_AsPyObject(runtime->asApiHandle(module->dictionary()));
}
