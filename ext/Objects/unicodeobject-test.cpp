#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using UnicodeExtensionApiTest = ExtensionApi;

TEST_F(UnicodeExtensionApiTest, AsUTF8FromNonStringReturnsNull) {
  // Pass a non string object
  char* cstring = PyUnicode_AsUTF8AndSize(Py_None, nullptr);
  EXPECT_EQ(nullptr, cstring);
}

TEST_F(UnicodeExtensionApiTest, AsUTF8WithNullSizeReturnsCString) {
  const char* str = "Some C String";
  PyObject* pyunicode = PyUnicode_FromString(str);

  // Pass a nullptr size
  char* cstring = PyUnicode_AsUTF8AndSize(pyunicode, nullptr);
  ASSERT_NE(nullptr, cstring);
  EXPECT_STREQ(str, cstring);
}

TEST_F(UnicodeExtensionApiTest, AsUTF8WithReferencedSizeReturnsCString) {
  const char* str = "Some C String";
  PyObject* pyunicode = PyUnicode_FromString(str);

  // Pass a size reference
  Py_ssize_t size = 0;
  char* cstring = PyUnicode_AsUTF8AndSize(pyunicode, &size);
  ASSERT_NE(nullptr, cstring);
  EXPECT_STREQ(str, cstring);
  EXPECT_EQ(size, static_cast<Py_ssize_t>(std::strlen(str)));

  // Repeated calls should return the same buffer and still set the size.
  size = 0;
  char* cstring2 = PyUnicode_AsUTF8AndSize(pyunicode, &size);
  ASSERT_NE(cstring2, nullptr);
  EXPECT_EQ(cstring2, cstring);
}

TEST_F(UnicodeExtensionApiTest, AsUTF8ReturnsCString) {
  const char* str = "Some other C String";
  PyObject* pyobj = PyUnicode_FromString(str);

  char* cstring = PyUnicode_AsUTF8(pyobj);
  ASSERT_NE(cstring, nullptr);
  EXPECT_STREQ(cstring, str);

  // Make sure repeated calls on the same object return the same buffer.
  char* cstring2 = PyUnicode_AsUTF8(pyobj);
  ASSERT_NE(cstring, nullptr);
  EXPECT_EQ(cstring2, cstring);
}

TEST_F(UnicodeExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyUnicode_ClearFreeList(), 0);
}

TEST_F(UnicodeExtensionApiTest, FromStringAndSizeCreatesSizedString) {
  const char* str = "Some string";
  PyObjectPtr pyuni(PyUnicode_FromStringAndSize(str, 11));
  ASSERT_NE(pyuni, nullptr);

  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(pyuni, str));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, FromStringAndSizeCreatesSmallerString) {
  PyObjectPtr str(PyUnicode_FromStringAndSize("1234567890", 5));
  ASSERT_NE(str, nullptr);

  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(str, "12345"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, FromStringAndSizeFailsNegSize) {
  PyObjectPtr pyuni(PyUnicode_FromStringAndSize("a", -1));
  ASSERT_EQ(pyuni, nullptr);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(UnicodeExtensionApiTest, FromStringAndSizeIncrementsRefCount) {
  PyObject* pyuni = PyUnicode_FromStringAndSize("Some string", 11);
  ASSERT_NE(pyuni, nullptr);
  EXPECT_GE(Py_REFCNT(pyuni), 1);
  Py_DECREF(pyuni);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, ReadyReturnsZero) {
  PyObject* pyunicode = PyUnicode_FromString("some string");
  int is_ready = PyUnicode_READY(pyunicode);
  EXPECT_EQ(0, is_ready);
  Py_DECREF(pyunicode);
}

TEST_F(UnicodeExtensionApiTest, Compare) {
  PyObject* s1 = PyUnicode_FromString("some string");
  PyObject* s2 = PyUnicode_FromString("some longer string");
  PyObject* s22 = PyUnicode_FromString("some longer string");

  int result = PyUnicode_Compare(s1, s2);
  EXPECT_EQ(result, 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  result = PyUnicode_Compare(s2, s1);
  EXPECT_EQ(result, -1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  result = PyUnicode_Compare(s2, s22);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  Py_DECREF(s22);
  Py_DECREF(s2);
  Py_DECREF(s1);
}

TEST_F(UnicodeExtensionApiTest, CompareBadInput) {
  PyObject* str_obj = PyUnicode_FromString("this is a string");
  PyObject* int_obj = PyLong_FromLong(1234);

  PyUnicode_Compare(str_obj, int_obj);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  PyErr_Clear();

  PyUnicode_Compare(int_obj, str_obj);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  PyErr_Clear();

  PyUnicode_Compare(int_obj, int_obj);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  PyErr_Clear();

  Py_DECREF(int_obj);
  Py_DECREF(str_obj);
}

TEST_F(UnicodeExtensionApiTest, EqSameLength) {
  PyObject* str1 = PyUnicode_FromString("some string");

  PyObject* str2 = PyUnicode_FromString("some other string");
  EXPECT_EQ(_PyUnicode_EQ(str1, str2), 0);
  EXPECT_EQ(_PyUnicode_EQ(str2, str1), 0);
  Py_DECREF(str2);

  PyObject* str3 = PyUnicode_FromString("some string");
  EXPECT_EQ(_PyUnicode_EQ(str1, str3), 1);
  EXPECT_EQ(_PyUnicode_EQ(str3, str1), 1);
  Py_DECREF(str3);

  Py_DECREF(str1);
}

TEST_F(UnicodeExtensionApiTest, EqDifferentLength) {
  PyObject* small = PyUnicode_FromString("123");
  PyObject* large = PyUnicode_FromString("1234567890");
  EXPECT_EQ(_PyUnicode_EQ(small, large), 0);
  EXPECT_EQ(_PyUnicode_EQ(large, small), 0);
  Py_DECREF(large);
  Py_DECREF(small);
}

TEST_F(UnicodeExtensionApiTest, EqualToASCIIString) {
  PyObject* unicode = PyUnicode_FromString("here's another string");

  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(unicode, "here's another string"));
  EXPECT_FALSE(
      _PyUnicode_EqualToASCIIString(unicode, "here is another string"));

  Py_DECREF(unicode);
}

TEST_F(UnicodeExtensionApiTest, CompareWithASCIIStringASCIINul) {
  PyObjectPtr pyunicode(PyUnicode_FromStringAndSize("large\0st", 8));

  // Less
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(pyunicode, "largz"), -1);

  // Greater
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(pyunicode, "large"), 1);
}

TEST_F(UnicodeExtensionApiTest, CompareWithASCIIStringASCII) {
  PyObjectPtr pyunicode(PyUnicode_FromString("large string"));

  // Equal
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(pyunicode, "large string"), 0);

  // Less
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(pyunicode, "large strings"), -1);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(pyunicode, "large tbigger"), -1);

  // Greater
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(pyunicode, "large strin"), 1);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(pyunicode, "large smaller"), 1);
}

TEST_F(UnicodeExtensionApiTest, GetLengthWithEmptyStrReturnsZero) {
  PyObjectPtr str(PyUnicode_FromString(""));
  EXPECT_EQ(PyUnicode_GetLength(str), 0);
}

TEST_F(UnicodeExtensionApiTest, GetLengthWithNonEmptyString) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  EXPECT_EQ(PyUnicode_GetLength(str), 3);
}

TEST_F(UnicodeExtensionApiTest, GetLengthWithNonStrReturnsNegative) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_EQ(PyUnicode_GetLength(list), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, GETLENGTHAndGetLengthSame) {
  PyObjectPtr mty(PyUnicode_FromString(""));
  PyObjectPtr str(PyUnicode_FromString("some string"));

  EXPECT_EQ(PyUnicode_GET_LENGTH(mty.get()), PyUnicode_GetLength(mty));
  EXPECT_EQ(PyUnicode_GET_LENGTH(str.get()), PyUnicode_GetLength(str));
}

TEST_F(UnicodeExtensionApiTest, GetSizeWithEmptyStrReturnsZero) {
  PyObjectPtr str(PyUnicode_FromString(""));
  EXPECT_EQ(PyUnicode_GetSize(str), 0);
}

TEST_F(UnicodeExtensionApiTest, GetSizeWithNonEmptyString) {
  PyObjectPtr str(PyUnicode_FromString("bar"));
  EXPECT_EQ(PyUnicode_GetSize(str), 3);
}

TEST_F(UnicodeExtensionApiTest, GetSizeWithNonStrReturnsNegative) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyUnicode_GetSize(dict), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, GETSIZEAndGetSizeSame) {
  PyObjectPtr mty(PyUnicode_FromString(""));
  PyObjectPtr str(PyUnicode_FromString("another string"));

  EXPECT_EQ(PyUnicode_GET_SIZE(mty.get()), PyUnicode_GetSize(mty));
  EXPECT_EQ(PyUnicode_GET_SIZE(str.get()), PyUnicode_GetSize(str));
}

TEST_F(UnicodeExtensionApiTest, FromUnicodeWithASCIIReturnsString) {
  PyObjectPtr unicode(PyUnicode_FromUnicode(L"abc123-", 7));
  ASSERT_TRUE(PyUnicode_Check(unicode));
  EXPECT_EQ(PyUnicode_GetSize(unicode), 7);
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(unicode, "abc123-"));
}

TEST_F(UnicodeExtensionApiTest, FromUnicodeWithNullBufferAbortsPyro) {
  EXPECT_DEATH(PyUnicode_FromUnicode(nullptr, 2),
               "unimplemented: _PyUnicode_New");
}

TEST_F(UnicodeExtensionApiTest,
       FromWideCharWithNullBufferAndZeroSizeReturnsEmpty) {
  PyObjectPtr empty(PyUnicode_FromWideChar(nullptr, 0));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_Check(empty));
  EXPECT_EQ(PyUnicode_GetSize(empty), 0);
}

TEST_F(UnicodeExtensionApiTest, FromWideCharWithNullBufferReturnsError) {
  PyObjectPtr empty(PyUnicode_FromWideChar(nullptr, 1));
  ASSERT_EQ(empty, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(UnicodeExtensionApiTest, FromWideCharWithUnknownSizeReturnsString) {
  PyObjectPtr unicode(PyUnicode_FromWideChar(L"abc123-", -1));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_Check(unicode));
  EXPECT_EQ(PyUnicode_GetSize(unicode), 7);
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(unicode, "abc123-"));
}

TEST_F(UnicodeExtensionApiTest, FromWideCharWithGivenSizeReturnsString) {
  PyObjectPtr unicode(PyUnicode_FromWideChar(L"abc123-", 6));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_Check(unicode));
  EXPECT_EQ(PyUnicode_GetSize(unicode), 6);
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(unicode, "abc123"));
}

TEST_F(UnicodeExtensionApiTest, FromWideCharWithBufferAndZeroSizeReturnsEmpty) {
  PyObjectPtr empty(PyUnicode_FromWideChar(L"abc", 0));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_Check(empty));
  EXPECT_EQ(PyUnicode_GetSize(empty), 0);
}

TEST_F(UnicodeExtensionApiTest, DecodeFSDefaultCreatesString) {
  PyObjectPtr unicode(PyUnicode_DecodeFSDefault("hello"));
  ASSERT_TRUE(PyUnicode_Check(unicode));
  EXPECT_EQ(PyUnicode_GetSize(unicode), 5);
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(unicode, "hello"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, DecodeFSDefaultAndSizeReturnsString) {
  PyObjectPtr unicode(PyUnicode_DecodeFSDefaultAndSize("hello", 5));
  ASSERT_TRUE(PyUnicode_Check(unicode));
  EXPECT_EQ(PyUnicode_GetSize(unicode), 5);
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(unicode, "hello"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest,
       DecodeFSDefaultAndSizeWithSmallerSizeReturnsString) {
  PyObjectPtr unicode(PyUnicode_DecodeFSDefaultAndSize("hello", 2));
  ASSERT_TRUE(PyUnicode_Check(unicode));
  EXPECT_EQ(PyUnicode_GetSize(unicode), 2);
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(unicode, "he"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
