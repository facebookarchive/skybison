#include "runtime.h"

namespace python {

static PyObject* typeObjectHandle(LayoutId id) {
  Thread* thread = Thread::currentThread();
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

PY_EXPORT void PyException_SetCause(PyObject* self, PyObject* cause) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  if (cause == nullptr) {
    exc.setCause(thread->runtime()->unboundValue());
    return;
  }
  ApiHandle* new_cause = ApiHandle::fromPyObject(cause);
  exc.setCause(new_cause->asObject());
  new_cause->decref();
}

PY_EXPORT PyObject* PyException_GetCause(PyObject* self) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object cause(&scope, exc.cause());
  if (cause.isUnboundValue()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *cause);
}

PY_EXPORT PyObject* PyException_GetContext(PyObject* self) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object context(&scope, exc.context());
  if (context.isUnboundValue()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *context);
}

PY_EXPORT void PyException_SetContext(PyObject* self, PyObject* context) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  if (context == nullptr) {
    exc.setContext(thread->runtime()->unboundValue());
    return;
  }
  ApiHandle* new_context = ApiHandle::fromPyObject(context);
  exc.setContext(new_context->asObject());
  new_context->decref();
}

PY_EXPORT int PyException_SetTraceback(PyObject* self, PyObject* tb) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (tb == nullptr) {
    thread->raiseTypeErrorWithCStr("__traceback__ may not be deleted");
    return -1;
  }
  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object tb_obj(&scope, ApiHandle::fromPyObject(tb)->asObject());
  if (!tb_obj.isNoneType() && !tb_obj.isTraceback()) {
    thread->raiseTypeErrorWithCStr("__traceback__ must be a traceback or None");
    return -1;
  }
  exc.setTraceback(*tb_obj);
  return 0;
}

PY_EXPORT PyObject* PyException_GetTraceback(PyObject* self) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  BaseException exc(&scope, ApiHandle::fromPyObject(self)->asObject());
  Object tb(&scope, exc.traceback());
  if (tb.isUnboundValue()) return nullptr;

  return ApiHandle::newReference(thread, exc.traceback());
}

PY_EXPORT PyObject* PyUnicodeDecodeError_Create(
    const char* /* g */, const char* /* t */, Py_ssize_t /* h */,
    Py_ssize_t /* t */, Py_ssize_t /* d */, const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_Create");
}

PY_EXPORT PyObject* PyUnicodeDecodeError_GetEncoding(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetEncoding");
}

PY_EXPORT int PyUnicodeDecodeError_GetEnd(PyObject* /* c */,
                                          Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetEnd");
}

PY_EXPORT PyObject* PyUnicodeDecodeError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetObject");
}

PY_EXPORT PyObject* PyUnicodeDecodeError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetReason");
}

PY_EXPORT int PyUnicodeDecodeError_GetStart(PyObject* /* c */,
                                            Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetStart");
}

PY_EXPORT int PyUnicodeDecodeError_SetEnd(PyObject* /* c */,
                                          Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetEnd");
}

PY_EXPORT int PyUnicodeDecodeError_SetReason(PyObject* /* c */,
                                             const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetReason");
}

PY_EXPORT int PyUnicodeDecodeError_SetStart(PyObject* /* c */,
                                            Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetStart");
}

PY_EXPORT PyObject* PyUnicodeEncodeError_GetEncoding(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetEncoding");
}

PY_EXPORT int PyUnicodeEncodeError_GetEnd(PyObject* /* c */,
                                          Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetEnd");
}

PY_EXPORT PyObject* PyUnicodeEncodeError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetObject");
}

PY_EXPORT PyObject* PyUnicodeEncodeError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetReason");
}

PY_EXPORT int PyUnicodeEncodeError_GetStart(PyObject* /* c */,
                                            Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetStart");
}

PY_EXPORT int PyUnicodeEncodeError_SetEnd(PyObject* /* c */,
                                          Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetEnd");
}

PY_EXPORT int PyUnicodeEncodeError_SetReason(PyObject* /* c */,
                                             const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetReason");
}

PY_EXPORT int PyUnicodeEncodeError_SetStart(PyObject* /* c */,
                                            Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetStart");
}

PY_EXPORT int PyUnicodeTranslateError_GetEnd(PyObject* /* c */,
                                             Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetEnd");
}

PY_EXPORT PyObject* PyUnicodeTranslateError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetObject");
}

PY_EXPORT PyObject* PyUnicodeTranslateError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetReason");
}

PY_EXPORT int PyUnicodeTranslateError_GetStart(PyObject* /* c */,
                                               Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetStart");
}

PY_EXPORT int PyUnicodeTranslateError_SetEnd(PyObject* /* c */,
                                             Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetEnd");
}

PY_EXPORT int PyUnicodeTranslateError_SetReason(PyObject* /* c */,
                                                const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetReason");
}

PY_EXPORT int PyUnicodeTranslateError_SetStart(PyObject* /* c */,
                                               Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetStart");
}

}  // namespace python
