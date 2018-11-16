#include "runtime.h"

namespace python {

extern "C" PyObject* PyFile_GetLine(PyObject* /* f */, int /* n */) {
  UNIMPLEMENTED("PyFile_GetLine");
}

extern "C" int PyFile_WriteObject(PyObject* /* v */, PyObject* /* f */,
                                  int /* s */) {
  UNIMPLEMENTED("PyFile_WriteObject");
}

extern "C" int PyFile_WriteString(const char* /* s */, PyObject* /* f */) {
  UNIMPLEMENTED("PyFile_WriteString");
}

extern "C" int PyObject_AsFileDescriptor(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_AsFileDescriptor");
}

extern "C" char* Py_UniversalNewlineFgets(char* /* f */, int /* n */,
                                          FILE* /* m */, PyObject* /* j */) {
  UNIMPLEMENTED("Py_UniversalNewlineFgets");
}

}  // namespace python
