#include "cpython-data.h"
#include "cpython-func.h"

#include "api-handle.h"
#include "builtins-module.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "runtime.h"
#include "type-utils.h"

namespace py {

PY_EXPORT PyTypeObject* PyDictProxy_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kMappingProxy)));
}

PY_EXPORT PyObject* PyDescr_NAME_Func(PyObject* obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object descr_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Runtime* runtime = thread->runtime();
  if (descr_obj.isFunction()) {
    // Method
    Function descr(&scope, *descr_obj);
    return ApiHandle::borrowedReference(runtime, descr.name());
  }
  if (descr_obj.isProperty()) {
    // GetSet
    Property descr(&scope, *descr_obj);
    Object getter(&scope, descr.getter());
    if (getter.isFunction()) {
      Function func(&scope, *getter);
      return ApiHandle::borrowedReference(runtime, func.name());
    }
    Object setter(&scope, descr.setter());
    if (setter.isFunction()) {
      Function func(&scope, *setter);
      return ApiHandle::borrowedReference(runtime, func.name());
    }
    Object deleter(&scope, descr.deleter());
    if (deleter.isFunction()) {
      Function func(&scope, *deleter);
      return ApiHandle::borrowedReference(runtime, func.name());
    }
    UNIMPLEMENTED("property without getter or setter");
  }
  // ClassMethod
  if (DCHECK_IS_ON()) {
    Object descrclassmethod(&scope,
                            runtime->lookupNameInModule(thread, ID(builtins),
                                                        ID(_descrclassmethod)));
    DCHECK(descrclassmethod.isType(), "missing _descrclassmethod");
    DCHECK(runtime->typeOf(*descr_obj) == descrclassmethod,
           "unexpected type in PyDescr_NAME");
  }
  Object fn(&scope, SmallStr::fromCStr("fn"));
  Function func(&scope, getAttribute(thread, descr_obj, fn));
  return ApiHandle::borrowedReference(runtime, func.name());
}

PY_EXPORT PyObject* PyDescr_NewClassMethod(PyTypeObject* type,
                                           PyMethodDef* method) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, method->ml_name));
  Object type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  return ApiHandle::newReferenceWithManaged(
      thread->runtime(), newClassMethod(thread, method, name, type_obj));
}

PY_EXPORT PyObject* PyDictProxy_New(PyObject* mapping) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object mapping_obj(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  Object result(&scope, thread->invokeFunction1(ID(builtins), ID(mappingproxy),
                                                mapping_obj));
  if (result.isError()) return nullptr;
  return ApiHandle::newReferenceWithManaged(thread->runtime(), *result);
}

PY_EXPORT PyObject* PyDescr_NewGetSet(PyTypeObject*, PyGetSetDef* def) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, def->name));
  return ApiHandle::newReferenceWithManaged(thread->runtime(),
                                            newGetSet(thread, name, def));
}

PY_EXPORT PyObject* PyDescr_NewMember(PyTypeObject* /* e */,
                                      PyMemberDef* /* r */) {
  UNIMPLEMENTED("PyDescr_NewMember");
}

PY_EXPORT PyObject* PyDescr_NewMethod(PyTypeObject* type, PyMethodDef* method) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Runtime::internStrFromCStr(thread, method->ml_name));
  Object type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  return ApiHandle::newReferenceWithManaged(
      thread->runtime(), newMethod(thread, method, name, type_obj));
}

PY_EXPORT PyTypeObject* PyProperty_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      runtime, runtime->typeAt(LayoutId::kProperty)));
}

PY_EXPORT PyObject* PyWrapper_New(PyObject* /* d */, PyObject* /* f */) {
  UNIMPLEMENTED("PyWrapper_New");
}

}  // namespace py
