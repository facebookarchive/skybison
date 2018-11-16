#include "gtest/gtest.h"

#include <limits>

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(FloatBuiltinsTest, CompareDoubleEq) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1.0
b = 2.0
a_eq_b = a == b
a_eq_a = a == a
b_eq_b = b == b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_eq_b(&scope, moduleAt(&runtime, main, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Bool::falseObj());
  Object a_eq_a(&scope, moduleAt(&runtime, main, "a_eq_a"));
  EXPECT_EQ(*a_eq_a, Bool::trueObj());
  Object b_eq_b(&scope, moduleAt(&runtime, main, "b_eq_b"));
  EXPECT_EQ(*b_eq_b, Bool::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleGe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1.0
b = 2.0
a_ge_a = a >= a
a_ge_b = a >= b
b_ge_a = b >= a
b_ge_b = b >= b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_ge_a(&scope, moduleAt(&runtime, main, "a_ge_a"));
  EXPECT_EQ(*a_ge_a, Bool::trueObj());
  Object a_ge_b(&scope, moduleAt(&runtime, main, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Bool::falseObj());
  Object b_ge_a(&scope, moduleAt(&runtime, main, "b_ge_a"));
  EXPECT_EQ(*b_ge_a, Bool::trueObj());
  Object b_ge_b(&scope, moduleAt(&runtime, main, "b_ge_b"));
  EXPECT_EQ(*b_ge_b, Bool::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleGt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1.0
b = 2.0
a_gt_a = a > a
a_gt_b = a > b
b_gt_a = b > a
b_gt_b = b > b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_gt_a(&scope, moduleAt(&runtime, main, "a_gt_a"));
  EXPECT_EQ(*a_gt_a, Bool::falseObj());
  Object a_gt_b(&scope, moduleAt(&runtime, main, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Bool::falseObj());
  Object b_gt_a(&scope, moduleAt(&runtime, main, "b_gt_a"));
  EXPECT_EQ(*b_gt_a, Bool::trueObj());
  Object b_gt_b(&scope, moduleAt(&runtime, main, "b_gt_b"));
  EXPECT_EQ(*b_gt_b, Bool::falseObj());
}

TEST(FloatBuiltinsTest, CompareDoubleLe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1.0
b = 2.0
a_le_a = a <= a
a_le_b = a <= b
b_le_a = b <= a
b_le_b = b <= b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_le_a(&scope, moduleAt(&runtime, main, "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
  Object a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());
  Object b_le_a(&scope, moduleAt(&runtime, main, "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());
  Object b_le_b(&scope, moduleAt(&runtime, main, "b_le_b"));
  EXPECT_EQ(*b_le_b, Bool::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleLt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1.0
b = 2.0
a_lt_a = a < a
a_lt_b = a < b
b_lt_a = b < a
b_lt_b = b < b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_lt_a(&scope, moduleAt(&runtime, main, "a_lt_a"));
  EXPECT_EQ(*a_lt_a, Bool::falseObj());
  Object a_lt_b(&scope, moduleAt(&runtime, main, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Bool::trueObj());
  Object b_lt_a(&scope, moduleAt(&runtime, main, "b_lt_a"));
  EXPECT_EQ(*b_lt_a, Bool::falseObj());
  Object b_lt_b(&scope, moduleAt(&runtime, main, "b_lt_b"));
  EXPECT_EQ(*b_lt_b, Bool::falseObj());
}

TEST(FloatBuiltinsTest, CompareDoubleNe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1.0
b = 2.0
a_ne_b = a != b
a_ne_a = a != a
b_ne_b = b != b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a_ne_b(&scope, moduleAt(&runtime, main, "a_ne_b"));
  EXPECT_EQ(*a_ne_b, Bool::trueObj());
  Object a_ne_a(&scope, moduleAt(&runtime, main, "a_ne_a"));
  EXPECT_EQ(*a_ne_a, Bool::falseObj());
  Object b_ne_b(&scope, moduleAt(&runtime, main, "b_ne_b"));
  EXPECT_EQ(*b_ne_b, Bool::falseObj());
}

TEST(FloatBuiltinsTest, BinaryAddDouble) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 2.0
b = 1.5
c = a + b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 3.5);
}

TEST(FloatBuiltinsTest, BinaryAddSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 2.5
b = 1
c = a + b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 3.5);
}

TEST(FloatBuiltinsDeathTest, AddWithNonFloatSelfThrows) {
  const char* src = R"(
float.__add__(None, 1.0)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "must be called with float instance as first argument");
}

TEST(FloatBuiltinsDeathTest, AddWithNonFloatOtherThrows) {
  const char* src = R"(
1.0 + None
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "unimplemented");
}

TEST(FloatBuiltinsTest, BinarySubtractDouble) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 2.0
b = 1.5
c = a - b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 0.5);
}

TEST(FloatBuiltinsTest, BinarySubtractSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 2.5
b = 1
c = a - b
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(RawFloat::cast(*c)->value(), 1.5);
}

TEST(FloatBuiltinsTest, DunderNewWithNoArgsReturnsZero) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = float.__new__(float)
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 0.0);
}

TEST(FloatBuiltinsTest, DunderNewWithFloatArgReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = float.__new__(float, 1.0)
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 1.0);
}

TEST(FloatBuiltinsTest, DunderNewWithUserDefinedTypeReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
class Foo:
  def __float__(self):
    return 1.0
a = float.__new__(float, Foo())
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Float a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(a->value(), 1.0);
}

TEST(FloatBuiltinsTest, DunderNewWithStringReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = float.__new__(float, "1.5")
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Float a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(a->value(), 1.5);
}

TEST(FloatBuiltinsTest, FloatSubclassReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
class Foo(float):
  def __new__(self, value):
    return super().__new__(self, value)
a = Foo(1.5)
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Float a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(a->value(), 1.5);
}

TEST(FloatBuiltinsTest, DunderNewWithStringOfHugeNumberReturnsInf) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = float.__new__(float, "1.18973e+4932")
b = float.__new__(float, "-1.18973e+4932")

)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Float a(&scope, moduleAt(&runtime, main, "a"));
  Float b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a->value(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(b->value(), -std::numeric_limits<double>::infinity());
}

TEST(FloatBuiltinsDeathTest, SubWithNonFloatSelfThrows) {
  const char* src = R"(
float.__sub__(None, 1.0)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "must be called with float instance as first argument");
}

TEST(FloatBuiltinsTest, PowFloatAndFloat) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
base = 2.0
x = base ** 4.0
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Float result(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_EQ(result->value(), 16.0);
}

TEST(FloatBuiltinsTest, PowFloatAndInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
base = 2.0
x = base ** 4
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Float result(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_EQ(result->value(), 16.0);
}

TEST(FloatBuiltinsTest, InplacePowFloatAndFloat) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
x = 2.0
x **= 4.0
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Float result(&scope, moduleAt(&runtime, main, "x"));
  EXPECT_EQ(result->value(), 16.0);
}

TEST(FloatBuiltinsTest, InplacePowFloatAndInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
x = 2.0
x **= 4
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Float result(&scope, moduleAt(&runtime, main, "x"));
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
  EXPECT_DEATH(runtime.runFromCStr(src), "aborting due to pending exception");
}

TEST(FloatBuiltinsDeathTest, DunderNewWithInvalidStringThrows) {
  Runtime runtime;
  HandleScope scope;

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = float.__new__(float, "abc")
)"),
               "aborting due to pending exception");
}

TEST(FloatBuiltinsDeathTest, SubWithNonFloatOtherThrows) {
  const char* src = R"(
1.0 - None
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "unimplemented");
}

}  // namespace python
