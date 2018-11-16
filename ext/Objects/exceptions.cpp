#include "runtime.h"

namespace python {

static PyObject* typeObjectHandle(LayoutId id) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return ApiHandle::fromBorrowedObject(runtime->typeAt(id));
}

extern "C" PyObject* PyExc_BaseException_Ptr() {
  return typeObjectHandle(LayoutId::kBaseException);
}

extern "C" PyObject* PyExc_Exception_Ptr() {
  return typeObjectHandle(LayoutId::kException);
}

extern "C" PyObject* PyExc_StopAsyncIteration_Ptr() {
  return typeObjectHandle(LayoutId::kStopAsyncIteration);
}

extern "C" PyObject* PyExc_StopIteration_Ptr() {
  return typeObjectHandle(LayoutId::kStopIteration);
}

extern "C" PyObject* PyExc_GeneratorExit_Ptr() {
  return typeObjectHandle(LayoutId::kGeneratorExit);
}

extern "C" PyObject* PyExc_ArithmeticError_Ptr() {
  return typeObjectHandle(LayoutId::kArithmeticError);
}

extern "C" PyObject* PyExc_LookupError_Ptr() {
  return typeObjectHandle(LayoutId::kLookupError);
}

extern "C" PyObject* PyExc_AssertionError_Ptr() {
  return typeObjectHandle(LayoutId::kAssertionError);
}

extern "C" PyObject* PyExc_AttributeError_Ptr() {
  return typeObjectHandle(LayoutId::kAttributeError);
}

extern "C" PyObject* PyExc_BufferError_Ptr() {
  return typeObjectHandle(LayoutId::kBufferError);
}

extern "C" PyObject* PyExc_EOFError_Ptr() {
  return typeObjectHandle(LayoutId::kEOFError);
}

extern "C" PyObject* PyExc_FloatingPointError_Ptr() {
  return typeObjectHandle(LayoutId::kFloatingPointError);
}

extern "C" PyObject* PyExc_OSError_Ptr() {
  return typeObjectHandle(LayoutId::kOSError);
}

extern "C" PyObject* PyExc_ImportError_Ptr() {
  return typeObjectHandle(LayoutId::kImportError);
}

extern "C" PyObject* PyExc_ModuleNotFoundError_Ptr() {
  return typeObjectHandle(LayoutId::kModuleNotFoundError);
}

extern "C" PyObject* PyExc_IndexError_Ptr() {
  return typeObjectHandle(LayoutId::kIndexError);
}

extern "C" PyObject* PyExc_KeyError_Ptr() {
  return typeObjectHandle(LayoutId::kKeyError);
}

extern "C" PyObject* PyExc_KeyboardInterrupt_Ptr() {
  return typeObjectHandle(LayoutId::kKeyboardInterrupt);
}

extern "C" PyObject* PyExc_MemoryError_Ptr() {
  return typeObjectHandle(LayoutId::kMemoryError);
}

extern "C" PyObject* PyExc_NameError_Ptr() {
  return typeObjectHandle(LayoutId::kNameError);
}

extern "C" PyObject* PyExc_OverflowError_Ptr() {
  return typeObjectHandle(LayoutId::kOverflowError);
}

extern "C" PyObject* PyExc_RuntimeError_Ptr() {
  return typeObjectHandle(LayoutId::kRuntimeError);
}

extern "C" PyObject* PyExc_RecursionError_Ptr() {
  return typeObjectHandle(LayoutId::kRecursionError);
}

extern "C" PyObject* PyExc_NotImplementedError_Ptr() {
  return typeObjectHandle(LayoutId::kNotImplementedError);
}

extern "C" PyObject* PyExc_SyntaxError_Ptr() {
  return typeObjectHandle(LayoutId::kSyntaxError);
}

extern "C" PyObject* PyExc_IndentationError_Ptr() {
  return typeObjectHandle(LayoutId::kIndentationError);
}

extern "C" PyObject* PyExc_TabError_Ptr() {
  return typeObjectHandle(LayoutId::kTabError);
}

extern "C" PyObject* PyExc_ReferenceError_Ptr() {
  return typeObjectHandle(LayoutId::kReferenceError);
}

extern "C" PyObject* PyExc_SystemError_Ptr() {
  return typeObjectHandle(LayoutId::kSystemError);
}

extern "C" PyObject* PyExc_SystemExit_Ptr() {
  return typeObjectHandle(LayoutId::kSystemExit);
}

extern "C" PyObject* PyExc_TypeError_Ptr() {
  return typeObjectHandle(LayoutId::kTypeError);
}

extern "C" PyObject* PyExc_UnboundLocalError_Ptr() {
  return typeObjectHandle(LayoutId::kUnboundLocalError);
}

extern "C" PyObject* PyExc_UnicodeError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeError);
}

extern "C" PyObject* PyExc_UnicodeEncodeError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeEncodeError);
}

extern "C" PyObject* PyExc_UnicodeDecodeError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeDecodeError);
}

extern "C" PyObject* PyExc_UnicodeTranslateError_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeTranslateError);
}

extern "C" PyObject* PyExc_ValueError_Ptr() {
  return typeObjectHandle(LayoutId::kValueError);
}

extern "C" PyObject* PyExc_ZeroDivisionError_Ptr() {
  return typeObjectHandle(LayoutId::kZeroDivisionError);
}

extern "C" PyObject* PyExc_BlockingIOError_Ptr() {
  return typeObjectHandle(LayoutId::kBlockingIOError);
}

extern "C" PyObject* PyExc_BrokenPipeError_Ptr() {
  return typeObjectHandle(LayoutId::kBrokenPipeError);
}

extern "C" PyObject* PyExc_ChildProcessError_Ptr() {
  return typeObjectHandle(LayoutId::kChildProcessError);
}

extern "C" PyObject* PyExc_ConnectionError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionError);
}

extern "C" PyObject* PyExc_ConnectionAbortedError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionAbortedError);
}

extern "C" PyObject* PyExc_ConnectionRefusedError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionRefusedError);
}

extern "C" PyObject* PyExc_ConnectionResetError_Ptr() {
  return typeObjectHandle(LayoutId::kConnectionResetError);
}

extern "C" PyObject* PyExc_FileExistsError_Ptr() {
  return typeObjectHandle(LayoutId::kFileExistsError);
}

extern "C" PyObject* PyExc_FileNotFoundError_Ptr() {
  return typeObjectHandle(LayoutId::kFileNotFoundError);
}

extern "C" PyObject* PyExc_InterruptedError_Ptr() {
  return typeObjectHandle(LayoutId::kInterruptedError);
}

extern "C" PyObject* PyExc_IsADirectoryError_Ptr() {
  return typeObjectHandle(LayoutId::kIsADirectoryError);
}

extern "C" PyObject* PyExc_NotADirectoryError_Ptr() {
  return typeObjectHandle(LayoutId::kNotADirectoryError);
}

extern "C" PyObject* PyExc_PermissionError_Ptr() {
  return typeObjectHandle(LayoutId::kPermissionError);
}

extern "C" PyObject* PyExc_ProcessLookupError_Ptr() {
  return typeObjectHandle(LayoutId::kProcessLookupError);
}

extern "C" PyObject* PyExc_TimeoutError_Ptr() {
  return typeObjectHandle(LayoutId::kTimeoutError);
}

extern "C" PyObject* PyExc_Warning_Ptr() {
  return typeObjectHandle(LayoutId::kWarning);
}

extern "C" PyObject* PyExc_UserWarning_Ptr() {
  return typeObjectHandle(LayoutId::kUserWarning);
}

extern "C" PyObject* PyExc_DeprecationWarning_Ptr() {
  return typeObjectHandle(LayoutId::kDeprecationWarning);
}

extern "C" PyObject* PyExc_PendingDeprecationWarning_Ptr() {
  return typeObjectHandle(LayoutId::kPendingDeprecationWarning);
}

extern "C" PyObject* PyExc_SyntaxWarning_Ptr() {
  return typeObjectHandle(LayoutId::kSyntaxWarning);
}

extern "C" PyObject* PyExc_RuntimeWarning_Ptr() {
  return typeObjectHandle(LayoutId::kRuntimeWarning);
}

extern "C" PyObject* PyExc_FutureWarning_Ptr() {
  return typeObjectHandle(LayoutId::kFutureWarning);
}

extern "C" PyObject* PyExc_ImportWarning_Ptr() {
  return typeObjectHandle(LayoutId::kImportWarning);
}

extern "C" PyObject* PyExc_UnicodeWarning_Ptr() {
  return typeObjectHandle(LayoutId::kUnicodeWarning);
}

extern "C" PyObject* PyExc_BytesWarning_Ptr() {
  return typeObjectHandle(LayoutId::kBytesWarning);
}

extern "C" PyObject* PyExc_ResourceWarning_Ptr() {
  return typeObjectHandle(LayoutId::kResourceWarning);
}

extern "C" PyObject* PyException_GetTraceback(PyObject* /* f */) {
  UNIMPLEMENTED("PyException_GetTraceback");
}

extern "C" void PyException_SetCause(PyObject* /* f */, PyObject* /* e */) {
  UNIMPLEMENTED("PyException_SetCause");
}

extern "C" PyObject* PyException_GetCause(PyObject* /* f */) {
  UNIMPLEMENTED("PyException_GetCause");
}

extern "C" PyObject* PyException_GetContext(PyObject* /* f */) {
  UNIMPLEMENTED("PyException_GetContext");
}

extern "C" void PyException_SetContext(PyObject* /* f */, PyObject* /* t */) {
  UNIMPLEMENTED("PyException_SetContext");
}

extern "C" int PyException_SetTraceback(PyObject* /* f */, PyObject* /* b */) {
  UNIMPLEMENTED("PyException_SetTraceback");
}

extern "C" PyObject* PyUnicodeDecodeError_Create(
    const char* /* g */, const char* /* t */, Py_ssize_t /* h */,
    Py_ssize_t /* t */, Py_ssize_t /* d */, const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_Create");
}

extern "C" PyObject* PyUnicodeDecodeError_GetEncoding(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetEncoding");
}

extern "C" int PyUnicodeDecodeError_GetEnd(PyObject* /* c */,
                                           Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetEnd");
}

extern "C" PyObject* PyUnicodeDecodeError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetObject");
}

extern "C" PyObject* PyUnicodeDecodeError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetReason");
}

extern "C" int PyUnicodeDecodeError_GetStart(PyObject* /* c */,
                                             Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_GetStart");
}

extern "C" int PyUnicodeDecodeError_SetEnd(PyObject* /* c */,
                                           Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetEnd");
}

extern "C" int PyUnicodeDecodeError_SetReason(PyObject* /* c */,
                                              const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetReason");
}

extern "C" int PyUnicodeDecodeError_SetStart(PyObject* /* c */,
                                             Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeDecodeError_SetStart");
}

extern "C" PyObject* PyUnicodeEncodeError_GetEncoding(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetEncoding");
}

extern "C" int PyUnicodeEncodeError_GetEnd(PyObject* /* c */,
                                           Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetEnd");
}

extern "C" PyObject* PyUnicodeEncodeError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetObject");
}

extern "C" PyObject* PyUnicodeEncodeError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetReason");
}

extern "C" int PyUnicodeEncodeError_GetStart(PyObject* /* c */,
                                             Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_GetStart");
}

extern "C" int PyUnicodeEncodeError_SetEnd(PyObject* /* c */,
                                           Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetEnd");
}

extern "C" int PyUnicodeEncodeError_SetReason(PyObject* /* c */,
                                              const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetReason");
}

extern "C" int PyUnicodeEncodeError_SetStart(PyObject* /* c */,
                                             Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeEncodeError_SetStart");
}

extern "C" int PyUnicodeTranslateError_GetEnd(PyObject* /* c */,
                                              Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetEnd");
}

extern "C" PyObject* PyUnicodeTranslateError_GetObject(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetObject");
}

extern "C" PyObject* PyUnicodeTranslateError_GetReason(PyObject* /* c */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetReason");
}

extern "C" int PyUnicodeTranslateError_GetStart(PyObject* /* c */,
                                                Py_ssize_t* /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_GetStart");
}

extern "C" int PyUnicodeTranslateError_SetEnd(PyObject* /* c */,
                                              Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetEnd");
}

extern "C" int PyUnicodeTranslateError_SetReason(PyObject* /* c */,
                                                 const char* /* n */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetReason");
}

extern "C" int PyUnicodeTranslateError_SetStart(PyObject* /* c */,
                                                Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicodeTranslateError_SetStart");
}

}  // namespace python
