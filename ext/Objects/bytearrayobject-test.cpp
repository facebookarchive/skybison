#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using ByteArrayExtensionApiTest = ExtensionApi;

TEST_F(ByteArrayExtensionApiTest, CheckWithBytesReturnsFalse) {
  PyObjectPtr bytes(PyBytes_FromString("hello"));
  EXPECT_FALSE(PyByteArray_CheckExact(bytes));
  EXPECT_FALSE(PyByteArray_Check(bytes));
}

TEST_F(ByteArrayExtensionApiTest, ConcatWithNonBytesLikeSelfRaisesTypeError) {
  PyObjectPtr self(PyList_New(0));
  PyObjectPtr other(PyByteArray_FromStringAndSize("world", 5));
  ASSERT_EQ(PyByteArray_Concat(self, other), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ByteArrayExtensionApiTest, ConcatWithNonBytesLikeOtherRaisesTypeError) {
  PyObjectPtr self(PyByteArray_FromStringAndSize("hello", 5));
  PyObjectPtr other(PyList_New(0));
  ASSERT_EQ(PyByteArray_Concat(self, other), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ByteArrayExtensionApiTest, ConcatWithEmptyByteArraysReturnsEmpty) {
  PyObjectPtr self(PyByteArray_FromStringAndSize("", 0));
  PyObjectPtr other(PyByteArray_FromStringAndSize("", 0));
  PyObjectPtr result(PyByteArray_Concat(self, other));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(result), 0);
}

TEST_F(ByteArrayExtensionApiTest,
       ConcatWithBytesSelfReturnsNewConcatenatedByteArray) {
  const char* str1 = "hello";
  const char* str2 = "world";
  Py_ssize_t len1 = static_cast<Py_ssize_t>(std::strlen(str1));
  Py_ssize_t len2 = static_cast<Py_ssize_t>(std::strlen(str2));
  PyObjectPtr self(PyBytes_FromString(str1));
  PyObjectPtr other(PyBytes_FromString(str2));
  PyObjectPtr result(PyByteArray_Concat(self, other));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyBytes_Size(self), len1);
  ASSERT_TRUE(PyByteArray_CheckExact(result));
  EXPECT_EQ(PyByteArray_Size(result), len1 + len2);
}

TEST_F(ByteArrayExtensionApiTest,
       ConcatWithByteArraysReturnsNewConcatenatedByteArray) {
  const char* str1 = "hello";
  const char* str2 = "world";
  Py_ssize_t len1 = static_cast<Py_ssize_t>(std::strlen(str1));
  Py_ssize_t len2 = static_cast<Py_ssize_t>(std::strlen(str2));
  PyObjectPtr self(PyByteArray_FromStringAndSize(str1, len1));
  PyObjectPtr other(PyByteArray_FromStringAndSize(str2, len2));
  PyObjectPtr result(PyByteArray_Concat(self, other));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyByteArray_Size(self), len1);
  ASSERT_TRUE(PyByteArray_CheckExact(result));
  EXPECT_EQ(PyByteArray_Size(result), len1 + len2);
}

TEST_F(ByteArrayExtensionApiTest, FromStringAndSizeReturnsByteArray) {
  PyObjectPtr array(PyByteArray_FromStringAndSize("hello", 5));
  EXPECT_TRUE(PyByteArray_Check(array));
  EXPECT_TRUE(PyByteArray_CheckExact(array));
}

TEST_F(ByteArrayExtensionApiTest, FromStringAndSizeSetsSize) {
  PyObjectPtr array(PyByteArray_FromStringAndSize("hello", 3));
  ASSERT_TRUE(PyByteArray_CheckExact(array));
  EXPECT_EQ(PyByteArray_Size(array), 3);
}

TEST_F(ByteArrayExtensionApiTest,
       FromStringAndSizeWithNegativeSizeRaisesSystemError) {
  ASSERT_EQ(PyByteArray_FromStringAndSize("hello", -1), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ByteArrayExtensionApiTest, FromStringAndSizeWithNullReturnsNew) {
  PyObjectPtr array(PyByteArray_FromStringAndSize(nullptr, 10));
  ASSERT_TRUE(PyByteArray_CheckExact(array));
  EXPECT_EQ(PyByteArray_Size(array), 10);
}

TEST_F(ByteArrayExtensionApiTest, ResizeWithNonByteArrayRaisesTypeErrorPyro) {
  const char* hello = "hello";
  PyObjectPtr bytes(PyBytes_FromString(hello));
  ASSERT_EQ(PyByteArray_Resize(bytes, std::strlen(hello)), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ByteArrayExtensionApiTest, ResizeWithSameSizeIsNoop) {
  const char* hello = "hello";
  Py_ssize_t len = static_cast<Py_ssize_t>(std::strlen(hello));
  PyObjectPtr array(PyByteArray_FromStringAndSize(hello, len));
  ASSERT_EQ(PyByteArray_Resize(array, len), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(array), len);
}

TEST_F(ByteArrayExtensionApiTest, ResizeWithSmallerSizeShrinks) {
  const char* hello = "hello";
  Py_ssize_t len = static_cast<Py_ssize_t>(std::strlen(hello));
  PyObjectPtr array(PyByteArray_FromStringAndSize(hello, len));
  ASSERT_EQ(PyByteArray_Resize(array, len - 2), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(array), len - 2);
}

TEST_F(ByteArrayExtensionApiTest, ResizeWithLargerSizeGrows) {
  const char* hello = "hello";
  Py_ssize_t len = static_cast<Py_ssize_t>(std::strlen(hello));
  PyObjectPtr array(PyByteArray_FromStringAndSize(hello, len));
  ASSERT_EQ(PyByteArray_Resize(array, len + 2), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(array), len + 2);
}

TEST_F(ByteArrayExtensionApiTest, ResizeLargerThenSmaller) {
  const char* hello = "hello";
  Py_ssize_t len = static_cast<Py_ssize_t>(std::strlen(hello));
  PyObjectPtr array(PyByteArray_FromStringAndSize(hello, len));
  ASSERT_EQ(PyByteArray_Resize(array, len + 3), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(array), len + 3);

  ASSERT_EQ(PyByteArray_Resize(array, len - 1), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(array), len - 1);
}

TEST_F(ByteArrayExtensionApiTest, ResizeSmallerThenLarger) {
  const char* hello = "hello";
  Py_ssize_t len = static_cast<Py_ssize_t>(std::strlen(hello));
  PyObjectPtr array(PyByteArray_FromStringAndSize(hello, len));
  ASSERT_EQ(PyByteArray_Resize(array, len - 3), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(array), len - 3);

  ASSERT_EQ(PyByteArray_Resize(array, len + 1), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyByteArray_Size(array), len + 1);
}

TEST_F(ByteArrayExtensionApiTest, SizeWithNonByteArrayRaisesPyro) {
  PyObjectPtr bytes(PyBytes_FromString("hello"));
  ASSERT_EQ(PyByteArray_Size(bytes), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace python
