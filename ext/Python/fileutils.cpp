#include "runtime.h"

namespace python {

PY_EXPORT wchar_t* Py_DecodeLocale(const char* /* g */, size_t* /* n */) {
  UNIMPLEMENTED("Py_DecodeLocale");
}

PY_EXPORT char* Py_EncodeLocale(const wchar_t* /* t */, size_t* /* s */) {
  UNIMPLEMENTED("Py_EncodeLocale");
}

}  // namespace python
