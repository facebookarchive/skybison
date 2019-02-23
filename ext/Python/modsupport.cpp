#include <cstdarg>
#include <cwchar>

#include "cpython-data.h"
#include "cpython-types.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"

#define FLAG_SIZE_T 1

namespace python {

PY_EXPORT int PyModule_AddObject(PyObject* pymodule, const char* name,
                                 PyObject* obj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object module_obj(&scope, ApiHandle::fromPyObject(pymodule)->asObject());
  if (!module_obj.isModule()) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  if (name == nullptr) {
    // TODO(cshapiro): throw a TypeError
    return -1;
  }
  Object key(&scope, runtime->newStrFromCStr(name));
  if (!key.isStr()) {
    // TODO(cshapiro): throw a MemoryError
    return -1;
  }
  Module module(&scope, *module_obj);
  Object value(&scope, ApiHandle::fromPyObject(obj)->asObject());
  runtime->moduleAtPut(module, key, value);
  return 0;
}

PY_EXPORT int PyModule_AddIntConstant(PyObject* m, const char* name,
                                      long value) {
  PyObject* o = PyLong_FromLong(value);
  if (!o) return -1;
  if (PyModule_AddObject(m, name, o) == 0) return 0;
  Py_DECREF(o);
  return -1;
}

PY_EXPORT int PyModule_AddStringConstant(PyObject* pymodule, const char* name,
                                         const char* value) {
  PyObject* str = PyUnicode_FromString(value);
  if (!str) return -1;
  if (PyModule_AddObject(pymodule, name, str) == 0) return 0;
  Py_DECREF(str);
  return -1;
}

static PyObject* doMakeTuple(const char**, va_list*, char, Py_ssize_t, int);
static PyObject* doMakeList(const char**, va_list*, char, Py_ssize_t, int);
static PyObject* doMakeDict(const char**, va_list*, char, Py_ssize_t, int);
static PyObject* doMakeValue(const char**, va_list*, int);

static void doIgnore(const char** p_format, va_list* p_va, char endchar,
                     Py_ssize_t n, int flags) {
  DCHECK(PyErr_Occurred() != nullptr, "exception has not been raised");
  PyObject* v = PyTuple_New(n);
  for (Py_ssize_t i = 0; i < n; i++) {
    PyObject *exception, *value, *tb;
    PyErr_Fetch(&exception, &value, &tb);
    PyObject* w = doMakeValue(p_format, p_va, flags);
    PyErr_Restore(exception, value, tb);
    if (w != nullptr) {
      if (v != nullptr) {
        PyTuple_SET_ITEM(v, i, w);
      } else {
        Py_DECREF(w);
      }
    }
  }
  Py_XDECREF(v);
  if (**p_format != endchar) {
    PyErr_SetString(PyExc_SystemError, "Unmatched paren in format");
    return;
  }
  if (endchar != '\0') {
    ++*p_format;
  }
}

static PyObject* doMakeDict(const char** p_format, va_list* p_va, char endchar,
                            Py_ssize_t n, int flags) {
  if (n < 0) {
    return nullptr;
  }
  if (n % 2) {
    PyErr_SetString(PyExc_SystemError, "Bad dict format");
    doIgnore(p_format, p_va, endchar, n, flags);
    return nullptr;
  }
  // Note that we can't bail immediately on error as this will leak refcounts on
  // any 'N' arguments.
  PyObject* d = PyDict_New();
  if (d == nullptr) {
    doIgnore(p_format, p_va, endchar, n, flags);
    return nullptr;
  }
  for (Py_ssize_t i = 0; i < n; i += 2) {
    PyObject* k = doMakeValue(p_format, p_va, flags);
    if (k == nullptr) {
      doIgnore(p_format, p_va, endchar, n - i - 1, flags);
      Py_DECREF(d);
      return nullptr;
    }
    PyObject* v = doMakeValue(p_format, p_va, flags);
    if (v == nullptr || PyDict_SetItem(d, k, v) < 0) {
      doIgnore(p_format, p_va, endchar, n - i - 2, flags);
      Py_DECREF(k);
      Py_XDECREF(v);
      Py_DECREF(d);
      return nullptr;
    }
    Py_DECREF(k);
    Py_DECREF(v);
  }
  if (**p_format != endchar) {
    Py_DECREF(d);
    PyErr_SetString(PyExc_SystemError, "Unmatched paren in format");
    return nullptr;
  }
  if (endchar != '\0') {
    ++*p_format;
  }
  return d;
}

static PyObject* doMakeList(const char** p_format, va_list* p_va, char endchar,
                            Py_ssize_t n, int flags) {
  if (n < 0) {
    return nullptr;
  }
  // Note that we can't bail immediately on error as this will leak refcounts on
  // any 'N' arguments.
  PyObject* v = PyList_New(n);
  if (v == nullptr) {
    doIgnore(p_format, p_va, endchar, n, flags);
    return nullptr;
  }
  for (Py_ssize_t i = 0; i < n; i++) {
    PyObject* w = doMakeValue(p_format, p_va, flags);
    if (w == nullptr) {
      doIgnore(p_format, p_va, endchar, n - i - 1, flags);
      Py_DECREF(v);
      return nullptr;
    }
    PyList_SetItem(v, i, w);
  }
  if (**p_format != endchar) {
    Py_DECREF(v);
    PyErr_SetString(PyExc_SystemError, "Unmatched paren in format");
    return nullptr;
  }
  if (endchar != '\0') {
    ++*p_format;
  }
  return v;
}

static PyObject* doMakeTuple(const char** p_format, va_list* p_va, char endchar,
                             Py_ssize_t n, int flags) {
  if (n < 0) {
    return nullptr;
  }
  // Note that we can't bail immediately on error as this will leak refcounts on
  // any 'N' arguments.
  PyObject* v = PyTuple_New(n);
  if (v == nullptr) {
    doIgnore(p_format, p_va, endchar, n, flags);
    return nullptr;
  }
  for (Py_ssize_t i = 0; i < n; i++) {
    PyObject* w = doMakeValue(p_format, p_va, flags);
    if (w == nullptr) {
      doIgnore(p_format, p_va, endchar, n - i - 1, flags);
      Py_DECREF(v);
      return nullptr;
    }
    PyTuple_SET_ITEM(v, i, w);
  }
  if (**p_format != endchar) {
    Py_DECREF(v);
    PyErr_SetString(PyExc_SystemError, "Unmatched paren in format");
    return nullptr;
  }
  if (endchar != '\0') {
    ++*p_format;
  }
  return v;
}

static Py_ssize_t countFormat(const char* format, char endchar) {
  Py_ssize_t count = 0;
  int level = 0;
  while (level > 0 || *format != endchar) {
    switch (*format) {
      case '\0':
        // Premature end
        PyErr_SetString(PyExc_SystemError, "unmatched paren in format");
        return -1;
      case '(':
      case '[':
      case '{':
        if (level == 0) {
          count++;
        }
        level++;
        break;
      case ')':
      case ']':
      case '}':
        level--;
        break;
      case '#':
      case '&':
      case ',':
      case ':':
      case ' ':
      case '\t':
        break;
      default:
        if (level == 0) {
          count++;
        }
    }
    format++;
  }
  return count;
}

static PyObject* doMakeValue(const char** p_format, va_list* p_va, int flags) {
  for (;;) {
    switch (*(*p_format)++) {
      case '(':
        return doMakeTuple(p_format, p_va, ')', countFormat(*p_format, ')'),
                           flags);

      case '[':
        return doMakeList(p_format, p_va, ']', countFormat(*p_format, ']'),
                          flags);

      case '{':
        return doMakeDict(p_format, p_va, '}', countFormat(*p_format, '}'),
                          flags);

      case 'b':
      case 'B':
      case 'h':
      case 'i':
        return PyLong_FromLong(va_arg(*p_va, int));

      case 'H':
        return PyLong_FromLong(va_arg(*p_va, unsigned int));

      case 'I':
        return PyLong_FromUnsignedLong(va_arg(*p_va, unsigned int));

      case 'n':
        if (sizeof(size_t) != sizeof(long)) {
          return PyLong_FromSsize_t(va_arg(*p_va, Py_ssize_t));
        }
        // fall through
        // Falls through from 'n' to 'l' if Py_ssize_t is long (above comment
        // avoids compiler warning)
      case 'l':
        return PyLong_FromLong(va_arg(*p_va, long));

      case 'k': {
        return PyLong_FromUnsignedLong(va_arg(*p_va, unsigned long));
      }

      case 'L':
        return PyLong_FromLongLong(va_arg(*p_va, long long));

      case 'K':
        return PyLong_FromUnsignedLongLong(va_arg(*p_va, unsigned long long));

      case 'u': {
        Py_UNICODE* u = va_arg(*p_va, Py_UNICODE*);
        Py_ssize_t n;
        if (**p_format == '#') {
          ++*p_format;
          if (flags & FLAG_SIZE_T) {
            n = va_arg(*p_va, Py_ssize_t);
          } else {
            n = va_arg(*p_va, int);
          }
        } else {
          n = -1;
        }
        PyObject* v;
        if (u == nullptr) {
          v = Py_None;
          Py_INCREF(v);
        } else {
          if (n < 0) {
            n = std::wcslen(u);
          }
          v = PyUnicode_FromWideChar(u, n);
        }
        return v;
      }
      case 'f':
      case 'd':
        return PyFloat_FromDouble(va_arg(*p_va, double));

      case 'D':
        return PyComplex_FromCComplex(*va_arg(*p_va, Py_complex*));

      case 'c': {
        char ch = va_arg(*p_va, int);
        return PyBytes_FromStringAndSize(&ch, 1);
      }
      case 'C': {
        int i = va_arg(*p_va, int);
        return PyUnicode_FromOrdinal(i);
      }

      case 's':
      case 'z':
      case 'U': {  // XXX deprecated alias
        const char* str = va_arg(*p_va, const char*);
        Py_ssize_t n;
        if (**p_format == '#') {
          ++*p_format;
          if (flags & FLAG_SIZE_T) {
            n = va_arg(*p_va, Py_ssize_t);
          } else {
            n = va_arg(*p_va, int);
          }
        } else {
          n = -1;
        }
        PyObject* v;
        if (str == nullptr) {
          v = Py_None;
          Py_INCREF(v);
        } else {
          if (n < 0) {
            size_t m = std::strlen(str);
            if (m > kMaxWord) {
              PyErr_SetString(PyExc_OverflowError,
                              "string too long for Python string");
              return nullptr;
            }
            n = static_cast<Py_ssize_t>(m);
          }
          v = PyUnicode_FromStringAndSize(str, n);
        }
        return v;
      }

      case 'y': {
        PyObject* v;
        const char* str = va_arg(*p_va, const char*);
        Py_ssize_t n;
        if (**p_format == '#') {
          ++*p_format;
          if (flags & FLAG_SIZE_T) {
            n = va_arg(*p_va, Py_ssize_t);
          } else {
            n = va_arg(*p_va, int);
          }
        } else {
          n = -1;
        }
        if (str == nullptr) {
          v = Py_None;
          Py_INCREF(v);
        } else {
          if (n < 0) {
            size_t m = std::strlen(str);
            if (m > kMaxWord) {
              PyErr_SetString(PyExc_OverflowError,
                              "string too long for Python bytes");
              return nullptr;
            }
            n = static_cast<Py_ssize_t>(m);
          }
          v = PyBytes_FromStringAndSize(str, n);
        }
        return v;
      }

      case 'N':
      case 'S':
      case 'O':
        if (**p_format == '&') {
          typedef PyObject* (*converter)(void*);
          converter func = va_arg(*p_va, converter);
          void* arg = va_arg(*p_va, void*);
          ++*p_format;
          return (*func)(arg);
        } else {
          PyObject* v = va_arg(*p_va, PyObject*);
          if (v != nullptr) {
            if (*(*p_format - 1) != 'N') Py_INCREF(v);
          } else if (!PyErr_Occurred()) {
            // If a NULL was passed because a call that should have constructed
            // a value failed, that's OK, and we pass the error on; but if no
            // error occurred it's not clear that the caller knew what she was
            // doing.
            PyErr_SetString(PyExc_SystemError,
                            "NULL object passed to Py_BuildValue");
          }
          return v;
        }

      case ':':
      case ',':
      case ' ':
      case '\t':
        break;

      default:
        PyErr_SetString(PyExc_SystemError,
                        "bad format char passed to Py_BuildValue");
        return nullptr;
    }
  }
}

static PyObject* vaBuildValue(const char* format, va_list va, int flags) {
  Py_ssize_t n = countFormat(format, '\0');
  if (n < 0) {
    return nullptr;
  }
  if (n == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  va_list lva;
  va_copy(lva, va);
  const char* f = format;
  PyObject* retval;
  if (n == 1) {
    retval = doMakeValue(&f, &lva, flags);
  } else {
    retval = doMakeTuple(&f, &lva, '\0', n, flags);
  }
  va_end(lva);
  return retval;
}

PY_EXPORT PyObject* Py_VaBuildValue(const char* format, va_list va) {
  return vaBuildValue(format, va, 0);
}

PY_EXPORT PyObject* _Py_VaBuildValue_SizeT(const char* format, va_list va) {
  return vaBuildValue(format, va, FLAG_SIZE_T);
}

PY_EXPORT PyObject* Py_BuildValue(const char* format, ...) {
  va_list va;
  va_start(va, format);
  PyObject* retval = vaBuildValue(format, va, 0);
  va_end(va);
  return retval;
}

PY_EXPORT PyObject* _Py_BuildValue_SizeT(const char* format, ...) {
  va_list va;
  va_start(va, format);
  PyObject* retval = vaBuildValue(format, va, FLAG_SIZE_T);
  va_end(va);
  return retval;
}

PY_EXPORT PyObject* PyEval_CallFunction(PyObject* /* e */, const char* /* t */,
                                        ...) {
  UNIMPLEMENTED("PyEval_CallFunction");
}

PY_EXPORT PyObject* PyEval_CallMethod(PyObject* /* j */, const char* /* e */,
                                      const char* /* t */, ...) {
  UNIMPLEMENTED("PyEval_CallMethod");
}

}  // namespace python
