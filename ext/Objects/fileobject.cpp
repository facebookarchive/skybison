#include "runtime.h"

namespace python {

PY_EXPORT PyObject* PyFile_GetLine(PyObject* /* f */, int /* n */) {
  UNIMPLEMENTED("PyFile_GetLine");
}

PY_EXPORT int PyFile_WriteObject(PyObject* /* v */, PyObject* /* f */,
                                 int /* s */) {
  UNIMPLEMENTED("PyFile_WriteObject");
}

PY_EXPORT int PyFile_WriteString(const char* /* s */, PyObject* /* f */) {
  UNIMPLEMENTED("PyFile_WriteString");
}

PY_EXPORT int PyObject_AsFileDescriptor(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_AsFileDescriptor");
}

PY_EXPORT char* Py_UniversalNewlineFgets(char* /* f */, int /* n */,
                                         FILE* /* m */, PyObject* /* j */) {
  UNIMPLEMENTED("Py_UniversalNewlineFgets");
}

}  // namespace python
