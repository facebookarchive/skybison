// unicodeobject.c implementation
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <cwchar>

#include "cpython-data.h"
#include "cpython-func.h"

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "capi-handles.h"
#include "handles.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "utils.h"

const char* Py_FileSystemDefaultEncodeErrors = "surrogateescape";
// _Py_DecodeLocaleEx is defined in fileutils.c
extern "C" wchar_t* _Py_DecodeLocaleEx(const char*, size_t*, int);
// clang-format off
extern "C" const unsigned char _Py_ascii_whitespace[] = {  // NOLINT
    0, 0, 0, 0, 0, 0, 0, 0,
//     case 0x0009: * CHARACTER TABULATION
//     case 0x000A: * LINE FEED
//     case 0x000B: * LINE TABULATION
//     case 0x000C: * FORM FEED
//     case 0x000D: * CARRIAGE RETURN
    0, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
//     case 0x001C: * FILE SEPARATOR
//     case 0x001D: * GROUP SEPARATOR
//     case 0x001E: * RECORD SEPARATOR
//     case 0x001F: * UNIT SEPARATOR
    0, 0, 0, 0, 1, 1, 1, 1,
//     case 0x0020: * SPACE
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};
// clang-format on

namespace py {

typedef byte Py_UCS1;
typedef uint16_t Py_UCS2;

static const int kMaxLongLongChars = 19;  // len(str(2**63-1))
static const int kOverallocateFactor = 4;

PY_EXPORT PyTypeObject* PyUnicodeIter_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kStrIterator)));
}

static RawObject symbolFromError(Thread* thread, const char* error) {
  Runtime* runtime = thread->runtime();
  Symbols* symbols = runtime->symbols();
  if (error == nullptr || std::strcmp(error, "strict") == 0) {
    return symbols->at(ID(strict));
  }
  if (std::strcmp(error, "ignore") == 0) {
    return symbols->at(ID(ignore));
  }
  if (std::strcmp(error, "replace") == 0) {
    return symbols->at(ID(replace));
  }
  return Runtime::internStrFromCStr(thread, error);
}

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
  Str str(&scope, thread->runtime()->newStrFromUTF32(View<int32_t>(
                      static_cast<int32_t*>(writer->data), writer->pos)));
  PyMem_Free(writer->data);
  return ApiHandle::newReference(thread, *str);
}

PY_EXPORT void _PyUnicodeWriter_Init(_PyUnicodeWriter* writer) {
  std::memset(writer, 0, sizeof(*writer));
  writer->kind = PyUnicode_4BYTE_KIND;
}

static int _PyUnicodeWriter_PrepareInternal(_PyUnicodeWriter* writer,
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
    writer->data = PyMem_Malloc(newlen * sizeof(int32_t));
    if (writer->data == nullptr) return -1;
  } else if (newlen > writer->size) {
    if (writer->overallocate &&
        newlen <= (kMaxWord - newlen / kOverallocateFactor)) {
      // overallocate to limit the number of realloc()
      newlen += newlen / kOverallocateFactor;
    }
    writer->data = PyMem_Realloc(writer->data, newlen * sizeof(int32_t));
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
    writer->data = PyMem_Malloc(len * sizeof(int32_t));
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
  Object obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Str src(&scope, strUnderlying(*obj));
  Py_ssize_t len = src.charLength();
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
  Object obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Str src(&scope, strUnderlying(*obj));
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
        std::memmove(number + 2, number, std::strlen(number) + 1);
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
      Py_ssize_t len = std::strlen(p);
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  Str str(&scope, strUnderlying(*obj));
  return str.equalsCStr(c_str);
}

PY_EXPORT int _PyUnicode_EQ(PyObject* aa, PyObject* bb) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj_aa(&scope, ApiHandle::fromPyObject(aa)->asObject());
  Object obj_bb(&scope, ApiHandle::fromPyObject(bb)->asObject());
  Str lhs(&scope, strUnderlying(*obj_aa));
  Str rhs(&scope, strUnderlying(*obj_bb));
  return lhs.equals(*rhs);
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
  if (!thread->runtime()->isInstanceOfStr(*obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  Str str(&scope, strUnderlying(*obj));
  word length = str.charLength();
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
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "Negative size passed to PyUnicode_FromStringAndSize");
    return nullptr;
  }
  if (u == nullptr && size != 0) {
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
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "invalid maximum character passed to PyUnicode_New");
    return nullptr;
  }
  if (size < 0) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "Negative size passed to PyUnicode_New");
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

PY_EXPORT PyObject* _PyUnicode_AsASCIIString(PyObject* unicode,
                                             const char* errors) {
  DCHECK(unicode != nullptr, "unicode cannot be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  if (!runtime->isInstanceOfStr(*str)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object tuple_obj(&scope, thread->invokeFunction2(
                               ID(_codecs), ID(ascii_encode), str, errors_obj));
  if (tuple_obj.isError()) {
    return nullptr;
  }
  Tuple tuple(&scope, *tuple_obj);
  return ApiHandle::newReference(thread, tuple.at(0));
}

PY_EXPORT PyObject* PyUnicode_AsASCIIString(PyObject* unicode) {
  return _PyUnicode_AsASCIIString(unicode, "strict");
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

PY_EXPORT PyObject* PyUnicode_AsEncodedString(PyObject* unicode,
                                              const char* encoding,
                                              const char* errors) {
  DCHECK(unicode != nullptr, "unicode cannot be null");
  if (encoding == nullptr) {
    return _PyUnicode_AsUTF8String(unicode, errors);
  }
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  if (!runtime->isInstanceOfStr(*str)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Object encoding_obj(&scope, runtime->newStrFromCStr(encoding));
  Object errors_obj(&scope, errors == nullptr
                                ? Unbound::object()
                                : symbolFromError(thread, errors));
  Object result(&scope, thread->invokeFunction3(ID(_codecs), ID(encode), str,
                                                encoding_obj, errors_obj));
  if (result.isError()) {
    return nullptr;
  }
  if (runtime->isInstanceOfBytes(*result)) {
    return ApiHandle::newReference(thread, *result);
  }
  if (runtime->isInstanceOfByteArray(*result)) {
    // Equivalent to calling PyErr_WarnFormat
    if (!ensureBuiltinModuleById(thread, ID(warnings)).isErrorException()) {
      Object category(&scope, runtime->typeAt(LayoutId::kRuntimeWarning));
      Object message(&scope,
                     runtime->newStrFromFmt(
                         "encoder %s returned bytearray instead of bytes; "
                         "use codecs.encode() to encode to arbitrary types",
                         encoding));
      Object stack_level(&scope, runtime->newInt(1));
      Object source(&scope, NoneType::object());
      thread->invokeFunction4(ID(warnings), ID(warn), message, category,
                              stack_level, source);
      thread->clearPendingException();
    }
    ByteArray result_bytearray(&scope, *result);
    return ApiHandle::newReference(
        thread, byteArrayAsBytes(thread, runtime, result_bytearray));
  }
  thread->raiseWithFmt(LayoutId::kTypeError,
                       "'%s' encoder returned '%T' instead of 'bytes'; "
                       "use codecs.encode() to encode to arbitrary types",
                       encoding, *result);
  return nullptr;
}

PY_EXPORT PyObject* PyUnicode_AsEncodedUnicode(PyObject* /* e */,
                                               const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsEncodedUnicode");
}

PY_EXPORT PyObject* _PyUnicode_AsLatin1String(PyObject* unicode,
                                              const char* errors) {
  DCHECK(unicode != nullptr, "unicode cannot be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  if (!runtime->isInstanceOfStr(*str)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object tuple_obj(&scope,
                   thread->invokeFunction2(ID(_codecs), ID(latin_1_encode), str,
                                           errors_obj));
  if (tuple_obj.isError()) {
    return nullptr;
  }
  Tuple tuple(&scope, *tuple_obj);
  return ApiHandle::newReference(thread, tuple.at(0));
}

PY_EXPORT PyObject* PyUnicode_AsLatin1String(PyObject* unicode) {
  return _PyUnicode_AsLatin1String(unicode, "strict");
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

  Str str(&scope, strUnderlying(*obj));
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

PY_EXPORT PyObject* PyUnicode_AsUTF16String(PyObject* unicode) {
  return _PyUnicode_EncodeUTF16(unicode, nullptr, 0);
}

PY_EXPORT PyObject* PyUnicode_AsUTF32String(PyObject* unicode) {
  return _PyUnicode_EncodeUTF32(unicode, nullptr, 0);
}

PY_EXPORT PyObject* PyUnicode_AsUTF8String(PyObject* unicode) {
  return _PyUnicode_AsUTF8String(unicode, "strict");
}

PY_EXPORT PyObject* PyUnicode_AsUnicodeEscapeString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUnicodeEscapeString");
}

PY_EXPORT Py_ssize_t PyUnicode_AsWideChar(PyObject* /* e */, wchar_t* /* w */,
                                          Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_AsWideChar");
}

static wchar_t* unicodeAsWideChar(Thread* thread, const Str& str) {
  word len = str.codePointLength();
  wchar_t* buf =
      static_cast<wchar_t*>(PyMem_Malloc((len + 1) * sizeof(wchar_t)));
  word byte_count = str.charLength();
  for (word byte_index = 0, wchar_index = 0, num_bytes = 0;
       byte_index < byte_count; byte_index += num_bytes, wchar_index += 1) {
    int32_t cp = str.codePointAt(byte_index, &num_bytes);
    if (cp == '\0') {
      PyMem_Free(buf);
      thread->raiseWithFmt(LayoutId::kValueError, "embedded null character");
      return nullptr;
    }
    static_assert(sizeof(wchar_t) == sizeof(cp), "Requires 32bit wchar_t");
    buf[wchar_index] = static_cast<wchar_t>(cp);
  }
  buf[len] = '\0';
  return buf;
}

PY_EXPORT wchar_t* _PyUnicode_AsWideCharString(PyObject* unicode) {
  Thread* thread = Thread::current();
  if (unicode == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object unicode_obj(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  if (!thread->runtime()->isInstanceOfStr(*unicode_obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Str unicode_str(&scope, strUnderlying(*unicode_obj));
  return unicodeAsWideChar(thread, unicode_str);
}

PY_EXPORT wchar_t* PyUnicode_AsWideCharString(PyObject* /* e */,
                                              Py_ssize_t* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsWideCharString");
}

PY_EXPORT PyObject* PyUnicode_BuildEncodingMap(PyObject* /* g */) {
  UNIMPLEMENTED("PyUnicode_BuildEncodingMap");
}

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
    Str left_str(&scope, strUnderlying(*left_obj));
    Str right_str(&scope, strUnderlying(*right_obj));
    word result = left_str.compare(*right_str);
    return result > 0 ? 1 : (result < 0 ? -1 : 0);
  }
  thread->raiseWithFmt(LayoutId::kTypeError, "Can't compare %T and %T",
                       &left_obj, &right_obj);
  return -1;
}

PY_EXPORT int PyUnicode_CompareWithASCIIString(PyObject* uni, const char* str) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(uni)->asObject());
  Str str_obj(&scope, strUnderlying(*obj));
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
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "can only concatenate str to str");
    return nullptr;
  }
  Str left_str(&scope, strUnderlying(*left_obj));
  Str right_str(&scope, strUnderlying(*right_obj));
  word dummy;
  if (__builtin_add_overflow(left_str.charLength(), right_str.charLength(),
                             &dummy)) {
    thread->raiseWithFmt(LayoutId::kOverflowError,
                         "strings are too large to concat");
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
  Object result(&scope,
                thread->invokeMethodStatic2(LayoutId::kStr, ID(__contains__),
                                            str_obj, substr_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "could not call str.__contains__");
    }
    return -1;
  }
  DCHECK(result.isBool(), "result of __contains__ should be bool");
  return Bool::cast(*result).value();
}

PY_EXPORT Py_ssize_t PyUnicode_CopyCharacters(PyObject*, Py_ssize_t, PyObject*,
                                              Py_ssize_t, Py_ssize_t) {
  UNIMPLEMENTED("PyUnicode_CopyCharacters");
}

PY_EXPORT Py_ssize_t PyUnicode_Count(PyObject* /* r */, PyObject* /* r */,
                                     Py_ssize_t /* t */, Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicode_Count");
}

PY_EXPORT PyObject* PyUnicode_Decode(const char* c_str, Py_ssize_t size,
                                     const char* encoding, const char* errors) {
  DCHECK(c_str != nullptr, "c_str cannot be null");
  if (encoding == nullptr) {
    return PyUnicode_DecodeUTF8Stateful(c_str, size, errors, nullptr);
  }

  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Bytes bytes(&scope, runtime->newBytesWithAll(View<byte>(
                          reinterpret_cast<const byte*>(c_str), size)));
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object encoding_obj(&scope, runtime->newStrFromCStr(encoding));
  Object result(&scope, thread->invokeFunction3(ID(_codecs), ID(decode), bytes,
                                                encoding_obj, errors_obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyUnicode_DecodeASCII(const char* c_str, Py_ssize_t size,
                                          const char* errors) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Bytes bytes(&scope, runtime->newBytesWithAll(View<byte>(
                          reinterpret_cast<const byte*>(c_str), size)));
  Str errors_obj(&scope, symbolFromError(thread, errors));
  Object result_obj(
      &scope, thread->invokeFunction2(ID(_codecs), ID(ascii_decode), bytes,
                                      errors_obj));
  if (result_obj.isError()) {
    if (result_obj.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kSystemError,
                           "could not call _codecs.ascii_decode");
    }
    return nullptr;
  }
  Tuple result(&scope, *result_obj);
  return ApiHandle::newReference(thread, result.at(0));
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

PY_EXPORT PyObject* PyUnicode_DecodeLatin1(const char* c_str, Py_ssize_t size,
                                           const char* /* errors */) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Bytes bytes(&scope, runtime->newBytesWithAll(View<byte>(
                          reinterpret_cast<const byte*>(c_str), size)));
  Object result_obj(
      &scope, thread->invokeFunction1(ID(_codecs), ID(latin_1_decode), bytes));
  if (result_obj.isError()) {
    if (result_obj.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kSystemError,
                           "could not call _codecs.latin_1_decode");
    }
    return nullptr;
  }
  Tuple result(&scope, *result_obj);
  return ApiHandle::newReference(thread, result.at(0));
}

PY_EXPORT PyObject* PyUnicode_DecodeLocale(const char* str,
                                           const char* errors) {
  return PyUnicode_DecodeLocaleAndSize(str, std::strlen(str), errors);
}

static word mbstowcsErrorpos(const char* str, size_t len) {
  const char* start = str;
  mbstate_t mbs;
  std::memset(&mbs, 0, sizeof(mbs));
  while (len != 0) {
    wchar_t ch;
    size_t converted = std::mbrtowc(&ch, str, len, &mbs);
    if (converted == 0) {
      // Reached end of string
      break;
    }
    if (converted == static_cast<size_t>(-1) ||
        converted == static_cast<size_t>(-2)) {
      // Conversion error or incomplete character
      return str - start;
    }
    str += converted;
    len -= converted;
  }
  // failed to find the undecodable byte sequence
  return 0;
}

PY_EXPORT PyObject* PyUnicode_DecodeLocaleAndSize(const char* str,
                                                  Py_ssize_t size,
                                                  const char* errors) {
  if (str[size] != '\0' || static_cast<size_t>(size) != std::strlen(str)) {
    PyErr_SetString(PyExc_ValueError, "embedded null byte");
    return nullptr;
  }
  PyObject* result = nullptr;
  // TODO(T42479157): Make more efficient by following CPython's implementation
  if (errors == nullptr || std::strcmp(errors, "strict") == 0) {
    wchar_t* wstr = PyMem_New(wchar_t, size + 1);
    if (wstr == nullptr) {
      PyErr_NoMemory();
      return nullptr;
    }

    size_t wlen = std::mbstowcs(wstr, str, size + 1);
    if (wlen != static_cast<size_t>(-1)) {
      result = PyUnicode_FromWideChar(wstr, wlen);
    }
    PyMem_Free(wstr);
  } else if (std::strcmp(errors, "surrogateescape") == 0) {
    size_t wlen;
    wchar_t* wstr = _Py_DecodeLocaleEx(str, &wlen, 1);
    if (wstr == nullptr) {
      if (wlen == static_cast<size_t>(-1)) {
        PyErr_NoMemory();
      } else {
        PyErr_SetFromErrno(PyExc_OSError);
      }
      return nullptr;
    }
    result = PyUnicode_FromWideChar(wstr, wlen);
    PyMem_Free(wstr);
  } else {
    PyErr_Format(PyExc_ValueError,
                 "only 'strict' and 'surrogateescape' error handlers "
                 "are supported, not '%s'",
                 errors);
    return nullptr;
  }
  if (result != nullptr) {
    return result;
  }

  word error_pos = mbstowcsErrorpos(str, size);
  // TODO(T42609513): Use Py_DecodeLocale and std::strerror to create a better
  // error message
  PyObject* reason = PyUnicode_FromString(
      "mbstowcs() encountered an invalid multibyte sequence");
  if (reason == nullptr) {
    return nullptr;
  }

  PyObject* exc =
      PyObject_CallFunction(PyExc_UnicodeDecodeError, "sy#nnO", "locale", str,
                            size, error_pos, error_pos + 1, reason);
  Py_DECREF(reason);
  if (exc != nullptr) {
    PyCodec_StrictErrors(exc);
    Py_XDECREF(exc);
  }
  return nullptr;
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
                                         const char* errors) {
  return PyUnicode_DecodeUTF8Stateful(c_str, size, errors, nullptr);
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF8Stateful(const char* c_str,
                                                 Py_ssize_t size,
                                                 const char* errors,
                                                 Py_ssize_t* consumed) {
  DCHECK(c_str != nullptr, "c_str cannot be null");

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word i = 0;
  const byte* byte_str = reinterpret_cast<const byte*>(c_str);
  for (; i < size; ++i) {
    if (byte_str[i] > kMaxASCII) break;
  }
  if (i == size) {
    if (consumed != nullptr) {
      *consumed = size;
    }
    return ApiHandle::newReference(thread,
                                   runtime->newStrWithAll({byte_str, size}));
  }
  Object bytes(&scope, runtime->newBytesWithAll(View<byte>({byte_str, size})));
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object is_final(&scope, Bool::fromBool(consumed == nullptr));
  Object result_obj(
      &scope, thread->invokeFunction3(ID(_codecs), ID(utf_8_decode), bytes,
                                      errors_obj, is_final));
  if (result_obj.isError()) {
    if (result_obj.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kSystemError,
                           "could not call _codecs._utf_8_decode_stateful");
    }
    return nullptr;
  }
  Tuple result(&scope, *result_obj);
  if (consumed != nullptr) {
    *consumed = Int::cast(result.at(1)).asWord();
  }
  return ApiHandle::newReference(thread, result.at(0));
}

PY_EXPORT PyObject* PyUnicode_DecodeUnicodeEscape(const char* c_str,
                                                  Py_ssize_t size,
                                                  const char* errors) {
  DCHECK(c_str != nullptr, "c_str cannot be null");
  const char* first_invalid_escape;
  PyObject* result = _PyUnicode_DecodeUnicodeEscape(c_str, size, errors,
                                                    &first_invalid_escape);
  if (result == nullptr) {
    return nullptr;
  }
  if (first_invalid_escape != nullptr) {
    if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                         "invalid escape sequence '\\%c'",
                         static_cast<byte>(*first_invalid_escape)) < 0) {
      Py_DECREF(result);
      return nullptr;
    }
  }
  return result;
}

PY_EXPORT PyObject* _PyUnicode_DecodeUnicodeEscape(
    const char* c_str, Py_ssize_t size, const char* errors,
    const char** first_invalid_escape) {
  DCHECK(c_str != nullptr, "c_str cannot be null");
  DCHECK(first_invalid_escape != nullptr,
         "first_invalid_escape cannot be null");

  // So we can remember if we've seen an invalid escape char or not
  *first_invalid_escape = nullptr;

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object bytes(&scope, thread->runtime()->newBytesWithAll(View<byte>(
                           reinterpret_cast<const byte*>(c_str), size)));
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object result_obj(
      &scope,
      thread->invokeFunction2(ID(_codecs), ID(_unicode_escape_decode_stateful),
                              bytes, errors_obj));
  if (result_obj.isError()) {
    if (result_obj.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kSystemError,
                           "could not call _codecs.unicode_escape_decode");
    }
    return nullptr;
  }
  Tuple result(&scope, *result_obj);
  Int first_invalid_index(&scope, result.at(2));
  word invalid_index = first_invalid_index.asWord();
  if (invalid_index > -1) {
    *first_invalid_escape = c_str + invalid_index;
  }
  return ApiHandle::newReference(thread, result.at(0));
}

PY_EXPORT PyObject* PyUnicode_EncodeCodePage(int /* e */, PyObject* /* e */,
                                             const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_EncodeCodePage");
}

PY_EXPORT PyObject* PyUnicode_EncodeLocale(PyObject* /* e */,
                                           const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_EncodeLocale");
}

PY_EXPORT PyObject* _PyUnicode_EncodeUTF16(PyObject* unicode,
                                           const char* errors, int byteorder) {
  DCHECK(unicode != nullptr, "unicode cannot be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  if (!runtime->isInstanceOfStr(*str)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object byteorder_obj(&scope, runtime->newInt(byteorder));
  Object tuple_obj(&scope,
                   thread->invokeFunction3(ID(_codecs), ID(utf_16_encode), str,
                                           errors_obj, byteorder_obj));
  if (tuple_obj.isError()) {
    return nullptr;
  }
  Tuple tuple(&scope, *tuple_obj);
  return ApiHandle::newReference(thread, tuple.at(0));
}

PY_EXPORT PyObject* PyUnicode_EncodeUTF16(const Py_UNICODE* unicode,
                                          Py_ssize_t size, const char* errors,
                                          int byteorder) {
  PyObject* str = PyUnicode_FromUnicode(unicode, size);
  if (str == nullptr) return nullptr;
  PyObject* result = _PyUnicode_EncodeUTF16(str, errors, byteorder);
  Py_DECREF(str);
  return result;
}

PY_EXPORT PyObject* _PyUnicode_EncodeUTF32(PyObject* unicode,
                                           const char* errors, int byteorder) {
  DCHECK(unicode != nullptr, "unicode cannot be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  if (!runtime->isInstanceOfStr(*str)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object byteorder_obj(&scope, runtime->newInt(byteorder));
  Object tuple_obj(&scope,
                   thread->invokeFunction3(ID(_codecs), ID(utf_32_encode), str,
                                           errors_obj, byteorder_obj));
  if (tuple_obj.isError()) {
    return nullptr;
  }
  Tuple tuple(&scope, *tuple_obj);
  return ApiHandle::newReference(thread, tuple.at(0));
}

PY_EXPORT PyObject* PyUnicode_EncodeUTF32(const Py_UNICODE* unicode,
                                          Py_ssize_t size, const char* errors,
                                          int byteorder) {
  PyObject* str = PyUnicode_FromUnicode(unicode, size);
  if (str == nullptr) return nullptr;
  PyObject* result = _PyUnicode_EncodeUTF32(str, errors, byteorder);
  Py_DECREF(str);
  return result;
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

PY_EXPORT int PyUnicode_FSDecoder(PyObject* arg, void* addr) {
  if (arg == nullptr) {
    Py_DECREF(*(PyObject**)addr);
    *reinterpret_cast<PyObject**>(addr) = nullptr;
    return 1;
  }

  bool is_buffer = PyObject_CheckBuffer(arg);
  PyObject* path;
  if (!is_buffer) {
    path = PyOS_FSPath(arg);
    if (path == nullptr) return 0;
  } else {
    path = arg;
    Py_INCREF(arg);
  }

  PyObject* output;
  if (PyUnicode_Check(path)) {
    output = path;
  } else if (PyBytes_Check(path) || is_buffer) {
    if (!PyBytes_Check(path) &&
        PyErr_WarnFormat(
            PyExc_DeprecationWarning, 1,
            "path should be string, bytes, or os.PathLike, not %.200s",
            PyObject_TypeName(arg))) {
      Py_DECREF(path);
      return 0;
    }
    PyObject* path_bytes = PyBytes_FromObject(path);
    Py_DECREF(path);
    if (!path_bytes) return 0;
    output = PyUnicode_DecodeFSDefaultAndSize(PyBytes_AS_STRING(path_bytes),
                                              PyBytes_GET_SIZE(path_bytes));
    Py_DECREF(path_bytes);
    if (!output) return 0;
  } else {
    PyErr_Format(PyExc_TypeError,
                 "path should be string, bytes, or os.PathLike, not %.200s",
                 PyObject_TypeName(arg));
    Py_DECREF(path);
    return 0;
  }

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str output_str(&scope, ApiHandle::fromPyObject(output)->asObject());
  if (strFindAsciiChar(output_str, '\0') >= 0) {
    PyErr_SetString(PyExc_ValueError, "embedded null character");
    Py_DECREF(output);
    return 0;
  }
  *reinterpret_cast<PyObject**>(addr) = output;
  return Py_CLEANUP_SUPPORTED;
}

PY_EXPORT Py_ssize_t PyUnicode_Find(PyObject* str, PyObject* substr,
                                    Py_ssize_t start, Py_ssize_t end,
                                    int direction) {
  DCHECK(str != nullptr, "str must be non-null");
  DCHECK(substr != nullptr, "substr must be non-null");
  DCHECK(direction == -1 || direction == 1, "direction must be -1 or 1");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object haystack_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Object needle_obj(&scope, ApiHandle::fromPyObject(substr)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*haystack_obj)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "PyUnicode_Find requires a 'str' instance");
    return -2;
  }
  Str haystack(&scope, strUnderlying(*haystack_obj));
  if (!runtime->isInstanceOfStr(*needle_obj)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "PyUnicode_Find requires a 'str' instance");
    return -2;
  }
  Str needle(&scope, strUnderlying(*needle_obj));
  if (direction == 1) return strFindWithRange(haystack, needle, start, end);
  return strRFind(haystack, needle, start, end);
}

PY_EXPORT Py_ssize_t PyUnicode_FindChar(PyObject* str, Py_UCS4 ch,
                                        Py_ssize_t start, Py_ssize_t end,
                                        int direction) {
  DCHECK(str != nullptr, "str must not be null");
  DCHECK(direction == 1 || direction == -1, "direction must be -1 or 1");
  Thread* thread = Thread::current();
  if (start < 0 || end < 0) {
    thread->raiseWithFmt(LayoutId::kIndexError, "string index out of range");
    return -2;
  }
  HandleScope scope(thread);
  Object haystack_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*haystack_obj),
         "PyUnicode_FindChar requires a 'str' instance");
  Str haystack(&scope, strUnderlying(*haystack_obj));
  Str needle(&scope, SmallStr::fromCodePoint(ch));
  if (direction == 1) return strFindWithRange(haystack, needle, start, end);
  return strRFind(haystack, needle, start, end);
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
  writer.min_length = std::strlen(format) + 100;
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

PY_EXPORT PyObject* PyUnicode_FromOrdinal(int ordinal) {
  Thread* thread = Thread::current();
  if (ordinal < 0 || ordinal > kMaxUnicode) {
    thread->raiseWithFmt(LayoutId::kValueError,
                         "chr() arg not in range(0x110000)");
    return nullptr;
  }
  return ApiHandle::newReference(thread, SmallStr::fromCodePoint(ordinal));
}

PY_EXPORT PyObject* PyUnicode_FromWideChar(const wchar_t* buffer,
                                           Py_ssize_t size) {
  Thread* thread = Thread::current();
  if (buffer == nullptr && size != 0) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  RawObject result = size == -1
                         ? newStrFromWideChar(thread, buffer)
                         : newStrFromWideCharWithLength(thread, buffer, size);
  return result.isErrorException() ? nullptr
                                   : ApiHandle::newReference(thread, result);
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
  Str str(&scope, strUnderlying(*obj));
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
  Str obj_str(&scope, *obj);
  Object result(&scope, Runtime::internStr(thread, obj_str));
  if (result != obj_str) {
    Py_DECREF(*pobj);
    *pobj = ApiHandle::newReference(thread, *result);
  }
}

PY_EXPORT int PyUnicode_IsIdentifier(PyObject* str) {
  DCHECK(str != nullptr, "str must not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  if (str_obj == Str::empty()) {
    return false;
  }
  Object result(&scope, thread->invokeMethodStatic1(LayoutId::kStr,
                                                    ID(isidentifier), str_obj));
  DCHECK(!result.isErrorNotFound(), "could not call str.isidentifier");
  CHECK(!result.isError(), "this function should not error");
  return Bool::cast(*result).value();
}

PY_EXPORT PyObject* PyUnicode_Join(PyObject* sep, PyObject* seq) {
  DCHECK(sep != nullptr, "sep should not be null");
  DCHECK(seq != nullptr, "seq should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object sep_obj(&scope, ApiHandle::fromPyObject(sep)->asObject());
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(&scope, thread->invokeMethodStatic2(LayoutId::kStr, ID(join),
                                                    sep_obj, seq_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError, "could not call str.join");
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
  Object result(&scope, thread->invokeMethodStatic2(
                            LayoutId::kStr, ID(partition), str_obj, sep_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "could not call str.partition");
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
  Object result(&scope, thread->invokeMethodStatic2(
                            LayoutId::kStr, ID(rpartition), str_obj, sep_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "could not call str.rpartition");
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
                thread->invokeMethodStatic3(LayoutId::kStr, ID(rsplit), str_obj,
                                            sep_obj, maxsplit_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError, "could not call str.rsplit");
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
    thread->raiseWithFmt(LayoutId::kTypeError, "str must be str");
    return nullptr;
  }

  Object substr_obj(&scope, ApiHandle::fromPyObject(substr)->asObject());
  if (!runtime->isInstanceOfStr(*substr_obj)) {
    thread->raiseWithFmt(LayoutId::kTypeError, "substr must be str");
    return nullptr;
  }

  Object replstr_obj(&scope, ApiHandle::fromPyObject(replstr)->asObject());
  if (!runtime->isInstanceOfStr(*replstr_obj)) {
    thread->raiseWithFmt(LayoutId::kTypeError, "replstr must be str");
    return nullptr;
  }

  Str str_str(&scope, strUnderlying(*str_obj));
  Str substr_str(&scope, strUnderlying(*substr_obj));
  Str replstr_str(&scope, strUnderlying(*replstr_obj));
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
                thread->invokeMethodStatic3(LayoutId::kStr, ID(split), str_obj,
                                            sep_obj, maxsplit_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError, "could not call str.split");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyUnicode_Splitlines(PyObject* str, int keepends) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(str)->asObject());
  if (!thread->runtime()->isInstanceOfStr(*str_obj)) {
    thread->raiseWithFmt(LayoutId::kTypeError, "must be str, not '%T'",
                         &str_obj);
    return nullptr;
  }
  Str str_str(&scope, strUnderlying(*str_obj));
  return ApiHandle::newReference(thread,
                                 strSplitlines(thread, str_str, keepends));
}

PY_EXPORT PyObject* PyUnicode_Substring(PyObject* pyobj, Py_ssize_t start,
                                        Py_ssize_t end) {
  DCHECK(pyobj != nullptr, "null argument to PyUnicode_Substring");
  Thread* thread = Thread::current();
  if (start < 0 || end < 0) {
    thread->raiseWithFmt(LayoutId::kIndexError, "string index out of range");
    return nullptr;
  }
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfStr(*obj),
         "PyUnicode_Substring requires a 'str' instance");
  Str self(&scope, strUnderlying(*obj));
  word len = self.charLength();
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

PY_EXPORT PyTypeObject* PyUnicode_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kStr)));
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
  // TODO(T41785453): Remove the StrArray intermediary
  StrArray array(&scope, runtime->newStrArray());
  runtime->strArrayEnsureCapacity(thread, array, size);
  for (word i = 0; i < size; ++i) {
    runtime->strArrayAddCodePoint(thread, array, cp[i]);
  }
  return ApiHandle::newReference(thread, runtime->strFromStrArray(array));
}

PY_EXPORT PyObject* PyUnicode_FromKindAndData(int kind, const void* buffer,
                                              Py_ssize_t size) {
  Thread* thread = Thread::current();
  if (size < 0) {
    thread->raiseWithFmt(LayoutId::kValueError, "size must be positive");
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
  thread->raiseWithFmt(LayoutId::kSystemError, "invalid kind");
  return nullptr;
}

PY_EXPORT PyObject* PyUnicode_FromUnicode(const Py_UNICODE* code_units,
                                          Py_ssize_t size) {
  if (code_units == nullptr) {
    // TODO(T36562134): Implement _PyUnicode_New
    UNIMPLEMENTED("_PyUnicode_New");
  }

  Thread* thread = Thread::current();
  RawObject result = newStrFromWideCharWithLength(thread, code_units, size);
  return result.isErrorException() ? nullptr
                                   : ApiHandle::newReference(thread, result);
}

PY_EXPORT int PyUnicode_KIND_Func(PyObject* obj) {
  // TODO(T47682853): Introduce new PyUnicode_VARBYTE_KIND
  CHECK(PyUnicode_IS_ASCII_Func(obj), "only ASCII allowed");
  return PyUnicode_1BYTE_KIND;
}

// NOTE: This will return a cached and managed C-string buffer that is a copy
// of the Str internal buffer. It is NOT a direct pointer into the string
// object, so writing into this buffer will do nothing. This is different
// behavior from CPython, where changing the data in the buffer changes the
// string object.
PY_EXPORT void* PyUnicode_DATA_Func(PyObject* str) {
  return PyUnicode_AsUTF8(str);
}

PY_EXPORT Py_UCS4 PyUnicode_READ_Func(int kind, void* data, Py_ssize_t index) {
  if (kind == PyUnicode_1BYTE_KIND) return static_cast<Py_UCS1*>(data)[index];
  if (kind == PyUnicode_2BYTE_KIND) return static_cast<Py_UCS2*>(data)[index];
  DCHECK(kind == PyUnicode_4BYTE_KIND, "kind must be PyUnicode_4BYTE_KIND");
  return static_cast<Py_UCS4*>(data)[index];
}

PY_EXPORT Py_UCS4 PyUnicode_READ_CHAR_Func(PyObject* obj, Py_ssize_t index) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());
  DCHECK(thread->runtime()->isInstanceOfStr(*str_obj),
         "PyUnicode_READ_CHAR must receive a unicode object");
  Str str(&scope, strUnderlying(*str_obj));
  word byte_offset = str.offsetByCodePoints(0, index);
  if (byte_offset == str.charLength()) return Py_UCS4{0};
  word num_bytes;
  return static_cast<Py_UCS4>(str.codePointAt(byte_offset, &num_bytes));
}

PY_EXPORT int PyUnicode_IS_ASCII_Func(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str(&scope, ApiHandle::fromPyObject(obj)->asObject());
  DCHECK(thread->runtime()->isInstanceOfStr(*str),
         "strIsASCII must receive a unicode object");
  return strUnderlying(*str).isASCII() ? 1 : 0;
}

PY_EXPORT int _Py_normalize_encoding(const char* encoding, char* lower,
                                     size_t lower_len) {
  char* buffer = lower;
  const char* lower_end = &lower[lower_len - 1];
  bool has_punct = false;
  for (char ch = *encoding; ch != '\0'; ch = *++encoding) {
    if (Py_ISALNUM(ch) || ch == '.') {
      if (has_punct && buffer != lower) {
        if (buffer == lower_end) {
          return 0;
        }
        *buffer++ = '_';
      }
      has_punct = false;

      if (buffer == lower_end) {
        return 0;
      }
      *buffer++ = Py_TOLOWER(ch);
    } else {
      has_punct = true;
    }
  }
  *buffer = '\0';
  return 1;
}

PY_EXPORT PyObject* _PyUnicode_AsUTF8String(PyObject* unicode,
                                            const char* errors) {
  DCHECK(unicode != nullptr, "unicode cannot be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  if (!runtime->isInstanceOfStr(*str)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Object errors_obj(&scope, symbolFromError(thread, errors));
  Object tuple_obj(&scope, thread->invokeFunction2(
                               ID(_codecs), ID(utf_8_encode), str, errors_obj));
  if (tuple_obj.isError()) {
    return nullptr;
  }
  Tuple tuple(&scope, *tuple_obj);
  return ApiHandle::newReference(thread, tuple.at(0));
}

PY_EXPORT wchar_t* _Py_DecodeUTF8_surrogateescape(const char* c_str,
                                                  Py_ssize_t size) {
  DCHECK(c_str != nullptr, "c_str cannot be null");
  wchar_t* wc_str =
      static_cast<wchar_t*>(PyMem_RawMalloc((size + 1) * sizeof(wchar_t)));
  for (Py_ssize_t i = 0; i < size; i++) {
    char ch = c_str[i];
    // TODO(T57811636): Support UTF-8 arguments on macOS.
    // We don't have UTF-8 decoding machinery that is decoupled from the
    // runtime
    if (ch & 0x80) {
      UNIMPLEMENTED("UTF-8 argument support unimplemented");
    }
    wc_str[i] = static_cast<wchar_t>(ch);
  }
  wc_str[size] = '\0';
  return wc_str;
}

// UTF-8 encoder using the surrogateescape error handler .
//
// On success, return 0 and write the newly allocated character string (use
// PyMem_Free() to free the memory) into *str.
//
// On encoding failure, return -2 and write the position of the invalid
// surrogate character into *error_pos (if error_pos is set) and the decoding
// error message into *reason (if reason is set).
//
// On memory allocation failure, return -1.
PY_EXPORT int _Py_EncodeUTF8Ex(const wchar_t* text, char** str,
                               size_t* error_pos, const char** reason,
                               int raw_malloc, int surrogateescape) {
  const Py_ssize_t max_char_size = 4;
  Py_ssize_t len = std::wcslen(text);
  DCHECK(len >= 0, "len must be non-negative");

  if (len > PY_SSIZE_T_MAX / max_char_size - 1) {
    return -1;
  }
  char* bytes;
  if (raw_malloc) {
    bytes = reinterpret_cast<char*>(PyMem_RawMalloc((len + 1) * max_char_size));
  } else {
    bytes = reinterpret_cast<char*>(PyMem_Malloc((len + 1) * max_char_size));
  }
  if (bytes == nullptr) {
    return -1;
  }

  char* p = bytes;
  for (Py_ssize_t i = 0; i < len; i++) {
    Py_UCS4 ch = text[i];

    if (ch < 0x80) {
      // Encode ASCII
      *p++ = (char)ch;

    } else if (ch < 0x0800) {
      // Encode Latin-1
      *p++ = (char)(0xc0 | (ch >> 6));
      *p++ = (char)(0x80 | (ch & 0x3f));
    } else if (Py_UNICODE_IS_SURROGATE(ch)) {
      // surrogateescape error handler
      if (!surrogateescape || !(0xDC80 <= ch && ch <= 0xDCFF)) {
        if (error_pos != nullptr) {
          *error_pos = (size_t)i;
        }
        if (reason != nullptr) {
          *reason = "encoding error";
        }
        if (raw_malloc) {
          PyMem_RawFree(bytes);
        } else {
          PyMem_Free(bytes);
        }
        return -2;
      }
      *p++ = (char)(ch & 0xff);
    } else if (ch < 0x10000) {
      *p++ = (char)(0xe0 | (ch >> 12));
      *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
      *p++ = (char)(0x80 | (ch & 0x3f));
    } else {
      // ch >= 0x10000
      DCHECK(ch <= kMaxUnicode, "ch must be a valid unicode code point");
      // Encode UCS4 Unicode ordinals
      *p++ = (char)(0xf0 | (ch >> 18));
      *p++ = (char)(0x80 | ((ch >> 12) & 0x3f));
      *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
      *p++ = (char)(0x80 | (ch & 0x3f));
    }
  }
  *p++ = '\0';

  size_t final_size = (p - bytes);
  char* bytes2;
  if (raw_malloc) {
    bytes2 = reinterpret_cast<char*>(PyMem_RawRealloc(bytes, final_size));
  } else {
    bytes2 = reinterpret_cast<char*>(PyMem_Realloc(bytes, final_size));
  }
  if (bytes2 == nullptr) {
    if (error_pos != nullptr) {
      *error_pos = (size_t)-1;
    }
    if (raw_malloc) {
      PyMem_RawFree(bytes);
    } else {
      PyMem_Free(bytes);
    }
    return -1;
  }
  *str = bytes2;
  return 0;
}

}  // namespace py
