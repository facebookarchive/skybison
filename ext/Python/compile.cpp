// Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "api-handle.h"
#include "builtins-module.h"
#include "compile-utils.h"
#include "globals.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "utils.h"

// Declarations from `Python-ast.h` and `ast.h` which are not part of
// `Python.h` so we shouldn't add them to `cpython-*.h`.
extern "C" {
_mod* PyAST_FromNode(const _node*, PyCompilerFlags*, const char*, PyArena*);
PyObject* PyAST_mod2obj(_mod* t);
enum _mod_kind {
  Module_kind = 1,
  Interactive_kind = 2,
  Expression_kind = 3,
  Suite_kind = 4
};
struct _mod {
  enum _mod_kind kind;
  void* dummy;
};
}

namespace py {

static_assert(Code::kCompileFlagsMask == PyCF_MASK, "flags mismatch");

PY_EXPORT PyObject* _Py_Mangle(PyObject* privateobj, PyObject* ident) {
  if (privateobj == nullptr) {
    Py_INCREF(ident);
    return ident;
  }
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object privateobj_obj(&scope,
                        ApiHandle::fromPyObject(privateobj)->asObject());
  Runtime* runtime = thread->runtime();
  Object ident_obj(&scope, ApiHandle::fromPyObject(ident)->asObject());
  Str ident_str(&scope, strUnderlying(*ident_obj));
  Object mangled(&scope, mangle(thread, privateobj_obj, ident_str));
  if (mangled == ident_str) {
    Py_INCREF(ident);
    return ident;
  }
  return ApiHandle::newReference(runtime, *mangled);
}

PY_EXPORT PyCodeObject* PyNode_Compile(_node* node, const char* filename) {
  PyArena* arena = PyArena_New();
  if (!arena) return nullptr;
  _mod* mod = PyAST_FromNode(node, nullptr, filename, arena);
  if (mod == nullptr) {
    PyArena_Free(arena);
    return nullptr;
  }
  PyCodeObject* code = PyAST_Compile(mod, filename, nullptr, arena);
  PyArena_Free(arena);
  return code;
}

PY_EXPORT PyCodeObject* PyAST_Compile(_mod* mod, const char* filename,
                                      PyCompilerFlags* flags, PyArena* arena) {
  return PyAST_CompileEx(mod, filename, flags, -1, arena);
}

PY_EXPORT PyCodeObject* PyAST_CompileEx(_mod* mod, const char* filename_str,
                                        PyCompilerFlags* flags, int optimize,
                                        PyArena* arena) {
  PyObject* filename = PyUnicode_DecodeFSDefault(filename_str);
  if (filename == nullptr) return nullptr;
  PyCodeObject* co = PyAST_CompileObject(mod, filename, flags, optimize, arena);
  Py_DECREF(filename);
  return co;
}

PY_EXPORT PyCodeObject* PyAST_CompileObject(_mod* mod, PyObject* pyfilename,
                                            PyCompilerFlags* flags_ptr,
                                            int optimize, PyArena*) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  PyObject* pyast = PyAST_mod2obj(mod);
  if (pyast == nullptr) return nullptr;
  Object ast(&scope, ApiHandle::fromPyObject(pyast)->asObject());
  Object filename(&scope, ApiHandle::fromPyObject(pyfilename)->asObject());
  SymbolId mode_id;
  switch (mod->kind) {
    case Module_kind:
      mode_id = ID(exec);
      break;
    case Interactive_kind:
      mode_id = ID(single);
      break;
    case Expression_kind:
      mode_id = ID(eval);
      break;
    default:
      UNREACHABLE("Unknown module kind");
  }
  word flags = flags_ptr ? flags_ptr->cf_flags : 0;
  // PyCF_SOURCE_IS_UTF8 is not a valid flag to pass through managed code.
  flags &= ~PyCF_SOURCE_IS_UTF8;
  if (optimize == -1) {
    optimize = Py_OptimizeFlag;
  }
  Object result(&scope,
                compile(thread, ast, filename, mode_id, flags, optimize));
  if (result.isErrorException()) return nullptr;
  return reinterpret_cast<PyCodeObject*>(
      ApiHandle::newReference(thread->runtime(), *result));
}

PY_EXPORT int PyCompile_OpcodeStackEffect(int /*opcode*/, int /*oparg*/) {
  UNIMPLEMENTED("PyCompile_OpcodeStackEffect");
}

}  // namespace py
