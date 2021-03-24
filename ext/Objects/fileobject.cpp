#include <cerrno>

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "capi-handles.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyObject* PyFile_GetLine(PyObject* /* f */, int /* n */) {
  UNIMPLEMENTED("PyFile_GetLine");
}

PY_EXPORT int PyFile_SetOpenCodeHook(Py_OpenCodeHookFunction, void*) {
  // TODO(T87346777): add hook to io.open_code()
  UNIMPLEMENTED("PyFile_SetOpenCodeHook");
}

PY_EXPORT int PyFile_WriteObject(PyObject* pyobj, PyObject* pyfile, int flags) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  if (pyfile == nullptr) {
    thread->raiseWithFmt(LayoutId::kTypeError, "writeobject with NULL file");
    return -1;
  }

  Object file(&scope, ApiHandle::fromPyObject(pyfile)->asObject());
  Runtime* runtime = thread->runtime();
  Object obj(&scope, NoneType::object());
  if (pyobj != nullptr) {
    obj = ApiHandle::fromPyObject(pyobj)->asObject();
    obj = thread->invokeFunction1(
        ID(builtins), flags & Py_PRINT_RAW ? ID(str) : ID(repr), obj);
    if (obj.isError()) return -1;
    DCHECK(runtime->isInstanceOfStr(*obj), "str() and repr() must return str");
  } else {
    obj = SmallStr::fromCStr("<NULL>");
  }

  Object result(&scope, thread->invokeMethod2(file, ID(write), obj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT int PyFile_WriteString(const char* str, PyObject* pyfile) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  if (thread->hasPendingException()) return -1;
  if (pyfile == nullptr) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "null file for PyFile_WriteString");
    return -1;
  }

  Object file(&scope, ApiHandle::fromPyObject(pyfile)->asObject());
  Object str_obj(&scope, thread->runtime()->newStrFromCStr(str));
  Object result(&scope, thread->invokeMethod2(file, ID(write), str_obj));
  return result.isError() ? -1 : 0;
}

PY_EXPORT int PyObject_AsFileDescriptor(PyObject* obj) {
  DCHECK(obj != nullptr, "obj must not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*object)) {
    object = thread->invokeMethod1(object, ID(fileno));
    if (object.isError()) {
      if (object.isErrorNotFound()) {
        thread->raiseWithFmt(
            LayoutId::kTypeError,
            "argument must be an int, or have a fileno() method.");
      }
      return -1;
    }
    if (!runtime->isInstanceOfInt(*object)) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "fileno() returned a non-integer");
      return -1;
    }
  }
  Int result(&scope, intUnderlying(*object));
  auto const optint = result.asInt<int>();
  if (optint.error != CastError::None) {
    thread->raiseWithFmt(LayoutId::kOverflowError,
                         "Python int too big to convert to C int");
    return -1;
  }
  int fd = optint.value;
  if (fd < 0) {
    thread->raiseWithFmt(LayoutId::kValueError,
                         "file descriptor cannot be a negative integer (%d)",
                         fd);
    return -1;
  }
  return fd;
}

PY_EXPORT char* Py_UniversalNewlineFgets(char* buf, int buf_size, FILE* stream,
                                         PyObject* fobj) {
  if (fobj != nullptr) {
    errno = ENXIO;
    return nullptr;
  }

  enum Newline {
    CR = 0x1,
    LF = 0x2,
    CRLF = 0x4,
  };
  char* ptr = buf;
  bool skipnextlf = false;
  for (char ch; --buf_size > 0 && (ch = std::getc(stream)) != EOF;) {
    int newlinetypes = 0;
    if (skipnextlf) {
      skipnextlf = false;
      if (ch == '\n') {
        // Seeing a \n here with skipnextlf true means we saw a \r before.
        newlinetypes |= Newline::CRLF;
        ch = std::getc(stream);
        if (ch == EOF) break;
      } else {
        // Note that c == EOF also brings us here, so we're okay
        // if the last char in the file is a CR.
        newlinetypes |= Newline::CR;
      }
    }
    if (ch == '\r') {
      // A \r is translated into a \n, and we skip  an adjacent \n, if any.
      // We don't set the newlinetypes flag until we've seen the next char.
      skipnextlf = true;
      ch = '\n';
    } else if (ch == '\n') {
      newlinetypes |= Newline::LF;
    }
    *ptr++ = ch;
    if (ch == '\n') break;
  }
  *ptr = '\0';
  if (skipnextlf) {
    // If we have no file object we cannot save the skipnextlf flag.
    // We have to readahead, which will cause a pause if we're reading
    // from an interactive stream, but that is very unlikely unless we're
    // doing something silly like exec(open("/dev/tty").read()).
    char ch = std::getc(stream);
    if (ch != '\n') std::ungetc(ch, stream);
  }
  if (ptr == buf) return nullptr;
  return buf;
}

}  // namespace py
