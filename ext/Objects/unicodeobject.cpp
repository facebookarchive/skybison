// unicodeobject.c implementation
#include <cwchar>

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "utils.h"

namespace python {

PY_EXPORT int _PyUnicode_EqualToASCIIString(PyObject* unicode,
                                            const char* c_str) {
  DCHECK(unicode, "nullptr argument");
  DCHECK(c_str, "nullptr argument");

  HandleScope scope;
  Str str(&scope, ApiHandle::fromPyObject(unicode)->asObject());
  return str->equalsCStr(c_str);
}

PY_EXPORT int _PyUnicode_EQ(PyObject* aa, PyObject* bb) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Str lhs(&scope, ApiHandle::fromPyObject(aa)->asObject());
  Str rhs(&scope, ApiHandle::fromPyObject(bb)->asObject());
  word diff = lhs->compare(*rhs);
  return diff == 0 ? 1 : 0;
}

PY_EXPORT size_t Py_UNICODE_strlen(const Py_UNICODE* /* u */) {
  UNIMPLEMENTED("Py_UNICODE_strlen");
}

PY_EXPORT int _PyUnicode_Ready(PyObject* /* unicode */) {
  UNIMPLEMENTED("_PyUnicode_Ready");
}

PY_EXPORT int PyUnicode_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isStr();
}

PY_EXPORT int PyUnicode_Check_Func(PyObject* obj) {
  if (PyUnicode_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubclass(Thread::currentThread(),
                                                  LayoutId::kStr);
}

PY_EXPORT PyObject* PyUnicode_FromString(const char* c_string) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newStrFromCStr(c_string));
  return ApiHandle::newReference(thread, *value);
}

PY_EXPORT char* PyUnicode_AsUTF8AndSize(PyObject* pyunicode, Py_ssize_t* size) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pyunicode == nullptr) {
    thread->raiseBadArgument();
    return nullptr;
  }

  auto handle = ApiHandle::fromPyObject(pyunicode);
  Object obj(&scope, handle->asObject());
  if (!obj->isStr()) {
    if (thread->runtime()->isInstanceOfStr(*obj)) {
      UNIMPLEMENTED("RawStr subclass");
    }
    thread->raiseBadInternalCall();
    return nullptr;
  }

  Str str(&scope, *obj);
  word length = str->length();
  if (size) *size = length;
  if (void* cache = handle->cache()) return static_cast<char*>(cache);
  auto result = static_cast<byte*>(std::malloc(length + 1));
  str->copyTo(result, length);
  result[length] = '\0';
  handle->setCache(result);
  return reinterpret_cast<char*>(result);
}

PY_EXPORT char* PyUnicode_AsUTF8(PyObject* unicode) {
  return PyUnicode_AsUTF8AndSize(unicode, nullptr);
}

PY_EXPORT PyObject* PyUnicode_FromStringAndSize(const char* u,
                                                Py_ssize_t size) {
  Thread* thread = Thread::currentThread();

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
  return ApiHandle::newReference(thread, value);
}

PY_EXPORT PyObject* PyUnicode_EncodeFSDefault(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_EncodeFSDefault");
}

PY_EXPORT PyObject* PyUnicode_New(Py_ssize_t /* e */, Py_UCS4 /* r */) {
  UNIMPLEMENTED("PyUnicode_New");
}

PY_EXPORT void PyUnicode_Append(PyObject** /* p_left */, PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_Append");
}

PY_EXPORT void PyUnicode_AppendAndDel(PyObject** /* pleft */,
                                      PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_AppendAndDel");
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

PY_EXPORT Py_UCS4* PyUnicode_AsUCS4(PyObject* /* g */, Py_UCS4* /* t */,
                                    Py_ssize_t /* e */, int /* l */) {
  UNIMPLEMENTED("PyUnicode_AsUCS4");
}

PY_EXPORT Py_UCS4* PyUnicode_AsUCS4Copy(PyObject* /* g */) {
  UNIMPLEMENTED("PyUnicode_AsUCS4Copy");
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
  Thread* thread = Thread::currentThread();
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
    return RawStr::cast(*left_obj)->compare(*right_obj);
  }

  Str ltype(&scope, Type::cast(runtime->typeOf(*left_obj))->name());
  Str rtype(&scope, Type::cast(runtime->typeOf(*right_obj))->name());
  // TODO(T32655200): Once we have a real string formatter, use that instead of
  // converting the names to C strings here.
  unique_c_ptr<char> ltype_name(ltype->toCStr());
  unique_c_ptr<char> rtype_name(rtype->toCStr());

  thread->raiseTypeError(runtime->newStrFromFormat(
      "Can't compare %.100s and %.100s", ltype_name.get(), rtype_name.get()));
  return -1;
}

PY_EXPORT int PyUnicode_CompareWithASCIIString(PyObject* uni, const char* str) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Str str_obj(&scope, ApiHandle::fromPyObject(uni)->asObject());
  // TODO(atalaba): Allow for proper comparison against Latin-1 strings. For
  // example, in CPython: "\xC3\xA9" (UTF-8) == "\xE9" (Latin-1), and
  // "\xE9 longer" > "\xC3\xA9".
  return str_obj->compareCStr(str);
}

PY_EXPORT PyObject* PyUnicode_Concat(PyObject* /* t */, PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_Concat");
}

PY_EXPORT int PyUnicode_Contains(PyObject* /* r */, PyObject* /* r */) {
  UNIMPLEMENTED("PyUnicode_Contains");
}

PY_EXPORT Py_ssize_t PyUnicode_Count(PyObject* /* r */, PyObject* /* r */,
                                     Py_ssize_t /* t */, Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicode_Count");
}

PY_EXPORT PyObject* PyUnicode_Decode(const char* /* s */, Py_ssize_t /* e */,
                                     const char* /* g */, const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_Decode");
}

PY_EXPORT PyObject* PyUnicode_DecodeASCII(const char* /* s */,
                                          Py_ssize_t /* e */,
                                          const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeASCII");
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
  Thread* thread = Thread::currentThread();
  return ApiHandle::newReference(thread,
                                 thread->runtime()->newStrFromCStr(c_str));
}

PY_EXPORT PyObject* PyUnicode_DecodeFSDefaultAndSize(const char* c_str,
                                                     Py_ssize_t size) {
  Thread* thread = Thread::currentThread();
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

PY_EXPORT PyObject* PyUnicode_DecodeUTF8(const char* /* s */,
                                         Py_ssize_t /* e */,
                                         const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF8");
}

PY_EXPORT PyObject* PyUnicode_DecodeUTF8Stateful(const char* /* s */,
                                                 Py_ssize_t /* e */,
                                                 const char* /* s */,
                                                 Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF8Stateful");
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

PY_EXPORT int PyUnicode_FSConverter(PyObject* /* g */, void* /* r */) {
  UNIMPLEMENTED("PyUnicode_FSConverter");
}

PY_EXPORT int PyUnicode_FSDecoder(PyObject* /* g */, void* /* r */) {
  UNIMPLEMENTED("PyUnicode_FSDecoder");
}

PY_EXPORT Py_ssize_t PyUnicode_Find(PyObject* /* r */, PyObject* /* r */,
                                    Py_ssize_t /* t */, Py_ssize_t /* d */,
                                    int /* n */) {
  UNIMPLEMENTED("PyUnicode_Find");
}

PY_EXPORT Py_ssize_t PyUnicode_FindChar(PyObject* /* r */, Py_UCS4 /* h */,
                                        Py_ssize_t /* t */, Py_ssize_t /* d */,
                                        int /* n */) {
  UNIMPLEMENTED("PyUnicode_FindChar");
}

PY_EXPORT PyObject* PyUnicode_Format(PyObject* /* t */, PyObject* /* s */) {
  UNIMPLEMENTED("PyUnicode_Format");
}

PY_EXPORT PyObject* PyUnicode_FromEncodedObject(PyObject* /* j */,
                                                const char* /* g */,
                                                const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_FromEncodedObject");
}

PY_EXPORT PyObject* PyUnicode_FromFormat(const char* /* t */, ...) {
  UNIMPLEMENTED("PyUnicode_FromFormat");
}

PY_EXPORT PyObject* PyUnicode_FromFormatV(const char* /* t */,
                                          va_list /* s */) {
  UNIMPLEMENTED("PyUnicode_FromFormatV");
}

PY_EXPORT PyObject* PyUnicode_FromObject(PyObject* /* j */) {
  UNIMPLEMENTED("PyUnicode_FromObject");
}

PY_EXPORT PyObject* PyUnicode_FromOrdinal(int /* l */) {
  UNIMPLEMENTED("PyUnicode_FromOrdinal");
}

PY_EXPORT PyObject* PyUnicode_FromWideChar(const wchar_t* buffer,
                                           Py_ssize_t size) {
  Thread* thread = Thread::currentThread();
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

PY_EXPORT Py_ssize_t PyUnicode_GetLength(PyObject* py_str) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object str_obj(&scope, ApiHandle::fromPyObject(py_str)->asObject());
  if (!runtime->isInstanceOfStr(str_obj)) {
    thread->raiseBadArgument();
    return -1;
  }

  if (!str_obj->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }

  Str str(&scope, *str_obj);
  return str->length();  // TODO(T36613745): this should return the number of
                         // code points, not bytes
}

PY_EXPORT Py_ssize_t PyUnicode_GetSize(PyObject* py_str) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object str_obj(&scope, ApiHandle::fromPyObject(py_str)->asObject());
  if (!runtime->isInstanceOfStr(str_obj)) {
    thread->raiseBadArgument();
    return -1;
  }

  if (!str_obj->isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }

  Str str(&scope, *str_obj);
  return str->length();  // TODO(T36613745): this should return the number of
                         // code units, not bytes
}

PY_EXPORT PyObject* PyUnicode_InternFromString(const char* /* p */) {
  UNIMPLEMENTED("PyUnicode_InternFromString");
}

PY_EXPORT void PyUnicode_InternImmortal(PyObject** /* p */) {
  UNIMPLEMENTED("PyUnicode_InternImmortal");
}

PY_EXPORT void PyUnicode_InternInPlace(PyObject** /* p */) {
  UNIMPLEMENTED("PyUnicode_InternInPlace");
}

PY_EXPORT int PyUnicode_IsIdentifier(PyObject* /* f */) {
  UNIMPLEMENTED("PyUnicode_IsIdentifier");
}

PY_EXPORT PyObject* PyUnicode_Join(PyObject* /* r */, PyObject* /* q */) {
  UNIMPLEMENTED("PyUnicode_Join");
}

PY_EXPORT PyObject* PyUnicode_Partition(PyObject* /* j */, PyObject* /* j */) {
  UNIMPLEMENTED("PyUnicode_Partition");
}

PY_EXPORT PyObject* PyUnicode_RPartition(PyObject* /* j */, PyObject* /* j */) {
  UNIMPLEMENTED("PyUnicode_RPartition");
}

PY_EXPORT PyObject* PyUnicode_RSplit(PyObject* /* s */, PyObject* /* p */,
                                     Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicode_RSplit");
}

PY_EXPORT Py_UCS4 PyUnicode_ReadChar(PyObject* /* e */, Py_ssize_t /* x */) {
  UNIMPLEMENTED("PyUnicode_ReadChar");
}

PY_EXPORT PyObject* PyUnicode_Replace(PyObject* /* r */, PyObject* /* r */,
                                      PyObject* /* r */, Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicode_Replace");
}

PY_EXPORT int PyUnicode_Resize(PyObject** /* p_unicode */, Py_ssize_t /* h */) {
  UNIMPLEMENTED("PyUnicode_Resize");
}

PY_EXPORT PyObject* PyUnicode_RichCompare(PyObject* /* t */, PyObject* /* t */,
                                          int /* p */) {
  UNIMPLEMENTED("PyUnicode_RichCompare");
}

PY_EXPORT PyObject* PyUnicode_Split(PyObject* /* s */, PyObject* /* p */,
                                    Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicode_Split");
}

PY_EXPORT PyObject* PyUnicode_Splitlines(PyObject* /* g */, int /* s */) {
  UNIMPLEMENTED("PyUnicode_Splitlines");
}

PY_EXPORT PyObject* PyUnicode_Substring(PyObject* /* f */, Py_ssize_t /* t */,
                                        Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicode_Substring");
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

PY_EXPORT PyObject* PyUnicode_FromKindAndData(int /* d */, const void* /* r */,
                                              Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_FromKindAndData");
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
  Thread* thread = Thread::currentThread();
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

}  // namespace python
