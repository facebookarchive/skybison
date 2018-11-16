#include "gtest/gtest.h"

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
  EXPECT_EQ(size, strlen(str));

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

TEST_F(UnicodeExtensionApiTest, ClearFreeListReturnsZero) {
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

  const char* expected_message =
      "Negative size passed to PyUnicode_FromStringAndSize";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));
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
  EXPECT_EQ(0, PyUnicode_READY(pyunicode));
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
  PyObject* s = PyUnicode_FromString("this is a string");
  PyObject* l = PyLong_FromLong(1234);

  int result = PyUnicode_Compare(s, l);
  EXPECT_TRUE(
      testing::exceptionValueMatches("Can't compare largestr and smallint"));
  PyErr_Clear();

  result = PyUnicode_Compare(l, s);
  EXPECT_TRUE(
      testing::exceptionValueMatches("Can't compare smallint and largestr"));
  PyErr_Clear();

  result = PyUnicode_Compare(l, l);
  EXPECT_TRUE(
      testing::exceptionValueMatches("Can't compare smallint and smallint"));
  PyErr_Clear();

  Py_DECREF(l);
  Py_DECREF(s);
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
}  // namespace python
