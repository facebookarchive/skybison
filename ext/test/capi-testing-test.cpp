#include "gtest/gtest.h"

#include <limits>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using ExtensionApiTestingUtilsTest = ExtensionApi;

TEST_F(ExtensionApiTestingUtilsTest, ImportNonExistingModuleReturnsNull) {
  PyObject* pyname = PyUnicode_FromString("foo");
  EXPECT_EQ(testing::importGetModule(pyname), nullptr);
  Py_DECREF(pyname);
}

TEST_F(ExtensionApiTestingUtilsTest, ImportExistingModuleReturnsModule) {
  const char* c_name = "sys";
  PyObject* pyname = PyUnicode_FromString(c_name);
  PyObject* sysmodule = testing::importGetModule(pyname);
  ASSERT_NE(sysmodule, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(sysmodule));
  Py_DECREF(pyname);

  PyObject* sysmodule_name = PyModule_GetNameObject(sysmodule);
  char* c_sysmodule_name = PyUnicode_AsUTF8(sysmodule_name);
  EXPECT_STREQ(c_sysmodule_name, c_name);
  Py_DECREF(sysmodule_name);
  Py_DECREF(sysmodule);
}

TEST_F(ExtensionApiTestingUtilsTest, IsLongEqualsLong) {
  PyObjectPtr ten(PyLong_FromLong(10));

  ::testing::AssertionResult ok = isLongEqualsLong(ten, 10);
  EXPECT_TRUE(ok);

  ::testing::AssertionResult bad_value = isLongEqualsLong(ten, 24);
  ASSERT_FALSE(bad_value);
  EXPECT_STREQ(bad_value.message(), "10 is not equal to 24");

  PyObjectPtr max_long(PyLong_FromLong(std::numeric_limits<long>::max()));
  PyObjectPtr big_long(PyNumber_Multiply(max_long, ten));
  ::testing::AssertionResult bad_big_value = isLongEqualsLong(big_long, 1234);
  ASSERT_FALSE(bad_big_value);
  EXPECT_STREQ(bad_big_value.message(),
               "92233720368547758070 is not equal to 1234");

  PyObjectPtr string(PyUnicode_FromString("hello, there!"));
  ::testing::AssertionResult bad_type = isLongEqualsLong(string, 5678);
  ASSERT_FALSE(bad_type);
  EXPECT_STREQ(bad_type.message(), "'hello, there!' is not equal to 5678");
}

}  // namespace py
