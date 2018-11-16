#pragma once

#include "objects.h"

typedef struct _object PyObject;

namespace python {

class ApiTypeHandle;

enum class ExtensionTypes {
  kType = 0,
  kBaseObject,
  kBool,
  kDict,
  kFloat,
  kGetSetDescr,
  kList,
  kLong,
  kMemberDescr,
  kMethodDescr,
  kModule,
  kNone,
  kStr,
  kTuple,
};

// An isomorphic structure to CPython's PyObject
class ApiHandle {
 public:
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

  bool isBorrowed() { return (ob_refcnt_ & kBorrowedBit) != 0; }

  void setBorrowed() { ob_refcnt_ |= kBorrowedBit; }

  void clearBorrowed() { ob_refcnt_ &= ~kBorrowedBit; }

  PyObject* type() { return reinterpret_cast<PyObject*>(ob_type_); }

 private:
  ApiHandle() = delete;
  ApiHandle(Object* reference, long refcnt)
      : reference_(reference), ob_refcnt_(refcnt), ob_type_(nullptr) {}

  void* reference_;
  long ob_refcnt_;
  void* ob_type_;

  static const long kBorrowedBit = 1L << 31;
};

}  // namespace python
