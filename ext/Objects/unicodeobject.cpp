// unicodeobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

PY_EXPORT int _PyUnicode_EqualToASCIIString(PyObject* /* unicode */,
                                            const char* /* str */) {
  UNIMPLEMENTED("_PyUnicode_EqualToASCIIString");
}

PY_EXPORT int _PyUnicode_EQ(PyObject* /* aa */, PyObject* /* bb */) {
  UNIMPLEMENTED("_PyUnicode_EQ");
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
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kStr);
}

PY_EXPORT PyObject* PyUnicode_FromString(const char* c_string) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object value(&scope, runtime->newStrFromCStr(c_string));
  return ApiHandle::fromObject(*value);
}

PY_EXPORT char* PyUnicode_AsUTF8AndSize(PyObject* pyunicode, Py_ssize_t* size) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pyunicode == nullptr) {
    UNIMPLEMENTED("PyErr_BadArgument");
    return nullptr;
  }

  auto handle = ApiHandle::fromPyObject(pyunicode);
  Object obj(&scope, handle->asObject());
  if (!obj->isStr()) {
    if (thread->runtime()->hasSubClassFlag(*obj, Type::Flag::kStrSubclass)) {
      UNIMPLEMENTED("RawStr subclass");
    }
    thread->raiseSystemErrorWithCStr("bad argument to internal function");
    return nullptr;
  }

  Str str(&scope, obj);
  word length = str->length();
  if (size) *size = length;
  if (void* cache = handle->cache()) return static_cast<char*>(cache);
  auto result = static_cast<byte*>(std::malloc(length + 1));
  str->copyTo(result, length);
  result[length] = '\0';
  handle->setCache(result);
  return reinterpret_cast<char*>(result);
}

PY_EXPORT const char* PyUnicode_AsUTF8(PyObject* unicode) {
  return PyUnicode_AsUTF8AndSize(unicode, nullptr);
}

PY_EXPORT PyObject* PyUnicode_FromStringAndSize(const char*, Py_ssize_t) {
  UNIMPLEMENTED("PyUnicode_FromStringAndSize");
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

PY_EXPORT int PyUnicode_ClearFreeList() {
  UNIMPLEMENTED("PyUnicode_ClearFreeList");
}

PY_EXPORT int PyUnicode_Compare(PyObject* /* t */, PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_Compare");
}

PY_EXPORT int PyUnicode_CompareWithASCIIString(PyObject* /* i */,
                                               const char* /* r */) {
  UNIMPLEMENTED("PyUnicode_CompareWithASCIIString");
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

PY_EXPORT PyObject* PyUnicode_DecodeFSDefault(const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeFSDefault");
}

PY_EXPORT PyObject* PyUnicode_DecodeFSDefaultAndSize(const char* /* s */,
                                                     Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_DecodeFSDefaultAndSize");
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

PY_EXPORT PyObject* PyUnicode_FromWideChar(const wchar_t* /* u */,
                                           Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_FromWideChar");
}

PY_EXPORT const char* PyUnicode_GetDefaultEncoding() {
  UNIMPLEMENTED("PyUnicode_GetDefaultEncoding");
}

PY_EXPORT Py_ssize_t PyUnicode_GetLength(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_GetLength");
}

PY_EXPORT Py_ssize_t PyUnicode_GetSize(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_GetSize");
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

PY_EXPORT PyObject* PyUnicode_FromUnicode(const Py_UNICODE* /* u */,
                                          Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_FromUnicode");
}

}  // namespace python
