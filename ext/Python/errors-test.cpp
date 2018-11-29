#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ErrorsExtensionApiTest = ExtensionApi;

TEST_F(ErrorsExtensionApiTest, CompareErrorMessageOnThread) {
  ASSERT_EQ(nullptr, PyErr_Occurred());

  PyErr_SetString(PyExc_Exception, "An exception occured");
  ASSERT_EQ(PyExc_Exception, PyErr_Occurred());
}

TEST_F(ErrorsExtensionApiTest, SetObjectSetsTypeAndValue) {
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, nullptr);
  EXPECT_EQ(value, nullptr);
  EXPECT_EQ(traceback, nullptr);

  PyErr_SetObject(PyExc_Exception, Py_True);
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_Exception);
  EXPECT_EQ(value, Py_True);
  EXPECT_EQ(traceback, nullptr);
}

TEST_F(ErrorsExtensionApiTest, ClearClearsExceptionState) {
  // Set the exception state
  Py_INCREF(PyExc_Exception);
  Py_INCREF(Py_True);
  PyErr_Restore(PyExc_Exception, Py_True, nullptr);

  // Check that an exception is pending
  EXPECT_EQ(PyErr_Occurred(), PyExc_Exception);

  // Clear the exception
  PyErr_Clear();

  // Read the exception state again and check for null
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, nullptr);
  EXPECT_EQ(value, nullptr);
  EXPECT_EQ(traceback, nullptr);
}

TEST_F(ErrorsExtensionApiTest, BadArgumentRaisesTypeError) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_BadArgument(), 0);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_TypeError);

  PyObject* message =
      PyUnicode_FromString("bad argument type for built-in operation");
  ASSERT_TRUE(PyUnicode_Check(message));
  EXPECT_EQ(_PyUnicode_EQ(value, message), 1);
  Py_DECREF(message);

  EXPECT_EQ(traceback, nullptr);
}

TEST_F(ErrorsExtensionApiTest, NoMemoryRaisesMemoryError) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_NoMemory(), nullptr);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_MemoryError);
  EXPECT_EQ(value, nullptr);
  EXPECT_EQ(traceback, nullptr);
}

#pragma push_macro("PyErr_BadInternalCall")
#undef PyErr_BadInternalCall
TEST_F(ErrorsExtensionApiTest, BadInternalCallRaisesSystemError) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyErr_BadInternalCall();

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_SystemError);

  PyObject* message = PyUnicode_FromString("bad argument to internal function");
  ASSERT_TRUE(PyUnicode_Check(message));
  EXPECT_EQ(_PyUnicode_EQ(value, message), 1);
  Py_DECREF(message);

  EXPECT_EQ(traceback, nullptr);
}
#pragma pop_macro("PyErr_BadInternalCall")

TEST_F(ErrorsExtensionApiTest, UnderBadInternalCallRaisesSystemError) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  _PyErr_BadInternalCall("abc", 123);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_SystemError);

  PyObject* message =
      PyUnicode_FromString("abc:123: bad argument to internal function");
  ASSERT_TRUE(PyUnicode_Check(message));
  EXPECT_EQ(_PyUnicode_EQ(value, message), 1);
  Py_DECREF(message);

  EXPECT_EQ(traceback, nullptr);
}

TEST_F(ErrorsExtensionApiTest, ExceptionMatches) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyErr_NoMemory();
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_MemoryError));
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_Exception));
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_BaseException));
}

TEST_F(ErrorsExtensionApiTest, Fetch) {
  PyErr_SetObject(PyExc_Exception, Py_True);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_Exception);
  EXPECT_EQ(value, Py_True);
  EXPECT_EQ(traceback, nullptr);
}

TEST_F(ErrorsExtensionApiTest, GivenExceptionMatches) {
  // An exception matches itself and all of its super types up to and including
  // BaseException.
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_MemoryError),
            1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_Exception), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_BaseException),
            1);

  // An exception should not match a disjoint exception type.
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_IOError), 0);

  // If the objects are not exceptions or exception classes, the matching falls
  // back to an identity comparison.
  EXPECT_TRUE(PyErr_GivenExceptionMatches(Py_True, Py_True));
}

TEST_F(ErrorsExtensionApiTest, GivenExceptionMatchesWithNullptr) {
  // If any argument is a null pointer zero is returned.
  EXPECT_EQ(PyErr_GivenExceptionMatches(nullptr, nullptr), 0);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemError, nullptr), 0);
  EXPECT_EQ(PyErr_GivenExceptionMatches(nullptr, PyExc_SystemError), 0);
}

TEST_F(ErrorsExtensionApiTest, GivenExceptionMatchesWithTuple) {
  PyObject* exc1 = PyTuple_Pack(1, PyExc_Exception);
  ASSERT_NE(exc1, nullptr);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, exc1), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemExit, exc1), 0);
  Py_DECREF(exc1);

  // Linear search
  PyObject* exc2 = PyTuple_Pack(2, PyExc_Warning, PyExc_Exception);
  ASSERT_NE(exc2, nullptr);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, exc2), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemExit, exc2), 0);
  Py_DECREF(exc2);

  // Recursion
  PyObject* exc3 =
      PyTuple_Pack(2, PyTuple_Pack(1, PyExc_Exception), PyExc_Warning);
  ASSERT_NE(exc3, nullptr);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, exc3), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemExit, exc3), 0);
  Py_DECREF(exc3);
}

TEST_F(ErrorsExtensionApiTest, Restore) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_INCREF(PyExc_Exception);
  Py_INCREF(Py_True);
  PyErr_Restore(PyExc_Exception, Py_True, nullptr);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_Exception);
  EXPECT_EQ(value, Py_True);
  EXPECT_EQ(traceback, nullptr);
}

}  // namespace python
