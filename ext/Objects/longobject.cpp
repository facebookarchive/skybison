// longobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" int PyLong_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isInt();
}

extern "C" int PyLong_Check_Func(PyObject* obj) {
  if (PyLong_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kInt);
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

extern "C" long PyLong_AsLong(PyObject* pylong) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pylong == nullptr) {
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return -1;
  }

  Handle<Object> longobj(&scope, ApiHandle::fromPyObject(pylong)->asObject());
  if (!longobj->isInt()) {
    // TODO: Handle calling __int__ on pylong
    return -1;
  }
  Handle<Int> integer(&scope, *longobj);

  // TODO: Handle overflows
  return integer->asWord();
}

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

extern "C" long PyLong_AsLongAndOverflow(PyObject* /* v */, int* /* w */) {
  UNIMPLEMENTED("PyLong_AsLongAndOverflow");
}

extern "C" PyObject* PyLong_FromDouble(double /* l */) {
  UNIMPLEMENTED("PyLong_FromDouble");
}

extern "C" PyObject* PyLong_FromSize_t(size_t /* l */) {
  UNIMPLEMENTED("PyLong_FromSize_t");
}

extern "C" PyObject* PyLong_FromString(const char* /* r */, char** /* pend */,
                                       int /* e */) {
  UNIMPLEMENTED("PyLong_FromString");
}

extern "C" double PyLong_AsDouble(PyObject* /* v */) {
  UNIMPLEMENTED("PyLong_AsDouble");
}

extern "C" long long PyLong_AsLongLongAndOverflow(PyObject* /* v */,
                                                  int* /* w */) {
  UNIMPLEMENTED("PyLong_AsLongLongAndOverflow");
}

extern "C" size_t PyLong_AsSize_t(PyObject* /* v */) {
  UNIMPLEMENTED("PyLong_AsSize_t");
}

extern "C" unsigned long long PyLong_AsUnsignedLongLongMask(PyObject* /* p */) {
  UNIMPLEMENTED("PyLong_AsUnsignedLongLongMask");
}

extern "C" unsigned long PyLong_AsUnsignedLongMask(PyObject* /* p */) {
  UNIMPLEMENTED("PyLong_AsUnsignedLongMask");
}

extern "C" void* PyLong_AsVoidPtr(PyObject* /* v */) {
  UNIMPLEMENTED("PyLong_AsVoidPtr");
}

extern "C" PyObject* PyLong_FromVoidPtr(void* /* p */) {
  UNIMPLEMENTED("PyLong_FromVoidPtr");
}

extern "C" PyObject* PyLong_GetInfo(void) { UNIMPLEMENTED("PyLong_GetInfo"); }

}  // namespace python
