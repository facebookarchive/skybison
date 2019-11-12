#include <cstdarg>

#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "capi-handles.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"
#include "utils.h"

namespace py {

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
  Bytes bytes(&scope, bytesUnderlying(*obj));
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

  Bytes self(&scope, bytesUnderlying(*obj));
  Bytes other(&scope, bytesUnderlying(*newpart_obj));
  Bytes result(&scope, runtime->bytesConcat(thread, self, other));
  *pyobj = ApiHandle::newReference(thread, *result);
  obj_handle->decref();
}

PY_EXPORT void PyBytes_ConcatAndDel(PyObject** pyobj, PyObject* newpart) {
  PyBytes_Concat(pyobj, newpart);
  Py_XDECREF(newpart);
}

PY_EXPORT PyObject* PyBytes_DecodeEscape(const char* c_str, Py_ssize_t size,
                                         const char* errors, Py_ssize_t unicode,
                                         const char* recode_encoding) {
  const char* first_invalid_escape;
  PyObject* result = _PyBytes_DecodeEscape(
      c_str, size, errors, unicode, recode_encoding, &first_invalid_escape);
  if (result == nullptr) return nullptr;
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

PY_EXPORT PyObject* _PyBytes_DecodeEscape(const char* c_str, Py_ssize_t size,
                                          const char* errors,
                                          Py_ssize_t /* unicode */,
                                          const char* recode_encoding,
                                          const char** first_invalid_escape) {
  DCHECK(c_str != nullptr, "c_str cannot be null");
  DCHECK(first_invalid_escape != nullptr,
         "first_invalid_escape cannot be null");

  // So we can remember if we've seen an invalid escape char or not
  *first_invalid_escape = nullptr;

  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object bytes(&scope, runtime->newBytesWithAll(View<byte>(
                           reinterpret_cast<const byte*>(c_str), size)));
  Object recode_obj(&scope, recode_encoding == nullptr
                                ? Str::empty()
                                : runtime->newStrFromCStr(recode_encoding));
  Object errors_obj(&scope, Str::empty());
  Symbols* symbols = runtime->symbols();
  if (errors == nullptr || std::strcmp(errors, "strict") == 0) {
    errors_obj = symbols->Strict();
  } else if (std::strcmp(errors, "ignore") == 0) {
    errors_obj = symbols->Ignore();
  } else if (std::strcmp(errors, "replace") == 0) {
    errors_obj = symbols->Replace();
  }
  Object result_obj(
      &scope, thread->invokeFunction3(SymbolId::kUnderCodecs,
                                      SymbolId::kUnderEscapeDecodeStateful,
                                      bytes, errors_obj, recode_obj));
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
  Bytes self(&scope, bytesUnderlying(*obj));
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

  Bytes bytes(&scope, bytesUnderlying(*bytes_obj));
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
  Bytes bytes(&scope, bytesUnderlying(*obj));
  if (bytes.length() == newsize) return 0;
  // we don't check here that Py_REFCNT(*pyobj) == 1
  *pyobj = ApiHandle::newReference(
      thread, runtime->bytesCopyWithSize(thread, bytes, newsize));
  handle->decref();
  return 0;
}

// _PyBytesWriter API

// Returns the beginning of the buffer currently used for writing.
static byte* writerBufferStart(_PyBytesWriter* writer) {
  return writer->use_heap_buffer ? writer->heap_buffer : writer->stack_buffer;
}

// Checks internal consistency of the writer struct. This function should only
// be called in a DCHECK. Always returns true, but does checks of its own.
static bool writerIsConsistent(_PyBytesWriter* writer) {
  if (writer->use_heap_buffer) {
    CHECK(writer->heap_buffer, "heap buffer is not allocated");
  } else {
    CHECK(!writer->heap_buffer, "heap buffer was allocated too early");
  }
  if (writer->use_bytearray) {
    CHECK(!writer->overallocate, "ByteArray has its own overallocation scheme");
  }
  CHECK(0 <= writer->allocated, "allocated size must be non-negative");
  CHECK_BOUND(writer->min_size, writer->allocated);

  const byte* start = writerBufferStart(writer);
  const byte* end = start + writer->allocated;
  CHECK(*end == 0, "byte string must be null-terminated");
  CHECK(writer->ptr, "current pointer cannot be null");
  CHECK(start <= writer->ptr, "pointer is before the start of the buffer");
  CHECK(writer->ptr <= end, "pointer is past the end of the buffer");
  return true;
}

// Allocates the writer and prepares it to write the specified number of bytes.
// Uses the small stack buffer if possible.
PY_EXPORT void* _PyBytesWriter_Alloc(_PyBytesWriter* writer, Py_ssize_t size) {
  DCHECK(writer->min_size == 0 && writer->heap_buffer == nullptr,
         "writer has already been allocated");
  writer->allocated = sizeof(writer->stack_buffer) - 1;
  return _PyBytesWriter_Prepare(writer, writer->stack_buffer, size);
}

// Frees the writer's heap-allocated buffer.
PY_EXPORT void _PyBytesWriter_Dealloc(_PyBytesWriter* writer) {
  if (writer->heap_buffer) std::free(writer->heap_buffer);
}

// Converts the memory written to the writer into a Bytes or ByteArray object.
// Assumes that str points to the end of the written data. Frees all memory
// that was allocated by malloc.
PY_EXPORT PyObject* _PyBytesWriter_Finish(_PyBytesWriter* writer, void* str) {
  writer->ptr = reinterpret_cast<byte*>(str);
  DCHECK(writerIsConsistent(writer), "invariants broken");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  const byte* start = writerBufferStart(writer);
  word size = writer->ptr - start;
  if (size == 0) {
    return ApiHandle::newReference(thread, writer->use_bytearray
                                               ? runtime->newByteArray()
                                               : Bytes::empty());
  }
  if (writer->use_bytearray) {
    HandleScope scope(thread);
    ByteArray result(&scope, runtime->newByteArray());
    runtime->byteArrayExtend(thread, result, View<byte>{start, size});
    return ApiHandle::newReference(thread, *result);
  }
  PyObject* result = ApiHandle::newReference(
      thread, runtime->newBytesWithAll(View<byte>{start, size}));
  return result;
}

// Initializes the _PyBytesWriter struct.
PY_EXPORT void _PyBytesWriter_Init(_PyBytesWriter* writer) {
  // Unlike CPython, zero out the stack buffer as well.
  std::memset(writer, 0, sizeof(*writer));
}

// Prepares the writer for the specified number of bytes. Reallocates if the new
// size exceeds the currently allocated buffer. Returns the current pointer into
// the buffer if the allocation succeeds. Returns null with a MemoryError set
// if growing would exceed SmallInt::kMaxValue.
PY_EXPORT void* _PyBytesWriter_Prepare(_PyBytesWriter* writer, void* str,
                                       Py_ssize_t growth) {
  writer->ptr = reinterpret_cast<byte*>(str);
  DCHECK(writerIsConsistent(writer), "invariants broken");
  if (growth == 0) return str;
  DCHECK(growth > 0, "size must be non-negative");
  if (growth > SmallInt::kMaxValue - writer->min_size) {
    PyErr_NoMemory();
    _PyBytesWriter_Dealloc(writer);
    return nullptr;
  }
  word new_min_size = writer->min_size + growth;
  if (new_min_size > writer->allocated) {
    str = _PyBytesWriter_Resize(writer, str, new_min_size);
  }
  writer->min_size = new_min_size;
  writer->ptr = reinterpret_cast<byte*>(str);
  return str;
}

static const word kOverallocateFactor = 4;

// Grows the writer to at least the provided size. Overallocates by 1/4 if
// writer->overallocate or writer->use_bytearray is set.
PY_EXPORT void* _PyBytesWriter_Resize(_PyBytesWriter* writer, void* str,
                                      Py_ssize_t new_size) {
  writer->ptr = reinterpret_cast<byte*>(str);
  DCHECK(writerIsConsistent(writer), "invariants broken");
  DCHECK(writer->allocated < new_size, "resize should only be called to grow");
  DCHECK_BOUND(new_size, SmallInt::kMaxValue);
  if ((writer->overallocate || writer->use_bytearray) &&
      new_size <= SmallInt::kMaxValue - new_size / kOverallocateFactor) {
    new_size += new_size / kOverallocateFactor;
  }

  word len;
  byte* new_buffer = reinterpret_cast<byte*>(std::malloc(new_size + 1));
  if (writer->use_heap_buffer) {
    len = writer->ptr - writer->heap_buffer;
    std::memcpy(new_buffer, writer->heap_buffer, len);
    std::free(writer->heap_buffer);
  } else {
    len = writer->ptr - writer->stack_buffer;
    std::memcpy(new_buffer, writer->stack_buffer, len);
  }
  new_buffer[new_size] = '\0';

  writer->allocated = new_size;
  writer->heap_buffer = new_buffer;
  writer->ptr = new_buffer + len;
  writer->use_heap_buffer = true;
  return writer->ptr;
}

// Writes the specified bytes. Grows writer.min_size by the specified length.
// Do not use to write into memory already allocated by _PyBytesWriter_Prepare.
PY_EXPORT void* _PyBytesWriter_WriteBytes(_PyBytesWriter* writer, void* str,
                                          const void* bytes, Py_ssize_t len) {
  str = _PyBytesWriter_Prepare(writer, str, len);
  if (str == nullptr) return nullptr;
  std::memcpy(str, bytes, len);
  writer->ptr = reinterpret_cast<byte*>(str) + len;
  return writer->ptr;
}

}  // namespace py
