// object.c implementation

#include "cpython-func.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyNone_Ptr() {
  return ApiHandle::fromObject(None::object())->asPyObject();
}

extern "C" void _Py_Dealloc_Func(PyObject* obj) { std::free(obj); }

extern "C" int PyObject_GenericSetAttr(PyObject* obj, PyObject* name,
                                       PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!(object->isType() || object->isModule() || object->isInstance())) {
    return -1;
  }

  Handle<Object> name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Handle<Object> value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  runtime->attributeAtPut(thread, object, name_obj, value_obj);

  // An error was produced. No value was set.
  if (thread->hasPendingException()) {
    return -1;
  }

  return 0;
}

extern "C" PyObject* PyObject_GenericGetAttr(PyObject* obj, PyObject* name) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!(object->isType() || object->isModule() || object->isInstance())) {
    return nullptr;
  }

  Handle<Object> name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Handle<Object> result(&scope, runtime->attributeAt(thread, object, name_obj));

  // An error was produced. No value was returned.
  if (thread->hasPendingException() || result->isError()) {
    return nullptr;
  }
  return ApiHandle::fromBorrowedObject(*result)->asPyObject();
}

}  // namespace python
