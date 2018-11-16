#include "gtest/gtest.h"

#include "Python.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(UnicodeObject, AsUTF8FromNonStringReturnsNull) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> integer_obj(&scope, runtime.newInt(15));
  PyObject* pylong = ApiHandle::fromObject(*integer_obj)->asPyObject();

  // Pass a non string object
  char* cstring = PyUnicode_AsUTF8AndSize(pylong, nullptr);
  EXPECT_EQ(nullptr, cstring);
}

TEST(UnicodeObject, AsUTF8WithNullSizeReturnsCString) {
  Runtime runtime;
  HandleScope scope;

  const char* str = "Some C String";
  Handle<String> string_obj(&scope, runtime.newStringFromCString(str));
  PyObject* pyunicode = ApiHandle::fromObject(*string_obj)->asPyObject();

  // Pass a nullptr size
  char* cstring = PyUnicode_AsUTF8AndSize(pyunicode, nullptr);
  ASSERT_NE(nullptr, cstring);
  EXPECT_STREQ(str, cstring);
  std::free(cstring);
}

TEST(UnicodeObject, AsUTF8WithReferencedSizeReturnsCString) {
  Runtime runtime;
  HandleScope scope;

  const char* str = "Some C String";
  Handle<String> string_obj(&scope, runtime.newStringFromCString(str));
  PyObject* pyunicode = ApiHandle::fromObject(*string_obj)->asPyObject();

  // Pass a size reference
  Py_ssize_t size = 0;
  char* cstring = PyUnicode_AsUTF8AndSize(pyunicode, &size);
  ASSERT_NE(nullptr, cstring);
  EXPECT_STREQ(str, cstring);
  EXPECT_EQ(size, strlen(str));
  std::free(cstring);
}

}  // namespace python
