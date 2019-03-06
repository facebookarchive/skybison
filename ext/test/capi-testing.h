#pragma once

#include "Python.h"

namespace python {
namespace testing {

// Holder for a reference to PyObject - reference count is decremented when
// the object goes out of scope or is assigned another pointer. PyObjectPtr
// always takes the ownership of the reference, so do not use this for
// borrowed references. This never increments the reference count.
class PyObjectPtr {
 public:
  // PyObjectPtr can only hold a reference for opaque types that are upcastable
  // to PyObject. Do not use with fully defined types (i.e. PyLong_Type).
  explicit PyObjectPtr(PyObject* obj) : obj_(obj) {}
  explicit PyObjectPtr(PyTypeObject* obj)
      : obj_(reinterpret_cast<PyObject*>(obj)) {}

  ~PyObjectPtr() { Py_XDECREF(obj_); }

  // Release current object (decref) and take ownership of a different PyObject
  PyObjectPtr& operator=(PyObject* obj) {
    Py_XDECREF(obj_);
    obj_ = obj;
    return *this;
  };

  operator PyObject*() const { return obj_; }

  PyObject* get() const { return obj_; }

  PyLongObject* asLongObject() const {
    // Only downcast to PyLongObject it it's holding a long reference
    assert(PyLong_Check(obj_));
    return reinterpret_cast<PyLongObject*>(obj_);
  }

  PyTypeObject* asTypeObject() const {
    // Only downcast to PyTypeObject it it's holding a type reference
    assert(PyType_Check(obj_));
    return reinterpret_cast<PyTypeObject*>(obj_);
  }

  // disable copy and assignment
  PyObjectPtr(const PyObjectPtr&) = delete;
  PyObjectPtr& operator=(PyObjectPtr const&) = delete;

 private:
  PyObject* obj_ = nullptr;
};

PyObject* moduleGet(const char* module, const char* name);
int moduleSet(const char* module, const char* name, PyObject*);

// Returns a new reference to the already imported module with the given name.
// If no module is found, return a nullptr
PyObject* importGetModule(PyObject* name);

::testing::AssertionResult isUnicodeEqualsCStr(PyObject* obj, const char* str);

}  // namespace testing
}  // namespace python
