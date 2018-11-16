#include "runtime.h"

namespace python {

struct PyFrameObject;
struct PyThreadState;
typedef int (*Py_tracefunc)(PyObject*, struct _frame*, int, PyObject*);

extern "C" PyObject* PyEval_GetBuiltins(void) {
  UNIMPLEMENTED("PyEval_GetBuiltins");
}

extern "C" PyObject* PyEval_GetGlobals(void) {
  UNIMPLEMENTED("PyEval_GetGlobals");
}

extern "C" int PyEval_MergeCompilerFlags(PyCompilerFlags* /* f */) {
  UNIMPLEMENTED("PyEval_MergeCompilerFlags");
}

extern "C" void PyEval_AcquireLock(void) {
  UNIMPLEMENTED("PyEval_AcquireLock");
}

extern "C" void PyEval_AcquireThread(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyEval_AcquireThread");
}

extern "C" PyObject* PyEval_EvalCode(PyObject* /* o */, PyObject* /* s */,
                                     PyObject* /* s */) {
  UNIMPLEMENTED("PyEval_EvalCode");
}

extern "C" PyObject* PyEval_EvalCodeEx(PyObject* /* o */, PyObject* /* s */,
                                       PyObject* /* s */,
                                       PyObject* const* /* s */, int /* t */,
                                       PyObject* const* /* s */, int /* t */,
                                       PyObject* const* /* s */, int /* t */,
                                       PyObject* /* s */, PyObject* /* e */) {
  UNIMPLEMENTED("PyEval_EvalCodeEx");
}

extern "C" PyObject* PyEval_EvalFrame(PyFrameObject* /* f */) {
  UNIMPLEMENTED("PyEval_EvalFrame");
}

extern "C" PyObject* PyEval_EvalFrameEx(PyFrameObject* /* f */, int /* g */) {
  UNIMPLEMENTED("PyEval_EvalFrameEx");
}

extern "C" PyFrameObject* PyEval_GetFrame(void) {
  UNIMPLEMENTED("PyEval_GetFrame");
}

extern "C" const char* PyEval_GetFuncDesc(PyObject* /* c */) {
  UNIMPLEMENTED("PyEval_GetFuncDesc");
}

extern "C" const char* PyEval_GetFuncName(PyObject* /* c */) {
  UNIMPLEMENTED("PyEval_GetFuncName");
}

extern "C" PyObject* PyEval_GetLocals(void) {
  UNIMPLEMENTED("PyEval_GetLocals");
}

extern "C" void PyEval_InitThreads(void) {
  UNIMPLEMENTED("PyEval_InitThreads");
}

extern "C" void PyEval_ReInitThreads(void) {
  UNIMPLEMENTED("PyEval_ReInitThreads");
}

extern "C" void PyEval_ReleaseLock(void) {
  UNIMPLEMENTED("PyEval_ReleaseLock");
}

extern "C" void PyEval_ReleaseThread(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyEval_ReleaseThread");
}

extern "C" void PyEval_RestoreThread(PyThreadState* /* e */) {
  UNIMPLEMENTED("PyEval_RestoreThread");
}

extern "C" PyThreadState* PyEval_SaveThread(void) {
  UNIMPLEMENTED("PyEval_SaveThread");
}

extern "C" int Py_AddPendingCall(int (*/* func */)(void*), void* /* g */) {
  UNIMPLEMENTED("Py_AddPendingCall");
}

extern "C" int Py_GetRecursionLimit(void) {
  UNIMPLEMENTED("Py_GetRecursionLimit");
}

extern "C" int Py_MakePendingCalls(void) {
  UNIMPLEMENTED("Py_MakePendingCalls");
}

extern "C" void Py_SetRecursionLimit(int /* t */) {
  UNIMPLEMENTED("Py_SetRecursionLimit");
}

extern "C" int _Py_CheckRecursiveCall(const char* /* e */) {
  UNIMPLEMENTED("_Py_CheckRecursiveCall");
}

extern "C" void PyEval_SetProfile(Py_tracefunc /* c */, PyObject* /* g */) {
  UNIMPLEMENTED("PyEval_SetProfile");
}

extern "C" void PyEval_SetTrace(Py_tracefunc /* c */, PyObject* /* g */) {
  UNIMPLEMENTED("PyEval_SetTrace");
}

extern "C" PyObject* PyEval_CallObjectWithKeywords(PyObject* /* e */,
                                                   PyObject* /* s */,
                                                   PyObject* /* s */) {
  UNIMPLEMENTED("PyEval_CallObjectWithKeywords");
}

extern "C" PyObject* _PyEval_EvalFrameDefault(PyFrameObject* /* f */,
                                              int /* g */) {
  UNIMPLEMENTED("_PyEval_EvalFrameDefault");
}

}  // namespace python
