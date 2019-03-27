// unicodeobject.c implementation
#include <cstdarg>
#include <cstring>
#include <cwchar>

#include "bytearray-builtins.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "utils.h"

const char* Py_FileSystemDefaultEncodeErrors = "surrogateescape";

namespace python {

typedef byte Py_UCS1;
typedef uint16_t Py_UCS2;

static const int kMaxLongLongChars = 19;  // len(str(2**63-1))
static const int kOverallocateFactor = 4;

struct _PyUnicodeWriter {  // NOLINT
  PyObject* buffer;
  void* data;
  enum PyUnicode_Kind kind;
  Py_UCS4 maxchar;
  Py_ssize_t size;
  Py_ssize_t pos;
  Py_ssize_t min_length;
  Py_UCS4 min_char;
  unsigned char overallocate;
  unsigned char readonly;
};  // NOLINT

PY_EXPORT void PyUnicode_WRITE_Func(enum PyUnicode_Kind kind, void* data,
                                    Py_ssize_t index, Py_UCS4 value) {
  if (kind == PyUnicode_1BYTE_KIND) {
    static_cast<Py_UCS1*>(data)[index] = static_cast<Py_UCS1>(value);
  } else if (kind == PyUnicode_2BYTE_KIND) {
    static_cast<Py_UCS2*>(data)[index] = static_cast<Py_UCS2>(value);
  } else {
    DCHECK(kind == PyUnicode_4BYTE_KIND, "kind must be PyUnicode_4BYTE_KIND");
    static_cast<Py_UCS4*>(data)[index] = static_cast<Py_UCS4>(value);
  }
}

PY_EXPORT void _PyUnicodeWriter_Dealloc(_PyUnicodeWriter* writer) {
  PyMem_Free(writer->data);
}

PY_EXPORT PyObject* _PyUnicodeWriter_Finish(_PyUnicodeWriter* writer) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, thread->runtime()->newStrFromUTF32(View<int32>(
                      static_cast<int32*>(writer->data), writer->pos)));
  PyMem_Free(writer->data);
  return ApiHandle::newReference(thread, *str);
}

PY_EXPORT void _PyUnicodeWriter_Init(_PyUnicodeWriter* writer) {
  std::memset(writer, 0, sizeof(*writer));
  writer->kind = PyUnicode_4BYTE_KIND;
}

PY_EXPORT int _PyUnicodeWriter_PrepareInternal(_PyUnicodeWriter* writer,
                                               Py_ssize_t length,
                                               Py_UCS4 /* maxchar */) {
  writer->maxchar = kMaxUnicode;
  if (length > kMaxWord - writer->pos) {
    Thread::current()->raiseMemoryError();
    return -1;
  }
  Py_ssize_t newlen = writer->pos + length;
  if (writer->data == nullptr) {
    if (writer->overallocate &&
        newlen <= (kMaxWord - newlen / kOverallocateFactor)) {
      // overallocate to limit the number of realloc()
      newlen += newlen / kOverallocateFactor;
    }
    writer->data = PyMem_Malloc(newlen * sizeof(int32));
    if (writer->data == nullptr) return -1;
  } else if (newlen > writer->size) {
    if (writer->overallocate &&
        newlen <= (kMaxWord - newlen / kOverallocateFactor)) {
      // overallocate to limit the number of realloc()
      newlen += newlen / kOverallocateFactor;
    }
    writer->data = PyMem_Realloc(writer->data, newlen * sizeof(int32));
    if (writer->data == nullptr) return -1;
  }
  writer->size = newlen;
  return 0;
}

PY_EXPORT int _PyUnicodeWriter_Prepare(_PyUnicodeWriter* writer,
                                       Py_ssize_t length, Py_UCS4 maxchar) {
  if (length <= writer->size - writer->pos || length == 0) return 0;
  return _PyUnicodeWriter_PrepareInternal(writer, length, maxchar);
}

PY_EXPORT int _PyUnicodeWriter_WriteASCIIString(_PyUnicodeWriter* writer,
                                                const char* ascii,
                                                Py_ssize_t len) {
  if (len == -1) len = std::strlen(ascii);
  if (writer->data == nullptr && !writer->overallocate) {
    writer->data = PyMem_Malloc(len * sizeof(int32));
    writer->size = len;
  }

  if (_PyUnicodeWriter_Prepare(writer, len, kMaxUnicode) == -1) return -1;
  for (Py_ssize_t i = 0; i < len; ++i, writer->pos++) {
    CHECK(ascii[i] >= 0, "_PyUnicodeWriter_WriteASCIIString only takes ASCII");
    PyUnicode_WRITE(PyUnicode_4BYTE_KIND, writer->data, writer->pos, ascii[i]);
  }
  return 0;
}

PY_EXPORT int _PyUnicodeWriter_WriteCharInline(_PyUnicodeWriter* writer,
                                               Py_UCS4 ch) {
  if (_PyUnicodeWriter_Prepare(writer, 1, ch) < 0) return -1;
  PyUnicode_WRITE(PyUnicode_4BYTE_KIND, writer->data, writer->pos, ch);
  writer->pos++;
  return 0;
}

PY_EXPORT int _PyUnicodeWriter_WriteChar(_PyUnicodeWriter* writer, Py_UCS4 ch) {
  return _PyUnicodeWriter_WriteCharInline(writer, ch);
}

PY_EXPORT int _PyUnicodeWriter_WriteLatin1String(_PyUnicodeWriter* writer,
                                                 const char* str,
                                                 Py_ssize_t len) {
  if (_PyUnicodeWriter_Prepare(writer, len, kMaxUnicode) == -1) return -1;
  for (Py_ssize_t i = 0; i < len; ++i, writer->pos++) {
    PyUnicode_WRITE(PyUnicode_4BYTE_KIND, writer->data, writer->pos,
                    str[i] & 0xFF);
  }
  return 0;
}

PY_EXPORT int _PyUnicodeWriter_WriteStr(_PyUnicodeWriter* writer,
                                        PyObject* str) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str src(&scope, ApiHandle::fromPyObject(str)->asObject());
  Py_ssize_t len = src.length();
  if (_PyUnicodeWriter_Prepare(writer, len, kMaxUnicode) == -1) return -1;
  for (Py_ssize_t i = 0; i < len; ++i, writer->pos++) {
    PyUnicode_WRITE(PyUnicode_4BYTE_KIND, writer->data, writer->pos,
                    src.charAt(i));
  }
  return 0;
}

PY_EXPORT int _PyUnicodeWriter_WriteSubstring(_PyUnicodeWriter* writer,
                                              PyObject* str, Py_ssize_t start,
                                              Py_ssize_t end) {
  if (end == 0) return 0;
  Py_ssize_t len = end - start;
  if (_PyUnicodeWriter_Prepare(writer, len, kMaxUnicode) < 0) return -1;

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str src(&scope, ApiHandle::fromPyObject(str)->asObject());
  for (Py_ssize_t i = start; i < end; ++i, writer->pos++) {
    PyUnicode_WRITE(PyUnicode_4BYTE_KIND, writer->data, writer->pos,
                    src.charAt(i));
  }
  return 0;
}

// Facebook: D13491655
// Most of the following helper functions, along with PyUnicode_FromFormat and
// PyUnicode_FromFormatV are directly imported from CPython. The following
// modifications have been made:
//
// - Since our internal strings are always UTF-8, we don't need maxchar or any
// of the helper functions required to calculate it
//
// - Since our strings are immutable, we can't use PyUnicode_Fill. However,
// since the helper functions always use it to append to strings, we can get
// away with just writing characters in a loop.
//
// - Since our internal strings are always UTF-8, there is no need to check
// a character's 'Kind' before writing it to a string
static int writeStr(_PyUnicodeWriter* writer, PyObject* str, Py_ssize_t width,
                    Py_ssize_t precision) {
  if (PyUnicode_READY(str) == -1) return -1;

  Py_ssize_t length = PyUnicode_GET_LENGTH(str);
  if ((precision == -1 || precision >= length) && width <= length) {
    return _PyUnicodeWriter_WriteStr(writer, str);
  }

  if (precision != -1) length = Py_MIN(precision, length);

  Py_ssize_t arglen = Py_MAX(length, width);
  // Facebook: Our internal strings are always UTF-8, don't need maxchar
  // (D13491655)
  if (_PyUnicodeWriter_Prepare(writer, arglen, 0) == -1) return -1;

  if (width > length) {
    Py_ssize_t fill = width - length;
    // Facebook: Our internal strings are immutable, can't use PyUnicode_Fill
    // (D13491655)
    for (Py_ssize_t i = 0; i < fill; ++i) {
      if (_PyUnicodeWriter_WriteCharInline(writer, ' ') == -1) return -1;
    }
  }
  // Facebook: Since we only have one internal representation, we don't have
  // to worry about changing a string's 'Kind' (D13491655)
  return _PyUnicodeWriter_WriteSubstring(writer, str, 0, length);
}

static int writeCStr(_PyUnicodeWriter* writer, const char* str,
                     Py_ssize_t width, Py_ssize_t precision) {
  Py_ssize_t length = std::strlen(str);
  if (precision != -1) length = Py_MIN(length, precision);
  PyObject* unicode =
      PyUnicode_DecodeUTF8Stateful(str, length, "replace", nullptr);
  if (unicode == nullptr) return -1;

  int res = writeStr(writer, unicode, width, -1);
  Py_DECREF(unicode);
  return res;
}

static const char* writeArg(_PyUnicodeWriter* writer, const char* f,
                            va_list* vargs) {
  const char* p = f;
  f++;
  int zeropad = 0;
  if (*f == '0') {
    zeropad = 1;
    f++;
  }

  // parse the width.precision part, e.g. "%2.5s" => width=2, precision=5
  Py_ssize_t width = -1;
  if (Py_ISDIGIT(static_cast<unsigned>(*f))) {
    width = *f - '0';
    f++;
    while (Py_ISDIGIT(static_cast<unsigned>(*f))) {
      if (width > (kMaxWord - (static_cast<int>(*f) - '0')) / 10) {
        PyErr_SetString(PyExc_ValueError, "width too big");
        return nullptr;
      }
      width = (width * 10) + (*f - '0');
      f++;
    }
  }
  Py_ssize_t precision = -1;
  if (*f == '.') {
    f++;
    if (Py_ISDIGIT(static_cast<unsigned>(*f))) {
      precision = (*f - '0');
      f++;
      while (Py_ISDIGIT(static_cast<unsigned>(*f))) {
        if (precision > (kMaxWord - (static_cast<int>(*f) - '0')) / 10) {
          PyErr_SetString(PyExc_ValueError, "precision too big");
          return nullptr;
        }
        precision = (precision * 10) + (*f - '0');
        f++;
      }
    }
    if (*f == '%') {
      // "%.3%s" => f points to "3"
      f--;
    }
  }
  if (*f == '\0') {
    // bogus format "%.123" => go backward, f points to "3"
    f--;
  }

  // Handle %ld, %lu, %lld and %llu.
  int longflag = 0;
  int longlongflag = 0;
  int size_tflag = 0;
  if (*f == 'l') {
    if (f[1] == 'd' || f[1] == 'u' || f[1] == 'i') {
      longflag = 1;
      ++f;
    } else if (f[1] == 'l' && (f[2] == 'd' || f[2] == 'u' || f[2] == 'i')) {
      longlongflag = 1;
      f += 2;
    }
  }
  // handle the size_t flag.
  else if (*f == 'z' && (f[1] == 'd' || f[1] == 'u' || f[1] == 'i')) {
    size_tflag = 1;
    ++f;
  }

  if (f[1] == '\0') writer->overallocate = 0;

  switch (*f) {
    case 'c': {
      int ordinal = va_arg(*vargs, int);
      if (ordinal < 0 || ordinal > kMaxUnicode) {
        PyErr_SetString(PyExc_OverflowError,
                        "character argument not in range(0x110000)");
        return nullptr;
      }
      if (_PyUnicodeWriter_WriteCharInline(writer, ordinal) < 0) return nullptr;
      break;
    }

    case 'i':
    case 'd':
    case 'u':
    case 'x': {
      // used by sprintf
      char buffer[kMaxLongLongChars];
      Py_ssize_t len;

      if (*f == 'u') {
        if (longflag) {
          len = std::sprintf(buffer, "%lu", va_arg(*vargs, unsigned long));
        } else if (longlongflag) {
          len =
              std::sprintf(buffer, "%llu", va_arg(*vargs, unsigned long long));
        } else if (size_tflag) {
          len = std::sprintf(buffer, "%" PY_FORMAT_SIZE_T "u",
                             va_arg(*vargs, size_t));
        } else {
          len = std::sprintf(buffer, "%u", va_arg(*vargs, unsigned int));
        }
      } else if (*f == 'x') {
        len = std::sprintf(buffer, "%x", va_arg(*vargs, int));
      } else {
        if (longflag) {
          len = std::sprintf(buffer, "%li", va_arg(*vargs, long));
        } else if (longlongflag) {
          len = std::sprintf(buffer, "%lli", va_arg(*vargs, long long));
        } else if (size_tflag) {
          len = std::sprintf(buffer, "%" PY_FORMAT_SIZE_T "i",
                             va_arg(*vargs, Py_ssize_t));
        } else {
          len = std::sprintf(buffer, "%i", va_arg(*vargs, int));
        }
      }
      DCHECK(len >= 0, "len must be >= 0");

      if (precision < len) precision = len;

      Py_ssize_t arglen = Py_MAX(precision, width);
      if (_PyUnicodeWriter_Prepare(writer, arglen, 127) == -1) return nullptr;

      if (width > precision) {
        Py_ssize_t fill = width - precision;
        Py_UCS4 fillchar = zeropad ? '0' : ' ';
        // Facebook: Our internal strings are immutable, can't use
        // PyUnicode_Fill (D13491655)
        for (Py_ssize_t i = 0; i < fill; ++i) {
          if (_PyUnicodeWriter_WriteCharInline(writer, fillchar) == -1) {
            return nullptr;
          }
        }
      }
      if (precision > len) {
        Py_ssize_t fill = precision - len;
        // Facebook: Our internal strings are immutable, can't use
        // PyUnicode_Fill (D13491655)
        for (Py_ssize_t i = 0; i < fill; ++i) {
          if (_PyUnicodeWriter_WriteCharInline(writer, '0') == -1) {
            return nullptr;
          }
        }
      }

      if (_PyUnicodeWriter_WriteASCIIString(writer, buffer, len) < 0) {
        return nullptr;
      }
      break;
    }

    case 'p': {
      char number[kMaxLongLongChars];

      Py_ssize_t len = std::sprintf(number, "%p", va_arg(*vargs, void*));
      DCHECK(len >= 0, "len must be >= 0");

      // %p is ill-defined:  ensure leading 0x.
      if (number[1] == 'X') {
        number[1] = 'x';
      } else if (number[1] != 'x') {
        std::memmove(number + 2, number, strlen(number) + 1);
        number[0] = '0';
        number[1] = 'x';
        len += 2;
      }

      if (_PyUnicodeWriter_WriteASCIIString(writer, number, len) < 0) {
        return nullptr;
      }
      break;
    }

    case 's': {
      // UTF-8
      const char* s = va_arg(*vargs, const char*);
      if (writeCStr(writer, s, width, precision) < 0) {
        return nullptr;
      }
      break;
    }

    case 'U': {
      PyObject* obj = va_arg(*vargs, PyObject*);
      // This used to call _PyUnicode_CHECK, which is deprecated, and which we
      // have not imported.
      DCHECK(obj, "obj must not be null");

      if (writeStr(writer, obj, width, precision) == -1) {
        return nullptr;
      }
      break;
    }

    case 'V': {
      PyObject* obj = va_arg(*vargs, PyObject*);
      const char* str = va_arg(*vargs, const char*);
      if (obj) {
        // This used to DCHECK _PyUnicode_CHECK, which is deprecated, and which
        // we have not imported.
        if (writeStr(writer, obj, width, precision) == -1) {
          return nullptr;
        }
      } else {
        DCHECK(str != nullptr, "str must not be null");
        if (writeCStr(writer, str, width, precision) < 0) {
          return nullptr;
        }
      }
      break;
    }

    case 'S': {
      PyObject* obj = va_arg(*vargs, PyObject*);
      DCHECK(obj, "obj must not be null");
      PyObject* str = PyObject_Str(obj);
      if (!str) return nullptr;
      if (writeStr(writer, str, width, precision) == -1) {
        Py_DECREF(str);
        return nullptr;
      }
      Py_DECREF(str);
      break;
    }

    case 'R': {
      PyObject* obj = va_arg(*vargs, PyObject*);
      DCHECK(obj, "obj must not be null");
      PyObject* repr = PyObject_Repr(obj);
      if (!repr) return nullptr;
      if (writeStr(writer, repr, width, precision) == -1) {
        Py_DECREF(repr);
        return nullptr;
      }
      Py_DECREF(repr);
      break;
    }

    case 'A': {
      PyObject* obj = va_arg(*vargs, PyObject*);
      DCHECK(obj, "obj must not be null");
      PyObject* ascii = PyObject_ASCII(obj);
      if (!ascii) return nullptr;
      if (writeStr(writer, ascii, width, precision) == -1) {
        Py_DECREF(ascii);
        return nullptr;
      }
      Py_DECREF(ascii);
      break;
    }

    case '%':
      if (_PyUnicodeWriter_WriteCharInline(writer, '%') < 0) return nullptr;
      break;

    default: {
      // if we stumble upon an unknown formatting code, copy the rest
      // of the format string to the output string. (we cannot just
      // skip the code, since there's no way to know what's in the
      // argument list)
      Py_ssize_t len = strlen(p);
      if (_PyUnicodeWriter_WriteLatin1String(writer, p, len) == -1) {
        return nullptr;
      }
      f = p + len;
      return f;
    }
  }

  f++;
  return f;
}

PY_EXPORT int _PyUnicode_EqualToASCIIString(PyObject* unicode,
                                            const char* c_str) {
  DCHECK(unicode, "nullptr argument");
  DCHECK(c_str, "nullptr argument");

  HandleScope scope;
  Str str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  return str.equalsCStr(c_str);
}

PY_EXPORT int _PyUnicode_EQ(PyObject* aa, PyObject* bb) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str lhs(&scope, ApiHandle::fromPyObject(aa)->asObject());
  Str rhs(&scope, ApiHandle::fromPyObject(bb)->asObject());
  word diff = lhs.compare(*rhs);
  return diff == 0 ? 1 : 0;
}

PY_EXPORT size_t Py_UNICODE_strlen(const Py_UNICODE* u) {
  DCHECK(u != nullptr, "u should not be null");
  return std::wcslen(u);
}

PY_EXPORT int _PyUnicode_Ready(PyObject* /* unicode */) { return 0; }

PY_EXPORT int PyUnicode_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isStr();
}

PY_EXPORT int PyUnicode_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfStr(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT PyObject* PyUnicode_FromString(const char* c_string) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newStrFromCStr(c_string));
  return ApiHandle::newReference(thread, *value);
}

PY_EXPORT char* PyUnicode_AsUTF8AndSize(PyObject* pyunicode, Py_ssize_t* size) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  if (pyunicode == nullptr) {
    thread->raiseBadArgument();
    return nullptr;
  }

  auto handle = ApiHandle::fromPyObject(pyunicode);
  Object obj(&scope, handle->asObject());
  if (!obj.isStr()) {
    if (thread->runtime()->isInstanceOfStr(*obj)) {
      UNIMPLEMENTED("RawStr subclass");
    }
    thread->raiseBadInternalCall();
    return nullptr;
  }

  Str str(&scope, *obj);
  word length = str.length();
  if (size) *size = length;
  if (void* cache = handle->cache()) return static_cast<char*>(cache);
  auto result = static_cast<byte*>(std::malloc(length + 1));
  str.copyTo(result, length);
  result[length] = '\0';
  handle->setCache(result);
  return reinterpret_cast<char*>(result);
}

PY_EXPORT char* PyUnicode_AsUTF8(PyObject* unicode) {
  return PyUnicode_AsUTF8AndSize(unicode, nullptr);
}

PY_EXPORT PyObject* PyUnicode_FromStringAndSize(const char* u,
                                                Py_ssize_t size) {
  Thread* thread = Thread::current();

  if (size < 0) {
    thread->raiseSystemErrorWithCStr(
        "Negative size passed to PyUnicode_FromStringAndSize");
    return nullptr;
  }
  if (u == nullptr) {
    // TODO(T36562134): Implement _PyUnicode_New
    UNIMPLEMENTED("_PyUnicode_New");
  }
  HandleScope scope(thread);
  const byte* data = reinterpret_cast<const byte*>(u);
  Object value(&scope,
               thread->runtime()->newStrWithAll(View<byte>(data, size)));
  return ApiHandle::newReference(thread, *value);
}

PY_EXPORT PyObject* PyUnicode_EncodeFSDefault(PyObject* unicode) {
  // TODO(T40363016): Allow arbitrary encodings instead of defaulting to utf-8
  return _PyUnicode_AsUTF8String(unicode, Py_FileSystemDefaultEncodeErrors);
}

PY_EXPORT PyObject* PyUnicode_New(Py_ssize_t size, Py_UCS4 maxchar) {
  Thread* thread = Thread::current();
  // Since CPython optimizes for empty string, we must do so as well to make
  // sure we don't fail if maxchar is invalid
  if (size == 0) {
    return ApiHandle::newReference(thread, Str::empty());
  }
  if (maxchar > kMaxUnicode) {
    thread->raiseSystemErrorWithCStr(
        "invalid maximum character passed to PyUnicode_New");
    return nullptr;
  }
  if (size < 0) {
    thread->raiseSystemErrorWithCStr("Negative size passed to PyUnicode_New");
    return nullptr;
  }
  // TODO(T41498010): Add modifiable string state
  UNIMPLEMENTED("Cannot create mutable strings yet");
}

PY_EXPORT void PyUnicode_Append(PyObject** p_left, PyObject* right) {
  if (p_left == nullptr) {
    if (!PyErr_Occurred()) {
      PyErr_BadInternalCall();
    }
    return;
  }

  PyObject* left = *p_left;
  if (left == nullptr || right == nullptr || !PyUnicode_Check(left) ||
      !PyUnicode_Check(right)) {
    if (!PyErr_Occurred()) {
      PyErr_BadInternalCall();
    }
    Py_CLEAR(*p_left);
    return;
  }
  *p_left = PyUnicode_Concat(left, right);
  Py_DECREF(left);
}

PY_EXPORT void PyUnicode_AppendAndDel(PyObject** p_left, PyObject* right) {
  PyUnicode_Append(p_left, right);
  Py_XDECREF(right);
}

PY_EXPORT PyObject* PyUnicode_AsASCIIString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsASCIIString");
}

PY_EXPORT PyObject* PyUnicode_AsCharmapString(PyObject* /* e */,
                                              PyObject* /* g */) {
  UNIMPLEMENTED("PyUnicode_AsCharmapString");
}

PY_EXPORT PyObject* PyUnicode_AsDecodedObject(PyObject* /* e */,
                                              const char* /* g */,
                                              const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsDecodedObject");
}

PY_EXPORT PyObject* PyUnicode_AsDecodedUnicode(PyObject* /* e */,
                                               const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsDecodedUnicode");
}

PY_EXPORT PyObject* PyUnicode_AsEncodedObject(PyObject* /* e */,
                                              const char* /* g */,
                                              const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsEncodedObject");
}

PY_EXPORT PyObject* PyUnicode_AsEncodedString(PyObject* /* e */,
                                              const char* /* g */,
                                              const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsEncodedString");
}

PY_EXPORT PyObject* PyUnicode_AsEncodedUnicode(PyObject* /* e */,
                                               const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsEncodedUnicode");
}

PY_EXPORT PyObject* PyUnicode_AsLatin1String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsLatin1String");
}

PY_EXPORT PyObject* PyUnicode_AsMBCSString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsMBCSString");
}

PY_EXPORT PyObject* PyUnicode_AsRawUnicodeEscapeString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsRawUnicodeEscapeString");
}

PY_EXPORT Py_UCS4* PyUnicode_AsUCS4(PyObject* u, Py_UCS4* buffer,
                                    Py_ssize_t buflen, int copy_null) {
  if (buffer == nullptr || buflen < 0) {
    PyErr_BadInternalCall();
    return nullptr;
  }

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(u)->asObject());
  if (!thread->runtime()->isInstanceOfStr(*obj)) {
    thread->raiseBadArgument();
  }
  if (!obj.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *obj);

  word num_codepoints = str.codePointLength();
  word target_buflen = copy_null ? num_codepoints + 1 : num_codepoints;
  if (buflen < target_buflen) {
    PyErr_Format(PyExc_SystemError, "string is longer than the buffer");
    if (copy_null != 0 && 0 < buflen) {
      buffer[0] = 0;
    }
    return nullptr;
  }

  for (word i = 0, offset = 0; i < num_codepoints; i++) {
    word num_bytes;
    buffer[i] = str.codePointAt(offset, &num_bytes);
    offset += num_bytes;
  }
  if (copy_null != 0) buffer[num_codepoints] = 0;

  return buffer;
}

PY_EXPORT Py_UCS4* PyUnicode_AsUCS4Copy(PyObject* str) {
  Py_ssize_t len = PyUnicode_GET_LENGTH(str) + 1;
  Py_UCS4* result = static_cast<Py_UCS4*>(PyMem_Malloc(len * sizeof(Py_UCS4)));
  if (result == nullptr) {
    PyErr_NoMemory();
    return nullptr;
  }
  return PyUnicode_AsUCS4(str, result, len, 1);
}

PY_EXPORT PyObject* PyUnicode_AsUTF16String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUTF16String");
}

PY_EXPORT PyObject* PyUnicode_AsUTF32String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUTF32String");
}

PY_EXPORT PyObject* PyUnicode_AsUTF8String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUTF8String");
}

PY_EXPORT PyObject* PyUnicode_AsUnicodeEscapeString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUnicodeEscapeString");
}

PY_EXPORT Py_ssize_t PyUnicode_AsWideChar(PyObject* /* e */, wchar_t* /* w */,
                                          Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_AsWideChar");
}

PY_EXPORT wchar_t* PyUnicode_AsWideCharString(PyObject* /* e */,
                                              Py_ssize_t* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsWideCharString");
}

PY_EXPORT PyObject* PyUnicode_BuildEncodingMap(PyObject* /* g */) {
  UNIMPLEMENTED("PyUnicode_BuildEncodingMap");
}

PY_EXPORT int PyUnicode_ClearFreeList() { return 0; }

PY_EXPORT int PyUnicode_Compare(PyObject* left, PyObject* right) {
  Thread* thread = Thread::current();
  if (left == nullptr || right == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object left_obj(&scope, ApiHandle::fromPyObject(left)->asObject());
  Object right_obj(&scope, ApiHandle::fromPyObject(right)->asObject());
  if (runtime->isInstanceOfStr(*left_obj) &&
      runtime->isInstanceOfStr(*right_obj)) {
    Str left_str(&scope, *left_obj);
    return left_str.compare(*right_obj);
  }
  thread->raiseTypeError(runtime->newStrFromFormat("Can't compare %T and %T",
                                                   &left_obj, &right_obj));
  return -1;
}

PY_EXPORT int PyUnicode_CompareWithASCIIString(PyObject* uni, const char* str) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str_obj(&scope, ApiHandle::fromPyObject(uni)->asObject());
  // TODO(atalaba): Allow for proper comparison against Latin-1 strings. For
  // example, in CPython: "\xC3\xA9" (UTF-8) == "\xE9" (Latin-1), and
  // "\xE9 longer" > "\xC3\xA9".
  return str_obj.compareCStr(str);
}

PY_EXPORT PyObject* PyUnicode_Concat(PyObject* left, PyObject* right) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object left_obj(&scope, ApiHandle::fromPyObject(left)->asObject());
  Object right_obj(&scope, ApiHandle::fromPyObject(right)->asObject());
  if (!runtime->isInstanceOfStr(*left_obj) ||
      !runtime->isInstanceOfStr(*right_obj)) {
    thread->raiseTypeErrorWithCStr("can only concatenate str to str");
    return nullptr;
  }
  // TODO(T36619828): Implement Str subclass support
  Str left_str(&scope, *left_obj);
  Str right_str(&scope, *right_obj);
  word dummy;
  if (__builtin_add_overflow(left_str.length(), right_str.length(), &dummy)) {
    thread->raiseOverflowErrorWithCStr("strings are too large to concat");
    return nullptr;
  }
  return ApiHandle::newReference(
      thread, runtime->strConcat(thread, left_str, right_str));
}

PY_EXPORT int PyUnicode_Contains(PyObject* str, PyObject* substr) {
  DCHECK(str != nullptr, "str should not be null");
  DCHECK(substr != nullptr, "substr should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Object substr_obj(&scope, ApiHandle::fromPyObject(substr)->asObject());
  Object result(&scope, thread->invokeMethodStatic2(LayoutId::kStr,
                                                    SymbolId::kDunderContains,
                                                    str_obj, substr_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call str.__contains__");
    }
    return -1;
  }
  DCHECK(result.isBool(), "result of __contains__ should be bool");
  return RawBool::cast(*result).value();
}

PY_EXPORT Py_ssize_t PyUnicode_Count(PyObject* /* r */, PyObject* /* r */,
                                     Py_ssize_t /* t */, Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicode_Count");
}

PY_EXPORT PyObject* PyUnicode_Decode(const char* /* s */, Py_ssize_t /* e */,
                                     const char* /* g */, const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_Decode");
}

PY_EXPORT PyObject* PyUnicode_DecodeASCII(const char* c_str, Py_ssize_t size,
                                          const char* /* errors */) {
  // TODO(T38200137): Make use of the errors handler
  Thread* thread = Thread::current();
  return ApiHandle::newReference(
      thread, thread->runtime()->newStrWithAll(
                  View<byte>(reinterpret_cast<const byte*>(c_str), size)));
}

PY_EXPORT PyObject* PyUnicode_DecodeCharmap(const char* /* s */,
                                            Py_ssize_t /* e */,
                                            PyObject* /* g */,
                                            const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeCharmap");
}

PY_EXPORT PyObject* PyUnicode_DecodeCodePageStateful(int /* e */,
                                                     const char* /* s */,
                                                     Py_ssize_t /* e */,
                                                     const char* /* s */,
                                                     Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeCodePageStateful");
}

PY_EXPORT PyObject* PyUnicode_DecodeFSDefault(const char* c_str) {
  Thread* thread = Thread::current();
  return ApiHandle::newReference(thread,
                                 thread->runtime()->newStrFromCStr(c_str));
}

PY_EXPORT PyObject* PyUnicode_DecodeFSDefaultAndSize(const char* c_str,
                                                     Py_ssize_t size) {
  Thread* thread = Thread::current();
  View<byte> str(reinterpret_cast<const byte*>(c_str), size);
  return ApiHandle::newReference(thread, thread->runtime()->newStrWithAll(str));
}

PY_EXPORT PyObject* PyUnicode_DecodeLatin1(const char* /* s */,
                                           Py_ssize_t /* e */,
                                           const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeLatin1");
}

PY_EXPORT PyObject* PyUnicode_DecodeLocale(const char* /* r */,
                                           const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeLocale");
}

PY_EXPORT PyObject* PyUnicode_DecodeLocaleAndSize(const char* /* r */,
                                                  Py_ssize_t /* n */,
                                                  const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeLocaleAndSize");
}

PY_EXPORT PyObject* PyUnicode_DecodeMBCS(const char* /* s */,
                                         Py_ssize_t /* e */,
                                         const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeMBCS");
}

PY_EXPORT PyObject* PyUnicode_DecodeMBCSStateful(const char* /* s */,
                                                 Py_ssize_t /* e */,
                                                 const char* /* s */,
                                                 Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeMBCSStateful");
}

PY_EXPORT PyObject* PyUnicode_DecodeRawUnicodeEscape(const char* /* s */,
                                                     Py_ssize_t /* e */,
                                                     const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeRawUnicodeEscape");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF16(const char* /* s */,
                                          Py_ssize_t /* e */,
                                          const char* /* s */, int* /* r */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF16");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF16Stateful(const char* /* s */,
                                                  Py_ssize_t /* e */,
                                                  const char* /* s */,
                                                  int* /* r */,
                                                  Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF16Stateful");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF32(const char* /* s */,
                                          Py_ssize_t /* e */,
                                          const char* /* s */, int* /* r */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF32");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF32Stateful(const char* /* s */,
                                                  Py_ssize_t /* e */,
                                                  const char* /* s */,
                                                  int* /* r */,
                                                  Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF32Stateful");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF7(const char* /* s */,
                                         Py_ssize_t /* e */,
                                         const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF7");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF7Stateful(const char* /* s */,
                                                 Py_ssize_t /* e */,
                                                 const char* /* s */,
                                                 Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF7Stateful");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF8(const char* c_str, Py_ssize_t size,
                                         const char* /* errors */) {
  // TODO(T38200137): Make use of the error_handler argument
  Thread* thread = Thread::current();
  return ApiHandle::newReference(
      thread, thread->runtime()->newStrWithAll(
                  View<byte>(reinterpret_cast<const byte*>(c_str), size)));
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF8Stateful(const char* c_str,
                                                 Py_ssize_t size,
                                                 const char* /* errors */,
                                                 Py_ssize_t* /* consumed */) {
  // TODO(T38200137): Make use of the errors argument
  // TODO(T38320199): Make use of the consumed argument
  Thread* thread = Thread::current();
  return ApiHandle::newReference(
      thread, thread->runtime()->newStrWithAll(
                  View<byte>(reinterpret_cast<const byte*>(c_str), size)));
}

PY_EXPORT PyObject* PyUnicode_DecodeUnicodeEscape(const char* /* s */,
                                                  Py_ssize_t /* e */,
                                                  const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeUnicodeEscape");
}

PY_EXPORT PyObject* PyUnicode_EncodeCodePage(int /* e */, PyObject* /* e */,
                                             const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_EncodeCodePage");
}

PY_EXPORT PyObject* PyUnicode_EncodeLocale(PyObject* /* e */,
                                           const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_EncodeLocale");
}

PY_EXPORT int PyUnicode_FSConverter(PyObject* arg, void* addr) {
  if (arg == nullptr) {
    Py_DECREF(*reinterpret_cast<PyObject**>(addr));
    *reinterpret_cast<PyObject**>(addr) = nullptr;
    return 1;
  }
  PyObject* path = PyOS_FSPath(arg);
  if (path == nullptr) {
    return 0;
  }
  PyObject* output = nullptr;
  if (PyBytes_Check(path)) {
    output = path;
  } else {
    // PyOS_FSPath() guarantees its returned value is bytes or str.
    output = PyUnicode_EncodeFSDefault(path);
    Py_DECREF(path);
    if (output == nullptr) {
      return 0;
    }
    CHECK(PyBytes_Check(output), "output must be a bytes object");
  }
  for (int i = 0; i < PyBytes_Size(output); i++) {
    PyObject* item = PySequence_GetItem(output, i);
    if (PyLong_AsLong(item) == 0) {
      PyErr_SetString(PyExc_ValueError, "embedded null byte");
      Py_DECREF(output);
      Py_DECREF(item);
      return 0;
    }
    Py_DECREF(item);
  }
  *reinterpret_cast<PyObject**>(addr) = output;
  return Py_CLEANUP_SUPPORTED;
}

PY_EXPORT int PyUnicode_FSDecoder(PyObject* /* g */, void* /* r */) {
  UNIMPLEMENTED("PyUnicode_FSDecoder");
}

PY_EXPORT Py_ssize_t PyUnicode_Find(PyObject* str, PyObject* substr,
                                    Py_ssize_t start, Py_ssize_t end,
                                    int direction) {
  DCHECK(str != nullptr, "str must be non-null");
  DCHECK(substr != nullptr, "substr must be non-null");
  DCHECK(direction == -1 || direction == 1, "direction must be -1 or 1");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Object substr_obj(&scope, ApiHandle::fromPyObject(substr)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*str_obj)) {
    thread->raiseTypeErrorWithCStr("PyUnicode_Find requires a 'str' instance");
    return -2;
  }
  Str str_str(&scope, *str_obj);
  if (!runtime->isInstanceOfStr(*substr_obj)) {
    thread->raiseTypeErrorWithCStr("PyUnicode_Find requires a 'str' instance");
    return -2;
  }
  Str substr_str(&scope, *substr_obj);
  auto fn = (direction == 1) ? strFind : strRFind;
  Int result(&scope, (*fn)(str_str, substr_str, start, end));
  OptInt<Py_ssize_t> maybe_int = result.asInt<Py_ssize_t>();
  if (maybe_int.error == CastError::None) {
    return maybe_int.value;
  }
  thread->raiseOverflowErrorWithCStr("int overflow or underflow");
  return -2;
}

PY_EXPORT Py_ssize_t PyUnicode_FindChar(PyObject* str, Py_UCS4 ch,
                                        Py_ssize_t start, Py_ssize_t end,
                                        int direction) {
  DCHECK(str != nullptr, "str must not be null");
  DCHECK(direction == 1 || direction == -1, "direction must be -1 or 1");
  Thread* thread = Thread::current();
  if (start < 0 || end < 0) {
    thread->raiseIndexErrorWithCStr("string index out of range");
    return -2;
  }
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*str_obj),
         "PyUnicode_FindChar requires a 'str' instance");
  Str str_str(&scope, *str_obj);
  Str substr(&scope, SmallStr::fromCodePoint(ch));
  auto fn = (direction == 1) ? strFind : strRFind;
  Int result(&scope, (*fn)(str_str, substr, start, end));
  OptInt<Py_ssize_t> maybe_int = result.asInt<Py_ssize_t>();
  if (maybe_int.error == CastError::None) {
    return maybe_int.value;
  }
  thread->raiseOverflowErrorWithCStr("int overflow or underflow");
  return -2;
}

PY_EXPORT PyObject* PyUnicode_Format(PyObject* /* t */, PyObject* /* s */) {
  UNIMPLEMENTED("PyUnicode_Format");
}

PY_EXPORT PyObject* PyUnicode_FromEncodedObject(PyObject* /* j */,
                                                const char* /* g */,
                                                const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_FromEncodedObject");
}

PY_EXPORT PyObject* PyUnicode_FromFormat(const char* format, ...) {
  va_list vargs;

  va_start(vargs, format);
  PyObject* ret = PyUnicode_FromFormatV(format, vargs);
  va_end(vargs);
  return ret;
}

PY_EXPORT PyObject* PyUnicode_FromFormatV(const char* format, va_list vargs) {
  va_list vargs2;
  _PyUnicodeWriter writer;

  _PyUnicodeWriter_Init(&writer);
  writer.min_length = strlen(format) + 100;
  writer.overallocate = 1;

  // This copy seems unnecessary but it may have been needed by CPython for
  // historical reasons.
  va_copy(vargs2, vargs);

  for (const char* f = format; *f;) {
    if (*f == '%') {
      f = writeArg(&writer, f, &vargs2);
      if (f == nullptr) goto fail;
    } else {
      const char* p = f;
      do {
        if (static_cast<unsigned char>(*p) > 127) {
          PyErr_Format(
              PyExc_ValueError,
              "PyUnicode_FromFormatV() expects an ASCII-encoded format "
              "string, got a non-ASCII byte: 0x%02x",
              static_cast<unsigned char>(*p));
          goto fail;
        }
        p++;
      } while (*p != '\0' && *p != '%');
      Py_ssize_t len = p - f;

      if (*p == '\0') writer.overallocate = 0;

      if (_PyUnicodeWriter_WriteASCIIString(&writer, f, len) < 0) goto fail;

      f = p;
    }
  }
  va_end(vargs2);
  return _PyUnicodeWriter_Finish(&writer);

fail:
  va_end(vargs2);
  _PyUnicodeWriter_Dealloc(&writer);
  return nullptr;
}

PY_EXPORT PyObject* PyUnicode_FromObject(PyObject* /* j */) {
  UNIMPLEMENTED("PyUnicode_FromObject");
}

PY_EXPORT PyObject* PyUnicode_FromOrdinal(int /* l */) {
  UNIMPLEMENTED("PyUnicode_FromOrdinal");
}

PY_EXPORT PyObject* PyUnicode_FromWideChar(const wchar_t* buffer,
                                           Py_ssize_t size) {
  Thread* thread = Thread::current();
  if (buffer == nullptr && size != 0) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  if (size == -1) {
    size = std::wcslen(buffer);
  }

  if (sizeof(*buffer) * kBitsPerByte == 16) {
    // TODO(T38042082): Implement newStrFromUTF16
    UNIMPLEMENTED("newStrFromUTF16");
  }
  CHECK(sizeof(*buffer) * kBitsPerByte == 32,
        "size of wchar_t should be either 16 or 32 bits");
  for (Py_ssize_t i = 0; i < size; ++i) {
    if (buffer[i] > kMaxUnicode) {
      thread->raiseValueErrorWithCStr("character is not in range");
      return nullptr;
    }
  }
  return ApiHandle::newReference(
      thread, thread->runtime()->newStrFromUTF32(
                  View<int32>(bit_cast<int32*>(buffer), size)));
}

PY_EXPORT const char* PyUnicode_GetDefaultEncoding() {
  UNIMPLEMENTED("PyUnicode_GetDefaultEncoding");
}

PY_EXPORT Py_ssize_t PyUnicode_GetLength(PyObject* pyobj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  if (!thread->runtime()->isInstanceOfStr(*obj)) {
    thread->raiseBadArgument();
    return -1;
  }
  if (!obj.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str str(&scope, *obj);
  return str.codePointLength();
}

PY_EXPORT Py_ssize_t PyUnicode_GetSize(PyObject* pyobj) {
  // This function returns the number of UTF-16 or UTF-32 code units, depending
  // on the size of wchar_t on the operating system. On the machines that we
  // currently use for testing, this is the same as the number of Unicode code
  // points. This must be modified when we support operating systems with
  // different wchar_t (e.g. Windows).
  return PyUnicode_GetLength(pyobj);
}

PY_EXPORT PyObject* PyUnicode_InternFromString(const char* c_str) {
  DCHECK(c_str != nullptr, "c_str must not be nullptr");
  PyObject* str = PyUnicode_FromString(c_str);
  if (str == nullptr) {
    return nullptr;
  }
  PyUnicode_InternInPlace(&str);
  return str;
}

PY_EXPORT void PyUnicode_InternImmortal(PyObject** /* p */) {
  UNIMPLEMENTED("PyUnicode_InternImmortal");
}

PY_EXPORT void PyUnicode_InternInPlace(PyObject** pobj) {
  DCHECK(pobj != nullptr, "pobj should not be null");
  if (*pobj == nullptr) {
    return;
  }
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(*pobj)->asObject());
  if (!obj.isLargeStr()) {
    return;
  }
  HeapObject heap_obj(&scope, *obj);
  HeapObject result(&scope, thread->runtime()->internStr(obj));
  if (result.address() != heap_obj.address()) {
    *pobj = ApiHandle::newReference(thread, *result);
  }
}

PY_EXPORT int PyUnicode_IsIdentifier(PyObject* /* f */) {
  UNIMPLEMENTED("PyUnicode_IsIdentifier");
}

PY_EXPORT PyObject* PyUnicode_Join(PyObject* sep, PyObject* seq) {
  DCHECK(sep != nullptr, "sep should not be null");
  DCHECK(seq != nullptr, "seq should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object sep_obj(&scope, ApiHandle::fromPyObject(sep)->asObject());
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(&scope, thread->invokeMethodStatic2(
                            LayoutId::kStr, SymbolId::kJoin, sep_obj, seq_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call str.join");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyUnicode_Partition(PyObject* str, PyObject* sep) {
  DCHECK(str != nullptr, "str should not be null");
  DCHECK(sep != nullptr, "sep should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Object sep_obj(&scope, ApiHandle::fromPyObject(sep)->asObject());
  Object result(
      &scope, thread->invokeMethodStatic2(LayoutId::kStr, SymbolId::kPartition,
                                          str_obj, sep_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call str.partition");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyUnicode_RPartition(PyObject* str, PyObject* sep) {
  DCHECK(str != nullptr, "str should not be null");
  DCHECK(sep != nullptr, "sep should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Object sep_obj(&scope, ApiHandle::fromPyObject(sep)->asObject());
  Object result(
      &scope, thread->invokeMethodStatic2(LayoutId::kStr, SymbolId::kRPartition,
                                          str_obj, sep_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call str.rpartition");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyUnicode_RSplit(PyObject* str, PyObject* sep,
                                     Py_ssize_t maxsplit) {
  DCHECK(str != nullptr, "str must not be null");
  DCHECK(sep != nullptr, "sep must not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Object sep_obj(&scope, ApiHandle::fromPyObject(sep)->asObject());
  Object maxsplit_obj(&scope, thread->runtime()->newInt(maxsplit));
  Object result(&scope,
                thread->invokeMethodStatic3(LayoutId::kStr, SymbolId::kRSplit,
                                            str_obj, sep_obj, maxsplit_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call str.rsplit");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT Py_UCS4 PyUnicode_ReadChar(PyObject* /* e */, Py_ssize_t /* x */) {
  UNIMPLEMENTED("PyUnicode_ReadChar");
}

PY_EXPORT PyObject* PyUnicode_Replace(PyObject* str, PyObject* substr,
                                      PyObject* replstr, Py_ssize_t maxcount) {
  DCHECK(str != nullptr, "str must not be null");
  DCHECK(substr != nullptr, "substr must not be null");
  DCHECK(replstr != nullptr, "replstr must not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  if (!runtime->isInstanceOfStr(*str_obj)) {
    thread->raiseTypeErrorWithCStr("str must be str");
    return nullptr;
  }
  if (!str_obj.isStr()) UNIMPLEMENTED("str subclass");

  Object substr_obj(&scope, ApiHandle::fromPyObject(substr)->asObject());
  if (!runtime->isInstanceOfStr(*substr_obj)) {
    thread->raiseTypeErrorWithCStr("substr must be str");
    return nullptr;
  }
  if (!substr_obj.isStr()) UNIMPLEMENTED("str subclass");

  Object replstr_obj(&scope, ApiHandle::fromPyObject(replstr)->asObject());
  if (!runtime->isInstanceOfStr(*replstr_obj)) {
    thread->raiseTypeErrorWithCStr("replstr must be str");
    return nullptr;
  }
  if (!replstr_obj.isStr()) UNIMPLEMENTED("str subclass");

  Str str_str(&scope, *str_obj);
  Str substr_str(&scope, *substr_obj);
  Str replstr_str(&scope, *replstr_obj);
  // TODO(T42259916): Make sure the return value is of 'str' type once str
  // subclass is supported.
  Object result(&scope, runtime->strReplace(thread, str_str, substr_str,
                                            replstr_str, maxcount));
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyUnicode_Resize(PyObject** /* p_unicode */, Py_ssize_t /* h */) {
  UNIMPLEMENTED("PyUnicode_Resize");
}

PY_EXPORT PyObject* PyUnicode_RichCompare(PyObject* /* t */, PyObject* /* t */,
                                          int /* p */) {
  UNIMPLEMENTED("PyUnicode_RichCompare");
}

PY_EXPORT PyObject* PyUnicode_Split(PyObject* str, PyObject* sep,
                                    Py_ssize_t maxsplit) {
  DCHECK(str != nullptr, "str must not be null");
  DCHECK(sep != nullptr, "sep must not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Object sep_obj(&scope, ApiHandle::fromPyObject(sep)->asObject());
  Object maxsplit_obj(&scope, thread->runtime()->newInt(maxsplit));
  Object result(&scope,
                thread->invokeMethodStatic3(LayoutId::kStr, SymbolId::kSplit,
                                            str_obj, sep_obj, maxsplit_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call str.split");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyUnicode_Splitlines(PyObject* /* g */, int /* s */) {
  UNIMPLEMENTED("PyUnicode_Splitlines");
}

PY_EXPORT PyObject* PyUnicode_Substring(PyObject* pyobj, Py_ssize_t start,
                                        Py_ssize_t end) {
  DCHECK(pyobj != nullptr, "null argument to PyUnicode_Substring");
  Thread* thread = Thread::current();
  if (start < 0 || end < 0) {
    thread->raiseIndexErrorWithCStr("string index out of range");
    return nullptr;
  }
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*obj),
         "PyUnicode_Substring requires a 'str' instance");
  Str self(&scope, *obj);
  word len = self.length();
  word start_index = self.offsetByCodePoints(0, start);
  if (start_index == len || end <= start) {
    return ApiHandle::newReference(thread, Str::empty());
  }
  word end_index = self.offsetByCodePoints(start_index, end - start);
  if (end_index == len) {
    if (start_index == 0) {
      handle->incref();
      return pyobj;
    }
  }
  return ApiHandle::newReference(
      thread,
      runtime->strSubstr(thread, self, start_index, end_index - start_index));
}

PY_EXPORT Py_ssize_t PyUnicode_Tailmatch(PyObject* /* r */, PyObject* /* r */,
                                         Py_ssize_t /* t */, Py_ssize_t /* d */,
                                         int /* n */) {
  UNIMPLEMENTED("PyUnicode_Tailmatch");
}

PY_EXPORT PyObject* PyUnicode_Translate(PyObject* /* r */, PyObject* /* g */,
                                        const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_Translate");
}

PY_EXPORT int PyUnicode_WriteChar(PyObject* /* e */, Py_ssize_t /* x */,
                                  Py_UCS4 /* h */) {
  UNIMPLEMENTED("PyUnicode_WriteChar");
}

PY_EXPORT Py_UNICODE* PyUnicode_AsUnicode(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUnicode");
}

PY_EXPORT Py_UNICODE* PyUnicode_AsUnicodeAndSize(PyObject* /* unicode */,
                                                 Py_ssize_t* /* size */) {
  UNIMPLEMENTED("PyUnicode_AsUnicodeAndSize");
}

template <typename T>
static PyObject* decodeUnicodeToString(Thread* thread, const void* src,
                                       word size) {
  DCHECK(src != nullptr, "Must pass in a non-null buffer");
  const T* cp = static_cast<const T*>(src);
  if (size == 1) {
    return ApiHandle::newReference(thread, SmallStr::fromCodePoint(cp[0]));
  }
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // TODO(T41785453): Remove the ByteArray intermediary
  ByteArray dst(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, dst, size);
  for (word i = 0; i < size; ++i) {
    RawStr str = RawStr::cast(SmallStr::fromCodePoint(cp[i]));
    for (word j = 0; j < str.length(); ++j) {
      byteArrayAdd(thread, runtime, dst, str.charAt(j));
    }
  }
  return ApiHandle::newReference(thread, runtime->newStrFromByteArray(dst));
}

PY_EXPORT PyObject* PyUnicode_FromKindAndData(int kind, const void* buffer,
                                              Py_ssize_t size) {
  Thread* thread = Thread::current();
  if (size < 0) {
    thread->raiseValueErrorWithCStr("size must be positive");
    return nullptr;
  }
  if (size == 0) {
    return ApiHandle::newReference(thread, Str::empty());
  }
  switch (kind) {
    case PyUnicode_1BYTE_KIND:
      return decodeUnicodeToString<Py_UCS1>(thread, buffer, size);
    case PyUnicode_2BYTE_KIND:
      return decodeUnicodeToString<Py_UCS2>(thread, buffer, size);
    case PyUnicode_4BYTE_KIND:
      return decodeUnicodeToString<Py_UCS4>(thread, buffer, size);
  }
  thread->raiseSystemErrorWithCStr("invalid kind");
  return nullptr;
}

PY_EXPORT PyObject* PyUnicode_FromUnicode(const Py_UNICODE* code_units,
                                          Py_ssize_t size) {
  if (code_units == nullptr) {
    // TODO(T36562134): Implement _PyUnicode_New
    UNIMPLEMENTED("_PyUnicode_New");
  }

  if (sizeof(*code_units) * kBitsPerByte == 16) {
    // TODO(T38042082): Implement newStrFromUTF16
    UNIMPLEMENTED("newStrFromUTF16");
  }
  CHECK(sizeof(*code_units) * kBitsPerByte == 32,
        "size of Py_UNICODE should be either 16 or 32 bits");
  Thread* thread = Thread::current();
  for (Py_ssize_t i = 0; i < size; ++i) {
    if (code_units[i] > kMaxUnicode) {
      thread->raiseValueErrorWithCStr("character is not in range");
      return nullptr;
    }
  }
  return ApiHandle::newReference(
      thread, thread->runtime()->newStrFromUTF32(
                  View<int32>(bit_cast<int32*>(code_units), size)));
}

PY_EXPORT int PyUnicode_KIND_Func(PyObject*) {
  UNIMPLEMENTED("PyUnicode_KIND_Func");
}

PY_EXPORT void* PyUnicode_DATA_Func(PyObject*) {
  UNIMPLEMENTED("PyUnicode_DATA_Func");
}

PY_EXPORT Py_UCS4 PyUnicode_READ_Func(int, void*, Py_ssize_t) {
  UNIMPLEMENTED("PyUnicode_READ_Func");
}

PY_EXPORT Py_UCS4 PyUnicode_READ_CHAR_Func(PyObject*, Py_ssize_t) {
  UNIMPLEMENTED("PyUnicode_READ_CHAR_Func");
}

PY_EXPORT int _Py_normalize_encoding(const char* /* encoding */,
                                     char* /* lower */,
                                     size_t /* lower_len */) {
  UNIMPLEMENTED("_Py_normalize_encoding");
}

PY_EXPORT PyObject* _PyUnicode_AsUTF8String(PyObject* unicode,
                                            const char* /* errors */) {
  if (!PyUnicode_Check(unicode)) {
    PyErr_BadArgument();
    return nullptr;
  }
  return PyBytes_FromStringAndSize(PyUnicode_AsUTF8(unicode),
                                   PyUnicode_GetLength(unicode));
}

PY_EXPORT wchar_t* _Py_DecodeUTF8_surrogateescape(const char* /* s */,
                                                  Py_ssize_t /* size */) {
  UNIMPLEMENTED("_Py_DecodeUTF8_surrogateescape");
}

}  // namespace python
