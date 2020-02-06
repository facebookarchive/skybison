#include <cfloat>

#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using FloatExtensionApiTest = ExtensionApi;

TEST_F(FloatExtensionApiTest, FromDoubleReturnsFloat) {
  const double val = 15.4;
  PyObjectPtr flt(PyFloat_FromDouble(val));
  ASSERT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), val);
}

TEST_F(FloatExtensionApiTest, NegativeFromDoubleReturnsFloat) {
  const double val = -10000.123;
  PyObjectPtr flt(PyFloat_FromDouble(val));
  EXPECT_TRUE(PyFloat_CheckExact(flt));
  EXPECT_EQ(PyFloat_AsDouble(flt), val);
}

TEST_F(FloatExtensionApiTest, AsDoubleFromNullRaisesException) {
  double res = PyFloat_AsDouble(nullptr);
  EXPECT_EQ(res, -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, AsDoubleFromNonFloatRaisesException) {
  PyObjectPtr list(PyList_New(0));
  double res = PyFloat_AsDouble(list);
  EXPECT_EQ(res, -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(FloatExtensionApiTest, AsDoubleCallsDunderFloat) {
  PyRun_SimpleString(R"(
class FloatLikeClass:
  def __float__(self):
    return 1.5

f = FloatLikeClass();
  )");
  PyObjectPtr f(testing::moduleGet("__main__", "f"));
  double res = PyFloat_AsDouble(f);
  EXPECT_EQ(res, 1.5);
}

TEST_F(FloatExtensionApiTest, AsDoubleWithDunderFloatPropagatesException) {
  PyRun_SimpleString(R"(
class FloatLikeClass:
  @property
  def __float__(self):
    raise KeyError

f = FloatLikeClass();
  )");
  PyObjectPtr f(moduleGet("__main__", "f"));
  EXPECT_EQ(PyFloat_AsDouble(f), -1.0);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
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
  PyObjectPtr subfloat(testing::moduleGet("__main__", "subfloat"));
  ASSERT_FALSE(PyLong_CheckExact(subfloat));
  ASSERT_TRUE(PyFloat_Check(subfloat));
  EXPECT_EQ(1.5, PyFloat_AsDouble(subfloat));

  PyObjectPtr subfloat_foo(testing::moduleGet("__main__", "subfloat_foo"));
  ASSERT_TRUE(PyLong_Check(subfloat_foo));
  EXPECT_EQ(3, PyLong_AsLong(subfloat_foo));
}

TEST_F(FloatExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyFloat_ClearFreeList(), 0);
}

TEST_F(FloatExtensionApiTest, GetMaxReturnsDblMax) {
  EXPECT_EQ(PyFloat_GetMax(), DBL_MAX);
}

TEST_F(FloatExtensionApiTest, GetMinReturnsDblMin) {
  EXPECT_EQ(PyFloat_GetMin(), DBL_MIN);
}

TEST_F(FloatExtensionApiTest, Pack2) {
  double expected = 1.5;
  unsigned char ptr[2] = {};
  ASSERT_EQ(_PyFloat_Pack2(expected, ptr, /* le */ true), 0);
  // 00000000 00111110
  ASSERT_EQ(ptr[0], 0);
  ASSERT_EQ(ptr[1], 62);
  EXPECT_EQ(_PyFloat_Unpack2(ptr, /* le */ true), 1.5);
}

TEST_F(FloatExtensionApiTest, Pack4) {
  double expected = 1.5;
  unsigned char ptr[4] = {};
  ASSERT_EQ(_PyFloat_Pack4(expected, ptr, /* le */ true), 0);
  // 0000000 0000000 11000000 00111111
  ASSERT_EQ(ptr[0], 0);
  ASSERT_EQ(ptr[1], 0);
  ASSERT_EQ(ptr[2], 192);
  ASSERT_EQ(ptr[3], 63);
  EXPECT_EQ(_PyFloat_Unpack4(ptr, /* le */ true), 1.5);
}

TEST_F(FloatExtensionApiTest, Pack8) {
  double expected = 1.5;
  unsigned char ptr[8] = {};
  ASSERT_EQ(_PyFloat_Pack8(expected, ptr, /* le */ true), 0);
  // 0000000 0000000 0000000 0000000 0000000 0000000 11111000 00111111
  ASSERT_EQ(ptr[0], 0);
  ASSERT_EQ(ptr[1], 0);
  ASSERT_EQ(ptr[2], 0);
  ASSERT_EQ(ptr[3], 0);
  ASSERT_EQ(ptr[4], 0);
  ASSERT_EQ(ptr[5], 0);
  ASSERT_EQ(ptr[6], 248);
  ASSERT_EQ(ptr[7], 63);
  EXPECT_EQ(_PyFloat_Unpack8(ptr, /* le */ true), 1.5);
}

}  // namespace testing
}  // namespace py
