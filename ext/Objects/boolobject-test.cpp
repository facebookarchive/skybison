#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using BoolExtensionApiTest = ExtensionApi;

TEST_F(BoolExtensionApiTest, ConvertLongToBool) {
  // Test True
  testing::PyObjectPtr pybool_true(PyBool_FromLong(1));
  EXPECT_EQ(pybool_true, Py_True);

  // Test False
  testing::PyObjectPtr pybool_false(PyBool_FromLong(0));
  EXPECT_EQ(pybool_false, Py_False);
}

TEST_F(BoolExtensionApiTest, CheckBoolIdentity) {
  // Test Identitiy
  testing::PyObjectPtr pybool_true(PyBool_FromLong(1));
  testing::PyObjectPtr pybool1(PyBool_FromLong(2));
  EXPECT_EQ(pybool_true, pybool1);

  testing::PyObjectPtr pybool_false(PyBool_FromLong(0));
  testing::PyObjectPtr pybool2(PyBool_FromLong(0));
  EXPECT_EQ(pybool_false, pybool2);
}

TEST_F(BoolExtensionApiTest, TestPyReturnTrueReturnsTrue) {
  PyObjectPtr module(PyModule_New("mod"));
  binaryfunc meth = [](PyObject*, PyObject*) { Py_RETURN_TRUE; };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_EQ(result, Py_True);
}

TEST_F(BoolExtensionApiTest, TestPyReturnTrueReturnsFalse) {
  PyObjectPtr module(PyModule_New("mod"));
  binaryfunc meth = [](PyObject*, PyObject*) { Py_RETURN_FALSE; };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_EQ(result, Py_False);
}

}  // namespace testing
}  // namespace py
