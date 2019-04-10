#include "cpython-func.h"
#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyCodec_BackslashReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_BackslashReplaceErrors");
}

PY_EXPORT PyObject* PyCodec_Decode(PyObject* /* t */, const char* /* g */,
                                   const char* /* s */) {
  UNIMPLEMENTED("PyCodec_Decode");
}

PY_EXPORT PyObject* PyCodec_Decoder(const char* /* g */) {
  UNIMPLEMENTED("PyCodec_Decoder");
}

PY_EXPORT PyObject* PyCodec_Encode(PyObject* /* t */, const char* /* g */,
                                   const char* /* s */) {
  UNIMPLEMENTED("PyCodec_Encode");
}

PY_EXPORT PyObject* PyCodec_Encoder(const char* /* g */) {
  UNIMPLEMENTED("PyCodec_Encoder");
}

PY_EXPORT PyObject* PyCodec_IgnoreErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_IgnoreErrors");
}

static PyObject* makeIncrementalCodec(PyObject* codec_info, const char* errors,
                                      const char* attrname) {
  PyObject* inc_codec = PyObject_GetAttrString(codec_info, attrname);
  if (inc_codec == nullptr) return nullptr;
  PyObject* ret = nullptr;
  if (errors) {
    ret = PyObject_CallFunction(inc_codec, "s", errors);
  } else {
    ret = PyObject_CallFunction(inc_codec, nullptr);
  }
  Py_DECREF(inc_codec);
  return ret;
}

PY_EXPORT PyObject* _PyCodecInfo_GetIncrementalDecoder(PyObject* codec_info,
                                                       const char* errors) {
  return makeIncrementalCodec(codec_info, errors, "incrementaldecoder");
}

PY_EXPORT PyObject* PyCodec_IncrementalDecoder(const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyCodec_IncrementalDecoder");
}

PY_EXPORT PyObject* _PyCodecInfo_GetIncrementalEncoder(PyObject* codec_info,
                                                       const char* errors) {
  return makeIncrementalCodec(codec_info, errors, "incrementalencoder");
}

PY_EXPORT PyObject* PyCodec_IncrementalEncoder(const char* /* g */,
                                               const char* /* s */) {
  UNIMPLEMENTED("PyCodec_IncrementalEncoder");
}

PY_EXPORT int PyCodec_KnownEncoding(const char* /* g */) {
  UNIMPLEMENTED("PyCodec_KnownEncoding");
}

PY_EXPORT PyObject* _PyCodec_LookupTextEncoding(const char* encoding,
                                                const char* alternate_command) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Str encoding_str(&scope, runtime->newStrFromCStr(encoding));
  Str alt_command(&scope, runtime->newStrFromCStr(alternate_command));
  Object result(&scope, thread->invokeFunction2(SymbolId::kUnderCodecs,
                                                SymbolId::kUnderLookupText,
                                                encoding_str, alt_command));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseSystemErrorWithCStr("could not call _codecs.lookup");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyCodec_LookupError(const char* /* e */) {
  UNIMPLEMENTED("PyCodec_LookupError");
}

PY_EXPORT PyObject* PyCodec_NameReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_NameReplaceErrors");
}

PY_EXPORT int PyCodec_Register(PyObject* /* n */) {
  UNIMPLEMENTED("PyCodec_Register");
}

PY_EXPORT int PyCodec_RegisterError(const char* /* e */, PyObject* /* r */) {
  UNIMPLEMENTED("PyCodec_RegisterError");
}

PY_EXPORT PyObject* PyCodec_ReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_ReplaceErrors");
}

PY_EXPORT PyObject* PyCodec_StreamReader(const char* /* g */, PyObject* /* m */,
                                         const char* /* s */) {
  UNIMPLEMENTED("PyCodec_StreamReader");
}

PY_EXPORT PyObject* PyCodec_StreamWriter(const char* /* g */, PyObject* /* m */,
                                         const char* /* s */) {
  UNIMPLEMENTED("PyCodec_StreamWriter");
}

PY_EXPORT PyObject* PyCodec_StrictErrors(PyObject* exc) {
  DCHECK(exc != nullptr, "exception should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object exc_obj(&scope, ApiHandle::fromPyObject(exc)->asObject());
  Object result(&scope,
                thread->invokeFunction1(SymbolId::kUnderCodecs,
                                        SymbolId::kStrictErrors, exc_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call _codecs.strict_errors");
    }
  }
  return nullptr;
}

PY_EXPORT PyObject* PyCodec_XMLCharRefReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_XMLCharRefReplaceErrors");
}

}  // namespace python
