#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(BoolObject, ConvertLongToBool) {
  Runtime runtime;
  HandleScope scope;

  // Test True
  PyObject* pybool_true = PyBool_FromLong(1);
  Handle<Object> bool_true(
      &scope, ApiHandle::fromPyObject(pybool_true)->asObject());
  ASSERT_TRUE(bool_true->isBoolean());
  EXPECT_TRUE(Boolean::cast(*bool_true)->value());

  // Test False
  PyObject* pybool_false = PyBool_FromLong(0);
  Handle<Object> bool_false(
      &scope, ApiHandle::fromPyObject(pybool_false)->asObject());
  ASSERT_TRUE(bool_false->isBoolean());
  EXPECT_FALSE(Boolean::cast(*bool_false)->value());
}

TEST(BoolObject, CheckBoolIdentity) {
  Runtime runtime;

  // Test Identitiy
  PyObject* pybool_true = PyBool_FromLong(1);
  PyObject* pybool1 = PyBool_FromLong(2);
  EXPECT_EQ(pybool_true, pybool1);

  PyObject* pybool_false = PyBool_FromLong(0);
  PyObject* pybool2 = PyBool_FromLong(0);
  EXPECT_EQ(pybool_false, pybool2);
}

} // namespace python
