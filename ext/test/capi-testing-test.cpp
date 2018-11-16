#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

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

}  // namespace python
