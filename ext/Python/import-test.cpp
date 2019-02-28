#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ImportExtensionApiTest = ExtensionApi;

TEST_F(ImportExtensionApiTest, AddNonExistingModuleReturnsNewModule) {
  const char* c_name = "foo";
  PyObject* pyname = PyUnicode_FromString(c_name);
  PyObject* new_module = PyImport_AddModuleObject(pyname);
  ASSERT_TRUE(PyModule_CheckExact(new_module));

  PyObject* module_name = PyModule_GetNameObject(new_module);
  EXPECT_STREQ(PyUnicode_AsUTF8(module_name), c_name);
  Py_DECREF(module_name);

  PyObject* module = testing::importGetModule(pyname);
  EXPECT_EQ(new_module, module);
  Py_DECREF(pyname);
  Py_DECREF(module);
}

TEST_F(ImportExtensionApiTest, AddExistingModuleReturnsModule) {
  PyObject* pyname = PyUnicode_FromString("sys");
  PyObject* module = PyImport_AddModuleObject(pyname);
  ASSERT_TRUE(PyModule_CheckExact(module));

  Py_ssize_t refcnt = Py_REFCNT(module);
  PyObject* module2 = PyImport_AddModuleObject(pyname);
  EXPECT_EQ(refcnt, Py_REFCNT(module2));
  EXPECT_EQ(module, module2);
  Py_DECREF(pyname);
}

TEST_F(ImportExtensionApiTest, PyImportAcquireLockAndReleaseLockDoesNothing) {
  _PyImport_AcquireLock();
  EXPECT_EQ(_PyImport_ReleaseLock(), 1);
}

TEST_F(ImportExtensionApiTest,
       PyImportReleaseLockWithoutAcquireLockReturnsMinusOne) {
  EXPECT_EQ(_PyImport_ReleaseLock(), -1);
}

}  // namespace python
