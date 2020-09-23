#include "complex-builtins.h"

#include <cmath>

#include "gtest/gtest.h"

#include "builtins.h"
#include "frame.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using ComplexBuiltinsTest = RuntimeFixture;

TEST_F(ComplexBuiltinsTest, AddWithIntReturnsComplex) {
  HandleScope scope(thread_);
  Complex c(&scope, runtime_->newComplex(1, 2));
  Int i(&scope, runtime_->newInt(10));
  Object result_obj(&scope, runBuiltin(METH(complex, __add__), c, i));
  ASSERT_FALSE(result_obj.isError());
  Complex result(&scope, *result_obj);
  EXPECT_EQ(result.real(), 11);
  EXPECT_EQ(result.imag(), 2);
}

TEST_F(ComplexBuiltinsTest, IntAddWithComplexReturnsComplex) {
  HandleScope scope(thread_);
  Int i(&scope, runtime_->newInt(10));
  Complex c(&scope, runtime_->newComplex(1, 2));
  Object result_obj(&scope, Interpreter::binaryOperation(
                                thread_, Interpreter::BinaryOp::ADD, i, c));
  ASSERT_FALSE(result_obj.isError());
  Complex result(&scope, *result_obj);
  EXPECT_EQ(result.real(), 11);
  EXPECT_EQ(result.imag(), 2);
}

TEST_F(ComplexBuiltinsTest, DunderReprHasRealAndImag) {
  ASSERT_FALSE(runFromCStr(runtime_, "result = repr(complex(1, 2))").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "(1.0+2.0j)"));
}

TEST_F(ComplexBuiltinsTest, BuiltinBaseIsComplex) {
  HandleScope scope(thread_);
  Type complex_type(&scope, runtime_->typeAt(LayoutId::kComplex));
  EXPECT_EQ(complex_type.builtinBase(), LayoutId::kComplex);
}

TEST_F(ComplexBuiltinsTest, ComplexMultipliesComplex) {
  HandleScope scope(thread_);
  Complex c1(&scope, runtime_->newComplex(1, 2));
  Complex c2(&scope, runtime_->newComplex(-1.5, 5));
  Object result_obj(&scope, runBuiltin(METH(complex, __mul__), c1, c2));
  ASSERT_FALSE(result_obj.isError());
  Complex result(&scope, *result_obj);
  EXPECT_EQ(result.real(), -11.5);
  EXPECT_EQ(result.imag(), 2);
}

TEST_F(ComplexBuiltinsTest, ComplexDividesComplexRealSmallerThanImag) {
  HandleScope scope(thread_);
  Complex c1(&scope, runtime_->newComplex(-1, 2));
  Complex c2(&scope, runtime_->newComplex(1, 2));
  Object result_obj(&scope, runBuiltin(METH(complex, __truediv__), c1, c2));
  ASSERT_FALSE(result_obj.isError());
  Complex result(&scope, *result_obj);
  EXPECT_EQ(result.real(), 0.6);
  EXPECT_EQ(result.imag(), 0.8);
}

TEST_F(ComplexBuiltinsTest, ComplexDividesComplexRealLargerThanImag) {
  HandleScope scope(thread_);
  Complex c1(&scope, runtime_->newComplex(-1, 2));
  Complex c2(&scope, runtime_->newComplex(2, 1));
  Object result_obj(&scope, runBuiltin(METH(complex, __truediv__), c1, c2));
  ASSERT_FALSE(result_obj.isError());
  Complex result(&scope, *result_obj);
  EXPECT_EQ(result.real(), 0);
  EXPECT_EQ(result.imag(), 1);
}

TEST_F(ComplexBuiltinsTest, ComplexDividesComplexWithNan) {
  HandleScope scope(thread_);
  Complex c1(&scope, runtime_->newComplex(-1, 2));
  Complex c2(&scope, runtime_->newComplex(2, std::nan("")));
  Object result_obj(&scope, runBuiltin(METH(complex, __truediv__), c1, c2));
  ASSERT_FALSE(result_obj.isError());
  Complex result(&scope, *result_obj);
  EXPECT_TRUE(std::isnan(result.real()));
  EXPECT_TRUE(std::isnan(result.imag()));
}

TEST_F(ComplexBuiltinsTest, ComplexDividesByZero) {
  HandleScope scope(thread_);
  Complex c1(&scope, runtime_->newComplex(-1, 2));
  Complex c2(&scope, runtime_->newComplex(0, 0));
  Object result_obj(&scope, runBuiltin(METH(complex, __truediv__), c1, c2));
  ASSERT_TRUE(result_obj.isError());
  EXPECT_TRUE(raisedWithStr(*result_obj, LayoutId::kZeroDivisionError,
                            "complex division by zero"));
}

}  // namespace testing
}  // namespace py
