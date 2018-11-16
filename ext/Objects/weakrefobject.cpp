#include "runtime.h"

namespace python {

extern "C" void PyObject_ClearWeakRefs(PyObject* /* t */) {
  UNIMPLEMENTED("PyObject_ClearWeakRefs");
}

extern "C" PyObject* PyWeakref_GetObject(PyObject* /* f */) {
  UNIMPLEMENTED("PyWeakref_GetObject");
}

extern "C" PyObject* PyWeakref_NewProxy(PyObject* /* b */, PyObject* /* k */) {
  UNIMPLEMENTED("PyWeakref_NewProxy");
}

extern "C" PyObject* PyWeakref_NewRef(PyObject* /* b */, PyObject* /* k */) {
  UNIMPLEMENTED("PyWeakref_NewRef");
}

}  // namespace python
