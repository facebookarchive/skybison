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
  ASSERT_NE(cstring2, nullptr);
  EXPECT_EQ(cstring2, cstring);
}

TEST_F(UnicodeExtensionApiTest, AsUTF8StringWithNonStringReturnsNull) {
  PyObjectPtr bytes(_PyUnicode_AsUTF8String(Py_None, nullptr));
  ASSERT_EQ(bytes, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, AsUTF8StringReturnsBytes) {
  PyObjectPtr unicode(PyUnicode_FromString("foo"));
  PyObjectPtr bytes(_PyUnicode_AsUTF8String(unicode, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 3);
  EXPECT_STREQ(PyBytes_AsString(bytes), "foo");
}

TEST_F(UnicodeExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyUnicode_ClearFreeList(), 0);
}

TEST_F(UnicodeExtensionApiTest, FindWithNonStrSelfRaisesTypeError) {
  PyObject* self = Py_None;
  PyObjectPtr sub(PyUnicode_FromString("ll"));
  EXPECT_EQ(PyUnicode_Find(self, sub, 0, 5, 1), -2);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, FindWithNonStrSubRaisesTypeError) {
  PyObjectPtr self(PyUnicode_FromString("hello"));
  PyObject* sub = Py_None;
  EXPECT_EQ(PyUnicode_Find(self, sub, 0, 5, 1), -2);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, FindForwardReturnsLeftmostStartIndex) {
  PyObjectPtr self(PyUnicode_FromString("hello"));
  PyObjectPtr sub(PyUnicode_FromString("ll"));
  EXPECT_EQ(PyUnicode_Find(self, sub, 0, 5, 1), 2);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, FindReturnsNegativeOneWithNonexistentSubstr) {
  PyObjectPtr self(PyUnicode_FromString("hello"));
  PyObjectPtr sub(PyUnicode_FromString("xx"));
  EXPECT_EQ(PyUnicode_Find(self, sub, 0, 5, 1), -1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest,
       FindReverseReturnsNegativeOneWithNonexistentSubstr) {
  PyObjectPtr self(PyUnicode_FromString("hello"));
  PyObjectPtr sub(PyUnicode_FromString("xx"));
  EXPECT_EQ(PyUnicode_Find(self, sub, 0, 5, -1), -1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, FindReverseReturnsRightmostStartIndex) {
  PyObjectPtr self(PyUnicode_FromString("helloll"));
  PyObjectPtr sub(PyUnicode_FromString("ll"));
  EXPECT_EQ(PyUnicode_Find(self, sub, 0, 7, -1), 5);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, FromStringAndSizeCreatesSizedString) {
  const char* str = "Some string";
  PyObjectPtr pyuni(PyUnicode_FromStringAndSize(str, 11));
  EXPECT_TRUE(isUnicodeEqualsCStr(pyuni, str));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, FromStringAndSizeCreatesSmallerString) {
  PyObjectPtr str(PyUnicode_FromStringAndSize("1234567890", 5));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "12345"));
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
  Py_ssize_t expected = 0;
  EXPECT_EQ(PyUnicode_GetLength(str), expected);
  EXPECT_EQ(PyUnicode_GetSize(str), expected);
  EXPECT_EQ(PyUnicode_GET_LENGTH(str.get()), expected);
  EXPECT_EQ(PyUnicode_GET_SIZE(str.get()), expected);
}

TEST_F(UnicodeExtensionApiTest, GetLengthWithNonEmptyString) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  Py_ssize_t expected = 3;
  EXPECT_EQ(PyUnicode_GetLength(str), expected);
  EXPECT_EQ(PyUnicode_GetSize(str), expected);
  EXPECT_EQ(PyUnicode_GET_LENGTH(str.get()), expected);
  EXPECT_EQ(PyUnicode_GET_SIZE(str.get()), expected);
}

TEST_F(UnicodeExtensionApiTest, GetLengthWithUTF8ReturnsCodePointLength) {
  PyObjectPtr str(PyUnicode_FromString("\xc3\xa9"));
  Py_ssize_t expected = 1;
  EXPECT_EQ(PyUnicode_GetLength(str), expected);
  EXPECT_EQ(PyUnicode_GetSize(str), expected);
  EXPECT_EQ(PyUnicode_GET_LENGTH(str.get()), expected);
  EXPECT_EQ(PyUnicode_GET_SIZE(str.get()), expected);
}

TEST_F(UnicodeExtensionApiTest, GetLengthWithNonStrRaisesTypeError) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_EQ(PyUnicode_GetLength(list), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, GetSizeWithNonStrRaisesTypeError) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_EQ(PyUnicode_GetSize(list), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, FromUnicodeWithASCIIReturnsString) {
  PyObjectPtr unicode(PyUnicode_FromUnicode(L"abc123-", 7));
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "abc123-"));
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
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "abc123-"));
}

TEST_F(UnicodeExtensionApiTest, FromWideCharWithGivenSizeReturnsString) {
  PyObjectPtr unicode(PyUnicode_FromWideChar(L"abc123-", 6));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "abc123"));
}

TEST_F(UnicodeExtensionApiTest, FromWideCharWithBufferAndZeroSizeReturnsEmpty) {
  PyObjectPtr empty(PyUnicode_FromWideChar(L"abc", 0));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_Check(empty));
  EXPECT_EQ(PyUnicode_GetSize(empty), 0);
}

TEST_F(UnicodeExtensionApiTest, DecodeFSDefaultCreatesString) {
  PyObjectPtr unicode(PyUnicode_DecodeFSDefault("hello"));
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "hello"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, DecodeFSDefaultAndSizeReturnsString) {
  PyObjectPtr unicode(PyUnicode_DecodeFSDefaultAndSize("hello", 5));
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "hello"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest,
       DecodeFSDefaultAndSizeWithSmallerSizeReturnsString) {
  PyObjectPtr unicode(PyUnicode_DecodeFSDefaultAndSize("hello", 2));
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "he"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(UnicodeExtensionApiTest, DecodeASCIIReturnsString) {
  PyObjectPtr str(PyUnicode_DecodeASCII("hello world", 11, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, PyUnicodeWriterPrepareWithLenZeroReturnsZero) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  ASSERT_EQ(_PyUnicodeWriter_Prepare(&writer, 0, 127), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, ""));
}

TEST_F(UnicodeExtensionApiTest,
       PyUnicodeWriterWithOverallocateSetOverallocates) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  writer.overallocate = 0;
  ASSERT_EQ(_PyUnicodeWriter_Prepare(&writer, 5, 127), 0);
  ASSERT_EQ(writer.size, 5);
  _PyUnicodeWriter_Dealloc(&writer);

  _PyUnicodeWriter_Init(&writer);
  writer.overallocate = 1;
  ASSERT_EQ(_PyUnicodeWriter_Prepare(&writer, 5, 127), 0);
  ASSERT_GT(writer.size, 5);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, ""));
}

TEST_F(UnicodeExtensionApiTest, PyUnicodeWriterCreatesEmptyString) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  PyObjectPtr empty(_PyUnicodeWriter_Finish(&writer));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_Check(empty));
  EXPECT_EQ(PyUnicode_GetSize(empty), 0);
}

TEST_F(UnicodeExtensionApiTest, PyUnicodeWriterWritesASCIIStrings) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  ASSERT_EQ(_PyUnicodeWriter_WriteASCIIString(&writer, "hello", 5), 0);
  ASSERT_EQ(_PyUnicodeWriter_WriteASCIIString(&writer, " world", 6), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "hello world"));
}

TEST_F(UnicodeExtensionApiTest,
       WriteASCIIStringWithNegativeLengthReturnsString) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  ASSERT_EQ(_PyUnicodeWriter_WriteASCIIString(&writer, "hello world", -1), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, WriteASCIIStringWithNonASCIIDeathTestPyro) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  EXPECT_DEATH(_PyUnicodeWriter_WriteASCIIString(&writer, "\xA0", 1),
               "_PyUnicodeWriter_WriteASCIIString only takes ASCII");
  _PyUnicodeWriter_Dealloc(&writer);
}

TEST_F(UnicodeExtensionApiTest, PyUnicodeWriterWritesChars) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  ASSERT_EQ(_PyUnicodeWriter_WriteChar(&writer, 'a'), 0);
  ASSERT_EQ(_PyUnicodeWriter_WriteChar(&writer, 'b'), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "ab"));
}

TEST_F(UnicodeExtensionApiTest, WriteCharWithNonASCIIDeathTestPyro) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  ASSERT_EQ(_PyUnicodeWriter_WriteChar(&writer, 0xA0), 0);
  EXPECT_DEATH(
      _PyUnicodeWriter_Finish(&writer),
      "unimplemented: PyUnicode currently only supports ASCII characters");
  _PyUnicodeWriter_Dealloc(&writer);
}

TEST_F(UnicodeExtensionApiTest, PyUnicodeWriterWritesLatin1String) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  ASSERT_EQ(_PyUnicodeWriter_WriteLatin1String(&writer, "hello", 5), 0);
  ASSERT_EQ(_PyUnicodeWriter_WriteLatin1String(&writer, " world", 6), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, WriteLatin1WithNonASCIIDeathTestPyro) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  ASSERT_EQ(_PyUnicodeWriter_WriteLatin1String(&writer, "\xA0", 1), 0);
  EXPECT_DEATH(
      _PyUnicodeWriter_Finish(&writer),
      "unimplemented: PyUnicode currently only supports ASCII characters");
  _PyUnicodeWriter_Dealloc(&writer);
}

TEST_F(UnicodeExtensionApiTest, PyUnicodeWriterWritesStringObject) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  PyObjectPtr hello_str(PyUnicode_FromString("hello"));
  PyObjectPtr world_str(PyUnicode_FromString(" world"));
  ASSERT_EQ(_PyUnicodeWriter_WriteStr(&writer, hello_str), 0);
  ASSERT_EQ(_PyUnicodeWriter_WriteStr(&writer, world_str), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, PyUnicodeWriterWritesSubStringObject) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  PyObjectPtr str(PyUnicode_FromString("hello world"));
  ASSERT_EQ(_PyUnicodeWriter_WriteSubstring(&writer, str, 0, 5), 0);
  ASSERT_EQ(_PyUnicodeWriter_WriteSubstring(&writer, str, 5, 11), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, WriteSubstringWithZeroEndReturnsString) {
  _PyUnicodeWriter writer;
  _PyUnicodeWriter_Init(&writer);
  PyObjectPtr str(PyUnicode_FromString("hello"));
  ASSERT_EQ(_PyUnicodeWriter_WriteSubstring(&writer, str, 0, 0), 0);
  PyObjectPtr unicode(_PyUnicodeWriter_Finish(&writer));

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(unicode, ""));
}

TEST_F(UnicodeExtensionApiTest, DecodeUTF8ReturnsString) {
  PyObjectPtr str(PyUnicode_DecodeUTF8("hello world", 11, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, DecodeUTF8StatefulReturnsString) {
  PyObjectPtr str(
      PyUnicode_DecodeUTF8Stateful("hello world", 11, nullptr, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, FromFormatWithNoArgsReturnsString) {
  PyObjectPtr str(PyUnicode_FromFormat("hello world"));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, FromFormatWithManyArgsReturnsString) {
  PyObjectPtr str(PyUnicode_FromFormat("h%c%s%%%2.i", 'e', "llo world", 2));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hello world% 2"));
}

TEST_F(UnicodeExtensionApiTest, FromFormatParsesNumberTypes) {
  {
    PyObjectPtr str(PyUnicode_FromFormat("%x", 123));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "7b"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%d", 124));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "124"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%i", 125));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "125"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%ld", 126));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "126"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%li", 127));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "127"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%lld", 128));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "128"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%lli", 129));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "129"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%u", 130));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "130"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%lu", 131));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "131"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%llu", 132));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "132"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%zd", 133));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "133"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%zu", 134));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "134"));
  }

  {
    PyObjectPtr str(PyUnicode_FromFormat("%zi", 135));
    EXPECT_TRUE(isUnicodeEqualsCStr(str, "135"));
  }
}

TEST_F(UnicodeExtensionApiTest, FromFormatParsesCharacters) {
  PyObjectPtr str(PyUnicode_FromFormat("%c%c", 'h', 'w'));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hw"));
}

TEST_F(UnicodeExtensionApiTest, FromFormatParsesPointer) {
  long value = 0;
  void* test = &value;
  char buff[18];
  std::snprintf(buff, 18, "%p", test);
  PyObjectPtr str(PyUnicode_FromFormat("%p", test));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, buff));
}

TEST_F(UnicodeExtensionApiTest, FromFormatParsesString) {
  PyObjectPtr str(PyUnicode_FromFormat("%s", "UTF-8"));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "UTF-8"));
}

TEST_F(UnicodeExtensionApiTest, FromFormatParsesStringObject) {
  PyObjectPtr unicode(PyUnicode_FromString("hello"));
  PyObjectPtr str(PyUnicode_FromFormat("%U", static_cast<PyObject*>(unicode)));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hello"));
}

TEST_F(UnicodeExtensionApiTest, FromFormatParsesStringObjectAndString) {
  PyObjectPtr unicode(PyUnicode_FromString("hello"));
  PyObjectPtr str(
      PyUnicode_FromFormat("%V", static_cast<PyObject*>(unicode), "world"));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "hello"));
}

TEST_F(UnicodeExtensionApiTest, FromFormatParsesNullAndString) {
  PyObjectPtr str(PyUnicode_FromFormat("%V", nullptr, "world"));
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "world"));
}

TEST_F(UnicodeExtensionApiTest, ConcatWithNonStringFails) {
  PyObjectPtr i(PyLong_FromLong(1));
  EXPECT_EQ(PyUnicode_Concat(i, i), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, ConcatWithEmptyArgumentReturnsString) {
  PyObjectPtr hello(PyUnicode_FromString("hello"));
  PyObjectPtr empty(PyUnicode_FromString(""));
  PyObjectPtr empty_right(PyUnicode_Concat(hello, empty));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(empty_right, "hello"));

  PyObjectPtr empty_left(PyUnicode_Concat(empty, hello));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(empty_left, "hello"));
}

TEST_F(UnicodeExtensionApiTest, ConcatWithTwoStringsReturnsString) {
  PyObjectPtr hello(PyUnicode_FromString("hello "));
  PyObjectPtr world(PyUnicode_FromString("world"));
  PyObjectPtr result(PyUnicode_Concat(hello, world));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "hello world"));
}

TEST_F(UnicodeExtensionApiTest, AppendWithNullFails) {
  PyUnicode_Append(nullptr, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(UnicodeExtensionApiTest, AppendWithNonStringFails) {
  PyObject* not_str = PyLong_FromLong(1);
  PyUnicode_Append(&not_str, not_str);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(UnicodeExtensionApiTest, AppendWithEmptyArgumentReturnsString) {
  PyObject* hello(PyUnicode_FromString("hello"));
  PyObject* empty(PyUnicode_FromString(""));
  PyUnicode_Append(&hello, empty);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(hello, "hello"));

  PyUnicode_Append(&empty, hello);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(empty, "hello"));
  Py_DECREF(hello);
  Py_DECREF(empty);
}

TEST_F(UnicodeExtensionApiTest, AppendWithTwoStringsReturnsString) {
  PyObject* hello = PyUnicode_FromString("hello ");
  PyObjectPtr world(PyUnicode_FromString("world"));
  PyUnicode_Append(&hello, world);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(hello, "hello world"));
  Py_DECREF(hello);
}

TEST_F(UnicodeExtensionApiTest, AppendAndDelWithStringDecreasesRefcnt) {
  PyObject* hello = PyUnicode_FromString("hello ");
  PyObject* world = PyUnicode_FromString("world");
  Py_INCREF(world);
  Py_ssize_t original_refcnt = Py_REFCNT(world);
  PyUnicode_AppendAndDel(&hello, world);

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(hello, "hello world"));
  Py_DECREF(hello);

  EXPECT_LT(Py_REFCNT(world), original_refcnt);
  Py_DECREF(world);
}

TEST_F(UnicodeExtensionApiTest, EncodeFSDefaultWithNonStringReturnsNull) {
  PyObjectPtr bytes(PyUnicode_EncodeFSDefault(Py_None));
  EXPECT_EQ(bytes, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, EncodeFSDefaultReturnsBytes) {
  PyObjectPtr unicode(PyUnicode_FromString("foo"));
  PyObjectPtr bytes(PyUnicode_EncodeFSDefault(unicode));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 3);
  EXPECT_STREQ(PyBytes_AsString(bytes), "foo");
}

TEST_F(UnicodeExtensionApiTest, FSConverterWithNullSetAddrToNull) {
  PyObject* result = PyLong_FromLong(1);
  ASSERT_EQ(PyUnicode_FSConverter(nullptr, &result), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
}

TEST_F(UnicodeExtensionApiTest, FSConverterWithBytesReturnsBytes) {
  PyObjectPtr bytes(PyBytes_FromString("foo"));
  PyObject* result = nullptr;
  ASSERT_EQ(PyUnicode_FSConverter(bytes, &result), Py_CLEANUP_SUPPORTED);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyBytes_Check(result));
  Py_DECREF(result);
}

TEST_F(UnicodeExtensionApiTest, FSConverterWithUnicodeReturnsBytes) {
  PyObjectPtr unicode(PyUnicode_FromString("foo"));
  PyObject* result = nullptr;
  ASSERT_EQ(PyUnicode_FSConverter(unicode, &result), Py_CLEANUP_SUPPORTED);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyBytes_Check(result));
  Py_DECREF(result);
}

TEST_F(UnicodeExtensionApiTest, FSConverterWithEmbeddedNullRaisesValueError) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo \0 bar", 9));
  PyObject* result = nullptr;
  ASSERT_EQ(PyUnicode_FSConverter(bytes, &result), 0);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  ASSERT_EQ(result, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(UnicodeExtensionApiTest, InternInPlaceWritesNewHandleBack) {
  PyObject* a = PyUnicode_FromString("hello world aaaaaaaaaa");
  PyObject* b = PyUnicode_FromString("hello world aaaaaaaaaa");
  PyObject* b_addr = b;
  EXPECT_NE(a, b);
  PyUnicode_InternInPlace(&a);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyUnicode_InternInPlace(&b);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_NE(b, b_addr);
  Py_DECREF(a);
}

TEST_F(UnicodeExtensionApiTest, InternFromStringReturnsStr) {
  PyObjectPtr result(PyUnicode_InternFromString("szechuan broccoli"));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyUnicode_CheckExact(result));
}

TEST_F(UnicodeExtensionApiTest, JoinWithEmptySeqReturnsEmptyStr) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr seq(PyList_New(0));
  PyObjectPtr result(PyUnicode_Join(sep, seq));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, ""));
}

TEST_F(UnicodeExtensionApiTest, JoinWithSeqJoinsElements) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr seq(PyList_New(0));
  PyObjectPtr elt0(PyUnicode_FromString("a"));
  PyList_Append(seq, elt0);
  PyObjectPtr elt1(PyUnicode_FromString("b"));
  PyList_Append(seq, elt1);
  PyObjectPtr result(PyUnicode_Join(sep, seq));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "a.b"));
}

TEST_F(UnicodeExtensionApiTest, JoinWithSeqContainingNonStrRaisesTypeError) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr seq(PyList_New(0));
  PyList_Append(seq, Py_None);
  PyObjectPtr result(PyUnicode_Join(sep, seq));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, JoinWithSeqContainingBytesRaisesTypeError) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr seq(PyList_New(0));
  PyObjectPtr elt0(PyBytes_FromString("a"));
  PyList_Append(seq, elt0);
  PyObjectPtr result(PyUnicode_Join(sep, seq));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, PartitionWithNonStrStrRaisesTypeError) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_Partition(Py_None, sep));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, PartitionWithNonStrSepRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("hello"));
  PyObjectPtr result(PyUnicode_Partition(str, Py_None));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, PartitionReturnsTuple) {
  PyObjectPtr str(PyUnicode_FromString("a.b"));
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_Partition(str, sep));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyTuple_CheckExact(result));
}

TEST_F(UnicodeExtensionApiTest, RPartitionWithNonStrStrRaisesTypeError) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_RPartition(Py_None, sep));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, RPartitionWithNonStrSepRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("hello"));
  PyObjectPtr result(PyUnicode_RPartition(str, Py_None));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, RPartitionReturnsTuple) {
  PyObjectPtr str(PyUnicode_FromString("a.b"));
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_RPartition(str, sep));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyTuple_CheckExact(result));
}

TEST_F(UnicodeExtensionApiTest, SplitWithNonStrStrRaisesTypeError) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_Split(Py_None, sep, 5));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, SplitWithNonStrSepRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("hello"));
  PyObjectPtr sep(PyLong_FromLong(8));
  PyObjectPtr result(PyUnicode_Split(str, sep, 5));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, SplitReturnsList) {
  PyObjectPtr str(PyUnicode_FromString("a.b"));
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_Split(str, sep, 5));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
}

TEST_F(UnicodeExtensionApiTest, RSplitWithNonStrStrRaisesTypeError) {
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_RSplit(Py_None, sep, 5));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, RSplitWithNonStrSepRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("hello"));
  PyObjectPtr sep(PyLong_FromLong(8));
  PyObjectPtr result(PyUnicode_RSplit(str, sep, 5));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(UnicodeExtensionApiTest, RSplitReturnsList) {
  PyObjectPtr str(PyUnicode_FromString("a.b"));
  PyObjectPtr sep(PyUnicode_FromString("."));
  PyObjectPtr result(PyUnicode_RSplit(str, sep, 5));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
}

TEST_F(UnicodeExtensionApiTest, StrlenWithEmptyStrReturnsZero) {
  const wchar_t* str = L"";
  ASSERT_EQ(Py_UNICODE_strlen(str), 0U);
}

TEST_F(UnicodeExtensionApiTest, StrlenWithStrReturnsNumberOfChars) {
  const wchar_t* str = L"hello";
  ASSERT_EQ(Py_UNICODE_strlen(str), 5U);
}

TEST_F(UnicodeExtensionApiTest, SubstringWithNegativeStartRaisesIndexError) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  ASSERT_EQ(PyUnicode_Substring(str, -1, 3), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(UnicodeExtensionApiTest, SubstringWithNegativeEndRaisesIndexError) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  ASSERT_EQ(PyUnicode_Substring(str, 0, -3), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(UnicodeExtensionApiTest, SubstringWithFullStringReturnsSameObject) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  PyObjectPtr result(PyUnicode_Substring(str, 0, 5));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, str);
}

TEST_F(UnicodeExtensionApiTest, SubstringWithSameStartAndEndReturnsEmpty) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  PyObjectPtr result(PyUnicode_Substring(str, 2, 2));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_CheckExact(result));
  EXPECT_STREQ(PyUnicode_AsUTF8(result), "");
}

TEST_F(UnicodeExtensionApiTest, SubstringWithASCIIReturnsSubstring) {
  PyObjectPtr str(PyUnicode_FromString("Hello world!"));
  PyObjectPtr result(PyUnicode_Substring(str, 3, 8));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_CheckExact(result));
  EXPECT_STREQ(PyUnicode_AsUTF8(result), "lo wo");
}

TEST_F(UnicodeExtensionApiTest, SubstringCountsCodePoints) {
  PyObjectPtr str(PyUnicode_FromString("cre\u0300me bru\u0302le\u0301e"));
  PyObjectPtr result(PyUnicode_Substring(str, 2, 11));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_CheckExact(result));
  EXPECT_STREQ(PyUnicode_AsUTF8(result), "e\u0300me bru\u0302");
}

TEST_F(UnicodeExtensionApiTest, NewWithInvalidSizeReturnsError) {
  PyObjectPtr str(PyUnicode_New(-1, 0));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(UnicodeExtensionApiTest, NewWithInvalidMaxCharReturnsError) {
  PyObjectPtr str(PyUnicode_New(1, 0x11FFFF));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(UnicodeExtensionApiTest, NewWithZeroSizeAndInvalidMaxCharReturnsStr) {
  PyObjectPtr empty(PyUnicode_New(0, 0x11FFFF));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyUnicode_CheckExact(empty));
  EXPECT_TRUE(isUnicodeEqualsCStr(empty, ""));
}

TEST_F(UnicodeExtensionApiTest, FromKindAndDataWithNegativeOneRaiseError) {
  char c = 'a';
  PyObject* empty(PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, &c, -1));
  EXPECT_EQ(empty, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(UnicodeExtensionApiTest, FromKindAndDataWithInvalidKindRaiseError) {
  char c = 'a';
  PyObject* empty(PyUnicode_FromKindAndData(100, &c, 1));
  EXPECT_EQ(empty, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(UnicodeExtensionApiTest,
       FromKindAndDataWithOneByteKindAndASCIICodePointsReturnsStr) {
  Py_UCS1 buffer[] = {'h', 'e', 'l', 'l', 'o'};
  PyObject* str(PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, buffer,
                                          Py_ARRAY_LENGTH(buffer)));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_CheckExact(str));
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(str, "hello"));
}

TEST_F(UnicodeExtensionApiTest,
       FromKindAndDataWithOneByteKindAndLatin1CodePointsReturnsStr) {
  Py_UCS1 buffer[] = {'h', 0xe4, 'l', 'l', 'o'};
  PyObject* str(PyUnicode_FromKindAndData(PyUnicode_1BYTE_KIND, buffer,
                                          Py_ARRAY_LENGTH(buffer)));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_CheckExact(str));
  EXPECT_STREQ(PyUnicode_AsUTF8(str), "h\xc3\xa4llo");
}

TEST_F(UnicodeExtensionApiTest,
       FromKindAndDataWithTwoByteKindAndBMPCodePointsReturnsStr) {
  Py_UCS2 buffer[] = {'h', 0xe4, 'l', 0x2cc0, 'o'};
  PyObject* str(PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, buffer,
                                          Py_ARRAY_LENGTH(buffer)));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_CheckExact(str));
  EXPECT_STREQ(PyUnicode_AsUTF8(str), "h\xc3\xa4l\xe2\xb3\x80o");
}

TEST_F(UnicodeExtensionApiTest,
       FromKindAndDataWithFourByteKindAndNonBMPCodePointsReturnsStr) {
  Py_UCS4 buffer[] = {0x1f192, 'h', 0xe4, 'l', 0x2cc0};
  PyObject* str(PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, buffer,
                                          Py_ARRAY_LENGTH(buffer)));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyUnicode_CheckExact(str));
  EXPECT_STREQ(PyUnicode_AsUTF8(str), "\xf0\x9f\x86\x92h\xc3\xa4l\xe2\xb3\x80");
}

}  // namespace python
