// listobject.c implementation

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

extern "C" PyObject* PyList_New(Py_ssize_t size) {
  if (size < 0) {
    return nullptr;
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  Handle<List> list(&scope, runtime->newList());
  Handle<ObjectArray> items(&scope, runtime->newObjectArray(size));
  list->setAllocated(size);
  list->setItems(*items);

  return runtime->asApiHandle(*list)->asPyObject();
}

} // namespace python
