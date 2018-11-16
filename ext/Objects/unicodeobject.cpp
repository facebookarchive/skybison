// unicodeobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

extern "C" PyObject* PyUnicode_FromString(const char* c_string) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> value(&scope, runtime->newStrFromCStr(c_string));
  return ApiHandle::fromObject(*value)->asPyObject();
}

extern "C" char* PyUnicode_AsUTF8AndSize(PyObject* pyunicode,
                                         Py_ssize_t* size) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pyunicode == nullptr) {
    UNIMPLEMENTED("PyErr_BadArgument");
    return nullptr;
  }

  Handle<Object> str_obj(&scope,
                         ApiHandle::fromPyObject(pyunicode)->asObject());
  if (!str_obj->isStr()) {
    return nullptr;
  }

  Handle<Str> str(&scope, *str_obj);
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
