#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using ComplexExtensionApiTest = ExtensionApi;

TEST_F(ComplexExtensionApiTest, AsCComplexWithComplexReturnsValue) {
  PyObjectPtr cmp(PyComplex_FromDoubles(1.0, 0.0));
  Py_complex result = PyComplex_AsCComplex(cmp);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result.real, 1.0);
  EXPECT_EQ(result.imag, 0.0);
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

}  // namespace python
