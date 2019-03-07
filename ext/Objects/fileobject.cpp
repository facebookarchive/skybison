#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyFile_GetLine(PyObject* /* f */, int /* n */) {
  UNIMPLEMENTED("PyFile_GetLine");
}

PY_EXPORT int PyFile_WriteObject(PyObject* /* v */, PyObject* /* f */,
                                 int /* s */) {
  UNIMPLEMENTED("PyFile_WriteObject");
}

PY_EXPORT int PyFile_WriteString(const char* /* s */, PyObject* /* f */) {
  UNIMPLEMENTED("PyFile_WriteString");
}

PY_EXPORT int PyObject_AsFileDescriptor(PyObject* obj) {
  DCHECK(obj != nullptr, "obj must not be null");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!object.isInt()) {
    Object result(&scope, thread->invokeMethod1(object, SymbolId::kFileno));
    if (result.isError()) {
      if (!thread->hasPendingException()) {
        thread->raiseTypeErrorWithCStr(
            "argument must be an int or have a fileno() method.");
      }
      return -1;
    }
    if (!result.isInt()) {
      thread->raiseTypeErrorWithCStr("fileno() returned non-int");
      return -1;
    }
    object = *result;
  }
  auto const optint = RawInt::cast(*object).asInt<int>();
  if (optint.error == CastError::Overflow) {
    thread->raiseOverflowErrorWithCStr(
        "integer too big to convert to file descriptor");
    return -1;
  }
  int fd = optint.value;
  if (fd < 0) {
    thread->raiseValueError(thread->runtime()->newStrFromFormat(
        "file descriptor cannot be a negative integer (%i)", fd));
    return -1;
  }
  return fd;
}

PY_EXPORT char* Py_UniversalNewlineFgets(char* /* f */, int /* n */,
                                         FILE* /* m */, PyObject* /* j */) {
  UNIMPLEMENTED("Py_UniversalNewlineFgets");
}

}  // namespace python
