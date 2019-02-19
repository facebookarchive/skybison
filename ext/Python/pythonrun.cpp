#include "cpython-func.h"
#include "runtime.h"

namespace python {

PY_EXPORT int PyRun_SimpleStringFlags(const char* str, PyCompilerFlags* flags) {
  // TODO(eelizondo): Implement the usage of flags
  if (flags != nullptr) {
    UNIMPLEMENTED("Can't specify compiler flags");
  }
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  runtime->run(Runtime::compileFromCStr(str).get());
  if (!thread->hasPendingException()) return 0;
  // TODO(T39917518): Print exception
  thread->clearPendingException();
  return -1;
}

PY_EXPORT void PyErr_Display(PyObject* /* n */, PyObject* /* e */,
                             PyObject* /* b */) {
  UNIMPLEMENTED("PyErr_Display");
}

PY_EXPORT void PyErr_Print() { UNIMPLEMENTED("PyErr_Print"); }

PY_EXPORT void PyErr_PrintEx(int /* s */) { UNIMPLEMENTED("PyErr_PrintEx"); }

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
