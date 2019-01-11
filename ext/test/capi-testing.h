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
  PyObjectPtr(PyObject* obj) : obj_(obj) {}

  ~PyObjectPtr() { Py_XDECREF(obj_); }

  // Release current object (decref) and take ownership of a different PyObject
  PyObjectPtr& operator=(PyObject* obj) {
    Py_XDECREF(obj_);
    obj_ = obj;
    return *this;
  };

  operator PyObject*() const { return obj_; }

  PyObject* get() const { return obj_; }

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

}  // namespace testing
}  // namespace python
