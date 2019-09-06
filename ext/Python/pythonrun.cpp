#include "compile.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "exception-builtins.h"
#include "module-builtins.h"
#include "runtime.h"

typedef struct _mod* mod_ty;

// Prevent clang-format from reordering these order-sensitive includes.
// clang-format off
#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "ast.h"
#include "errcode.h"
#include "token.h"
#include "code.h"
// clang-format on

// from pythonrun.h
#define PyCF_MASK                                                              \
  (CO_FUTURE_DIVISION | CO_FUTURE_ABSOLUTE_IMPORT | CO_FUTURE_WITH_STATEMENT | \
   CO_FUTURE_PRINT_FUNCTION | CO_FUTURE_UNICODE_LITERALS |                     \
   CO_FUTURE_BARRY_AS_BDFL | CO_FUTURE_GENERATOR_STOP)
#define PyCF_DONT_IMPLY_DEDENT 0x0200
#define PyCF_IGNORE_COOKIE 0x0800

// from graminit.c
extern grammar _PyParser_Grammar;

namespace python {

PY_EXPORT int PyRun_AnyFileExFlags(FILE* fp, const char* filename, int closeit,
                                   PyCompilerFlags* flags) {
  if (filename == nullptr) {
    filename = "???";
  }
  if (Py_FdIsInteractive(fp, filename)) {
    int err = PyRun_InteractiveLoopFlags(fp, filename, flags);
    if (closeit) fclose(fp);
    return err;
  }
  return PyRun_SimpleFileExFlags(fp, filename, closeit, flags);
}

static PyObject* runMod(mod_ty mod, PyObject* filename, PyObject* globals,
                        PyObject* locals, PyCompilerFlags* flags,
                        PyArena* arena) {
  PyCodeObject* code = PyAST_CompileObject(mod, filename, flags, -1, arena);
  if (code == nullptr) return nullptr;
  PyObject* v =
      PyEval_EvalCode(reinterpret_cast<PyObject*>(code), globals, locals);
  Py_DECREF(code);
  return v;
}

static void flushIO(void) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // PyErr_Fetch
  Object exc(&scope, thread->pendingExceptionType());
  Object val(&scope, thread->pendingExceptionValue());
  Object tb(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  Runtime* runtime = thread->runtime();
  Module sys(&scope, runtime->findModuleById(SymbolId::kSys));
  Object stderr_obj(&scope, moduleAtById(thread, sys, SymbolId::kStderr));
  if (!stderr_obj.isErrorNotFound()) {
    if (thread->invokeMethod1(stderr_obj, SymbolId::kFlush)
            .isErrorException()) {
      thread->clearPendingException();
    }
  }
  Object stdout_obj(&scope, moduleAtById(thread, sys, SymbolId::kStdout));
  if (!stdout_obj.isErrorNotFound()) {
    if (thread->invokeMethod1(stdout_obj, SymbolId::kFlush)
            .isErrorException()) {
      thread->clearPendingException();
    }
  }

  // PyErr_Restore
  thread->setPendingExceptionType(*exc);
  thread->setPendingExceptionValue(*val);
  thread->setPendingExceptionTraceback(*tb);
}

// A PyRun_InteractiveOneObject() auxiliary function that does not print the
// error on failure.
static int PyRun_InteractiveOneObjectEx(FILE* fp, PyObject* filename,
                                        PyCompilerFlags* flags) {
  PyObject* mod_name = PyUnicode_InternFromString("__main__"); /* borrowed */
  if (mod_name == nullptr) {
    return -1;
  }
  // TODO(T46532201): If fp == stdin, fetch encoding from sys.stdin if possible
  const char* ps1 = "";
  const char* ps2 = "";
  PyObject* ps1_obj = PySys_GetObject("ps1");
  if (ps1_obj != nullptr) {
    if ((ps1_obj = PyObject_Str(ps1_obj)) == nullptr) {
      PyErr_Clear();
    } else if (PyUnicode_Check(ps1_obj)) {
      if ((ps1 = PyUnicode_AsUTF8(ps1_obj)) == nullptr) {
        PyErr_Clear();
        ps1 = "";
      }
    }
    Py_XDECREF(ps1_obj);
  }
  PyObject* ps2_obj = PySys_GetObject("ps2");
  if (ps2_obj != nullptr) {
    if ((ps2_obj = PyObject_Str(ps2_obj)) == nullptr) {
      PyErr_Clear();
    } else if (PyUnicode_Check(ps2_obj)) {
      if ((ps2 = PyUnicode_AsUTF8(ps2_obj)) == nullptr) {
        PyErr_Clear();
        ps2 = "";
      }
    }
    Py_XDECREF(ps2_obj);
  }
  PyArena* arena = PyArena_New();
  if (arena == nullptr) {
    return -1;
  }
  char* enc = nullptr;
  int errcode = 0;
  mod_ty mod = PyParser_ASTFromFileObject(fp, filename, enc, Py_single_input,
                                          ps1, ps2, flags, &errcode, arena);
  if (mod == nullptr) {
    PyArena_Free(arena);
    if (errcode == E_EOF) {
      PyErr_Clear();
      return E_EOF;
    }
    return -1;
  }
  PyObject* module = PyImport_AddModuleObject(mod_name);
  if (module == nullptr) {
    PyArena_Free(arena);
    return -1;
  }
  // NOTE: This must be a Pyro module dictionary (ie one with ValueCells), not
  // any old user-facing dictionary.
  PyObject* module_dict = PyModule_GetDict(module);
  PyObject* result = runMod(mod, filename, /*globals=*/module_dict,
                            /*locals=*/module_dict, flags, arena);
  PyArena_Free(arena);
  if (result == nullptr) {
    return -1;
  }
  Py_DECREF(result);
  flushIO();
  return 0;
}

PY_EXPORT int PyRun_InteractiveLoopFlags(FILE* fp, const char* filename,
                                         PyCompilerFlags* flags) {
  PyObject* filename_str = PyUnicode_DecodeFSDefault(filename);
  if (filename_str == nullptr) {
    PyErr_Print();
    return -1;
  }

  PyCompilerFlags local_flags;
  if (flags == nullptr) {
    flags = &local_flags;
    local_flags.cf_flags = 0;
  }
  // TODO(T46358395): Set sys.ps{1,2} in sys module if they don't exist
  int err = 0;
  int ret;
  int nomem_count = 0;
  do {
    ret = PyRun_InteractiveOneObjectEx(fp, filename_str, flags);
    if (ret == -1 && PyErr_Occurred()) {
      // Prevent an endless loop after multiple consecutive MemoryErrors while
      // still allowing an interactive command to fail with a MemoryError.
      if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
        if (++nomem_count > 16) {
          PyErr_Clear();
          err = -1;
          break;
        }
      } else {
        nomem_count = 0;
      }
      PyErr_Print();
      flushIO();
    } else {
      nomem_count = 0;
    }
  } while (ret != E_EOF);
  Py_DECREF(filename_str);
  return err;
}

PY_EXPORT int PyRun_SimpleFileExFlags(FILE* /*fp*/, const char* /*filename*/,
                                      int /*closeit*/,
                                      PyCompilerFlags* /*flags*/) {
  UNIMPLEMENTED("PyRun_SimpleFileExFlags");
}

PY_EXPORT int PyRun_SimpleStringFlags(const char* str, PyCompilerFlags* flags) {
  // TODO(eelizondo): Implement the usage of flags
  if (flags != nullptr) {
    UNIMPLEMENTED("Can't specify compiler flags");
  }
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object code_obj(&scope, compileFromCStr(str, "<string>"));
  if (code_obj.isError()) {
    printPendingExceptionWithSysLastVars(thread);
    return -1;
  }
  Code code(&scope, *code_obj);
  Runtime* runtime = thread->runtime();
  Module main_module(&scope, runtime->findOrCreateMainModule());
  Object result(&scope, runtime->executeModule(code, main_module));
  DCHECK(thread->isErrorValueOk(*result), "Error/pending exception mismatch");
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

static void errInputCleanup(PyObject* msg_obj, perrdetail* err) {
  Py_XDECREF(msg_obj);
  if (err->text != nullptr) {
    PyObject_FREE(err->text);
    err->text = nullptr;
  }
}

static void errInput(perrdetail* err) {
  const char* msg = nullptr;
  PyObject* msg_obj = nullptr;
  PyObject* errtype = PyExc_SyntaxError;
  switch (err->error) {
    case E_ERROR:
      errInputCleanup(msg_obj, err);
      break;
    case E_SYNTAX:
      errtype = PyExc_IndentationError;
      if (err->expected == INDENT) {
        msg = "expected an indented block";
      } else if (err->token == INDENT) {
        msg = "unexpected indent";
      } else if (err->token == DEDENT) {
        msg = "unexpected unindent";
      } else if (err->expected == NOTEQUAL) {
        errtype = PyExc_SyntaxError;
        msg = "with Barry as BDFL, use '<>' instead of '!='";
      } else {
        errtype = PyExc_SyntaxError;
        msg = "invalid syntax";
      }
      break;
    case E_TOKEN:
      msg = "invalid token";
      break;
    case E_EOFS:
      msg = "EOF while scanning triple-quoted string literal";
      break;
    case E_EOLS:
      msg = "EOL while scanning string literal";
      break;
    case E_INTR:
      if (!PyErr_Occurred()) PyErr_SetNone(PyExc_KeyboardInterrupt);
      errInputCleanup(msg_obj, err);
      break;
    case E_NOMEM:
      PyErr_NoMemory();
      errInputCleanup(msg_obj, err);
      break;
    case E_EOF:
      msg = "unexpected EOF while parsing";
      break;
    case E_TABSPACE:
      errtype = PyExc_TabError;
      msg = "inconsistent use of tabs and spaces in indentation";
      break;
    case E_OVERFLOW:
      msg = "expression too long";
      break;
    case E_DEDENT:
      errtype = PyExc_IndentationError;
      msg = "unindent does not match any outer indentation level";
      break;
    case E_TOODEEP:
      errtype = PyExc_IndentationError;
      msg = "too many levels of indentation";
      break;
    case E_DECODE: {
      PyObject *type, *value, *tb;
      PyErr_Fetch(&type, &value, &tb);
      msg = "unknown decode error";
      if (value != nullptr) msg_obj = PyObject_Str(value);
      Py_XDECREF(type);
      Py_XDECREF(value);
      Py_XDECREF(tb);
      break;
    }
    case E_LINECONT:
      msg = "unexpected character after line continuation character";
      break;
    case E_IDENTIFIER:
      msg = "invalid character in identifier";
      break;
    case E_BADSINGLE:
      msg = "multiple statements found while compiling a single statement";
      break;
    default:
      fprintf(stderr, "error=%d\n", err->error);
      msg = "unknown parsing error";
      break;
  }
  // err->text may not be UTF-8 in case of decoding errors.
  // Explicitly convert to an object.
  int offset = err->offset;
  PyObject* errtext = nullptr;
  if (!err->text) {
    errtext = Py_None;
    Py_INCREF(Py_None);
  } else {
    errtext = PyUnicode_DecodeUTF8(err->text, err->offset, "replace");
    if (errtext != nullptr) {
      Py_ssize_t len = std::strlen(err->text);
      offset = static_cast<int>(PyUnicode_GET_LENGTH(errtext));
      if (len != err->offset) {
        Py_DECREF(errtext);
        errtext = PyUnicode_DecodeUTF8(err->text, len, "replace");
      }
    }
  }
  PyObject* error_tuple =
      Py_BuildValue("(OiiN)", err->filename, err->lineno, offset, errtext);
  PyObject* error_msg_tuple = nullptr;
  if (error_tuple != nullptr) {
    if (msg_obj != nullptr) {
      error_msg_tuple = Py_BuildValue("(OO)", msg_obj, error_tuple);
    } else {
      error_msg_tuple = Py_BuildValue("(sO)", msg, error_tuple);
    }
  }
  Py_XDECREF(error_tuple);
  PyErr_SetObject(errtype, error_msg_tuple);
  Py_XDECREF(error_msg_tuple);
  errInputCleanup(msg_obj, err);
}

PY_EXPORT struct _node* PyParser_SimpleParseStringFlagsFilename(
    const char* str, const char* filename, int start, int flags) {
  perrdetail err;
  node* mod;
  {
    // TODO(T53544009): Cache accelerators on runtime instead of having a
    // use-once shallow copy of the global grammar.
    grammar g = _PyParser_Grammar;
    mod = PyParser_ParseStringFlagsFilename(str, filename, &g, start, &err,
                                            flags);
    PyGrammar_RemoveAccelerators(&g);
  }
  if (mod == nullptr) errInput(&err);
  Py_CLEAR(err.filename);
  return mod;
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

static int parserFlags(PyCompilerFlags* flags) {
  if (!flags) return 0;

  int result = 0;
  if (flags->cf_flags & PyCF_DONT_IMPLY_DEDENT) {
    result |= PyPARSE_DONT_IMPLY_DEDENT;
  }
  if (flags->cf_flags & PyCF_IGNORE_COOKIE) {
    result |= PyPARSE_IGNORE_COOKIE;
  }
  if (flags->cf_flags & CO_FUTURE_BARRY_AS_BDFL) {
    result |= PyPARSE_BARRY_AS_BDFL;
  }
  return result;
}

PY_EXPORT mod_ty PyParser_ASTFromFileObject(FILE* fp, PyObject* filename,
                                            const char* enc, int start,
                                            const char* ps1, const char* ps2,
                                            PyCompilerFlags* flags,
                                            int* errcode, PyArena* arena) {
  perrdetail err;
  int iflags = parserFlags(flags);
  node* parse_tree;
  {
    // TODO(T53544009): Cache accelerators on runtime instead of having a
    // use-once shallow copy of the global grammar.
    grammar g = _PyParser_Grammar;
    parse_tree = PyParser_ParseFileObject(fp, filename, enc, &g, start, ps1,
                                          ps2, &err, &iflags);
    PyGrammar_RemoveAccelerators(&g);
  }
  PyCompilerFlags localflags;
  if (flags == nullptr) {
    localflags.cf_flags = 0;
    flags = &localflags;
  }
  mod_ty mod = nullptr;
  if (parse_tree == nullptr) {
    errInput(&err);
    if (errcode) *errcode = err.error;
  } else {
    flags->cf_flags |= iflags & PyCF_MASK;
    mod = PyAST_FromNodeObject(parse_tree, flags, filename, arena);
    PyNode_Free(parse_tree);
  }
  Py_CLEAR(err.filename);
  PyObject_FREE(err.text);
  return mod;
}

PY_EXPORT mod_ty PyParser_ASTFromFile(FILE* fp, const char* filename,
                                      const char* enc, int start,
                                      const char* ps1, const char* ps2,
                                      PyCompilerFlags* flags, int* errcode,
                                      PyArena* arena) {
  PyObject* filename_str = PyUnicode_DecodeFSDefault(filename);
  if (filename_str == nullptr) return nullptr;
  mod_ty mod = PyParser_ASTFromFileObject(fp, filename_str, enc, start, ps1,
                                          ps2, flags, errcode, arena);
  Py_DECREF(filename_str);
  return mod;
}

PY_EXPORT mod_ty PyParser_ASTFromStringObject(const char* s, PyObject* filename,
                                              int start, PyCompilerFlags* flags,
                                              PyArena* arena) {
  perrdetail err;
  int iflags = parserFlags(flags);
  node* n;
  {
    // TODO(T53544009): Cache accelerators on runtime instead of having a
    // use-once shallow copy of the global grammar.
    grammar g = _PyParser_Grammar;
    n = PyParser_ParseStringObject(s, filename, &g, start, &err, &iflags);
    PyGrammar_RemoveAccelerators(&g);
  }
  PyCompilerFlags localflags;
  if (flags == nullptr) {
    localflags.cf_flags = 0;
    flags = &localflags;
  }
  mod_ty mod = nullptr;
  if (n) {
    flags->cf_flags |= iflags & PyCF_MASK;
    mod = PyAST_FromNodeObject(n, flags, filename, arena);
    PyNode_Free(n);
  } else {
    errInput(&err);
  }
  Py_CLEAR(err.filename);
  return mod;
}

PY_EXPORT mod_ty PyParser_ASTFromString(const char* s, const char* filename,
                                        int start, PyCompilerFlags* flags,
                                        PyArena* arena) {
  PyObject* filename_str = PyUnicode_DecodeFSDefault(filename);
  if (filename_str == nullptr) return nullptr;
  mod_ty mod =
      PyParser_ASTFromStringObject(s, filename_str, start, flags, arena);
  Py_DECREF(filename_str);
  return mod;
}

}  // namespace python
