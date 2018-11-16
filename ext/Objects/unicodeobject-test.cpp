#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

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

TEST_F(UnicodeExtensionApiTest, ReadyReturnsZero) {
  PyObject* pyunicode = PyUnicode_FromString("some string");
  EXPECT_EQ(0, PyUnicode_READY(pyunicode));
}

}  // namespace python
