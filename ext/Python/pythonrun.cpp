#include "cpython-func.h"

#include "exception-builtins.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyRun_SimpleStringFlags(const char* str, PyCompilerFlags* flags) {
  // TODO(eelizondo): Implement the usage of flags
  if (flags != nullptr) {
    UNIMPLEMENTED("Can't specify compiler flags");
  }
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  RawObject result =
      runtime->run(Runtime::compileFromCStr(str, "<string>").get());
  DCHECK(thread->isErrorValueOk(result), "Error/pending exception mismatch");
  if (!result.isError()) return 0;
  printPendingExceptionWithSysLastVars(thread);
  return -1;
}

PY_EXPORT void PyErr_Display(PyObject* /* exc */, PyObject* value,
                             PyObject* tb) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  DCHECK(value != nullptr, "value must be given");
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object tb_obj(&scope, tb ? ApiHandle::fromPyObject(tb)->asObject()
                           : NoneType::object());
  if (displayException(thread, value_obj, tb_obj).isError()) {
    // Don't propagate any exceptions that happened during printing. This isn't
    // great, but it's necessary to match CPython.
    thread->clearPendingException();
  }
}

PY_EXPORT void PyErr_Print() { PyErr_PrintEx(1); }

PY_EXPORT void PyErr_PrintEx(int set_sys_last_vars) {
  Thread* thread = Thread::current();
  if (set_sys_last_vars) {
    printPendingExceptionWithSysLastVars(thread);
  } else {
    printPendingException(thread);
  }
}

PY_EXPORT int PyOS_CheckStack() { UNIMPLEMENTED("PyOS_CheckStack"); }

PY_EXPORT struct _node* PyParser_SimpleParseFileFlags(FILE* /* p */,
                                                      const char* /* e */,
                                                      int /* t */,
                                                      int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseFileFlags");
}

PY_EXPORT struct _node* PyParser_SimpleParseStringFlags(const char* /* r */,
                                                        int /* t */,
                                                        int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseStringFlags");
}

PY_EXPORT struct _node* PyParser_SimpleParseStringFlagsFilename(
    const char* /* r */, const char* /* e */, int /* t */, int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseStringFlagsFilename");
}

PY_EXPORT struct symtable* Py_SymtableString(const char* /* r */,
                                             const char* /* r */, int /* t */) {
  UNIMPLEMENTED("Py_SymtableString");
}

PY_EXPORT PyObject* PyRun_FileExFlags(FILE* /* p */, const char* /* r */,
                                      int /* t */, PyObject* /* s */,
                                      PyObject* /* s */, int /* t */,
                                      PyCompilerFlags* /* s */) {
  UNIMPLEMENTED("PyRun_FileExFlags");
}

PY_EXPORT PyObject* PyRun_StringFlags(const char* /* r */, int /* t */,
                                      PyObject* /* s */, PyObject* /* s */,
                                      PyCompilerFlags* /* s */) {
  UNIMPLEMENTED("PyRun_StringFlags");
}

}  // namespace python
