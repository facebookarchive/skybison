#include <cstdarg>

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"
#include "utils.h"

namespace python {

PY_EXPORT int PyBytes_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isBytes();
}

PY_EXPORT int PyBytes_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfBytes(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT char* PyBytes_AsString(PyObject* pyobj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  if (void* cache = handle->cache()) return static_cast<char*>(cache);
  Bytes bytes(&scope, bytesUnderlying(thread, obj));
  word len = bytes.length();
  auto cache = static_cast<byte*>(std::malloc(len + 1));
  bytes.copyTo(cache, len);
  cache[len] = '\0';
  handle->setCache(cache);
  return reinterpret_cast<char*>(cache);
}

PY_EXPORT int PyBytes_AsStringAndSize(PyObject* pybytes, char** buffer,
                                      Py_ssize_t* length) {
  if (buffer == nullptr) {
    PyErr_BadInternalCall();
    return -1;
  }
  char* str = PyBytes_AsString(pybytes);
  if (str == nullptr) return -1;
  Py_ssize_t len = PyBytes_Size(pybytes);
  if (length != nullptr) {
    *length = len;
  } else if (std::strlen(str) != static_cast<size_t>(len)) {
    PyErr_SetString(PyExc_ValueError, "embedded null byte");
    return -1;
  }
  *buffer = str;
  return 0;
}

PY_EXPORT void PyBytes_Concat(PyObject** pyobj, PyObject* newpart) {
  CHECK(pyobj != nullptr, "reference to bytes object must be non-null");
  if (*pyobj == nullptr) return;
  if (newpart == nullptr) {
    PyObject* tmp = *pyobj;
    *pyobj = nullptr;
    Py_DECREF(tmp);
    return;
  }

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ApiHandle* obj_handle = ApiHandle::fromPyObject(*pyobj);
  Object obj(&scope, obj_handle->asObject());
  Object newpart_obj(&scope, ApiHandle::fromPyObject(newpart)->asObject());
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytes(*obj) ||
      !runtime->isInstanceOfBytes(*newpart_obj)) {
    thread->raiseBadArgument();
    *pyobj = nullptr;
    obj_handle->decref();
    return;
  }

  Bytes self(&scope, bytesUnderlying(thread, obj));
  Bytes other(&scope, bytesUnderlying(thread, newpart_obj));
  Bytes result(&scope, runtime->bytesConcat(thread, self, other));
  *pyobj = ApiHandle::newReference(thread, *result);
  obj_handle->decref();
}

PY_EXPORT void PyBytes_ConcatAndDel(PyObject** pyobj, PyObject* newpart) {
  PyBytes_Concat(pyobj, newpart);
  Py_XDECREF(newpart);
}

PY_EXPORT PyObject* PyBytes_DecodeEscape(const char* /* s */,
                                         Py_ssize_t /* n */,
                                         const char* /* s */,
                                         Py_ssize_t /* e */,
                                         const char* /* g */) {
  UNIMPLEMENTED("PyBytes_DecodeEscape");
}

PY_EXPORT PyObject* PyBytes_FromFormat(const char* format, ...) {
  va_list vargs;
  va_start(vargs, format);
  PyObject* result = PyBytes_FromFormatV(format, vargs);
  va_end(vargs);
  return result;
}

static void writeBytes(Thread* thread, Runtime* runtime,
                       const ByteArray& writer, const char* buffer) {
  View<byte> array(reinterpret_cast<const byte*>(buffer), std::strlen(buffer));
  runtime->byteArrayExtend(thread, writer, array);
}

static const char* writeArg(Thread* thread, Runtime* runtime,
                            const ByteArray& writer, const char* start,
                            va_list vargs) {
  DCHECK(*start == '%', "index is not at a format specifier");
  const char* current = start + 1;

  // ignore the width (ex: 10 in "%10s")
  while (Py_ISDIGIT(*current)) current++;

  // parse the precision (ex: 10 in "%.10s")
  word precision = 0;
  if (*current == '.') {
    current++;
    for (; Py_ISDIGIT(*current); current++) {
      precision = precision * 10 + (*current - '0');
    }
  }

  // scan forward to the conversion specifier or the end of the string
  while (*current != '\0' && *current != '%' && !Py_ISALPHA(*current)) {
    current++;
  }

  // Handle the long flag ('l'), but only for %ld and %lu.
  // Others can be added when necessary.
  bool long_flag = false;
  if (*current == 'l' && (current[1] == 'd' || current[1] == 'u')) {
    long_flag = true;
    current++;
  }

  // Handle the size_t flag ('z'), but only for %zd and %zu.
  bool size_t_flag = false;
  if (*current == 'z' && (current[1] == 'd' || current[1] == 'u')) {
    size_t_flag = true;
    ++current;
  }

  // Longest 64-bit formatted numbers:
  // - "18446744073709551615\0" (21 bytes)
  // - "-9223372036854775808\0" (21 bytes)
  // Decimal takes the most space (it isn't enough for octal).
  // Longest 64-bit pointer representation: "0xffffffffffffffff\0" (19 bytes).
  char buffer[21];
  switch (*current) {
    case 'c': {
      int c = va_arg(vargs, int);
      if (c < 0 || c > 255) {
        thread->raiseWithFmt(LayoutId::kOverflowError,
                             "PyBytes_FromFormatV(): "
                             "%%c format expects an integer in [0,255]");
        return nullptr;
      }
      byteArrayAdd(thread, runtime, writer, static_cast<byte>(c));
      return current + 1;
    }
    case 'd':
      if (long_flag) {
        std::sprintf(buffer, "%ld", va_arg(vargs, long));
      } else if (size_t_flag) {
        std::sprintf(buffer, "%" PY_FORMAT_SIZE_T "d",
                     va_arg(vargs, Py_ssize_t));
      } else {
        std::sprintf(buffer, "%d", va_arg(vargs, int));
      }
      writeBytes(thread, runtime, writer, buffer);
      return current + 1;
    case 'u':
      if (long_flag) {
        std::sprintf(buffer, "%lu", va_arg(vargs, unsigned long));
      } else if (size_t_flag) {
        std::sprintf(buffer, "%" PY_FORMAT_SIZE_T "u", va_arg(vargs, size_t));
      } else {
        std::sprintf(buffer, "%u", va_arg(vargs, unsigned int));
      }
      writeBytes(thread, runtime, writer, buffer);
      return current + 1;
    case 'i':
      std::sprintf(buffer, "%i", va_arg(vargs, int));
      writeBytes(thread, runtime, writer, buffer);
      return current + 1;
    case 'x':
      std::sprintf(buffer, "%x", va_arg(vargs, int));
      writeBytes(thread, runtime, writer, buffer);
      return current + 1;
    case 's': {
      const char* arg = va_arg(vargs, const char*);
      word len = std::strlen(arg);
      if (precision > 0 && len > precision) {
        len = precision;
      }
      View<byte> array(reinterpret_cast<const byte*>(arg), len);
      runtime->byteArrayExtend(thread, writer, array);
      return current + 1;
    }
    case 'p':
      std::sprintf(buffer, "%p", va_arg(vargs, void*));
      // %p is ill-defined, ensure leading 0x
      if (buffer[1] == 'X') {
        buffer[1] = 'x';
      } else if (buffer[1] != 'x') {
        // missing 0x prefix, shift right and prepend
        std::memmove(buffer + 2, buffer, std::strlen(buffer) + 1);
        buffer[0] = '0';
        buffer[1] = 'x';
      }
      writeBytes(thread, runtime, writer, buffer);
      return current + 1;
    case '%':
      byteArrayAdd(thread, runtime, writer, '%');
      return current + 1;
    default:
      word len = static_cast<word>(std::strlen(start));
      View<byte> array(reinterpret_cast<const byte*>(start), len);
      runtime->byteArrayExtend(thread, writer, array);
      return start + len;
  }
}

PY_EXPORT PyObject* PyBytes_FromFormatV(const char* format, va_list vargs) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  ByteArray writer(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, writer, std::strlen(format));
  while (*format) {
    if (*format == '%') {
      format = writeArg(thread, runtime, writer, format, vargs);
      if (format == nullptr) return nullptr;
    } else {
      const char* next = format + 1;
      while (*next && *next != '%') next++;
      View<byte> view(reinterpret_cast<const byte*>(format), next - format);
      runtime->byteArrayExtend(thread, writer, view);
      format = next;
    }
  }
  return ApiHandle::newReference(thread,
                                 byteArrayAsBytes(thread, runtime, writer));
}

PY_EXPORT PyObject* PyBytes_FromObject(PyObject* pyobj) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(pyobj);
  Object obj(&scope, handle->asObject());
  if (obj.isBytes()) {
    handle->incref();
    return pyobj;
  }

  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kUnderBytesNew, obj));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyBytes_FromStringAndSize(const char* str,
                                              Py_ssize_t size) {
  Thread* thread = Thread::current();
  if (size < 0) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "Negative size passed to PyBytes_FromStringAndSize");
    return nullptr;
  }
  if (str == nullptr) {
    UNIMPLEMENTED("mutable, uninitialized bytes");
  }
  return ApiHandle::newReference(
      thread, thread->runtime()->newBytesWithAll(
                  {reinterpret_cast<const byte*>(str), size}));
}

PY_EXPORT PyObject* PyBytes_FromString(const char* str) {
  DCHECK(str, "nullptr argument");
  uword size = std::strlen(str);
  if (size > kMaxWord) {
    PyErr_SetString(PyExc_OverflowError, "byte string is too large");
    return nullptr;
  }

  return PyBytes_FromStringAndSize(str, static_cast<Py_ssize_t>(size));
}

PY_EXPORT PyObject* PyBytes_Repr(PyObject* pyobj, int smartquotes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  if (!thread->runtime()->isInstanceOfBytes(*obj)) {
    thread->raiseBadArgument();
    return nullptr;
  }
  Bytes self(&scope, bytesUnderlying(thread, obj));
  Object result(&scope, smartquotes ? bytesReprSmartQuotes(thread, self)
                                    : bytesReprSingleQuotes(thread, self));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT Py_ssize_t PyBytes_Size(PyObject* obj) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object bytes_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());

  if (!runtime->isInstanceOfBytes(*bytes_obj)) {
    thread->raiseWithFmt(LayoutId::kTypeError, "PyBytes_Size expected bytes");
    return -1;
  }

  Bytes bytes(&scope, bytesUnderlying(thread, bytes_obj));
  return bytes.length();
}

PY_EXPORT PyObject* _PyBytes_Join(PyObject* sep, PyObject* iter) {
  DCHECK(sep != nullptr && iter != nullptr, "null argument to _PyBytes_Join");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(sep)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfBytes(*obj),
         "non-bytes argument to _PyBytes_Join");
  Object iterable(&scope, ApiHandle::fromPyObject(iter)->asObject());
  Object result(&scope, thread->invokeMethodStatic2(
                            LayoutId::kBytes, SymbolId::kJoin, obj, iterable));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT int _PyBytes_Resize(PyObject** pyobj, Py_ssize_t newsize) {
  DCHECK(pyobj != nullptr, "_PyBytes_Resize given null argument");
  DCHECK(*pyobj != nullptr, "_PyBytes_Resize given pointer to null");
  Thread* thread = Thread::current();
  ApiHandle* handle = ApiHandle::fromPyObject(*pyobj);
  HandleScope scope(thread);
  Object obj(&scope, handle->asObject());
  Runtime* runtime = thread->runtime();
  if (newsize < 0 || !runtime->isInstanceOfBytes(*obj)) {
    *pyobj = nullptr;
    handle->decref();
    thread->raiseBadInternalCall();
    return -1;
  }
  Bytes bytes(&scope, bytesUnderlying(thread, obj));
  if (bytes.length() == newsize) return 0;
  // we don't check here that Py_REFCNT(*pyobj) == 1
  *pyobj = ApiHandle::newReference(
      thread, runtime->bytesCopyWithSize(thread, bytes, newsize));
  handle->decref();
  return 0;
}

}  // namespace python
