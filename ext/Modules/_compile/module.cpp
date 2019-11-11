#include <cstring>

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "ast.h"
#include "compile.h"

static PyObject* Py_CompileStringObject(const char* str, PyObject* filename,
                                        int start, PyCompilerFlags* flags,
                                        int optimize) {
  PyArena* arena = PyArena_New();
  if (arena == nullptr) return nullptr;

  mod_ty mod = PyParser_ASTFromStringObject(str, filename, start, flags, arena);
  if (mod == nullptr) {
    PyArena_Free(arena);
    return nullptr;
  }
  if (flags && (flags->cf_flags & PyCF_ONLY_AST)) {
    PyObject* result = PyAST_mod2obj(mod);
    PyArena_Free(arena);
    return result;
  }
  PyCodeObject* co =
      py::compile::PyAST_CompileObject(mod, filename, flags, optimize, arena);
  PyArena_Free(arena);
  return reinterpret_cast<PyObject*>(co);
}

static const char* source_as_string(PyObject* cmd, const char* funcname,
                                    const char* what, PyCompilerFlags* cf,
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
    /* Copy to NUL-terminated buffer. */
    *cmd_copy = PyBytes_FromStringAndSize((const char*)view.buf, view.len);
    PyBuffer_Release(&view);
    if (*cmd_copy == nullptr) {
      return nullptr;
    }
    str = PyBytes_AS_STRING(*cmd_copy);
    size = PyBytes_GET_SIZE(*cmd_copy);
  } else {
    PyErr_Format(PyExc_TypeError, "%s() arg 1 must be a %s object", funcname,
                 what);
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

static PyObject* _compile_compile_impl(PyObject*, PyObject* source,
                                       PyObject* filename, const char* mode,
                                       int flags, int optimize) {
  PyCompilerFlags cf;
  cf.cf_flags = flags | PyCF_SOURCE_IS_UTF8;

  if (flags & ~(PyCF_MASK | PyCF_MASK_OBSOLETE | PyCF_DONT_IMPLY_DEDENT |
                PyCF_ONLY_AST)) {
    PyErr_SetString(PyExc_ValueError, "compile(): unrecognised flags");
    Py_DECREF(filename);
    return nullptr;
  }
  /* XXX Warn if (supplied_flags & PyCF_MASK_OBSOLETE) != 0? */

  if (optimize < -1 || optimize > 2) {
    PyErr_SetString(PyExc_ValueError, "compile(): invalid optimize value");
    Py_DECREF(filename);
    return nullptr;
  }

  int compile_mode = -1;
  if (strcmp(mode, "exec") == 0)
    compile_mode = 0;
  else if (strcmp(mode, "eval") == 0)
    compile_mode = 1;
  else if (strcmp(mode, "single") == 0)
    compile_mode = 2;
  else {
    PyErr_SetString(PyExc_ValueError,
                    "compile() mode must be 'exec', 'eval' or 'single'");
    Py_DECREF(filename);
    return nullptr;
  }

  int is_ast = PyAST_Check(source);
  if (is_ast == -1) {
    Py_DECREF(filename);
    return nullptr;
  }
  PyObject* result;
  if (is_ast) {
    if (flags & PyCF_ONLY_AST) {
      Py_INCREF(source);
      result = source;
    } else {
      PyArena* arena;
      mod_ty mod;

      arena = PyArena_New();
      if (arena == nullptr) {
        Py_DECREF(filename);
        return nullptr;
      }
      mod = PyAST_obj2mod(source, arena, compile_mode);
      if (mod == nullptr) {
        PyArena_Free(arena);
      }
      if (!PyAST_Validate(mod)) {
        PyArena_Free(arena);
        Py_DECREF(filename);
        return nullptr;
      }
      result = (PyObject*)py::compile::PyAST_CompileObject(mod, filename, &cf,
                                                           optimize, arena);
      PyArena_Free(arena);
    }
    Py_DECREF(filename);
    return result;
  }

  PyObject* source_copy;
  const char* str = source_as_string(source, "compile", "string, bytes or AST",
                                     &cf, &source_copy);
  if (str == nullptr) {
    Py_DECREF(filename);
    return nullptr;
  }

  static const int start[] = {Py_file_input, Py_eval_input, Py_single_input};
  result =
      Py_CompileStringObject(str, filename, start[compile_mode], &cf, optimize);
  Py_XDECREF(source_copy);
  Py_DECREF(filename);
  return result;
}

static PyObject* _compile_compile(PyObject* module, PyObject** args,
                                  Py_ssize_t nargs, PyObject* kwnames) {
  static const char* const _keywords[] = {"source", "filename", "mode",
                                          "flags",  "optimize", nullptr};
  static _PyArg_Parser _parser = {"OO&s|ii:compile", _keywords, 0};
  PyObject* source;
  PyObject* filename;
  const char* mode;
  int flags = 0;
  int optimize = -1;
  if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser, &source,
                         PyUnicode_FSDecoder, &filename, &mode, &flags,
                         &optimize)) {
    return nullptr;
  }
  return _compile_compile_impl(module, source, filename, mode, flags, optimize);
}

static PyMethodDef _compile_methods[] = {
    {"compile",
     reinterpret_cast<PyCFunction>(
         reinterpret_cast<void (*)()>(_compile_compile)),
     METH_FASTCALL, ""},
    {nullptr, nullptr}};

static struct PyModuleDef _compilemodule = {
    PyModuleDef_HEAD_INIT,
    "_compile",
    "",
    0,
    _compile_methods,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

PyMODINIT_FUNC PyInit__compile() {
  PyObject* module = PyState_FindModule(&_compilemodule);
  if (module != nullptr) {
    Py_INCREF(module);
    return module;
  }
  module = PyModule_Create(&_compilemodule);
  if (module == nullptr) {
    return nullptr;
  }

  PyState_AddModule(module, &_compilemodule);
  return module;
}
