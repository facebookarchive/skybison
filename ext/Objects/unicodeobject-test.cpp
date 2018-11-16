#include "capi-fixture.h"

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
  std::free(cstring);
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
  std::free(cstring);
}

}  // namespace python
