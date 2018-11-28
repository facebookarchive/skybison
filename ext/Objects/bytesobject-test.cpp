#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using BytesExtensionApiTest = ExtensionApi;

TEST_F(BytesExtensionApiTest, FromStringAndSizeIncrementsRefCount) {
  PyObject* bytes = PyBytes_FromStringAndSize("foo", 3);
  ASSERT_NE(bytes, nullptr);
  EXPECT_GE(Py_REFCNT(bytes), 1);
  Py_DECREF(bytes);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeReturnsBytes) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo", 3));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 3);
}

TEST_F(BytesExtensionApiTest,
       FromStringAndSizeWithEmptyStringReturnsEmptyBytes) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("", 0));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 0);
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeWithNegativeSizeReturnsNull) {
  EXPECT_EQ(PyBytes_FromStringAndSize("foo", -1), nullptr);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  // TODO(miro): replace with PyErr_ExceptionMatches(PyExc_SystemError);
  const char* expected = "Negative size passed to PyBytes_FromStringAndSize";
  EXPECT_TRUE(testing::exceptionValueMatches(expected));
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeWithShorterSize) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo bar", 5));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 5);
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeWithSizeZero) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo bar", 0));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 0);
}

TEST_F(BytesExtensionApiTest, FromStringIncrementsRefCount) {
  PyObject* bytes = PyBytes_FromString("foo");
  ASSERT_NE(bytes, nullptr);
  EXPECT_GE(Py_REFCNT(bytes), 1);
  Py_DECREF(bytes);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(BytesExtensionApiTest, FromStringReturnsBytes) {
  PyObjectPtr bytes(PyBytes_FromString("foo"));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 3);
}

TEST_F(BytesExtensionApiTest, FromStringWithEmptyStringReturnsEmptyBytes) {
  PyObjectPtr bytes(PyBytes_FromString(""));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 0);
}

TEST_F(BytesExtensionApiTest, SizeWithNonBytesReturnsNegative) {
  PyObjectPtr dict(PyDict_New());

  EXPECT_EQ(PyBytes_Size(dict), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  // TODO(miro): replace with PyErr_ExceptionMatches(PyExc_TypeError);
  const char* expected = "PyBytes_Size expected bytes";
  EXPECT_TRUE(testing::exceptionValueMatches(expected));
}

}  // namespace python
