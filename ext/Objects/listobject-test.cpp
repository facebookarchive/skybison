#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ListExtensionApiTest = ExtensionApi;

TEST_F(ListExtensionApiTest, NewWithBadLengthReturnsNull) {
  Py_ssize_t invalid_length = -1;
  PyObject* pyresult = PyList_New(invalid_length);
  EXPECT_EQ(pyresult, nullptr);
}

TEST_F(ListExtensionApiTest, NewReturnsList) {
  Py_ssize_t length = 0;
  PyObject* pyresult = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(pyresult));

  // TODO(eelizondo): Check list size once PyList_Size is implemented
  Py_ssize_t length2 = 5;
  PyObject* pyresult2 = PyList_New(length2);
  EXPECT_TRUE(PyList_CheckExact(pyresult2));
}

TEST_F(ListExtensionApiTest, AppendToNonListReturnsNegative) {
  PyObject* dict = PyDict_New();
  PyObject* pylong = PyLong_FromLong(10);
  int result = PyList_Append(dict, pylong);
  EXPECT_EQ(result, -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);

  const char* expected_message = "bad argument to internal function";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));
}

TEST_F(ListExtensionApiTest, AppendWithNullValueReturnsNegative) {
  PyObject* list = PyList_New(0);
  int result = PyList_Append(list, nullptr);
  EXPECT_EQ(result, -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);

  const char* expected_message = "bad argument to internal function";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));
}

TEST_F(ListExtensionApiTest, AppendReturnsZero) {
  PyObject* list = PyList_New(0);
  PyObject* pylong = PyLong_FromLong(10);
  int result = PyList_Append(list, pylong);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
