#include "gtest/gtest.h"

#include "complex-builtins.h"
#include "frame.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using ComplexBuiltinsTest = RuntimeFixture;

TEST_F(ComplexBuiltinsTest, NewWithNoArgsReturnsZero) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = complex.__new__(complex)").isError());
  Complex cmplx(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(cmplx.real(), 0);
  EXPECT_EQ(cmplx.imag(), 0);
}

TEST_F(ComplexBuiltinsTest, NewWithOneNumberArgReturnsComplexWithReal) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = complex.__new__(complex, 1)").isError());
  Complex cmplx(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(cmplx.real(), 1.0);
  EXPECT_EQ(cmplx.imag(), 0);
}

TEST_F(ComplexBuiltinsTest, NewWithTwoNumberArgReturnsComplexWithReal) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_.typeAt(LayoutId::kComplex));
  Object int1(&scope, runtime_.newInt(1));
  Object int2(&scope, runtime_.newInt(2));
  Complex cmplx(&scope,
                runBuiltin(ComplexBuiltins::dunderNew, type, int1, int2));
  EXPECT_EQ(cmplx.real(), 1.0);
  EXPECT_EQ(cmplx.imag(), 2.0);
}

TEST_F(ComplexBuiltinsTest, NewWithComplexArgReturnsSameComplex) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_,
                  "result = complex.__new__(complex, complex(1.0, 2.0))")
          .isError());
  Complex cmplx(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(cmplx.real(), 1.0);
  EXPECT_EQ(cmplx.imag(), 2.0);
}

TEST_F(ComplexBuiltinsTest, DunderReprHasRealAndImag) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = repr(complex(1, 2))").isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "(1+2j)"));
}

TEST_F(ComplexBuiltinsTest, BuiltinBaseIsComplex) {
  HandleScope scope(thread_);
  Type complex_type(&scope, runtime_.typeAt(LayoutId::kComplex));
  EXPECT_EQ(complex_type.builtinBase(), LayoutId::kComplex);
}

}  // namespace python
