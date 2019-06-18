#include "cpython-data.h"
#include "cpython-func.h"
#include "exception-builtins.h"
#include "runtime.h"

typedef struct _mod* mod_ty;
struct grammar;

// Prevent clang-format from reordering these order-sensitive includes.
// clang-format off
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
      Py_ssize_t len = strlen(err->text);
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
}

PY_EXPORT struct _node* PyParser_SimpleParseStringFlagsFilename(
    const char* str, const char* filename, int start, int flags) {
  perrdetail err;
  node* mod = PyParser_ParseStringFlagsFilename(
      str, filename, &_PyParser_Grammar, start, &err, flags);
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
  node* parse_tree = PyParser_ParseFileObject(
      fp, filename, enc, &_PyParser_Grammar, start, ps1, ps2, &err, &iflags);
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
  return mod;
}

PY_EXPORT mod_ty PyParser_ASTFromFile(FILE* fp, const char* filename_str,
                                      const char* enc, int start,
                                      const char* ps1, const char* ps2,
                                      PyCompilerFlags* flags, int* errcode,
                                      PyArena* arena) {
  PyObject* filename = PyUnicode_DecodeFSDefault(filename_str);
  if (filename == nullptr) return nullptr;
  mod_ty mod = PyParser_ASTFromFileObject(fp, filename, enc, start, ps1, ps2,
                                          flags, errcode, arena);
  Py_DECREF(filename);
  return mod;
}

PY_EXPORT mod_ty PyParser_ASTFromStringObject(const char* s, PyObject* filename,
                                              int start, PyCompilerFlags* flags,
                                              PyArena* arena) {
  perrdetail err;
  int iflags = parserFlags(flags);
  node* n = PyParser_ParseStringObject(s, filename, &_PyParser_Grammar, start,
                                       &err, &iflags);
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

PY_EXPORT mod_ty PyParser_ASTFromString(const char* s, const char* filename_str,
                                        int start, PyCompilerFlags* flags,
                                        PyArena* arena) {
  PyObject* filename = PyUnicode_DecodeFSDefault(filename_str);
  if (filename == nullptr) return nullptr;
  mod_ty mod = PyParser_ASTFromStringObject(s, filename, start, flags, arena);
  Py_DECREF(filename);
  return mod;
}

}  // namespace python
