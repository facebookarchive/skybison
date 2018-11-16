#include "runtime.h"

namespace python {

PY_EXPORT Py_complex PyComplex_AsCComplex(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_AsCComplex");
}

PY_EXPORT PyObject* PyComplex_FromCComplex(Py_complex /* l */) {
  UNIMPLEMENTED("PyComplex_FromCComplex");
}

PY_EXPORT double PyComplex_ImagAsDouble(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_ImagAsDouble");
}

PY_EXPORT double PyComplex_RealAsDouble(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_RealAsDouble");
}

PY_EXPORT PyObject* PyComplex_FromDoubles(double /* l */, double /* g */) {
  UNIMPLEMENTED("PyComplex_FromDoubles");
}

}  // namespace python
