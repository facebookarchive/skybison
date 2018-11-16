/* moduleobject.c implementation */

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

#include "Python.h"

namespace py = python;

PyObject* PyModule_Create2(struct PyModuleDef* pymodule, int) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  const char* c_name = pymodule->m_name;
  py::Handle<py::Object> name(&scope, runtime->newStringFromCString(c_name));
  py::Handle<py::Module> module(&scope, runtime->newModule(name));
  runtime->addModule(module);

  // TODO: Check m_slots
  // TODO: Set md_state
  // TODO: Validate m_methods
  // TODO: Add methods
  // TODO: Add m_doc

  return Py_AsPyObject(runtime->asApiHandle(*module));
}

PyObject* PyModule_GetDict(PyObject* pymodule) {
  py::Thread* thread = py::Thread::currentThread();
  py::Runtime* runtime = thread->runtime();
  py::HandleScope scope(thread->handles());

  py::Handle<py::Module> module(
      &scope, runtime->asObject(Py_AsApiHandle(pymodule)));
  return Py_AsPyObject(runtime->asApiHandle(module->dictionary()));
}
