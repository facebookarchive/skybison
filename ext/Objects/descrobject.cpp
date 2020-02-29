#include "cpython-data.h"
#include "cpython-func.h"

#include "capi-handles.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "runtime.h"

namespace py {

PY_EXPORT PyTypeObject* PyDictProxy_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kMappingProxy)));
}

RawObject newClassMethod(Thread* thread, PyMethodDef* method,
                         const Object& name, const Object& type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, runtime->newExtensionFunction(
                                thread, name, bit_cast<void*>(method->ml_meth),
                                methodTypeFromMethodFlags(method->ml_flags)));
  if (method->ml_doc != nullptr) {
    function.setDoc(runtime->newStrFromCStr(method->ml_doc));
  }
  Object result(
      &scope, thread->invokeFunction2(ID(builtins), ID(_descrclassmethod), type,
                                      function));
  DCHECK(!result.isError(), "failed to create classmethod descriptor");
  return *result;
}

PY_EXPORT PyObject* PyDescr_NewClassMethod(PyTypeObject* type,
                                           PyMethodDef* method) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, method->ml_name));
  Object type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  return ApiHandle::newReference(
      thread, newClassMethod(thread, method, name, type_obj));
}

PY_EXPORT PyObject* PyDictProxy_New(PyObject* mapping) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object mapping_obj(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  Object result(&scope, thread->invokeFunction1(ID(builtins), ID(mappingproxy),
                                                mapping_obj));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyDescr_NewGetSet(PyTypeObject* /* e */,
                                      PyGetSetDef* /* t */) {
  UNIMPLEMENTED("PyDescr_NewGetSet");
}

PY_EXPORT PyObject* PyDescr_NewMember(PyTypeObject* /* e */,
                                      PyMemberDef* /* r */) {
  UNIMPLEMENTED("PyDescr_NewMember");
}

RawObject newMethod(Thread* thread, PyMethodDef* method, const Object& name,
                    const Object& /*type*/) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, runtime->newExtensionFunction(
                                thread, name, bit_cast<void*>(method->ml_meth),
                                methodTypeFromMethodFlags(method->ml_flags)));
  if (method->ml_doc != nullptr) {
    function.setDoc(runtime->newStrFromCStr(method->ml_doc));
  }
  // TODO(T62932301): We currently return the plain function here which means
  // we do not check the `self` parameter to be a proper subtype of `type`.
  // Should we wrap this with a new descriptor type?
  return *function;
}

PY_EXPORT PyObject* PyDescr_NewMethod(PyTypeObject* type, PyMethodDef* method) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, method->ml_name));
  Object type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  return ApiHandle::newReference(thread,
                                 newMethod(thread, method, name, type_obj));
}

PY_EXPORT PyTypeObject* PyProperty_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kProperty)));
}

PY_EXPORT PyObject* PyWrapper_New(PyObject* /* d */, PyObject* /* f */) {
  UNIMPLEMENTED("PyWrapper_New");
}

}  // namespace py
