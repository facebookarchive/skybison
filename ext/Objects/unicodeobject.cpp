// unicodeobject.c implementation

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

typedef int Py_UCS4;
typedef wchar_t Py_UNICODE;

extern "C" int PyUnicode_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isStr();
}

extern "C" int PyUnicode_Check_Func(PyObject* obj) {
  if (PyUnicode_CheckExact_Func(obj)) {
    return true;
  }
  return ApiHandle::fromPyObject(obj)->isSubClass(Thread::currentThread(),
                                                  LayoutId::kStr);
}

extern "C" PyObject* PyUnicode_FromString(const char* c_string) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> value(&scope, runtime->newStrFromCStr(c_string));
  return ApiHandle::fromObject(*value);
}

extern "C" char* PyUnicode_AsUTF8AndSize(PyObject* pyunicode,
                                         Py_ssize_t* size) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pyunicode == nullptr) {
    UNIMPLEMENTED("PyErr_BadArgument");
    return nullptr;
  }

  Handle<Object> str_obj(&scope,
                         ApiHandle::fromPyObject(pyunicode)->asObject());
  if (!str_obj->isStr()) {
    return nullptr;
  }

  Handle<Str> str(&scope, *str_obj);
  word length = str->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  str->copyTo(result, length);
  result[length] = '\0';
  if (size) {
    *size = length;
  }
  return reinterpret_cast<char*>(result);
}

extern "C" PyObject* PyUnicode_FromStringAndSize(const char*, Py_ssize_t) {
  UNIMPLEMENTED("PyUnicode_FromStringAndSize");
}

extern "C" PyObject* PyUnicode_EncodeFSDefault(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_EncodeFSDefault");
}

extern "C" PyObject* PyUnicode_New(Py_ssize_t /* e */, Py_UCS4 /* r */) {
  UNIMPLEMENTED("PyUnicode_New");
}

extern "C" void PyUnicode_Append(PyObject** /* p_left */, PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_Append");
}

extern "C" void PyUnicode_AppendAndDel(PyObject** /* pleft */,
                                       PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_AppendAndDel");
}

extern "C" PyObject* PyUnicode_AsASCIIString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsASCIIString");
}

extern "C" PyObject* PyUnicode_AsCharmapString(PyObject* /* e */,
                                               PyObject* /* g */) {
  UNIMPLEMENTED("PyUnicode_AsCharmapString");
}

extern "C" PyObject* PyUnicode_AsDecodedObject(PyObject* /* e */,
                                               const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsDecodedObject");
}

extern "C" PyObject* PyUnicode_AsDecodedUnicode(PyObject* /* e */,
                                                const char* /* g */,
                                                const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsDecodedUnicode");
}

extern "C" PyObject* PyUnicode_AsEncodedObject(PyObject* /* e */,
                                               const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsEncodedObject");
}

extern "C" PyObject* PyUnicode_AsEncodedString(PyObject* /* e */,
                                               const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsEncodedString");
}

extern "C" PyObject* PyUnicode_AsEncodedUnicode(PyObject* /* e */,
                                                const char* /* g */,
                                                const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_AsEncodedUnicode");
}

extern "C" PyObject* PyUnicode_AsLatin1String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsLatin1String");
}

extern "C" PyObject* PyUnicode_AsMBCSString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsMBCSString");
}

extern "C" PyObject* PyUnicode_AsRawUnicodeEscapeString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsRawUnicodeEscapeString");
}

extern "C" Py_UCS4* PyUnicode_AsUCS4(PyObject* /* g */, Py_UCS4* /* t */,
                                     Py_ssize_t /* e */, int /* l */) {
  UNIMPLEMENTED("PyUnicode_AsUCS4");
}

extern "C" Py_UCS4* PyUnicode_AsUCS4Copy(PyObject* /* g */) {
  UNIMPLEMENTED("PyUnicode_AsUCS4Copy");
}

extern "C" PyObject* PyUnicode_AsUTF16String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUTF16String");
}

extern "C" PyObject* PyUnicode_AsUTF32String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUTF32String");
}

extern "C" PyObject* PyUnicode_AsUTF8String(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUTF8String");
}

extern "C" PyObject* PyUnicode_AsUnicodeEscapeString(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUnicodeEscapeString");
}

extern "C" Py_ssize_t PyUnicode_AsWideChar(PyObject* /* e */, wchar_t* /* w */,
                                           Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_AsWideChar");
}

extern "C" wchar_t* PyUnicode_AsWideCharString(PyObject* /* e */,
                                               Py_ssize_t* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsWideCharString");
}

extern "C" PyObject* PyUnicode_BuildEncodingMap(PyObject* /* g */) {
  UNIMPLEMENTED("PyUnicode_BuildEncodingMap");
}

extern "C" int PyUnicode_ClearFreeList(void) {
  UNIMPLEMENTED("PyUnicode_ClearFreeList");
}

extern "C" int PyUnicode_Compare(PyObject* /* t */, PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_Compare");
}

extern "C" int PyUnicode_CompareWithASCIIString(PyObject* /* i */,
                                                const char* /* r */) {
  UNIMPLEMENTED("PyUnicode_CompareWithASCIIString");
}

extern "C" PyObject* PyUnicode_Concat(PyObject* /* t */, PyObject* /* t */) {
  UNIMPLEMENTED("PyUnicode_Concat");
}

extern "C" int PyUnicode_Contains(PyObject* /* r */, PyObject* /* r */) {
  UNIMPLEMENTED("PyUnicode_Contains");
}

extern "C" Py_ssize_t PyUnicode_Count(PyObject* /* r */, PyObject* /* r */,
                                      Py_ssize_t /* t */, Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicode_Count");
}

extern "C" PyObject* PyUnicode_Decode(const char* /* s */, Py_ssize_t /* e */,
                                      const char* /* g */,
                                      const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_Decode");
}

extern "C" PyObject* PyUnicode_DecodeASCII(const char* /* s */,
                                           Py_ssize_t /* e */,
                                           const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeASCII");
}

extern "C" PyObject* PyUnicode_DecodeCharmap(const char* /* s */,
                                             Py_ssize_t /* e */,
                                             PyObject* /* g */,
                                             const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeCharmap");
}

extern "C" PyObject* PyUnicode_DecodeCodePageStateful(int /* e */,
                                                      const char* /* s */,
                                                      Py_ssize_t /* e */,
                                                      const char* /* s */,
                                                      Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeCodePageStateful");
}

extern "C" PyObject* PyUnicode_DecodeFSDefault(const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeFSDefault");
}

extern "C" PyObject* PyUnicode_DecodeFSDefaultAndSize(const char* /* s */,
                                                      Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_DecodeFSDefaultAndSize");
}

extern "C" PyObject* PyUnicode_DecodeLatin1(const char* /* s */,
                                            Py_ssize_t /* e */,
                                            const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeLatin1");
}

extern "C" PyObject* PyUnicode_DecodeLocale(const char* /* r */,
                                            const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeLocale");
}

extern "C" PyObject* PyUnicode_DecodeLocaleAndSize(const char* /* r */,
                                                   Py_ssize_t /* n */,
                                                   const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeLocaleAndSize");
}

extern "C" PyObject* PyUnicode_DecodeMBCS(const char* /* s */,
                                          Py_ssize_t /* e */,
                                          const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeMBCS");
}

extern "C" PyObject* PyUnicode_DecodeMBCSStateful(const char* /* s */,
                                                  Py_ssize_t /* e */,
                                                  const char* /* s */,
                                                  Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeMBCSStateful");
}

extern "C" PyObject* PyUnicode_DecodeRawUnicodeEscape(const char* /* s */,
                                                      Py_ssize_t /* e */,
                                                      const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeRawUnicodeEscape");
}

extern "C" PyObject* PyUnicode_DecodeUTF16(const char* /* s */,
                                           Py_ssize_t /* e */,
                                           const char* /* s */, int* /* r */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF16");
}

extern "C" PyObject* PyUnicode_DecodeUTF16Stateful(const char* /* s */,
                                                   Py_ssize_t /* e */,
                                                   const char* /* s */,
                                                   int* /* r */,
                                                   Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF16Stateful");
}

extern "C" PyObject* PyUnicode_DecodeUTF32(const char* /* s */,
                                           Py_ssize_t /* e */,
                                           const char* /* s */, int* /* r */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF32");
}

extern "C" PyObject* PyUnicode_DecodeUTF32Stateful(const char* /* s */,
                                                   Py_ssize_t /* e */,
                                                   const char* /* s */,
                                                   int* /* r */,
                                                   Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF32Stateful");
}

extern "C" PyObject* PyUnicode_DecodeUTF7(const char* /* s */,
                                          Py_ssize_t /* e */,
                                          const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF7");
}

extern "C" PyObject* PyUnicode_DecodeUTF7Stateful(const char* /* s */,
                                                  Py_ssize_t /* e */,
                                                  const char* /* s */,
                                                  Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF7Stateful");
}

extern "C" PyObject* PyUnicode_DecodeUTF8(const char* /* s */,
                                          Py_ssize_t /* e */,
                                          const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF8");
}

extern "C" PyObject* PyUnicode_DecodeUTF8Stateful(const char* /* s */,
                                                  Py_ssize_t /* e */,
                                                  const char* /* s */,
                                                  Py_ssize_t* /* d */) {
  UNIMPLEMENTED("PyUnicode_DecodeUTF8Stateful");
}

extern "C" PyObject* PyUnicode_DecodeUnicodeEscape(const char* /* s */,
                                                   Py_ssize_t /* e */,
                                                   const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_DecodeUnicodeEscape");
}

extern "C" PyObject* PyUnicode_EncodeCodePage(int /* e */, PyObject* /* e */,
                                              const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_EncodeCodePage");
}

extern "C" PyObject* PyUnicode_EncodeLocale(PyObject* /* e */,
                                            const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_EncodeLocale");
}

extern "C" int PyUnicode_FSConverter(PyObject* /* g */, void* /* r */) {
  UNIMPLEMENTED("PyUnicode_FSConverter");
}

extern "C" int PyUnicode_FSDecoder(PyObject* /* g */, void* /* r */) {
  UNIMPLEMENTED("PyUnicode_FSDecoder");
}

extern "C" Py_ssize_t PyUnicode_Find(PyObject* /* r */, PyObject* /* r */,
                                     Py_ssize_t /* t */, Py_ssize_t /* d */,
                                     int /* n */) {
  UNIMPLEMENTED("PyUnicode_Find");
}

extern "C" Py_ssize_t PyUnicode_FindChar(PyObject* /* r */, Py_UCS4 /* h */,
                                         Py_ssize_t /* t */, Py_ssize_t /* d */,
                                         int /* n */) {
  UNIMPLEMENTED("PyUnicode_FindChar");
}

extern "C" PyObject* PyUnicode_Format(PyObject* /* t */, PyObject* /* s */) {
  UNIMPLEMENTED("PyUnicode_Format");
}

extern "C" PyObject* PyUnicode_FromEncodedObject(PyObject* /* j */,
                                                 const char* /* g */,
                                                 const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_FromEncodedObject");
}

extern "C" PyObject* PyUnicode_FromFormat(const char* /* t */, ...) {
  UNIMPLEMENTED("PyUnicode_FromFormat");
}

extern "C" PyObject* PyUnicode_FromFormatV(const char* /* t */,
                                           va_list /* s */) {
  UNIMPLEMENTED("PyUnicode_FromFormatV");
}

extern "C" PyObject* PyUnicode_FromObject(PyObject* /* j */) {
  UNIMPLEMENTED("PyUnicode_FromObject");
}

extern "C" PyObject* PyUnicode_FromOrdinal(int /* l */) {
  UNIMPLEMENTED("PyUnicode_FromOrdinal");
}

extern "C" PyObject* PyUnicode_FromWideChar(const wchar_t* /* u */,
                                            Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_FromWideChar");
}

extern "C" const char* PyUnicode_GetDefaultEncoding(void) {
  UNIMPLEMENTED("PyUnicode_GetDefaultEncoding");
}

extern "C" Py_ssize_t PyUnicode_GetLength(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_GetLength");
}

extern "C" Py_ssize_t PyUnicode_GetSize(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_GetSize");
}

extern "C" PyObject* PyUnicode_InternFromString(const char* /* p */) {
  UNIMPLEMENTED("PyUnicode_InternFromString");
}

extern "C" void PyUnicode_InternImmortal(PyObject** /* p */) {
  UNIMPLEMENTED("PyUnicode_InternImmortal");
}

extern "C" void PyUnicode_InternInPlace(PyObject** /* p */) {
  UNIMPLEMENTED("PyUnicode_InternInPlace");
}

extern "C" int PyUnicode_IsIdentifier(PyObject* /* f */) {
  UNIMPLEMENTED("PyUnicode_IsIdentifier");
}

extern "C" PyObject* PyUnicode_Join(PyObject* /* r */, PyObject* /* q */) {
  UNIMPLEMENTED("PyUnicode_Join");
}

extern "C" PyObject* PyUnicode_Partition(PyObject* /* j */, PyObject* /* j */) {
  UNIMPLEMENTED("PyUnicode_Partition");
}

extern "C" PyObject* PyUnicode_RPartition(PyObject* /* j */,
                                          PyObject* /* j */) {
  UNIMPLEMENTED("PyUnicode_RPartition");
}

extern "C" PyObject* PyUnicode_RSplit(PyObject* /* s */, PyObject* /* p */,
                                      Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicode_RSplit");
}

extern "C" Py_UCS4 PyUnicode_ReadChar(PyObject* /* e */, Py_ssize_t /* x */) {
  UNIMPLEMENTED("PyUnicode_ReadChar");
}

extern "C" PyObject* PyUnicode_Replace(PyObject* /* r */, PyObject* /* r */,
                                       PyObject* /* r */, Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicode_Replace");
}

extern "C" int PyUnicode_Resize(PyObject** /* p_unicode */,
                                Py_ssize_t /* h */) {
  UNIMPLEMENTED("PyUnicode_Resize");
}

extern "C" PyObject* PyUnicode_RichCompare(PyObject* /* t */, PyObject* /* t */,
                                           int /* p */) {
  UNIMPLEMENTED("PyUnicode_RichCompare");
}

extern "C" PyObject* PyUnicode_Split(PyObject* /* s */, PyObject* /* p */,
                                     Py_ssize_t /* t */) {
  UNIMPLEMENTED("PyUnicode_Split");
}

extern "C" PyObject* PyUnicode_Splitlines(PyObject* /* g */, int /* s */) {
  UNIMPLEMENTED("PyUnicode_Splitlines");
}

extern "C" PyObject* PyUnicode_Substring(PyObject* /* f */, Py_ssize_t /* t */,
                                         Py_ssize_t /* d */) {
  UNIMPLEMENTED("PyUnicode_Substring");
}

extern "C" Py_ssize_t PyUnicode_Tailmatch(PyObject* /* r */, PyObject* /* r */,
                                          Py_ssize_t /* t */,
                                          Py_ssize_t /* d */, int /* n */) {
  UNIMPLEMENTED("PyUnicode_Tailmatch");
}

extern "C" PyObject* PyUnicode_Translate(PyObject* /* r */, PyObject* /* g */,
                                         const char* /* s */) {
  UNIMPLEMENTED("PyUnicode_Translate");
}

extern "C" int PyUnicode_WriteChar(PyObject* /* e */, Py_ssize_t /* x */,
                                   Py_UCS4 /* h */) {
  UNIMPLEMENTED("PyUnicode_WriteChar");
}

extern "C" const char* PyUnicode_AsUTF8(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUTF8");
}

extern "C" Py_UNICODE* PyUnicode_AsUnicode(PyObject* /* e */) {
  UNIMPLEMENTED("PyUnicode_AsUnicode");
}

extern "C" PyObject* PyUnicode_FromKindAndData(int /* d */, const void* /* r */,
                                               Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_FromKindAndData");
}

extern "C" PyObject* PyUnicode_FromUnicode(const Py_UNICODE* /* u */,
                                           Py_ssize_t /* e */) {
  UNIMPLEMENTED("PyUnicode_FromUnicode");
}

}  // namespace python
