#include "gtest/gtest.h"

#include "complex-builtins.h"
#include "frame.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ComplexBuiltinsTest, NewWithNoArgsReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = complex.__new__(complex)").isError());
  Complex cmplx(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(cmplx.real(), 0);
  EXPECT_EQ(cmplx.imag(), 0);
}

TEST(ComplexBuiltinsTest, NewWithOneNumberArgReturnsComplexWithReal) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = complex.__new__(complex, 1)").isError());
  Complex cmplx(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(cmplx.real(), 1.0);
  EXPECT_EQ(cmplx.imag(), 0);
}

TEST(ComplexBuiltinsTest, NewWithTwoNumberArgReturnsComplexWithReal) {
  Runtime runtime;
  HandleScope scope;
  Object type(&scope, runtime.typeAt(LayoutId::kComplex));
  Object int1(&scope, runtime.newInt(1));
  Object int2(&scope, runtime.newInt(2));
  Complex cmplx(&scope,
                runBuiltin(ComplexBuiltins::dunderNew, type, int1, int2));
  EXPECT_EQ(cmplx.real(), 1.0);
  EXPECT_EQ(cmplx.imag(), 2.0);
}

TEST(ComplexBuiltinsTest, NewWithComplexArgReturnsSameComplex) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime,
                  "result = complex.__new__(complex, complex(1.0, 2.0))")
          .isError());
  Complex cmplx(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(cmplx.real(), 1.0);
  EXPECT_EQ(cmplx.imag(), 2.0);
}

TEST(ComplexBuiltinsTest, DunderReprHasRealAndImag) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = repr(complex(1, 2))").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "(1+2j)"));
}

TEST(ComplexBuiltinsTest, BuiltinBaseIsComplex) {
  Runtime runtime;
  HandleScope scope;
  Type complex_type(&scope, runtime.typeAt(LayoutId::kComplex));
  EXPECT_EQ(complex_type.builtinBase(), LayoutId::kComplex);
}

}  // namespace python
