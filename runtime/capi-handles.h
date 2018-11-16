#pragma once

#include "cpython-types.h"
#include "objects.h"

namespace python {

enum class ExtensionTypes {
  kType = 0,
  kBaseObject,
  kBool,
  kDict,
  kFloat,
  kList,
  kLong,
  kModule,
  kNone,
  kStr,
  kTuple,
};

class ApiTypeHandle;

class ApiHandle : public PyObject {
 public:
  // Wrap an Object as an ApiHandle to cross the CPython boundary
  // Create a new ApiHandle if there is not a pre-existing one
  static ApiHandle* fromObject(Object* obj);

  // Same as asApiHandle, but creates a borrowed ApiHandle if no handle exists
  static ApiHandle* fromBorrowedObject(Object* obj);

  static ApiHandle* fromPyObject(PyObject* py_obj) {
    return reinterpret_cast<ApiHandle*>(py_obj);
  }

  // TODO(T32685074): Handle PyTypeObject as any other ApiHandle
  static ApiHandle* fromPyTypeObject(PyTypeObject* pytype_obj) {
    return reinterpret_cast<ApiHandle*>(pytype_obj);
  }

  // Get the object from the handle's reference pointer. If non-existent
  // Either search the object in the runtime's extension types dictionary
  // or build a new extension instance.
  Object* asObject();

  PyObject* asPyObject() { return reinterpret_cast<PyObject*>(this); }

  // TODO(T32685074): Handle PyTypeObject as any other ApiHandle
  PyTypeObject* asPyTypeObject() {
    DCHECK(isType(), "ApiHandle must be type");
    return reinterpret_cast<PyTypeObject*>(this);
  }

  // TODO(T32685074): Handle PyTypeObject as any other ApiHandle
  // Use the ob_type member as an ApiTypeHandle
  ApiTypeHandle* type();

  // Check if the type is PyType_Type
  bool isType();

  bool isBorrowed() { return (ob_refcnt & kBorrowedBit) != 0; }

  void setBorrowed() { ob_refcnt |= kBorrowedBit; }

  void clearBorrowed() { ob_refcnt &= ~kBorrowedBit; }

 private:
  ApiHandle(Object* reference, long refcnt) {
    reference_ = reference;
    ob_refcnt = refcnt;
    ob_type = nullptr;
  }

  // Create a new runtime instance based on this ApiHandle
  Object* asInstance(Object* type);

  // Pull the ApiHandle from the instance's layout
  static ApiHandle* fromInstance(Object* obj);

  static const long kBorrowedBit = 1L << 31;

  DISALLOW_COPY_AND_ASSIGN(ApiHandle);
};

// TODO(T32685074): Remove class and make PyTypeObject work ApiHandle
class ApiTypeHandle : public PyTypeObject {
 public:
  // Wrap a new PyTypeObject as an ApiTypeHandle to cross the CPython boundary
  static ApiTypeHandle* newTypeHandle(const char* name, PyTypeObject* metatype);

  static ApiTypeHandle* fromPyObject(PyObject* pyobj) {
    return reinterpret_cast<ApiTypeHandle*>(pyobj);
  }

  static ApiTypeHandle* fromPyTypeObject(PyTypeObject* pytype_obj) {
    return reinterpret_cast<ApiTypeHandle*>(pytype_obj);
  }

  PyObject* asPyObject() { return reinterpret_cast<PyObject*>(this); }

  PyTypeObject* asPyTypeObject() {
    return reinterpret_cast<PyTypeObject*>(this);
  }

  // Check if the ApiTypeHandle is any of the builtin classes
  bool isBuiltin() { return tp_flags == ApiTypeHandle::kFlagsBuiltin; }

  // CPython's flags 15 and 16 are reserved for Stackless Python.
  // These flags can be safely reused
  static const int kFlagsBuiltin = 1 << 15;

 private:
  // ApiTypeHandle::fromObject should only be used through ApiHandle as this
  // does not deal with the creation of handles.
  friend ApiHandle* ApiHandle::fromObject(Object*);
  friend ApiHandle* ApiHandle::fromBorrowedObject(Object*);

  // Pull the extension type from the Class's layout
  static ApiTypeHandle* fromObject(Object* obj);

  // ApiTypeHandle::asObject should only be used through ApiHandle as this
  // does not deal with the check for the reference pointer
  friend Object* ApiHandle::asObject();

  // Get the class object stored in the runtime's extension type dictionary
  Object* asObject();

  DISALLOW_COPY_AND_ASSIGN(ApiTypeHandle);
};

}  // namespace python
