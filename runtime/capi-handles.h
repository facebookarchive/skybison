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

class ApiHandle : public PyObject {
 public:
  // Wrap an Object as an ApiHandle to cross the CPython boundary
  // Create a new ApiHandle if there is not a pre-existing one
  static ApiHandle* fromObject(Object* obj);

  // Same as asApiHandle, but creates a borrowed ApiHandle if no handle exists
  static ApiHandle* fromBorrowedObject(Object* obj);

  // Wrap a PyObject as an ApiHandle to cross the CPython boundary
  // Cast the PyObject into an ApiHandle
  static ApiHandle* fromPyObject(PyObject* py_obj) {
    return reinterpret_cast<ApiHandle*>(py_obj);
  }

  static ApiHandle* newHandle(Object* reference) {
    return new ApiHandle(reference, 1);
  }

  static ApiHandle* newBorrowedHandle(Object* reference) {
    return new ApiHandle(reference, kBorrowedBit);
  }

  Object* asObject();

  PyObject* asPyObject() { return reinterpret_cast<PyObject*>(this); }

  bool isBorrowed() { return (ob_refcnt & kBorrowedBit) != 0; }

  void setBorrowed() { ob_refcnt |= kBorrowedBit; }

  void clearBorrowed() { ob_refcnt &= ~kBorrowedBit; }

  PyObject* type() { return reinterpret_cast<PyObject*>(ob_type); }

 private:
  ApiHandle() = delete;
  ApiHandle(Object* reference, long refcnt) {
    reference_ = reference;
    ob_refcnt = refcnt;
    ob_type = nullptr;
  }

  static const long kBorrowedBit = 1L << 31;
};

}  // namespace python
