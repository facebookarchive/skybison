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

extern "C" int PyObject_CallFinalizerFromDealloc(PyObject* /* f */) {
  UNIMPLEMENTED("PyObject_CallFinalizerFromDealloc");
}

extern "C" PyVarObject* PyObject_InitVar(PyVarObject* /* p */,
                                         PyTypeObject* /* p */,
                                         Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyObject_InitVar");
}

extern "C" int PyObject_IsTrue(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_IsTrue");
}

extern "C" int PyCallable_Check(PyObject* /* x */) {
  UNIMPLEMENTED("PyCallable_Check");
}

extern "C" PyObject* PyObject_ASCII(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_ASCII");
}

extern "C" PyObject* PyObject_Bytes(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Bytes");
}

extern "C" PyObject* PyObject_Dir(PyObject* /* j */) {
  UNIMPLEMENTED("PyObject_Dir");
}

extern "C" int PyObject_GenericSetDict(PyObject* /* j */, PyObject* /* e */,
                                       void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericSetDict");
}

extern "C" PyObject* PyObject_GetAttr(PyObject* /* v */, PyObject* /* e */) {
  UNIMPLEMENTED("PyObject_GetAttr");
}

extern "C" PyObject* PyObject_GetAttrString(PyObject* /* v */,
                                            const char* /* e */) {
  UNIMPLEMENTED("PyObject_GetAttrString");
}

extern "C" int PyObject_HasAttr(PyObject* /* v */, PyObject* /* e */) {
  UNIMPLEMENTED("PyObject_HasAttr");
}

extern "C" int PyObject_HasAttrString(PyObject* /* v */, const char* /* e */) {
  UNIMPLEMENTED("PyObject_HasAttrString");
}

extern "C" Py_hash_t PyObject_Hash(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Hash");
}

extern "C" Py_hash_t PyObject_HashNotImplemented(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_HashNotImplemented");
}

extern "C" PyObject* PyObject_Init(PyObject* /* p */, PyTypeObject* /* p */) {
  UNIMPLEMENTED("PyObject_Init");
}

extern "C" int PyObject_Not(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Not");
}

extern "C" PyObject* PyObject_Repr(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Repr");
}

extern "C" PyObject* PyObject_RichCompare(PyObject* /* v */, PyObject* /* w */,
                                          int /* p */) {
  UNIMPLEMENTED("PyObject_RichCompare");
}

extern "C" int PyObject_RichCompareBool(PyObject* /* v */, PyObject* /* w */,
                                        int /* p */) {
  UNIMPLEMENTED("PyObject_RichCompareBool");
}

extern "C" PyObject* PyObject_SelfIter(PyObject* /* j */) {
  UNIMPLEMENTED("PyObject_SelfIter");
}

extern "C" int PyObject_SetAttr(PyObject* /* v */, PyObject* /* e */,
                                PyObject* /* e */) {
  UNIMPLEMENTED("PyObject_SetAttr");
}

extern "C" int PyObject_SetAttrString(PyObject* /* v */, const char* /* e */,
                                      PyObject* /* w */) {
  UNIMPLEMENTED("PyObject_SetAttrString");
}

extern "C" PyObject* PyObject_Str(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Str");
}

extern "C" void Py_DecRef(PyObject* /* o */) { UNIMPLEMENTED("Py_DecRef"); }

extern "C" void Py_IncRef(PyObject* /* o */) { UNIMPLEMENTED("Py_IncRef"); }

extern "C" int Py_ReprEnter(PyObject* /* j */) {
  UNIMPLEMENTED("Py_ReprEnter");
}

extern "C" void Py_ReprLeave(PyObject* /* j */) {
  UNIMPLEMENTED("Py_ReprLeave");
}

extern "C" PyObject* _PyObject_New(PyTypeObject* /* p */) {
  UNIMPLEMENTED("_PyObject_New");
}

extern "C" PyVarObject* _PyObject_NewVar(PyTypeObject* /* p */,
                                         Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_NewVar");
}

extern "C" void _PyTrash_deposit_object(PyObject* /* p */) {
  UNIMPLEMENTED("_PyTrash_deposit_object");
}

extern "C" void _PyTrash_destroy_chain(void) {
  UNIMPLEMENTED("_PyTrash_destroy_chain");
}

extern "C" void _PyTrash_thread_deposit_object(PyObject* /* p */) {
  UNIMPLEMENTED("_PyTrash_thread_deposit_object");
}

extern "C" void _PyTrash_thread_destroy_chain(void) {
  UNIMPLEMENTED("_PyTrash_thread_destroy_chain");
}

extern "C" int PyObject_Print(PyObject* /* p */, FILE* /* p */, int /* s */) {
  UNIMPLEMENTED("PyObject_Print");
}

}  // namespace python
