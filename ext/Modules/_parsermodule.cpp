#include <cstring>

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

extern "C" {
// Python-ast.h is an internal header. Redeclare the only things we need from
// it instead of including it.
typedef struct _mod* mod_ty;
PyObject* PyAST_mod2obj(mod_ty t);
}

static const char* source_as_string(PyObject* cmd, PyCompilerFlags* cf,
                                    PyObject** cmd_copy) {
  Py_buffer view;
  Py_ssize_t size;
  const char* str;
  *cmd_copy = nullptr;
  if (PyUnicode_Check(cmd)) {
    cf->cf_flags |= PyCF_IGNORE_COOKIE;
    str = PyUnicode_AsUTF8AndSize(cmd, &size);
    if (str == nullptr) return nullptr;
  } else if (PyBytes_Check(cmd)) {
    str = PyBytes_AS_STRING(cmd);
    size = PyBytes_GET_SIZE(cmd);
  } else if (PyByteArray_Check(cmd)) {
    str = PyByteArray_AS_STRING(cmd);
    size = PyByteArray_GET_SIZE(cmd);
  } else if (PyObject_GetBuffer(cmd, &view, PyBUF_SIMPLE) == 0) {
    // Copy to NUL-terminated buffer.
    *cmd_copy = PyBytes_FromStringAndSize(
        const_cast<const char*>(reinterpret_cast<char*>(view.buf)), view.len);
    PyBuffer_Release(&view);
    if (*cmd_copy == nullptr) {
      return nullptr;
    }
    str = PyBytes_AS_STRING(*cmd_copy);
    size = PyBytes_GET_SIZE(*cmd_copy);
  } else {
    PyErr_Format(PyExc_TypeError,
                 "parse() arg 1 must be a string, bytes or AST object");
    return nullptr;
  }

  if (strlen(str) != static_cast<size_t>(size)) {
    PyErr_SetString(PyExc_ValueError,
                    "source code string cannot contain null bytes");
    Py_CLEAR(*cmd_copy);
    return nullptr;
  }
  return str;
}

static PyObject* parse(PyObject* /* module */, PyObject** args,
                       Py_ssize_t nargs, PyObject* kwnames) {
  static const char* const parser_kwnames[] = {"source", "filename", "mode",
                                               "flags", nullptr};
  static _PyArg_Parser parser = {"OO&s|i:compile", parser_kwnames, nullptr};
  PyObject* source;
  PyObject* filename;
  const char* mode;
  int flags = 0;
  if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &parser, &source,
                                    PyUnicode_FSDecoder, &filename, &mode,
                                    &flags)) {
    return nullptr;
  }

  int compile_mode = -1;
  if (std::strcmp(mode, "exec") == 0) {
    compile_mode = 0;
  } else if (std::strcmp(mode, "eval") == 0) {
    compile_mode = 1;
  } else if (std::strcmp(mode, "single") == 0) {
    compile_mode = 2;
  } else {
    PyErr_SetString(PyExc_ValueError,
                    "parse() mode must be 'exec', 'eval' or 'single'");
    Py_DECREF(filename);
    return nullptr;
  }

  PyCompilerFlags cf = _PyCompilerFlags_INIT;
  cf.cf_flags = flags | PyCF_SOURCE_IS_UTF8;
  PyArena* arena = PyArena_New();
  if (arena == nullptr) return nullptr;

  PyObject* source_copy;
  const char* str = source_as_string(source, &cf, &source_copy);
  if (str == nullptr) {
    Py_DECREF(filename);
    PyArena_Free(arena);
    return nullptr;
  }

  static const int start[] = {Py_file_input, Py_eval_input, Py_single_input};
  mod_ty mod = PyParser_ASTFromStringObject(str, filename, start[compile_mode],
                                            &cf, arena);
  if (mod == nullptr) {
    Py_XDECREF(source_copy);
    PyArena_Free(arena);
    return nullptr;
  }
  PyObject* result = PyAST_mod2obj(mod);
  Py_XDECREF(source_copy);
  PyArena_Free(arena);
  return result;
}

static PyMethodDef _parse_methods[] = {
    {"parse",
     reinterpret_cast<PyCFunction>(reinterpret_cast<void (*)()>(parse)),
     METH_FASTCALL | METH_KEYWORDS, ""},
    {nullptr, nullptr}};

static struct PyModuleDef _parsermodule = {
    PyModuleDef_HEAD_INIT,
    "_parser",
    "",
    0,
    _parse_methods,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

PyMODINIT_FUNC PyInit__parser() {
  PyObject* module = PyState_FindModule(&_parsermodule);
  if (module != nullptr) {
    Py_INCREF(module);
    return module;
  }
  module = PyModule_Create(&_parsermodule);
  if (module == nullptr) {
    return nullptr;
  }

  PyState_AddModule(module, &_parsermodule);
  return module;
}
