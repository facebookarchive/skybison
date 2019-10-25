#include "gtest/gtest.h"

#include <cerrno>
#include <cmath>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using ComplexExtensionApiTest = ExtensionApi;

TEST_F(ComplexExtensionApiTest, PyCAbsReturnsComplexAbsoluteValue) {
  EXPECT_EQ(_Py_c_abs({12, 0}), 12);
  EXPECT_EQ(_Py_c_abs({0, 34}), 34);
  EXPECT_EQ(_Py_c_abs({INFINITY, 56}), INFINITY);
  EXPECT_EQ(_Py_c_abs({-INFINITY, 78}), INFINITY);
  EXPECT_EQ(_Py_c_abs({90, INFINITY}), INFINITY);
  EXPECT_EQ(_Py_c_abs({123, -INFINITY}), INFINITY);
  EXPECT_TRUE(std::isnan(_Py_c_abs({456, NAN})));
  EXPECT_TRUE(std::isnan(_Py_c_abs({NAN, 789})));
}

TEST_F(ComplexExtensionApiTest, PyCDiffReturnsComplexDifference) {
  Py_complex diff = _Py_c_diff({2.0, 5.0}, {4.0, -3.0});
  EXPECT_EQ(diff.real, -2.0);
  EXPECT_EQ(diff.imag, 8.0);
}

TEST_F(ComplexExtensionApiTest, PyCNegReturnsComplexNegation) {
  Py_complex neg = _Py_c_neg({-123.0, 456.0});
  EXPECT_EQ(neg.real, 123.0);
  EXPECT_EQ(neg.imag, -456);
}

TEST_F(ComplexExtensionApiTest, PyCProdReturnsComplexProduct) {
  Py_complex prod = _Py_c_prod({1.0, -2.0}, {-3.0, 4.0});
  EXPECT_EQ(prod.real, 5.0);
  EXPECT_EQ(prod.imag, 10.0);
}

TEST_F(ComplexExtensionApiTest, PyCQuotReturnsComplexQuotient) {
  // rhs.real > rhs.imag
  errno = 0;
  Py_complex q1 = _Py_c_quot({10, 20}, {2, 1});
  EXPECT_EQ(errno, 0);
  EXPECT_EQ(q1.real, 8.0);
  EXPECT_EQ(q1.imag, 6.0);

  // rhs.imag > rhs.real
  errno = 0;
  Py_complex q2 = _Py_c_quot({10, 20}, {1, 2});
  EXPECT_EQ(errno, 0);
  EXPECT_EQ(q2.real, 10.0);
  EXPECT_EQ(q2.imag, 0.0);

  // rhs.real = 0
  errno = 0;
  Py_complex q3 = _Py_c_quot({10, 10}, {0, 0});
  EXPECT_EQ(errno, EDOM);
  EXPECT_EQ(q3.real, 0.0);
  EXPECT_EQ(q3.imag, 0.0);

  // NAN
  errno = 0;
  Py_complex q4 = _Py_c_quot({1, 2}, {NAN, 4});
  EXPECT_EQ(errno, 0);
  EXPECT_TRUE(std::isnan(q4.real));
  EXPECT_TRUE(std::isnan(q4.imag));
}

TEST_F(ComplexExtensionApiTest, PyCSumReturnsComplexSum) {
  Py_complex sum = _Py_c_sum({2.0, 5.0}, {4.0, -3.0});
  EXPECT_EQ(sum.real, 6.0);
  EXPECT_EQ(sum.imag, 2.0);
}

TEST_F(ComplexExtensionApiTest, AsCComplexWithComplexReturnsValue) {
  PyObjectPtr cmp(PyComplex_FromDoubles(1.0, 0.0));
  Py_complex result = PyComplex_AsCComplex(cmp);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result.real, 1.0);
  EXPECT_EQ(result.imag, 0.0);
}

TEST_F(ComplexExtensionApiTest,
       AsComplexWithRaisingDescriptorPropagatesException) {
  PyRun_SimpleString(R"(
class Desc:
  def __get__(self, owner, fn):
    raise UserWarning("foo")
  def __call__(self, *args, **kwargs):
    raise "foo"
class Foo:
  __complex__ = Desc()
foo = Foo()
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  Py_complex result = PyComplex_AsCComplex(foo);
  EXPECT_EQ(result.real, -1.0);
  EXPECT_EQ(result.imag, 0.0);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

TEST_F(ComplexExtensionApiTest,
       AsComplexWithMistypedDunderComplexRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo:
  def __complex__(self):
    return 1
foo = Foo()
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  Py_complex result = PyComplex_AsCComplex(foo);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  ASSERT_EQ(result.real, -1.0);
  ASSERT_EQ(result.imag, 0.0);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ComplexExtensionApiTest, AsComplexWithDunderComplexReturnsValue) {
  PyRun_SimpleString(R"(
class Foo:
  def __complex__(self):
    return 1+0j
foo = Foo()
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  Py_complex result = PyComplex_AsCComplex(foo);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result.real, 1.0);
  EXPECT_EQ(result.imag, 0.0);
}

TEST_F(ComplexExtensionApiTest, AsCComplexWithFloatReturnsRealValue) {
  PyObjectPtr flt(PyFloat_FromDouble(1.0));
  Py_complex result = PyComplex_AsCComplex(flt);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result.real, 1.0);
  EXPECT_EQ(result.imag, 0.0);
}

TEST_F(ComplexExtensionApiTest, FromDoublesReturnsComplex) {
  PyObjectPtr cmp(PyComplex_FromDoubles(0.0, 0.0));
  EXPECT_TRUE(PyComplex_CheckExact(cmp));
}

TEST_F(ComplexExtensionApiTest, FromCComplexReturnsComplex) {
  Py_complex c = {1.0, 0.0};
  PyObjectPtr cmp(PyComplex_FromCComplex(c));
  EXPECT_TRUE(PyComplex_CheckExact(cmp));
}

TEST_F(ComplexExtensionApiTest, ImagAsDoubleWithComplexReturnsValue) {
  PyObjectPtr cmp(PyComplex_FromDoubles(0.0, 1.0));
  double result = PyComplex_ImagAsDouble(cmp);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 1.0);
}

TEST_F(ComplexExtensionApiTest, ImagAsDoubleWithNonComplexReturnsZero) {
  PyObjectPtr flt(PyFloat_FromDouble(1.0));
  double result = PyComplex_ImagAsDouble(flt);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 0.0);
}

TEST_F(ComplexExtensionApiTest, RealAsDoubleWithComplexReturnsValue) {
  PyObjectPtr cmp(PyComplex_FromDoubles(1.0, 0.0));
  double result = PyComplex_RealAsDouble(cmp);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 1.0);
}

TEST_F(ComplexExtensionApiTest, RealAsDoubleWithFloatReturnsFloatValue) {
  PyObjectPtr flt(PyFloat_FromDouble(1.0));
  double result = PyComplex_RealAsDouble(flt);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 1.0);
}

TEST_F(ComplexExtensionApiTest, RealAsDoubleWithNonFloatRaisesTypeError) {
  PyObjectPtr foo(PyTuple_New(0));
  double result = PyComplex_RealAsDouble(foo);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  ASSERT_EQ(result, -1.0);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace py
