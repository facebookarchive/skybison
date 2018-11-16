#include "runtime.h"

namespace python {

struct node;

namespace testing {
RawObject findModule(Runtime*, const char*);
RawObject moduleAt(Runtime*, const Module&, const char*);
}  // namespace testing

PY_EXPORT PyObject* PyRun_SimpleStringFlags(const char* str,
                                            PyCompilerFlags* flags) {
  // TODO(eelizondo): Implement the usage of flags
  if (flags != nullptr) {
    UNIMPLEMENTED("Can't specify compiler flags");
  }
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  return ApiHandle::fromObject(runtime->runFromCStr(str));
}

PY_EXPORT void PyErr_Display(PyObject* /* n */, PyObject* /* e */,
                             PyObject* /* b */) {
  UNIMPLEMENTED("PyErr_Display");
}

PY_EXPORT void PyErr_Print() { UNIMPLEMENTED("PyErr_Print"); }

PY_EXPORT void PyErr_PrintEx(int /* s */) { UNIMPLEMENTED("PyErr_PrintEx"); }

PY_EXPORT int PyOS_CheckStack() { UNIMPLEMENTED("PyOS_CheckStack"); }

PY_EXPORT node* PyParser_SimpleParseFileFlags(FILE* /* p */,
                                              const char* /* e */, int /* t */,
                                              int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseFileFlags");
}

PY_EXPORT node* PyParser_SimpleParseStringFlags(const char* /* r */,
                                                int /* t */, int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseStringFlags");
}

PY_EXPORT node* PyParser_SimpleParseStringFlagsFilename(const char* /* r */,
                                                        const char* /* e */,
                                                        int /* t */,
                                                        int /* s */) {
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
