#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyFile_GetLine(PyObject* /* f */, int /* n */) {
  UNIMPLEMENTED("PyFile_GetLine");
}

PY_EXPORT int PyFile_WriteObject(PyObject* pyobj, PyObject* pyfile, int flags) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  if (pyfile == nullptr) {
    thread->raiseTypeErrorWithCStr("writeobject with NULL file");
    return -1;
  }

  Object file(&scope, ApiHandle::fromPyObject(pyfile)->asObject());
  Runtime* runtime = thread->runtime();
  Object obj(&scope, NoneType::object());
  if (pyobj != nullptr) {
    obj = ApiHandle::fromPyObject(pyobj)->asObject();
    obj = thread->invokeFunction1(
        SymbolId::kBuiltins,
        flags & Py_PRINT_RAW ? SymbolId::kStr : SymbolId::kRepr, obj);
    if (obj.isError()) return -1;
    DCHECK(runtime->isInstanceOfStr(*obj), "str() and repr() must return str");
  } else {
    obj = thread->runtime()->symbols()->Null();
  }

  Object result(&scope, thread->invokeMethod2(file, SymbolId::kWrite, obj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT int PyFile_WriteString(const char* str, PyObject* pyfile) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  if (thread->hasPendingException()) return -1;
  if (pyfile == nullptr) {
    thread->raiseSystemErrorWithCStr("null file for PyFile_WriteString");
    return -1;
  }

  Object file(&scope, ApiHandle::fromPyObject(pyfile)->asObject());
  Object str_obj(&scope, thread->runtime()->newStrFromCStr(str));
  Object result(&scope, thread->invokeMethod2(file, SymbolId::kWrite, str_obj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT int PyObject_AsFileDescriptor(PyObject* obj) {
  DCHECK(obj != nullptr, "obj must not be null");
  Thread* thread = Thread::current();
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
    thread->raiseValueError(thread->runtime()->newStrFromFmt(
        "file descriptor cannot be a negative integer (%d)", fd));
    return -1;
  }
  return fd;
}

PY_EXPORT char* Py_UniversalNewlineFgets(char* /* f */, int /* n */,
                                         FILE* /* m */, PyObject* /* j */) {
  UNIMPLEMENTED("Py_UniversalNewlineFgets");
}

}  // namespace python
