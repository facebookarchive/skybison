#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using BoolExtensionApiTest = ExtensionApi;

TEST_F(BoolExtensionApiTest, ConvertLongToBool) {
  // Test True
  PyObject* pybool_true = PyBool_FromLong(1);
  EXPECT_EQ(pybool_true, Py_True);

  // Test False
  PyObject* pybool_false = PyBool_FromLong(0);
  EXPECT_EQ(pybool_false, Py_False);
}

TEST_F(BoolExtensionApiTest, CheckBoolIdentity) {
  // Test Identitiy
  PyObject* pybool_true = PyBool_FromLong(1);
  PyObject* pybool1 = PyBool_FromLong(2);
  EXPECT_EQ(pybool_true, pybool1);

  PyObject* pybool_false = PyBool_FromLong(0);
  PyObject* pybool2 = PyBool_FromLong(0);
  EXPECT_EQ(pybool_false, pybool2);
}

}  // namespace python
