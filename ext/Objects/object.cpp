// object.c implementation

#include "Python.h"

#include "runtime.h"

namespace python {

void PyNone_Type_Init(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  ApiTypeHandle* pytype_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType);
  ApiTypeHandle* pynone_type =
      ApiTypeHandle::newTypeHandle("NoneType", pytype_type);

  runtime->addBuiltinTypeHandle(pynone_type);
}

extern "C" PyTypeObject* PyNone_Type_Ptr() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  return static_cast<PyTypeObject*>(
      runtime->builtinTypeHandles(ExtensionTypes::kNone));
}

extern "C" PyObject* PyNone_Ptr() {
  return ApiHandle::fromObject(None::object())->asPyObject();
}

extern "C" void _Py_Dealloc(PyObject* op) {
  // TODO(T30365701): Add a deallocation strategy for ApiHandles
  if (Py_TYPE(op) && Py_TYPE(op)->tp_dealloc) {
    destructor dealloc = Py_TYPE(op)->tp_dealloc;
    _Py_ForgetReference(op);
    (*dealloc)(op);
  }
}

}  // namespace python
