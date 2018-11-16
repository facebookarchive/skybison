#include "runtime.h"

namespace python {

struct node;

namespace testing {
Object* findModule(Runtime*, const char*);
Object* moduleAt(Runtime*, const Handle<Module>&, const char*);
}  // namespace testing

extern "C" PyObject* PyRun_SimpleStringFlags(const char* str,
                                             PyCompilerFlags* flags) {
  // TODO: Implement the usage of flags
  if (flags != nullptr) {
    UNIMPLEMENTED("Can't specify compiler flags");
  }
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  return ApiHandle::fromObject(runtime->runFromCStr(str))->asPyObject();
}

extern "C" void PyErr_Display(PyObject* /* n */, PyObject* /* e */,
                              PyObject* /* b */) {
  UNIMPLEMENTED("PyErr_Display");
}

extern "C" void PyErr_Print(void) { UNIMPLEMENTED("PyErr_Print"); }

extern "C" void PyErr_PrintEx(int /* s */) { UNIMPLEMENTED("PyErr_PrintEx"); }

extern "C" int PyOS_CheckStack(void) { UNIMPLEMENTED("PyOS_CheckStack"); }

extern "C" node* PyParser_SimpleParseFileFlags(FILE* /* p */,
                                               const char* /* e */, int /* t */,
                                               int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseFileFlags");
}

extern "C" node* PyParser_SimpleParseStringFlags(const char* /* r */,
                                                 int /* t */, int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseStringFlags");
}

extern "C" node* PyParser_SimpleParseStringFlagsFilename(const char* /* r */,
                                                         const char* /* e */,
                                                         int /* t */,
                                                         int /* s */) {
  UNIMPLEMENTED("PyParser_SimpleParseStringFlagsFilename");
}

extern "C" struct symtable* Py_SymtableString(const char* /* r */,
                                              const char* /* r */,
                                              int /* t */) {
  UNIMPLEMENTED("Py_SymtableString");
}

extern "C" PyObject* PyRun_FileExFlags(FILE* /* p */, const char* /* r */,
                                       int /* t */, PyObject* /* s */,
                                       PyObject* /* s */, int /* t */,
                                       PyCompilerFlags* /* s */) {
  UNIMPLEMENTED("PyRun_FileExFlags");
}

extern "C" PyObject* PyRun_StringFlags(const char* /* r */, int /* t */,
                                       PyObject* /* s */, PyObject* /* s */,
                                       PyCompilerFlags* /* s */) {
  UNIMPLEMENTED("PyRun_StringFlags");
}

}  // namespace python
