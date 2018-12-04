// object.c implementation

#include "cpython-func.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyNone_Ptr() {
  return ApiHandle::borrowedReference(Thread::currentThread(),
                                      NoneType::object());
}

PY_EXPORT void _Py_Dealloc_Func(PyObject* obj) {
  ApiHandle::fromPyObject(obj)->dispose();
}

PY_EXPORT void Py_INCREF_Func(PyObject* obj) {
  ApiHandle::fromPyObject(obj)->incref();
}

PY_EXPORT void Py_DECREF_Func(PyObject* obj) {
  obj->ob_refcnt--;
  if (ApiHandle::fromPyObject(obj)->refcnt() == 0) _Py_Dealloc_Func(obj);
}

PY_EXPORT Py_ssize_t Py_REFCNT_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->refcnt();
}

PY_EXPORT int PyObject_GenericSetAttr(PyObject* obj, PyObject* name,
                                      PyObject* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!object->isHeapObject()) return -1;

  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  runtime->attributeAtPut(thread, object, name_obj, value_obj);

  // An error was produced. No value was set.
  if (thread->hasPendingException()) {
    return -1;
  }

  return 0;
}

PY_EXPORT PyObject* PyObject_GenericGetAttr(PyObject* obj, PyObject* name) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!object->isHeapObject()) return nullptr;

  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object result(&scope, runtime->attributeAt(thread, object, name_obj));

  // An error was produced. No value was returned.
  if (thread->hasPendingException() || result->isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_CallFinalizerFromDealloc(PyObject* /* f */) {
  UNIMPLEMENTED("PyObject_CallFinalizerFromDealloc");
}

PY_EXPORT PyVarObject* PyObject_InitVar(PyVarObject* /* p */,
                                        PyTypeObject* /* p */,
                                        Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyObject_InitVar");
}

PY_EXPORT int PyObject_IsTrue(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_IsTrue");
}

PY_EXPORT int PyCallable_Check(PyObject* /* x */) {
  UNIMPLEMENTED("PyCallable_Check");
}

PY_EXPORT PyObject* PyObject_ASCII(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_ASCII");
}

PY_EXPORT PyObject* PyObject_Bytes(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Bytes");
}

PY_EXPORT PyObject* PyObject_Dir(PyObject* /* j */) {
  UNIMPLEMENTED("PyObject_Dir");
}

PY_EXPORT int PyObject_GenericSetDict(PyObject* /* j */, PyObject* /* e */,
                                      void* /* t */) {
  UNIMPLEMENTED("PyObject_GenericSetDict");
}

PY_EXPORT PyObject* PyObject_GetAttr(PyObject* v, PyObject* name) {
  // TODO(miro): This does not support custom attribute getter set by
  // __getattr__ or tp_getattro slot
  return PyObject_GenericGetAttr(v, name);
}

PY_EXPORT PyObject* PyObject_GetAttrString(PyObject* v, const char* name) {
  PyObject* str = PyUnicode_FromString(name);
  if (str == nullptr) return nullptr;

  PyObject* attr = PyObject_GetAttr(v, str);
  Py_DECREF(str);
  return attr;
}

PY_EXPORT int PyObject_SetAttr(PyObject* v, PyObject* name, PyObject* w) {
  // TODO(miro): This does not support custom attribute setter set by
  // __setattr__ or tp_setattro slot
  return PyObject_GenericSetAttr(v, name, w);
}

PY_EXPORT int PyObject_SetAttrString(PyObject* v, const char* name,
                                     PyObject* w) {
  PyObject* str = PyUnicode_FromString(name);
  if (str == nullptr) return -1;

  int result = PyObject_SetAttr(v, str, w);
  Py_DECREF(str);
  return result;
}

PY_EXPORT int PyObject_HasAttr(PyObject* /* v */, PyObject* /* e */) {
  UNIMPLEMENTED("PyObject_HasAttr");
}

PY_EXPORT int PyObject_HasAttrString(PyObject* /* v */, const char* /* e */) {
  UNIMPLEMENTED("PyObject_HasAttrString");
}

PY_EXPORT Py_hash_t PyObject_Hash(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Hash");
}

PY_EXPORT Py_hash_t PyObject_HashNotImplemented(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_HashNotImplemented");
}

PY_EXPORT PyObject* PyObject_Init(PyObject* /* p */, PyTypeObject* /* p */) {
  UNIMPLEMENTED("PyObject_Init");
}

PY_EXPORT int PyObject_Not(PyObject* /* v */) { UNIMPLEMENTED("PyObject_Not"); }

PY_EXPORT PyObject* PyObject_Repr(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Repr");
}

PY_EXPORT PyObject* PyObject_RichCompare(PyObject* /* v */, PyObject* /* w */,
                                         int /* p */) {
  UNIMPLEMENTED("PyObject_RichCompare");
}

PY_EXPORT int PyObject_RichCompareBool(PyObject* /* v */, PyObject* /* w */,
                                       int /* p */) {
  UNIMPLEMENTED("PyObject_RichCompareBool");
}

PY_EXPORT PyObject* PyObject_SelfIter(PyObject* /* j */) {
  UNIMPLEMENTED("PyObject_SelfIter");
}

PY_EXPORT PyObject* PyObject_Str(PyObject* /* v */) {
  UNIMPLEMENTED("PyObject_Str");
}

PY_EXPORT void Py_DecRef(PyObject* obj) {
  if (obj == nullptr) return;
  Py_DECREF_Func(obj);
}

PY_EXPORT void Py_IncRef(PyObject* obj) {
  if (obj == nullptr) return;
  Py_INCREF_Func(obj);
}

PY_EXPORT int Py_ReprEnter(PyObject* /* j */) { UNIMPLEMENTED("Py_ReprEnter"); }

PY_EXPORT void Py_ReprLeave(PyObject* /* j */) {
  UNIMPLEMENTED("Py_ReprLeave");
}

PY_EXPORT PyObject* _PyObject_New(PyTypeObject* /* p */) {
  UNIMPLEMENTED("_PyObject_New");
}

PY_EXPORT PyVarObject* _PyObject_NewVar(PyTypeObject* /* p */,
                                        Py_ssize_t /* s */) {
  UNIMPLEMENTED("_PyObject_NewVar");
}

PY_EXPORT void _PyTrash_deposit_object(PyObject* /* p */) {
  UNIMPLEMENTED("_PyTrash_deposit_object");
}

PY_EXPORT void _PyTrash_destroy_chain() {
  UNIMPLEMENTED("_PyTrash_destroy_chain");
}

PY_EXPORT void _PyTrash_thread_deposit_object(PyObject* /* p */) {
  UNIMPLEMENTED("_PyTrash_thread_deposit_object");
}

PY_EXPORT void _PyTrash_thread_destroy_chain() {
  UNIMPLEMENTED("_PyTrash_thread_destroy_chain");
}

PY_EXPORT int PyObject_Print(PyObject* /* p */, FILE* /* p */, int /* s */) {
  UNIMPLEMENTED("PyObject_Print");
}

PY_EXPORT PyObject* _PyObject_GetAttrId(PyObject* /* v */,
                                        _Py_Identifier* /* e */) {
  UNIMPLEMENTED("_PyObject_GetAttrId");
}

PY_EXPORT int _PyObject_HasAttrId(PyObject* /* v */, _Py_Identifier* /* e */) {
  UNIMPLEMENTED("_PyObject_HasAttrId");
}

PY_EXPORT int _PyObject_SetAttrId(PyObject* /* v */, _Py_Identifier* /* e */,
                                  PyObject* /* w */) {
  UNIMPLEMENTED("_PyObject_SetAttrId");
}

}  // namespace python
