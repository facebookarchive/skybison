#include "runtime.h"

namespace python {

extern "C" PyObject* PyCodec_BackslashReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_BackslashReplaceErrors");
}

extern "C" PyObject* PyCodec_Decode(PyObject* /* t */, const char* /* g */,
                                    const char* /* s */) {
  UNIMPLEMENTED("PyCodec_Decode");
}

extern "C" PyObject* PyCodec_Decoder(const char* /* g */) {
  UNIMPLEMENTED("PyCodec_Decoder");
}

extern "C" PyObject* PyCodec_Encode(PyObject* /* t */, const char* /* g */,
                                    const char* /* s */) {
  UNIMPLEMENTED("PyCodec_Encode");
}

extern "C" PyObject* PyCodec_Encoder(const char* /* g */) {
  UNIMPLEMENTED("PyCodec_Encoder");
}

extern "C" PyObject* PyCodec_IgnoreErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_IgnoreErrors");
}

extern "C" PyObject* PyCodec_IncrementalDecoder(const char* /* g */,
                                                const char* /* s */) {
  UNIMPLEMENTED("PyCodec_IncrementalDecoder");
}

extern "C" PyObject* PyCodec_IncrementalEncoder(const char* /* g */,
                                                const char* /* s */) {
  UNIMPLEMENTED("PyCodec_IncrementalEncoder");
}

extern "C" int PyCodec_KnownEncoding(const char* /* g */) {
  UNIMPLEMENTED("PyCodec_KnownEncoding");
}

extern "C" PyObject* PyCodec_LookupError(const char* /* e */) {
  UNIMPLEMENTED("PyCodec_LookupError");
}

extern "C" PyObject* PyCodec_NameReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_NameReplaceErrors");
}

extern "C" int PyCodec_Register(PyObject* /* n */) {
  UNIMPLEMENTED("PyCodec_Register");
}

extern "C" int PyCodec_RegisterError(const char* /* e */, PyObject* /* r */) {
  UNIMPLEMENTED("PyCodec_RegisterError");
}

extern "C" PyObject* PyCodec_ReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_ReplaceErrors");
}

extern "C" PyObject* PyCodec_StreamReader(const char* /* g */,
                                          PyObject* /* m */,
                                          const char* /* s */) {
  UNIMPLEMENTED("PyCodec_StreamReader");
}

extern "C" PyObject* PyCodec_StreamWriter(const char* /* g */,
                                          PyObject* /* m */,
                                          const char* /* s */) {
  UNIMPLEMENTED("PyCodec_StreamWriter");
}

extern "C" PyObject* PyCodec_StrictErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_StrictErrors");
}

extern "C" PyObject* PyCodec_XMLCharRefReplaceErrors(PyObject* /* c */) {
  UNIMPLEMENTED("PyCodec_XMLCharRefReplaceErrors");
}

}  // namespace python
