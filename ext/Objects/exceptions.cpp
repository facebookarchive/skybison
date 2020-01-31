#include "cpython-data.h"
#include "cpython-func.h"

#include "capi-handles.h"
#include "runtime.h"

namespace py {

static PyObject* typeObjectHandle(LayoutId id) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  return ApiHandle::borrowedReference(thread, runtime->typeAt(id));
}

PY_EXPORT PyObject* PyExc_BaseException_Ptr() {
  return typeObjectHandle(LayoutId::kBaseException);
}

PY_EXPORT PyObject* PyExc_Exception_Ptr() {
  return typeObjectHandle(LayoutId::kException);
}

PY_EXPORT PyObject* PyExc_StopAsyncIteration_Ptr() {
  return typeObjectHandle(LayoutId::kStopAsyncIteration);
}

PY_EXPORT PyObject* PyExc_StopIteration_Ptr() {
  return typeObjectHandle(LayoutId::kStopIteration);
}

PY_EXPORT PyObject* PyExc_GeneratorExit_Ptr() {
  return typeObjectHandle(LayoutId::kGeneratorExit);
}

PY_EXPORT PyObject* PyExc_ArithmeticError_Ptr() {
  return typeObjectHandle(LayoutId::kArithmeticError);
}

PY_EXPORT PyObject* PyExc_LookupError_Ptr() {
  return typeObjectHandle(LayoutId::kLookupError);
}

PY_EXPORT PyObject* PyExc_AssertionError_Ptr() {
  return typeObjectHandle(LayoutId::kAssertionError);
}

PY_EXPORT PyObject* PyExc_AttributeError_Ptr() {
  return typeObjectHandle(LayoutId::kAttributeError);
}

PY_EXPORT PyObject* PyExc_BufferError_Ptr() {
  return typeObjectHandle(LayoutId::kBufferError);
}

PY_EXPORT PyObject* PyExc_EOFError_Ptr() {
  return typeObjectHandle(LayoutId::kEOFError);
}

PY_EXPORT PyObject* PyExc_FloatingPointError_Ptr() {
  return typeObjectHandle(LayoutId::kFloatingPointError);
}

PY_EXPORT PyObject* PyExc_OSError_Ptr() {
  return typeObjectHandle(LayoutId::kOSError);
}

PY_EXPORT PyObject* PyExc_ImportError_Ptr() {
  return typeObjectHandle(LayoutId::kImportError);
}

PY_EXPORT PyObject* PyExc_ModuleNotFoundError_Ptr() {
  return typeObjectHandle(LayoutId::kModuleNotFoundError);
}

PY_EXPORT PyObject* PyExc_IndexError_Ptr() {
  return typeObjectHandle(LayoutId::kIndexError);
}

PY_EXPORT PyObject* PyExc_KeyError_Ptr() {
  return typeObjectHandle(LayoutId::kKeyError);
}

PY_EXPORT PyObject* PyExc_KeyboardInterrupt_Ptr() {
  return typeObjectHandle(LayoutId::kKeyboardInterrupt);
}

PY_EXPORT PyObject* PyExc_MemoryError_Ptr() {
  return typeObjectHandle(LayoutId::kMemoryError);
}

PY_EXPORT PyObject* PyExc_NameError_Ptr() {
  return typeObjectHandle(LayoutId::kNameError);
}

PY_EXPORT PyObject* PyExc_OverflowError_Ptr() {
  return typeObjectHandle(LayoutId::kOverflowError);
}

PY_EXPORT PyObject* PyExc_RuntimeError_Ptr() {
  return typeObjectHandle(LayoutId::kRuntimeError);
}

PY_EXPORT PyObject* PyExc_RecursionError_Ptr() {
  return typeObjectHandle(LayoutId::kRecursionError);
}

PY_EXPORT PyObject* PyExc_NotImplementedError_Ptr() {
  return typeObjectHandle(LayoutId::kNotImplementedError);
}

PY_EXPORT PyObject* PyExc_SyntaxError_Ptr() {
  return typeObjectHandle(LayoutId::kSyntaxError);
}

PY_EXPORT PyObject* PyExc_IndentationError_Ptr() {
  return typeObjectHandle(LayoutId::kIndentationError);
}

PY_EXPORT PyObject* PyExc_TabError_Ptr() {
  return typeObjectHandle(LayoutId::kTabError);
}

PY_EXPORT PyObject* PyExc_ReferenceError_Ptr() {
  return typeObjectHandle(LayoutId::kReferenceError);
}

PY_EXPORT PyObject* PyExc_SystemError_Ptr() {
  return typeObjectHandle(LayoutId::kSystemError);
}

PY_EXPORT PyObject* PyExc_SystemExit_Ptr() {
  return typeObjectHandle(LayoutId::kSystemExit);
}

PY_EXPORT PyObject* PyExc_TypeError_Ptr() {
  return typeObjectHandle(LayoutId::kTypeError);
}

PY_EXPORT PyObject* PyExc_UnboundLocalError_Ptr() {
  return typeObjectHandle(LayoutId::kUnboundLocalError);
}

PY_EXPORT PyObject* PyExc_UnicodeError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeError);
}

PY_EXPORT PyObject* PyExc_UnicodeEncodeError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeEncodeError);
}

PY_EXPORT PyObject* PyExc_UnicodeDecodeError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeDecodeError);
}

PY_EXPORT PyObject* PyExc_UnicodeTranslateError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeTranslateError);
}

PY_EXPORT PyObject* PyExc_ValueError_Ptr() {
  return typeObjectHandle(LayoutId::kValueError);
}

PY_EXPORT PyObject* PyExc_ZeroDivisionError_Ptr() {
  return typeObjectHandle(LayoutId::kZeroDivisionError);
}

PY_EXPORT PyObject* PyExc_BlockingIOError_Ptr() {
  return typeObjectHandle(LayoutId::kBlockingIOError);
}

PY_EXPORT PyObject* PyExc_BrokenPipeError_Ptr() {
  return typeObjectHandle(LayoutId::kBrokenPipeError);
}

PY_EXPORT PyObject* PyExc_ChildProcessError_Ptr() {
  return typeObjectHandle(LayoutId::kChildProcessError);
}

PY_EXPORT PyObject* PyExc_ConnectionError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionError);
}

PY_EXPORT PyObject* PyExc_ConnectionAbortedError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionAbortedError);
}

PY_EXPORT PyObject* PyExc_ConnectionRefusedError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionRefusedError);
}

PY_EXPORT PyObject* PyExc_ConnectionResetError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionResetError);
}

PY_EXPORT PyObject* PyExc_FileExistsError_Ptr() {
  return typeObjectHandle(LayoutId::kFileExistsError);
}

PY_EXPORT PyObject* PyExc_FileNotFoundError_Ptr() {
  return typeObjectHandle(LayoutId::kFileNotFoundError);
}

PY_EXPORT PyObject* PyExc_InterruptedError_Ptr() {
  return typeObjectHandle(LayoutId::kInterruptedError);
}

PY_EXPORT PyObject* PyExc_IsADirectoryError_Ptr() {
  return typeObjectHandle(LayoutId::kIsADirectoryError);
}

PY_EXPORT PyObject* PyExc_NotADirectoryError_Ptr() {
  return typeObjectHandle(LayoutId::kNotADirectoryError);
}

PY_EXPORT PyObject* PyExc_PermissionError_Ptr() {
  return typeObjectHandle(LayoutId::kPermissionError);
}

PY_EXPORT PyObject* PyExc_ProcessLookupError_Ptr() {
  return typeObjectHandle(LayoutId::kProcessLookupError);
}

PY_EXPORT PyObject* PyExc_TimeoutError_Ptr() {
  return typeObjectHandle(LayoutId::kTimeoutError);
}

PY_EXPORT PyObject* PyExc_Warning_Ptr() {
  return typeObjectHandle(LayoutId::kWarning);
}

PY_EXPORT PyObject* PyExc_UserWarning_Ptr() {
  return typeObjectHandle(LayoutId::kUserWarning);
}

PY_EXPORT PyObject* PyExc_DeprecationWarning_Ptr() {
  return typeObjectHandle(LayoutId::kDeprecationWarning);
}

PY_EXPORT PyObject* PyExc_PendingDeprecationWarning_Ptr() {
  return typeObjectHandle(LayoutId::kPendingDeprecationWarning);
}

PY_EXPORT PyObject* PyExc_SyntaxWarning_Ptr() {
  return typeObjectHandle(LayoutId::kSyntaxWarning);
}

PY_EXPORT PyObject* PyExc_RuntimeWarning_Ptr() {
  return typeObjectHandle(LayoutId::kRuntimeWarning);
}

PY_EXPORT PyObject* PyExc_FutureWarning_Ptr() {
  return typeObjectHandle(LayoutId::kFutureWarning);
}

PY_EXPORT PyObject* PyExc_ImportWarning_Ptr() {
  return typeObjectHandle(LayoutId::kImportWarning);
}

PY_EXPORT PyObject* PyExc_UnicodeWarning_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeWarning);
}

PY_EXPORT PyObject* PyExc_BytesWarning_Ptr() {
  return typeObjectHandle(LayoutId::kBytesWarning);
}

PY_EXPORT PyObject* PyExc_ResourceWarning_Ptr() {
  return typeObjectHandle(LayoutId::kResourceWarning);
}

PY_EXPORT int PyExceptionInstance_Check_Func(PyObject* obj) {
  DCHECK(obj != nullptr, "obj should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  return thread->runtime()->isInstanceOfBaseException(*object);
}

PY_EXPORT void PyException_SetCause(PyObject* self, PyObject* cause) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  if (cause == nullptr) {
    exc.setCause(Unbound::object());
    return;
  }
  ApiHandle* new_cause = ApiHandle::fromPyObject(cause);
  exc.setCause(new_cause->asObject());
  new_cause->decref();
}

PY_EXPORT PyObject* PyException_GetCause(PyObject* self) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object cause(&scope, exc.causeOrUnbound());
  if (cause.isUnbound()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *cause);
}

PY_EXPORT PyObject* PyException_GetContext(PyObject* self) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object context(&scope, exc.contextOrUnbound());
  if (context.isUnbound()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *context);
}

PY_EXPORT void PyException_SetContext(PyObject* self, PyObject* context) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  if (context == nullptr) {
    exc.setContext(Unbound::object());
    return;
  }
  ApiHandle* new_context = ApiHandle::fromPyObject(context);
  exc.setContext(new_context->asObject());
  new_context->decref();
}

PY_EXPORT int PyException_SetTraceback(PyObject* self, PyObject* tb) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  if (tb == nullptr) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "__traceback__ may not be deleted");
    return -1;
  }
  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object tb_obj(&scope, ApiHandle::fromPyObject(tb)->asObject());
  if (!tb_obj.isNoneType() && !tb_obj.isTraceback()) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "__traceback__ must be a traceback or None");
    return -1;
  }
  exc.setTraceback(*tb_obj);
  return 0;
}

PY_EXPORT PyObject* PyException_GetTraceback(PyObject* self) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object tb(&scope, exc.tracebackOrUnbound());
  if (tb.isUnbound()) return nullptr;

  return ApiHandle::newReference(thread, exc.traceback());
}

PY_EXPORT PyObject* PyUnicodeDecodeError_Create(
    const char* encoding, const char* object, Py_ssize_t length,
    Py_ssize_t start, Py_ssize_t end, const char* reason) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Str encoding_obj(&scope, runtime->newStrFromCStr(encoding));
  Bytes object_obj(&scope, runtime->newBytesWithAll(View<byte>(
                               reinterpret_cast<const byte*>(object), length)));
  Int start_obj(&scope, SmallInt::fromWord(start));
  Int end_obj(&scope, SmallInt::fromWord(end));
  Str reason_obj(&scope, runtime->newStrFromCStr(reason));
  Object result(&scope, thread->invokeFunction5(
                            ID(builtins), ID(UnicodeDecodeError), encoding_obj,
                            object_obj, start_obj, end_obj, reason_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kRuntimeError,
                           "could not call UnicodeDecodeError()");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyUnicodeDecodeError_GetEncoding(PyObject* exc) {
  PyObject* encoding = PyObject_GetAttrString(exc, "encoding");
  if (encoding == nullptr) {
    PyErr_Format(PyExc_TypeError, "encoding attribute not set");
    return nullptr;
  }
  if (!PyUnicode_Check(encoding)) {
    PyErr_Format(PyExc_TypeError, "encoding attribute must be unicode");
    Py_DECREF(encoding);
    return nullptr;
  }
  return encoding;
}

PY_EXPORT int PyUnicodeDecodeError_GetEnd(PyObject* exc, Py_ssize_t* end) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(exc)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfUnicodeErrorBase(*exc_obj),
         "exc must be instance of UnicodeError");
  UnicodeErrorBase exc_err(&scope, *exc_obj);
  Object object_attr(&scope, exc_err.object());
  if (!runtime->isInstanceOfBytes(*object_attr)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "object must be instance of bytes");
    return -1;
  }
  Bytes object(&scope, bytesUnderlying(*object_attr));
  Object end_attr(&scope, exc_err.end());
  DCHECK(runtime->isInstanceOfInt(*end_attr), "end must be instance of int");
  Int end_int(&scope, intUnderlying(*end_attr));
  *end = end_int.asWord();
  if (*end < 1) {
    *end = 1;
  }
  word size = object.length();
  if (*end > size) {
    *end = size;
  }
  return 0;
}

PY_EXPORT PyObject* PyUnicodeDecodeError_GetObject(PyObject* exc) {
  PyObject* object = PyObject_GetAttrString(exc, "object");
  if (object == nullptr) {
    PyErr_Format(PyExc_TypeError, "object attribute not set");
    return nullptr;
  }
  if (!PyBytes_Check(object)) {
    PyErr_Format(PyExc_TypeError, "object attribute must be bytes");
    Py_DECREF(object);
    return nullptr;
  }
  return object;
}

PY_EXPORT PyObject* PyUnicodeDecodeError_GetReason(PyObject* exc) {
  PyObject* reason = PyObject_GetAttrString(exc, "reason");
  if (reason == nullptr) {
    PyErr_Format(PyExc_TypeError, "reason attribute not set");
    return nullptr;
  }
  if (!PyUnicode_Check(reason)) {
    PyErr_Format(PyExc_TypeError, "reason attribute must be unicode");
    Py_DECREF(reason);
    return nullptr;
  }
  return reason;
}

PY_EXPORT int PyUnicodeDecodeError_GetStart(PyObject* exc, Py_ssize_t* start) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(exc)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfUnicodeErrorBase(*exc_obj),
         "exc must be instance of UnicodeError");
  UnicodeErrorBase exc_err(&scope, *exc_obj);
  Object object_attr(&scope, exc_err.object());
  if (!runtime->isInstanceOfBytes(*object_attr)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "object must be instance of bytes");
    return -1;
  }
  Bytes object(&scope, bytesUnderlying(*object_attr));
  Object start_attr(&scope, exc_err.start());
  DCHECK(runtime->isInstanceOfInt(*start_attr),
         "start must be instance of int");
  Int start_int(&scope, intUnderlying(*start_attr));
  *start = start_int.asWord();
  if (*start < 0) {
    *start = 0;
  }
  word size = object.length();
  if (*start >= size) {
    *start = size - 1;
  }
  return 0;
}

static int unicodeErrorSetEnd(PyObject* unicode_error, Py_ssize_t end) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(unicode_error)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfUnicodeErrorBase(*exc_obj),
         "exc must be instance of UnicodeError");
  UnicodeErrorBase exc(&scope, *exc_obj);
  exc.setEnd(runtime->newInt(end));
  return 0;
}

PY_EXPORT int PyUnicodeDecodeError_SetEnd(PyObject* exc, Py_ssize_t end) {
  return unicodeErrorSetEnd(exc, end);
}

PY_EXPORT int PyUnicodeDecodeError_SetReason(PyObject* unicode_error,
                                             const char* reason) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(unicode_error)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfUnicodeErrorBase(*exc_obj),
         "exc must be instance of UnicodeError");
  UnicodeErrorBase exc(&scope, *exc_obj);
  exc.setReason(runtime->newStrFromCStr(reason));
  return 0;
}

static int unicodeErrorSetStart(PyObject* unicode_error, Py_ssize_t start) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(unicode_error)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfUnicodeErrorBase(*exc_obj),
         "exc must be instance of UnicodeError");
  UnicodeErrorBase exc(&scope, *exc_obj);
  exc.setStart(runtime->newInt(start));
  return 0;
}

PY_EXPORT int PyUnicodeDecodeError_SetStart(PyObject* exc, Py_ssize_t start) {
  return unicodeErrorSetStart(exc, start);
}

PY_EXPORT PyObject* PyUnicodeEncodeError_GetEncoding(PyObject* exc) {
  return PyUnicodeDecodeError_GetEncoding(exc);
}

PY_EXPORT int PyUnicodeEncodeError_GetEnd(PyObject* exc, Py_ssize_t* end) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(exc)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfUnicodeErrorBase(*exc_obj),
         "exc must be instance of UnicodeError");
  UnicodeErrorBase exc_err(&scope, *exc_obj);
  Object object_attr(&scope, exc_err.object());
  if (!runtime->isInstanceOfStr(*object_attr)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "object must be instance of str");
    return -1;
  }
  Str object(&scope, *object_attr);
  Object end_attr(&scope, exc_err.end());
  DCHECK(runtime->isInstanceOfInt(*end_attr), "end must be instance of int");
  Int end_int(&scope, intUnderlying(*end_attr));
  *end = end_int.asWord();
  if (*end < 1) {
    *end = 1;
  }
  word size = object.codePointLength();
  if (*end > size) {
    *end = size;
  }
  return 0;
}

PY_EXPORT PyObject* PyUnicodeEncodeError_GetObject(PyObject* exc) {
  PyObject* object = PyObject_GetAttrString(exc, "object");
  if (object == nullptr) {
    PyErr_Format(PyExc_TypeError, "object attribute not set");
    return nullptr;
  }
  if (!PyUnicode_Check(object)) {
    PyErr_Format(PyExc_TypeError, "object attribute must be str");
    Py_DECREF(object);
    return nullptr;
  }
  return object;
}

PY_EXPORT PyObject* PyUnicodeEncodeError_GetReason(PyObject* exc) {
  return PyUnicodeDecodeError_GetReason(exc);
}

PY_EXPORT int PyUnicodeEncodeError_GetStart(PyObject* exc, Py_ssize_t* start) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(exc)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfUnicodeErrorBase(*exc_obj),
         "exc must be instance of UnicodeError");
  UnicodeErrorBase exc_err(&scope, *exc_obj);
  Object object_attr(&scope, exc_err.object());
  if (!runtime->isInstanceOfStr(*object_attr)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "object must be instance of str");
    return -1;
  }
  Str object(&scope, *object_attr);
  Object start_attr(&scope, exc_err.start());
  DCHECK(runtime->isInstanceOfInt(*start_attr),
         "start must be instance of int");
  Int start_int(&scope, intUnderlying(*start_attr));
  *start = start_int.asWord();
  if (*start < 0) {
    *start = 0;
  }
  word size = object.codePointLength();
  if (*start >= size) {
    *start = size - 1;
  }
  return 0;
}

PY_EXPORT int PyUnicodeEncodeError_SetEnd(PyObject* exc, Py_ssize_t end) {
  return unicodeErrorSetEnd(exc, end);
}

PY_EXPORT int PyUnicodeEncodeError_SetReason(PyObject* exc,
                                             const char* reason) {
  return PyUnicodeDecodeError_SetReason(exc, reason);
}

PY_EXPORT int PyUnicodeEncodeError_SetStart(PyObject* exc, Py_ssize_t start) {
  return unicodeErrorSetStart(exc, start);
}

PY_EXPORT int PyUnicodeTranslateError_GetEnd(PyObject* exc, Py_ssize_t* end) {
  return PyUnicodeEncodeError_GetEnd(exc, end);
}

PY_EXPORT PyObject* PyUnicodeTranslateError_GetObject(PyObject* exc) {
  return PyUnicodeEncodeError_GetObject(exc);
}

PY_EXPORT PyObject* PyUnicodeTranslateError_GetReason(PyObject* exc) {
  return PyUnicodeEncodeError_GetReason(exc);
}

PY_EXPORT int PyUnicodeTranslateError_GetStart(PyObject* exc,
                                               Py_ssize_t* start) {
  return PyUnicodeEncodeError_GetStart(exc, start);
}

PY_EXPORT int PyUnicodeTranslateError_SetEnd(PyObject* exc, Py_ssize_t end) {
  return unicodeErrorSetEnd(exc, end);
}

PY_EXPORT int PyUnicodeTranslateError_SetReason(PyObject* exc,
                                                const char* reason) {
  return PyUnicodeEncodeError_SetReason(exc, reason);
}

PY_EXPORT int PyUnicodeTranslateError_SetStart(PyObject* exc,
                                               Py_ssize_t start) {
  return unicodeErrorSetStart(exc, start);
}

}  // namespace py
