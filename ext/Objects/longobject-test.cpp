#include "capi-fixture.h"
#include "capi-handles.h"
#include "capi-testing.h"

namespace python {

using LongExtensionApiTest = ExtensionApi;

TEST_F(LongExtensionApiTest, PyLongCheckOnInt) {
  PyObject* pylong = PyLong_FromLong(10);
  EXPECT_TRUE(PyLong_Check(pylong));
  EXPECT_TRUE(PyLong_CheckExact(pylong));
}

TEST_F(LongExtensionApiTest, PyLongCheckOnType) {
  PyObject* pylong = PyLong_FromLong(10);
  PyObject* type = ApiHandle::fromPyObject(pylong)->type();
  EXPECT_FALSE(PyLong_Check(type));
  EXPECT_FALSE(PyLong_CheckExact(type));
}

TEST_F(LongExtensionApiTest, AsLongWithNullReturnsNegative) {
  long res = PyLong_AsLong(nullptr);
  EXPECT_EQ(res, -1);

  const char* expected_message = "bad argument to internal function";
  EXPECT_TRUE(testing::exceptionMessageMatches(expected_message));
}

TEST_F(LongExtensionApiTest, AsLongWithNonIntegerReturnsNegative) {
  long res = PyLong_AsLong(Py_None);
  EXPECT_EQ(res, -1);
  // TODO: Add exception checks once PyLong_AsLong is fully implemented
}

TEST_F(LongExtensionApiTest, FromLongReturnsLong) {
  const int val = 10;
  PyObject* pylong = PyLong_FromLong(val);
  ASSERT_EQ(nullptr, PyErr_Occurred());

  long res = PyLong_AsLong(pylong);
  EXPECT_EQ(res, val);
}

}  // namespace python
