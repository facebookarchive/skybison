#pragma once

#include "cpython-types.h"
#include "objects.h"

namespace python {

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

  // Get the object from the handle's reference pointer. If non-existent
  // Either search the object in the runtime's extension types dictionary
  // or build a new extension instance.
  Object* asObject();

  PyObject* asPyObject() { return reinterpret_cast<PyObject*>(this); }

  ApiHandle* type();

  // Check if the object is subtype of a type at given layout. This is used as a
  // helper for PyXyz_Check functions
  bool isSubClass(Thread* thread, LayoutId layout_id);

  // Check if the type is PyType_Type
  bool isType();

  bool isBorrowed() { return (ob_refcnt & kBorrowedBit) != 0; }

  void setBorrowed() { ob_refcnt |= kBorrowedBit; }

  void clearBorrowed() { ob_refcnt &= ~kBorrowedBit; }

 private:
  ApiHandle(Object* reference, long refcnt);

  // Create a new runtime instance based on this ApiHandle
  Object* asInstance(Object* type);

  // Pull the ApiHandle from the instance's layout
  static ApiHandle* fromInstance(Object* obj);

  static const long kBorrowedBit = 1L << 31;

  DISALLOW_COPY_AND_ASSIGN(ApiHandle);
};

}  // namespace python
