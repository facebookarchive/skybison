// listobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyList_New(Py_ssize_t size) {
  if (size < 0) {
    return nullptr;
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<List> list(&scope, runtime->newList());
  Handle<ObjectArray> items(&scope, runtime->newObjectArray(size));
  list->setAllocated(size);
  list->setItems(*items);

  return ApiHandle::fromObject(*list)->asPyObject();
}

}  // namespace python
