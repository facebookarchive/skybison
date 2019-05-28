#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyEval_GetBuiltins() {
  UNIMPLEMENTED("PyEval_GetBuiltins");
}

PY_EXPORT PyObject* PyEval_GetGlobals() { UNIMPLEMENTED("PyEval_GetGlobals"); }

PY_EXPORT int PyEval_MergeCompilerFlags(PyCompilerFlags* /* f */) {
  UNIMPLEMENTED("PyEval_MergeCompilerFlags");
}

PY_EXPORT void PyEval_AcquireLock() { UNIMPLEMENTED("PyEval_AcquireLock"); }

PY_EXPORT void PyEval_AcquireThread(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyEval_AcquireThread");
}

PY_EXPORT PyObject* PyEval_EvalCode(PyObject* /* o */, PyObject* /* s */,
                                    PyObject* /* s */) {
  UNIMPLEMENTED("PyEval_EvalCode");
}

PY_EXPORT PyObject* PyEval_EvalCodeEx(PyObject* /* o */, PyObject* /* s */,
                                      PyObject* /* s */,
                                      PyObject* const* /* s */, int /* t */,
                                      PyObject* const* /* s */, int /* t */,
                                      PyObject* const* /* s */, int /* t */,
                                      PyObject* /* s */, PyObject* /* e */) {
  UNIMPLEMENTED("PyEval_EvalCodeEx");
}

PY_EXPORT PyObject* PyEval_EvalFrame(PyFrameObject* /* f */) {
  UNIMPLEMENTED("PyEval_EvalFrame");
}

PY_EXPORT PyObject* PyEval_EvalFrameEx(PyFrameObject* /* f */, int /* g */) {
  UNIMPLEMENTED("PyEval_EvalFrameEx");
}

PY_EXPORT PyFrameObject* PyEval_GetFrame() { UNIMPLEMENTED("PyEval_GetFrame"); }

PY_EXPORT const char* PyEval_GetFuncDesc(PyObject* /* c */) {
  UNIMPLEMENTED("PyEval_GetFuncDesc");
}

PY_EXPORT const char* PyEval_GetFuncName(PyObject* /* c */) {
  UNIMPLEMENTED("PyEval_GetFuncName");
}

PY_EXPORT PyObject* PyEval_GetLocals() { UNIMPLEMENTED("PyEval_GetLocals"); }

PY_EXPORT void PyEval_InitThreads() { UNIMPLEMENTED("PyEval_InitThreads"); }

PY_EXPORT void PyEval_ReInitThreads() { UNIMPLEMENTED("PyEval_ReInitThreads"); }

PY_EXPORT void PyEval_ReleaseLock() { UNIMPLEMENTED("PyEval_ReleaseLock"); }

PY_EXPORT void PyEval_ReleaseThread(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyEval_ReleaseThread");
}

PY_EXPORT void PyEval_RestoreThread(PyThreadState* /* e */) {
  // TODO(T37761469): Implement instead of making it a noop
}

PY_EXPORT PyThreadState* PyEval_SaveThread() {
  // TODO(T37761469): Implement instead of making it a noop
  return nullptr;
}

PY_EXPORT int Py_AddPendingCall(int (*/* func */)(void*), void* /* g */) {
  UNIMPLEMENTED("Py_AddPendingCall");
}

PY_EXPORT int Py_GetRecursionLimit() {
  return Thread::current()->recursionLimit();
}

PY_EXPORT int Py_EnterRecursiveCall_Func(const char* where) {
  Thread* thread = Thread::current();
  if (thread->increaseRecursionDepth() >= thread->recursionLimit()) {
    thread->raiseWithFmt(LayoutId::kRecursionError,
                         "maximum recursion depth exceeded %s", where);
    return -1;
  }
  return 0;
}

PY_EXPORT void Py_LeaveRecursiveCall_Func() {
  Thread::current()->decreaseRecursionDepth();
}

PY_EXPORT int Py_MakePendingCalls() { UNIMPLEMENTED("Py_MakePendingCalls"); }

PY_EXPORT void Py_SetRecursionLimit(int limit) {
  Thread::current()->setRecursionLimit(limit);
}

PY_EXPORT int _Py_CheckRecursiveCall(const char* /* where */) {
  // We don't implement this function because this recursion checking is left
  // up to the runtime.
  return 0;
}

PY_EXPORT void PyEval_SetProfile(Py_tracefunc /* c */, PyObject* /* g */) {
  UNIMPLEMENTED("PyEval_SetProfile");
}

PY_EXPORT void PyEval_SetTrace(Py_tracefunc /* c */, PyObject* /* g */) {
  UNIMPLEMENTED("PyEval_SetTrace");
}

PY_EXPORT PyObject* PyEval_CallObjectWithKeywords(PyObject* /* e */,
                                                  PyObject* /* s */,
                                                  PyObject* /* s */) {
  UNIMPLEMENTED("PyEval_CallObjectWithKeywords");
}

PY_EXPORT PyObject* _PyEval_EvalFrameDefault(PyFrameObject* /* f */,
                                             int /* g */) {
  UNIMPLEMENTED("_PyEval_EvalFrameDefault");
}

}  // namespace python
