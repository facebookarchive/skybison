#include "compile.h"

#include "capi-handles.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"
#include "marshal.h"
#include "runtime.h"

namespace python {

RawObject bytecodeToCode(Thread* thread, const char* buffer) {
  HandleScope scope(thread);
  Marshal::Reader reader(&scope, thread->runtime(), buffer);
  reader.readLong();
  reader.readLong();
  reader.readLong();
  return reader.readObject();
}

RawObject compileFromCStr(const char* buffer, const char* file_name) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  void* node =
      PyParser_ASTFromString(buffer, file_name, Py_file_input, &flags, arena);
  // TODO(T46359994): Return Error if node is null so we can raise exceptions
  DCHECK(node != nullptr, "PyParser_ASTFromString must return a valid node");
  PyObject* pycode = reinterpret_cast<PyObject*>(PyAST_CompileEx(
      reinterpret_cast<struct _mod*>(node), file_name, &flags, 0, arena));
  PyArena_Free(arena);
  DCHECK(pycode != nullptr, "PyAST_CompileEx must return a valid code object");
  return ApiHandle::fromPyObject(pycode)->asObject();
}

}  // namespace python
