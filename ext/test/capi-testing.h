#pragma once

#include "Python.h"
#include "gtest/gtest.h"

namespace py {
namespace testing {

// Holder for a reference to PyObject - reference count is decremented when
// the object goes out of scope or is assigned another pointer. PyObjectPtr
// always takes the ownership of the reference, so do not use this for
// borrowed references. This never increments the reference count.
class __attribute__((warn_unused)) PyObjectPtr {
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
  }

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

PyObject* mainModuleGet(const char* name);
PyObject* moduleGet(const char* module, const char* name);
int moduleSet(const char* module, const char* name, PyObject*);

// Returns a new reference to the already imported module with the given name.
// If no module is found, return a nullptr
PyObject* importGetModule(PyObject* name);

::testing::AssertionResult isBytesEqualsCStr(PyObject* obj, const char* c_str);

::testing::AssertionResult isLongEqualsLong(PyObject* obj, long value);

::testing::AssertionResult isUnicodeEqualsCStr(PyObject* obj,
                                               const char* c_str);

// Capture stdout and stderr of the current process. The contents of either one
// may be fetched with the corresponding functions, which should be called at
// most once each. The destructor ensures that the previous stdout/stderr are
// restored even if they aren't fetched by the user.
//
// TODO(T41323917): Once we have proper streams support, this class should
// modify sys.stdout/sys.stderr to write to in-memory buffers rather than
// redirecting the C-level files.
class CaptureStdStreams {
 public:
  CaptureStdStreams();
  ~CaptureStdStreams();

  // Return the captured stdout and restore the previous stream.
  std::string out();

  // Return the captured stderr and restore the previous stream.
  std::string err();

 private:
  bool restored_stdout_{false};
  bool restored_stderr_{false};
};

// Creates a temporary directory and cleans it up when the object is destroyed.
// TODO(tylerk): T57732104 Hoist this into the test harness (along with similar
// functionality in test-utils.h)
class TempDirectory {
 public:
  TempDirectory();
  TempDirectory(const char* prefix);
  ~TempDirectory();

  std::string path;
};

}  // namespace testing
}  // namespace py
