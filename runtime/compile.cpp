#include "compile.h"

#include "capi-handles.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"
#include "marshal.h"
#include "runtime.h"

namespace py {

// TODO(T47585202): Remove and expose through a C-API module.
RawObject compileFromCStr(const char* buffer, const char* file_name) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  void* node =
      PyParser_ASTFromString(buffer, file_name, Py_file_input, &flags, arena);
  if (node == nullptr) {
    PyArena_Free(arena);
    return Error::exception();
  }
  PyObject* pycode = reinterpret_cast<PyObject*>(PyAST_CompileEx(
      reinterpret_cast<struct _mod*>(node), file_name, &flags, 0, arena));
  PyArena_Free(arena);
  if (pycode == nullptr) return Error::exception();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pycode);
  Object result(&scope, handle->asObject());
  handle->decref();
  return *result;
}

int runInteractive(FILE* fp) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  return PyRun_AnyFileExFlags(fp, fp == stdin ? "<stdin>" : "???",
                              /*closeit=*/0, &flags);
}

}  // namespace py
