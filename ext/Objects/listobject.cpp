// listobject.c implementation

#include "Python.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

void PyList_Type_Init(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pylist_type =
      ApiTypeHandle::newTypeHandle("list", pytype_type);

  runtime->addBuiltinTypeHandle(pylist_type);
}

extern "C" PyTypeObject* PyList_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kList));
}

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
