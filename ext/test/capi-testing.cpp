#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-testing.h"

namespace python {
namespace testing {

PyObject* moduleGet(const char* module, const char* name) {
  PyObject* mods = PyImport_GetModuleDict();
  PyObject* module_name = PyUnicode_FromString(module);
  PyObject* mod = PyDict_GetItem(mods, module_name);
  if (mod == nullptr) return nullptr;
  Py_DECREF(module_name);
  return PyObject_GetAttrString(mod, name);
}

int moduleSet(const char* module, const char* name, PyObject* value) {
  PyObject* mods = PyImport_GetModuleDict();
  PyObject* module_name = PyUnicode_FromString(module);
  PyObject* mod = PyDict_GetItem(mods, module_name);
  if (mod == nullptr && strcmp(module, "__main__") == 0) {
    // create __main__ if not yet available
    PyRun_SimpleString("");
    mod = PyDict_GetItem(mods, module_name);
  }
  if (mod == nullptr) return -1;
  Py_DECREF(module_name);

  PyObject* name_obj = PyUnicode_FromString(name);
  int ret = PyObject_SetAttr(mod, name_obj, value);
  Py_DECREF(name_obj);
  return ret;
}

PyObject* importGetModule(PyObject* name) {
  PyObject* modules_dict = PyImport_GetModuleDict();
  PyObject* module = PyDict_GetItem(modules_dict, name);
  Py_XINCREF(module);  // Return a new reference
  return module;
}

template <typename T>
static ::testing::AssertionResult failNullObj(const T& expected,
                                              const char* delim) {
  PyObjectPtr exception(PyErr_Occurred());
  if (exception != nullptr) {
    PyErr_Clear();
    PyObjectPtr exception_repr(PyObject_Repr(exception));
    if (exception_repr != nullptr) {
      const char* exception_cstr = PyUnicode_AsUTF8(exception_repr);
      if (exception_cstr != nullptr) {
        return ::testing::AssertionFailure()
               << "pending exception: " << exception_cstr;
      }
    }
  }
  return ::testing::AssertionFailure()
         << "nullptr is not equal to " << delim << expected << delim;
}

template <typename T>
static ::testing::AssertionResult failBadValue(PyObject* obj, const T& expected,
                                               const char* delim) {
  PyObjectPtr repr_str(PyObject_Repr(obj));
  const char* repr_cstr = nullptr;
  if (repr_str != nullptr) {
    repr_cstr = PyUnicode_AsUTF8(repr_str);
  }
  repr_cstr = repr_cstr == nullptr ? "NULL" : repr_cstr;
  return ::testing::AssertionFailure()
         << repr_cstr << " is not equal to " << delim << expected << delim;
}

::testing::AssertionResult isLongEqualsLong(PyObject* obj, long value) {
  if (obj == nullptr) return failNullObj(value, "");

  if (PyLong_Check(obj)) {
    long longval = PyLong_AsLong(obj);
    if (longval == -1 && PyErr_Occurred() != nullptr) {
      PyErr_Clear();
    } else if (longval == value) {
      return ::testing::AssertionSuccess();
    }
  }

  return failBadValue(obj, value, "");
}

::testing::AssertionResult isUnicodeEqualsCStr(PyObject* obj,
                                               const char* c_str) {
  if (obj == nullptr) return failNullObj(c_str, "'");

  if (!PyUnicode_Check(obj) ||
      PyUnicode_CompareWithASCIIString(obj, c_str) != 0) {
    return failBadValue(obj, c_str, "'");
  }
  return ::testing::AssertionSuccess();
}

CaptureStdStreams::CaptureStdStreams() {
  ::testing::internal::CaptureStdout();
  ::testing::internal::CaptureStderr();
}

CaptureStdStreams::~CaptureStdStreams() {
  // Print any unread buffers to their respective streams to assist in
  // debugging.
  if (!restored_stdout_) std::cout << out();
  if (!restored_stderr_) std::cerr << err();
}

std::string CaptureStdStreams::out() {
  assert(!restored_stdout_);
  PyObject *exc, *value, *tb;
  PyErr_Fetch(&exc, &value, &tb);
  PyRun_SimpleString(R"(
import sys
if hasattr(sys, "stdout") and hasattr(sys.stdout, "flush"):
  sys.stdout.flush()
)");
  PyErr_Restore(exc, value, tb);
  restored_stdout_ = true;
  return ::testing::internal::GetCapturedStdout();
}

std::string CaptureStdStreams::err() {
  assert(!restored_stderr_);
  PyObject *exc, *value, *tb;
  PyErr_Fetch(&exc, &value, &tb);
  PyRun_SimpleString(R"(
import sys
if hasattr(sys, "stderr") and hasattr(sys.stderr, "flush"):
  sys.stderr.flush()
)");
  PyErr_Restore(exc, value, tb);
  restored_stderr_ = true;
  return ::testing::internal::GetCapturedStderr();
}

}  // namespace testing
}  // namespace python
