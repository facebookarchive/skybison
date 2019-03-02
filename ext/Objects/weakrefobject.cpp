#include "runtime.h"

namespace python {

PY_EXPORT void PyObject_ClearWeakRefs(PyObject* /* obj */) {
  // Do nothing and delegated to the garbage collector.
}

PY_EXPORT PyObject* PyWeakref_GetObject(PyObject* /* f */) {
  UNIMPLEMENTED("PyWeakref_GetObject");
}

PY_EXPORT PyObject* PyWeakref_NewProxy(PyObject* /* b */, PyObject* /* k */) {
  UNIMPLEMENTED("PyWeakref_NewProxy");
}

PY_EXPORT PyObject* PyWeakref_NewRef(PyObject* /* b */, PyObject* /* k */) {
  UNIMPLEMENTED("PyWeakref_NewRef");
}

}  // namespace python
