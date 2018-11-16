#include "runtime.h"

namespace python {

extern "C" wchar_t* Py_DecodeLocale(const char* /* g */, size_t* /* n */) {
  UNIMPLEMENTED("Py_DecodeLocale");
}

extern "C" char* Py_EncodeLocale(const wchar_t* /* t */, size_t* /* s */) {
  UNIMPLEMENTED("Py_EncodeLocale");
}

}  // namespace python
