#include "capi-handles.h"
#include "runtime.h"
#include "traceback-builtins.h"

namespace py {

struct PyFrameObject;

PY_EXPORT int PyTraceBack_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isTraceback();
}

PY_EXPORT int PyTraceBack_Here(PyFrameObject* frame) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  FrameProxy proxy(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(frame))->asObject());
  Traceback new_tb(&scope, thread->runtime()->newTraceback());
  new_tb.setFunction(proxy.function());
  new_tb.setLasti(proxy.lasti());
  new_tb.setNext(thread->pendingExceptionTraceback());
  thread->setPendingExceptionTraceback(*new_tb);
  return 0;
}

PY_EXPORT int PyTraceBack_Print(PyObject* traceback, PyObject* file) {
  if (traceback == nullptr) {
    return 0;
  }

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object tb_obj(&scope, ApiHandle::fromPyObject(traceback)->asObject());
  if (!tb_obj.isTraceback()) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Traceback tb(&scope, *tb_obj);
  Object file_obj(&scope, ApiHandle::fromPyObject(file)->asObject());
  return tracebackWrite(thread, tb, file_obj).isErrorException() ? -1 : 0;
}

PY_EXPORT void _PyTraceback_Add(const char* funcname, const char* filename,
                                int lineno) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_tuple(&scope, runtime->emptyTuple());
  Object filename_obj(&scope, Runtime::internStrFromCStr(thread, filename));
  Object name_obj(&scope, Runtime::internStrFromCStr(thread, funcname));
  Code code(&scope, runtime->newCode(/*argcount=*/0,
                                     /*posonlyargcount=*/0,
                                     /*kwonlyargcount=*/0,
                                     /*nlocals=*/0,
                                     /*stacksize=*/0,
                                     /*flags=*/0,
                                     /*code=*/empty_bytes,
                                     /*consts=*/empty_tuple,
                                     /*names=*/empty_tuple,
                                     /*varnames=*/empty_tuple,
                                     /*freevars=*/empty_tuple,
                                     /*cellvars=*/empty_tuple,
                                     /*filename=*/filename_obj,
                                     /*name=*/name_obj,
                                     /*firstlineno=*/lineno,
                                     /*lnotab=*/empty_bytes));
  Object module(&scope, runtime->findModuleById(ID(builtins)));

  Traceback new_tb(&scope, runtime->newTraceback());
  new_tb.setFunction(
      runtime->newFunctionWithCode(thread, name_obj, code, module));
  new_tb.setLineno(SmallInt::fromWord(lineno));
  new_tb.setNext(thread->pendingExceptionTraceback());
  thread->setPendingExceptionTraceback(*new_tb);
}

}  // namespace py
