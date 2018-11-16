// unicodeobject.c implementation

#include "Python.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

void PyUnicode_Type_Init(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pyunicode_type =
      ApiTypeHandle::newTypeHandle("str", pytype_type);

  runtime->addBuiltinTypeHandle(pyunicode_type);
}

extern "C" PyTypeObject* PyUnicode_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kStr));
}

extern "C" PyObject* PyUnicode_FromString(const char* c_string) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> value(&scope, runtime->newStringFromCString(c_string));
  return ApiHandle::fromObject(*value)->asPyObject();
}

extern "C" PyObject* _PyUnicode_FromId(_Py_Identifier* id) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> result(&scope, runtime->internStringFromCString(id->string));
  return ApiHandle::fromObject(*result)->asPyObject();
}

extern "C" char* PyUnicode_AsUTF8AndSize(PyObject* pyunicode,
                                         Py_ssize_t* size) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pyunicode == nullptr) {
    PyErr_BadArgument();
    return nullptr;
  }

  Handle<Object> str_obj(&scope,
                         ApiHandle::fromPyObject(pyunicode)->asObject());
  if (!str_obj->isString()) {
    return nullptr;
  }

  Handle<String> str(&scope, *str_obj);
  word length = str->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  str->copyTo(result, length);
  result[length] = '\0';
  if (size) {
    *size = length;
  }
  return reinterpret_cast<char*>(result);
}

extern "C" PyObject* PyUnicode_FromStringAndSize(const char*, Py_ssize_t) {
  UNIMPLEMENTED("PyUnicode_FromStringAndSize");
}

}  // namespace python
