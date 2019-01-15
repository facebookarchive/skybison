#include "gtest/gtest.h"

#include <limits>

#include "float-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(FloatBuiltinsTest, CompareDoubleEq) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 1.0
b = 2.0
a_eq_b = a == b
a_eq_a = a == a
b_eq_b = b == b
)");

  Object a_eq_b(&scope, moduleAt(&runtime, "__main__", "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Bool::falseObj());
  Object a_eq_a(&scope, moduleAt(&runtime, "__main__", "a_eq_a"));
  EXPECT_EQ(*a_eq_a, Bool::trueObj());
  Object b_eq_b(&scope, moduleAt(&runtime, "__main__", "b_eq_b"));
  EXPECT_EQ(*b_eq_b, Bool::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleGe) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 1.0
b = 2.0
a_ge_a = a >= a
a_ge_b = a >= b
b_ge_a = b >= a
b_ge_b = b >= b
)");

  Object a_ge_a(&scope, moduleAt(&runtime, "__main__", "a_ge_a"));
  EXPECT_EQ(*a_ge_a, Bool::trueObj());
  Object a_ge_b(&scope, moduleAt(&runtime, "__main__", "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Bool::falseObj());
  Object b_ge_a(&scope, moduleAt(&runtime, "__main__", "b_ge_a"));
  EXPECT_EQ(*b_ge_a, Bool::trueObj());
  Object b_ge_b(&scope, moduleAt(&runtime, "__main__", "b_ge_b"));
  EXPECT_EQ(*b_ge_b, Bool::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleGt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 1.0
b = 2.0
a_gt_a = a > a
a_gt_b = a > b
b_gt_a = b > a
b_gt_b = b > b
)");

  Object a_gt_a(&scope, moduleAt(&runtime, "__main__", "a_gt_a"));
  EXPECT_EQ(*a_gt_a, Bool::falseObj());
  Object a_gt_b(&scope, moduleAt(&runtime, "__main__", "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Bool::falseObj());
  Object b_gt_a(&scope, moduleAt(&runtime, "__main__", "b_gt_a"));
  EXPECT_EQ(*b_gt_a, Bool::trueObj());
  Object b_gt_b(&scope, moduleAt(&runtime, "__main__", "b_gt_b"));
  EXPECT_EQ(*b_gt_b, Bool::falseObj());
}

TEST(FloatBuiltinsTest, CompareDoubleLe) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 1.0
b = 2.0
a_le_a = a <= a
a_le_b = a <= b
b_le_a = b <= a
b_le_b = b <= b
)");

  Object a_le_a(&scope, moduleAt(&runtime, "__main__", "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
  Object a_le_b(&scope, moduleAt(&runtime, "__main__", "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());
  Object b_le_a(&scope, moduleAt(&runtime, "__main__", "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());
  Object b_le_b(&scope, moduleAt(&runtime, "__main__", "b_le_b"));
  EXPECT_EQ(*b_le_b, Bool::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleLt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 1.0
b = 2.0
a_lt_a = a < a
a_lt_b = a < b
b_lt_a = b < a
b_lt_b = b < b
)");

  Object a_lt_a(&scope, moduleAt(&runtime, "__main__", "a_lt_a"));
  EXPECT_EQ(*a_lt_a, Bool::falseObj());
  Object a_lt_b(&scope, moduleAt(&runtime, "__main__", "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Bool::trueObj());
  Object b_lt_a(&scope, moduleAt(&runtime, "__main__", "b_lt_a"));
  EXPECT_EQ(*b_lt_a, Bool::falseObj());
  Object b_lt_b(&scope, moduleAt(&runtime, "__main__", "b_lt_b"));
  EXPECT_EQ(*b_lt_b, Bool::falseObj());
}

TEST(FloatBuiltinsTest, CompareDoubleNe) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 1.0
b = 2.0
a_ne_b = a != b
a_ne_a = a != a
b_ne_b = b != b
)");

  Object a_ne_b(&scope, moduleAt(&runtime, "__main__", "a_ne_b"));
  EXPECT_EQ(*a_ne_b, Bool::trueObj());
  Object a_ne_a(&scope, moduleAt(&runtime, "__main__", "a_ne_a"));
  EXPECT_EQ(*a_ne_a, Bool::falseObj());
  Object b_ne_b(&scope, moduleAt(&runtime, "__main__", "b_ne_b"));
  EXPECT_EQ(*b_ne_b, Bool::falseObj());
}

TEST(FloatBuiltinsTest, BinaryAddDouble) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 2.0
b = 1.5
c = a + b
)");

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 3.5);
}

TEST(FloatBuiltinsTest, BinaryAddSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 2.5
b = 1
c = a + b
)");

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 3.5);
}

TEST(FloatBuiltinsDeathTest, AddWithNonFloatSelfThrows) {
  const char* src = R"(
float.__add__(None, 1.0)
)";
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, src),
               "must be called with float instance as first argument");
}

TEST(FloatBuiltinsDeathTest, AddWithNonFloatOtherThrows) {
  const char* src = R"(
1.0 + None
)";
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, src), "'__add__' is not supported");
}

TEST(FloatBuiltinsTest, BinarySubtractDouble) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 2.0
b = 1.5
c = a - b
)");

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 0.5);
}

TEST(FloatBuiltinsTest, BinarySubtractSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = 2.5
b = 1
c = a - b
)");

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 1.5);
}

TEST(FloatBuiltinsTest, DunderNewWithNoArgsReturnsZero) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = float.__new__(float)
)");

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 0.0);
}

TEST(FloatBuiltinsTest, DunderNewWithFloatArgReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = float.__new__(float, 1.0)
)");

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 1.0);
}

TEST(FloatBuiltinsTest, DunderNewWithUserDefinedTypeReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class Foo:
  def __float__(self):
    return 1.0
a = float.__new__(float, Foo())
)");

  Float a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(a->value(), 1.0);
}

TEST(FloatBuiltinsTest, DunderNewWithStringReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = float.__new__(float, "1.5")
)");

  Float a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(a->value(), 1.5);
}

TEST(FloatBuiltinsTest, FloatSubclassReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class SubFloat(float):
  def __new__(self, value):
    self.foo = 3
    return super().__new__(self, value)
subfloat = SubFloat(1.5)
subfloat_foo = subfloat.foo
)");

  // Check that it's a subtype of float
  Object subfloat(&scope, moduleAt(&runtime, "__main__", "subfloat"));
  ASSERT_FALSE(subfloat->isFloat());
  ASSERT_TRUE(runtime.isInstanceOfFloat(*subfloat));

  UserFloatBase user_float(&scope, *subfloat);
  Object float_value(&scope, user_float->floatValue());
  ASSERT_TRUE(float_value->isFloat());
  EXPECT_EQ(Float::cast(float_value)->value(), 1.5);

  Object foo_attr(&scope, moduleAt(&runtime, "__main__", "subfloat_foo"));
  ASSERT_TRUE(foo_attr->isInt());
  EXPECT_EQ(3, Int::cast(foo_attr)->asWord());
}

TEST(FloatBuiltins, FloatSubclassKeepsFloatInMro) {
  const char* src = R"(
class Test(float):
  pass
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Object value(&scope, moduleAt(&runtime, "__main__", "Test"));
  ASSERT_TRUE(value->isType());

  Type type(&scope, *value);
  ASSERT_TRUE(type->mro()->isTuple());

  Tuple mro(&scope, type->mro());
  ASSERT_EQ(mro->length(), 3);
  EXPECT_EQ(mro->at(0), *type);
  EXPECT_EQ(mro->at(1), runtime.typeAt(LayoutId::kFloat));
  EXPECT_EQ(mro->at(2), runtime.typeAt(LayoutId::kObject));
}

TEST(FloatBuiltinsTest, DunderNewWithStringOfHugeNumberReturnsInf) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = float.__new__(float, "1.18973e+4932")
b = float.__new__(float, "-1.18973e+4932")

)");
  Float a(&scope, moduleAt(&runtime, "__main__", "a"));
  Float b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_EQ(a->value(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(b->value(), -std::numeric_limits<double>::infinity());
}

TEST(FloatBuiltinsDeathTest, SubWithNonFloatSelfThrows) {
  const char* src = R"(
float.__sub__(None, 1.0)
)";
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, src),
               "must be called with float instance as first argument");
}

TEST(FloatBuiltinsTest, PowFloatAndFloat) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
base = 2.0
x = base ** 4.0
)");
  Float result(&scope, moduleAt(&runtime, "__main__", "x"));
  EXPECT_EQ(result->value(), 16.0);
}

TEST(FloatBuiltinsTest, PowFloatAndInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
base = 2.0
x = base ** 4
)");
  Float result(&scope, moduleAt(&runtime, "__main__", "x"));
  EXPECT_EQ(result->value(), 16.0);
}

TEST(FloatBuiltinsTest, InplacePowFloatAndFloat) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
x = 2.0
x **= 4.0
)");
  Float result(&scope, moduleAt(&runtime, "__main__", "x"));
  EXPECT_EQ(result->value(), 16.0);
}

TEST(FloatBuiltinsTest, InplacePowFloatAndInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
x = 2.0
x **= 4
)");
  Float result(&scope, moduleAt(&runtime, "__main__", "x"));
  EXPECT_EQ(result->value(), 16.0);
}

TEST(FloatBuiltinsDeathTest, FloatNewWithDunderFloatReturnsStringThrows) {
  const char* src = R"(
class Foo:
  def __float__(self):
    return "non-float"
a = float.__new__(Foo)
)";
  Runtime runtime;
  EXPECT_DEATH(runFromCStr(&runtime, src), "aborting due to pending exception");
}

TEST(FloatBuiltinsDeathTest, DunderNewWithInvalidStringThrows) {
  Runtime runtime;
  HandleScope scope;

  EXPECT_DEATH(runFromCStr(&runtime, R"(
a = float.__new__(float, "abc")
)"),
               "aborting due to pending exception");
}

TEST(FloatBuiltinsDeathTest, SubWithNonFloatOtherThrows) {
  const char* src = R"(
1.0 - None
)";
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, src), "'__sub__' is not supported");
}

TEST(FloatBuiltins, NanIsNeqNan) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
nan = float("nan")
)");
  HandleScope scope;
  Float nan(&scope, moduleAt(&runtime, "__main__", "nan"));

  Object result(&scope, runBuiltin(FloatBuiltins::dunderEq, nan, nan));
  EXPECT_EQ(*result, RawBool::falseObj());
}

TEST(FloatBuiltinsTest, DunderFloatWithFloatLiteralReturnsSameObject) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, "a = (7.0).__float__()");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 7.0);
}

TEST(FloatBuiltinsTest, DunderFloatFromFloatClassReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;

  Float a_float(&scope, runtime.newFloat(7.0));
  Object a(&scope, runBuiltin(FloatBuiltins::dunderFloat, a_float));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 7.0);
}

TEST(FloatBuiltinsTest, DunderFloatWithFloatSubclassReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class FloatSub(float):
  pass
a = FloatSub(1.0).__float__())");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 1.0);
}

TEST(FloatBuiltinsTest, DunderFloatWithNonFloatReturnsError) {
  Runtime runtime;
  HandleScope scope;

  Int i(&scope, runtime.newInt(1));
  Object i_res(&scope, runBuiltin(FloatBuiltins::dunderFloat, i));
  EXPECT_TRUE(i_res->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(FloatBuiltinsTest, DunderFloatWithMissingSelfReturnsError) {
  Runtime runtime;
  HandleScope scope;

  Object error(&scope, runBuiltin(FloatBuiltins::dunderFloat));
  EXPECT_TRUE(error->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(FloatBuiltinsTest, DunderFloatWithTooManyArgumentsReturnsError) {
  Runtime runtime;
  HandleScope scope;

  Float flt(&scope, runtime.newFloat(7.0));
  Object error(&scope, runBuiltin(FloatBuiltins::dunderFloat, flt, flt));
  EXPECT_TRUE(error->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

}  // namespace python
