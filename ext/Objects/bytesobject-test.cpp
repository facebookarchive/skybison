#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using BytesExtensionApiTest = ExtensionApi;

TEST_F(BytesExtensionApiTest,
       AsStringFromNonBytesReturnsNullAndRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(0));
  EXPECT_EQ(PyBytes_AsString(num), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, AsStringWithBytesReturnsByteString) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  EXPECT_STREQ(PyBytes_AsString(bytes), str);
}

TEST_F(BytesExtensionApiTest, AsStringWithZeroByteReturnsByteString) {
  const char* str = "foo \0 bar";
  PyObjectPtr bytes(PyBytes_FromStringAndSize(str, 9));
  char* result = PyBytes_AsString(bytes);
  std::vector<char> expected(str, str + 9);
  std::vector<char> actual(result, result + 9);
  EXPECT_EQ(actual, expected);
}

TEST_F(BytesExtensionApiTest, AsStringReturnsSameBufferTwice) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer1 = PyBytes_AsString(bytes);
  char* buffer2 = PyBytes_AsString(bytes);
  EXPECT_EQ(buffer1, buffer2);
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithNullBufferReturnsNegativeAndRaisesSystemError) {
  PyObjectPtr bytes(PyBytes_FromString(""));
  Py_ssize_t length;
  EXPECT_EQ(PyBytes_AsStringAndSize(bytes, nullptr, &length), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithNonBytesReturnsNegativeAndRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(0));
  char* buffer;
  EXPECT_EQ(PyBytes_AsStringAndSize(num, &buffer, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, AsStringAndSizeWithBytesReturnsStringAndSize) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer;
  Py_ssize_t length;
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer, &length), 0);
  EXPECT_STREQ(buffer, str);
  EXPECT_EQ(length, 3);
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithBytesAndNullLengthReturnsString) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer;
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer, nullptr), 0);
  EXPECT_STREQ(buffer, str);
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithZeroByteAndNullLengthRaisesValueError) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo \0 bar", 9));
  char* buffer;
  EXPECT_EQ(PyBytes_AsStringAndSize(bytes, &buffer, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(BytesExtensionApiTest, AsStringAndSizeReturnsSameBufferTwice) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer1;
  char* buffer2;
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer1, nullptr), 0);
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer2, nullptr), 0);
  EXPECT_EQ(buffer1, buffer2);
}

TEST_F(BytesExtensionApiTest, ConcatWithPointerToNullIsNoop) {
  PyObject* foo = nullptr;
  PyObjectPtr bar(PyLong_FromLong(0));
  PyBytes_Concat(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNullSetsFirstArgNull) {
  PyObject* foo = PyBytes_FromString("foo");
  PyBytes_Concat(&foo, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(foo, nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesFirstArgSetsFirstArgNull) {
  PyObject* foo = PyLong_FromLong(0);
  PyObjectPtr bar(PyBytes_FromString("bar"));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(foo, nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesFirstArgRaisesTypeError) {
  PyObject* foo = PyLong_FromLong(0);
  PyObjectPtr bar(PyBytes_FromString("bar"));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesSecondArgSetsFirstArgNull) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyLong_FromLong(0));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(foo, nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesSecondArgRaisesTypeError) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyLong_FromLong(0));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, ConcatWithBytesConcatenatesByteStrings) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyBytes_FromString("bar"));
  PyBytes_Concat(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyBytes_Size(foo), 6);
}

TEST_F(BytesExtensionApiTest, ConcatDoesNotDecrefSecondArg) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyBytes_FromString("bar"));
  long refcnt = Py_REFCNT(bar);
  ASSERT_GE(refcnt, 1);
  PyBytes_Concat(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(bar), refcnt);
}

TEST_F(BytesExtensionApiTest, ConcatAndDelDecrefsSecondArg) {
  PyObject* foo = nullptr;
  PyObject* bar = PyBytes_FromString("bar");
  long refcnt = Py_REFCNT(bar);
  ASSERT_GE(refcnt, 1);
  Py_INCREF(bar);  // prevent bar from being deallocated
  PyBytes_ConcatAndDel(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(bar), refcnt);
  Py_DECREF(bar);
}

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
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace python
