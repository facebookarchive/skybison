#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using FloatExtensionApiTest = ExtensionApi;

TEST_F(FloatExtensionApiTest, FromDoubleReturnsFloat) {
  const double val = 15.4;
  PyObject* flt = PyFloat_FromDouble(val);
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), val);
}

TEST_F(FloatExtensionApiTest, NegativeFromDoubleReturnsFloat) {
  const double val = -10000.123;
  PyObject* flt = PyFloat_FromDouble(val);
  EXPECT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), val);
}

TEST_F(FloatExtensionApiTest, AsDoubleFromNullThrowsException) {
  auto res = PyFloat_AsDouble(nullptr);
  EXPECT_EQ(res, -1);

  const char* expected_message = "bad argument to internal function";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));
}

TEST_F(FloatExtensionApiTest, AsDoubleFromNonFloatThrowsException) {
  PyObject* list = PyList_New(0);
  auto res = PyFloat_AsDouble(list);
  EXPECT_EQ(res, -1);

  const char* expected_message = "must be a real number";
  EXPECT_TRUE(testing::exceptionValueMatches(expected_message));
}

TEST_F(FloatExtensionApiTest, AsDoubleCallsDunderFloat) {
  PyRun_SimpleString(R"(
class FloatLikeClass:
  def __float__(self):
    return 1.5

f = FloatLikeClass();
  )");
  PyObject* f = testing::moduleGet("__main__", "f");
  double res = PyFloat_AsDouble(f);
  EXPECT_EQ(res, 1.5);
}

TEST_F(FloatExtensionApiTest, AsDoubleFromFloatSubclassReturnsFloat) {
  PyRun_SimpleString(R"(
class SubFloat(float):
  def __new__(self, value):
    self.foo = 3
    return super().__new__(self, value)
subfloat = SubFloat(1.5)
subfloat_foo = subfloat.foo
  )");
  PyObject* subfloat = testing::moduleGet("__main__", "subfloat");
  ASSERT_FALSE(PyLong_CheckExact(subfloat));
  ASSERT_TRUE(PyFloat_Check(subfloat));
  EXPECT_EQ(1.5, PyFloat_AsDouble(subfloat));

  PyObject* subfloat_foo = testing::moduleGet("__main__", "subfloat_foo");
  ASSERT_TRUE(PyLong_Check(subfloat_foo));
  EXPECT_EQ(3, PyLong_AsLong(subfloat_foo));
}

TEST_F(FloatExtensionApiTest, ClearFreeListReturnsZero) {
  EXPECT_EQ(PyFloat_ClearFreeList(), 0);
}

}  // namespace python
