#pragma once

#include "cpython-types.h"
#include "handles.h"
#include "objects.h"

namespace python {

class ApiHandle : public PyObject {
 public:
  // Returns an ApiHandle wrapping an object to cross the CPython boundary.  If
  // this object has not been wrapped before, a new handle is allocated with a
  // reference count of 1.  Otherwise, increments the reference count of the
  // object's existing handle.
  static ApiHandle* fromObject(RawObject obj);

  // Same as asApiHandle, but creates a borrowed ApiHandle if no handle exists
  static ApiHandle* fromBorrowedObject(RawObject obj);

  static ApiHandle* fromPyObject(PyObject* py_obj) {
    return static_cast<ApiHandle*>(py_obj);
  }

  // Get the object from the handle's reference pointer. If non-existent
  // Either search the object in the runtime's extension types dictionary
  // or build a new extension instance.
  RawObject asObject();

  ApiHandle* type();

  // Each ApiHandle can have one pointer to cached data, which will be freed
  // when the handle is destroyed.
  void* cache();
  void setCache(void* value);

  // Remove the ApiHandle from the dictionary and free its memory
  void dispose();

  // Check if the object is subtype of a type at given layout. This is used as a
  // helper for PyXyz_Check functions
  bool isSubClass(Thread* thread, LayoutId layout_id);

  // Check if the type is PyType_Type
  bool isType();

  bool isBorrowed() { return (ob_refcnt & kBorrowedBit) != 0; }

  void setBorrowed() { ob_refcnt |= kBorrowedBit; }

  void clearBorrowed() { ob_refcnt &= ~kBorrowedBit; }

  void incref() {
    DCHECK(refcnt() < kBorrowedBit - 1, "Reference count overflowed");
    ++ob_refcnt;
  }

  void decref() {
    DCHECK(refcnt() > 0, "Reference count underflowed");
    --ob_refcnt;
  }

  word refcnt() { return ob_refcnt & ~kBorrowedBit; }

  DISALLOW_IMPLICIT_CONSTRUCTORS(ApiHandle);

 private:
  static ApiHandle* create(RawObject reference, long refcnt);

  // Cast RawObject to ApiHandle* and set borrowed bit if needed
  static ApiHandle* castFromObject(RawObject value, bool borrowed);

  // Create a new runtime instance based on this ApiHandle
  RawObject asInstance(RawObject type);

  // Get ExtensionPtr attribute from obj; returns Error if not an extension
  // instance
  static RawObject getExtensionPtrAttr(Thread* thread, const Object& obj);

  static const long kBorrowedBit = 1L << 31;
};

static_assert(sizeof(ApiHandle) == sizeof(PyObject),
              "ApiHandle must not add members to PyObject");

}  // namespace python
