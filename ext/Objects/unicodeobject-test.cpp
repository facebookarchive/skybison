#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(ExtensionTest, _PyUnicode_FromId) {
  Runtime runtime;
  HandleScope scope;

  _Py_IDENTIFIER(__name__);
  const char* str = "__name__";
  PyObject* pyunicode = _PyUnicode_FromId(&PyId___name__);
  Handle<Object> string_obj(&scope, runtime.asObject(pyunicode));
  EXPECT_TRUE(string_obj->isString());
  EXPECT_TRUE(String::cast(*string_obj)->equalsCString(str));
}

TEST(ExtensionTest, PyUnicode_AsUTF8AndSize) {
  Runtime runtime;
  HandleScope scope;

  // Pass a non string object
  Handle<Object> integer_obj(&scope, runtime.newInteger(15));
  PyObject* pylong = runtime.asPyObject(*integer_obj);
  char* null_cstring2 = PyUnicode_AsUTF8AndSize(pylong, nullptr);
  EXPECT_EQ(nullptr, null_cstring2);

  const char* str = "Some C String";
  Handle<String> string_obj(&scope, runtime.newStringFromCString(str));
  PyObject* pyunicode = runtime.asPyObject(*string_obj);

  // Pass a nullptr size
  char* cstring = PyUnicode_AsUTF8AndSize(pyunicode, nullptr);
  EXPECT_STREQ(str, cstring);
  std::free(cstring);

  // Pass a size reference
  Py_ssize_t size = 0;
  char* cstring2 = PyUnicode_AsUTF8AndSize(pyunicode, &size);
  EXPECT_STREQ(str, cstring2);
  EXPECT_EQ(size, strlen(str));
  std::free(cstring2);
}

} // namespace python
