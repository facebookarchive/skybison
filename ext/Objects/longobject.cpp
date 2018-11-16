// longobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

void PyLong_Type_Init() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pylong_type =
      ApiTypeHandle::newTypeHandle("long", pytype_type);

  runtime->addBuiltinTypeHandle(pylong_type);
}

extern "C" PyTypeObject* PyLong_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kLong));
}

extern "C" PyObject* PyLong_FromLong(long ival) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  word val = reinterpret_cast<word>(ival);
  Handle<Object> value(&scope, runtime->newInt(val));
  return ApiHandle::fromObject(*value)->asPyObject();
}

extern "C" PyObject* PyLong_FromLongLong(long long) {
  UNIMPLEMENTED("PyLong_FromLongLong");
}

extern "C" PyObject* PyLong_FromUnsignedLong(unsigned long) {
  UNIMPLEMENTED("PyLong_FromUnsignedLong");
}

extern "C" PyObject* PyLong_FromUnsignedLongLong(unsigned long long) {
  UNIMPLEMENTED("PyLong_FromUnsignedLongLong");
}

extern "C" PyObject* PyLong_FromSsize_t(Py_ssize_t) {
  UNIMPLEMENTED("PyLong_FromSsize_t");
}

extern "C" long PyLong_AsLong(PyObject*) { UNIMPLEMENTED("PyLong_AsLong"); }

extern "C" long long PyLong_AsLongLong(PyObject*) {
  UNIMPLEMENTED("PYLong_AsLongLong");
}

extern "C" unsigned long PyLong_AsUnsignedLong(PyObject*) {
  UNIMPLEMENTED("PyLong_AsUnsignedLong");
}

extern "C" unsigned long long PyLong_AsUnsignedLongLong(PyObject*) {
  UNIMPLEMENTED("PyLong_AsUnsignedLongLong");
}

extern "C" Py_ssize_t PyLong_AsSsize_t(PyObject*) {
  UNIMPLEMENTED("PyLong_AsSsize_t");
}

}  // namespace python
