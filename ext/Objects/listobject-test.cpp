#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using ListExtensionApiTest = ExtensionApi;

TEST_F(ListExtensionApiTest, NewWithBadLengthReturnsNull) {
  Py_ssize_t invalid_length = -1;
  PyObject* pyresult = PyList_New(invalid_length);
  EXPECT_EQ(pyresult, nullptr);
}

TEST_F(ListExtensionApiTest, NewReturnsEmptyList) {
  Py_ssize_t length = 0;
  PyObject* pyresult = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(pyresult));
  EXPECT_EQ(PyList_Size(pyresult), length);

  Py_DECREF(pyresult);
}

TEST_F(ListExtensionApiTest, NewReturnsList) {
  Py_ssize_t length = 5;
  PyObject* pyresult = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(pyresult));
  EXPECT_EQ(PyList_Size(pyresult), length);

  Py_DECREF(pyresult);
}

TEST_F(ListExtensionApiTest, AppendToNonListReturnsNegative) {
  PyObject* dict = PyDict_New();
  PyObject* pylong = PyLong_FromLong(10);
  int result = PyList_Append(dict, pylong);
  EXPECT_EQ(result, -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AppendWithNullValueReturnsNegative) {
  PyObject* list = PyList_New(0);
  int result = PyList_Append(list, nullptr);
  EXPECT_EQ(result, -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AppendReturnsZero) {
  PyObject* list = PyList_New(0);
  PyObject* pylong = PyLong_FromLong(10);
  int result = PyList_Append(list, pylong);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ListExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyList_ClearFreeList(), 0);
}

TEST_F(ListExtensionApiTest, SizeIncreasesAfterAppend) {
  Py_ssize_t length = 4;
  PyObject* list = PyList_New(length);
  EXPECT_TRUE(PyList_CheckExact(list));
  EXPECT_EQ(PyList_Size(list), length);

  PyObject* item = PyLong_FromLong(1);
  EXPECT_EQ(PyList_Append(list, item), 0);
  EXPECT_EQ(PyList_Size(list), length + 1);

  Py_DECREF(item);
  Py_DECREF(list);
}

TEST_F(ListExtensionApiTest, SizeWithNonListReturnsNegative) {
  PyObject* dict = PyDict_New();
  EXPECT_EQ(PyList_Size(dict), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));

  Py_DECREF(dict);
}

TEST_F(ListExtensionApiTest, AsTupleWithNullRaisesSystemError) {
  ASSERT_EQ(PyList_AsTuple(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AsTupleWithNonListRaisesSystemError) {
  ASSERT_EQ(PyList_AsTuple(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ListExtensionApiTest, AsTupleWithListReturnsAllElementsFromList) {
  PyObjectPtr list(PyList_New(0));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyObjectPtr three(PyLong_FromLong(3));
  PyList_Append(list, one);
  PyList_Append(list, two);
  PyList_Append(list, three);

  PyObjectPtr tuple(PyList_AsTuple(list));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyTuple_CheckExact(tuple));
  ASSERT_EQ(PyTuple_Size(tuple), 3);
  EXPECT_EQ(PyTuple_GetItem(tuple, 0), one);
  EXPECT_EQ(PyTuple_GetItem(tuple, 1), two);
  EXPECT_EQ(PyTuple_GetItem(tuple, 2), three);
}

}  // namespace python
