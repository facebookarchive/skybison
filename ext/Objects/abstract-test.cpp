#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using AbstractExtensionApiTest = ExtensionApi;

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnIntReturnsSelf) {
  PyObject* pylong = PyLong_FromLong(666);
  EXPECT_EQ(pylong, PyNumber_Index(pylong));
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexCallsIndex) {
  PyRun_SimpleString(R"(
class IntLikeClass:
  def __index__(self):
    return 42;

i = IntLikeClass();
  )");
  PyObject* i = testing::moduleGet("__main__", "i");
  PyObject* index = PyNumber_Index(i);
  EXPECT_NE(index, nullptr);
  EXPECT_TRUE(PyLong_CheckExact(index));
  EXPECT_EQ(42, PyLong_AsLong(index));
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnNonInt) {
  PyObject* str = PyUnicode_FromString("not an int");
  EXPECT_EQ(PyNumber_Index(str), nullptr);
  // TODO(T34841408): check the error message
  EXPECT_NE(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexWithIndexReturningNonInt) {
  PyRun_SimpleString(R"(
class IntLikeClass:
  def __index__(self):
    return "not an int";

i = IntLikeClass();
  )");
  PyObject* i = testing::moduleGet("__main__", "i");
  EXPECT_EQ(PyNumber_Index(i), nullptr);
  // TODO(T34841408): check the error message
  EXPECT_NE(PyErr_Occurred(), nullptr);
}

}  // namespace python
