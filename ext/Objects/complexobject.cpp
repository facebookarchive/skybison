#include "runtime.h"

namespace python {

typedef struct {
  double real;
  double imag;
} Py_complex;

extern "C" Py_complex PyComplex_AsCComplex(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_AsCComplex");
}

extern "C" PyObject* PyComplex_FromCComplex(Py_complex /* l */) {
  UNIMPLEMENTED("PyComplex_FromCComplex");
}

extern "C" double PyComplex_ImagAsDouble(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_ImagAsDouble");
}

extern "C" double PyComplex_RealAsDouble(PyObject* /* p */) {
  UNIMPLEMENTED("PyComplex_RealAsDouble");
}

extern "C" PyObject* PyComplex_FromDoubles(double /* l */, double /* g */) {
  UNIMPLEMENTED("PyComplex_FromDoubles");
}

}  // namespace python
