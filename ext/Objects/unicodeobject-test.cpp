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

} // namespace python
