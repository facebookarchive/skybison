#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(FloatBuiltinsTest, CompareDoubleEq) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1.0
b = 2.0
a_eq_b = a == b
a_eq_a = a == a
b_eq_b = b == b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_eq_b(&scope, moduleAt(&runtime, main, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Boolean::falseObj());
  Handle<Object> a_eq_a(&scope, moduleAt(&runtime, main, "a_eq_a"));
  EXPECT_EQ(*a_eq_a, Boolean::trueObj());
  Handle<Object> b_eq_b(&scope, moduleAt(&runtime, main, "b_eq_b"));
  EXPECT_EQ(*b_eq_b, Boolean::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleGe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1.0
b = 2.0
a_ge_a = a >= a
a_ge_b = a >= b
b_ge_a = b >= a
b_ge_b = b >= b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_ge_a(&scope, moduleAt(&runtime, main, "a_ge_a"));
  EXPECT_EQ(*a_ge_a, Boolean::trueObj());
  Handle<Object> a_ge_b(&scope, moduleAt(&runtime, main, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Boolean::falseObj());
  Handle<Object> b_ge_a(&scope, moduleAt(&runtime, main, "b_ge_a"));
  EXPECT_EQ(*b_ge_a, Boolean::trueObj());
  Handle<Object> b_ge_b(&scope, moduleAt(&runtime, main, "b_ge_b"));
  EXPECT_EQ(*b_ge_b, Boolean::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleGt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1.0
b = 2.0
a_gt_a = a > a
a_gt_b = a > b
b_gt_a = b > a
b_gt_b = b > b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_gt_a(&scope, moduleAt(&runtime, main, "a_gt_a"));
  EXPECT_EQ(*a_gt_a, Boolean::falseObj());
  Handle<Object> a_gt_b(&scope, moduleAt(&runtime, main, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Boolean::falseObj());
  Handle<Object> b_gt_a(&scope, moduleAt(&runtime, main, "b_gt_a"));
  EXPECT_EQ(*b_gt_a, Boolean::trueObj());
  Handle<Object> b_gt_b(&scope, moduleAt(&runtime, main, "b_gt_b"));
  EXPECT_EQ(*b_gt_b, Boolean::falseObj());
}

TEST(FloatBuiltinsTest, CompareDoubleLe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1.0
b = 2.0
a_le_a = a <= a
a_le_b = a <= b
b_le_a = b <= a
b_le_b = b <= b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_le_a(&scope, moduleAt(&runtime, main, "a_le_a"));
  EXPECT_EQ(*a_le_a, Boolean::trueObj());
  Handle<Object> a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Boolean::trueObj());
  Handle<Object> b_le_a(&scope, moduleAt(&runtime, main, "b_le_a"));
  EXPECT_EQ(*b_le_a, Boolean::falseObj());
  Handle<Object> b_le_b(&scope, moduleAt(&runtime, main, "b_le_b"));
  EXPECT_EQ(*b_le_b, Boolean::trueObj());
}

TEST(FloatBuiltinsTest, CompareDoubleLt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1.0
b = 2.0
a_lt_a = a < a
a_lt_b = a < b
b_lt_a = b < a
b_lt_b = b < b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_lt_a(&scope, moduleAt(&runtime, main, "a_lt_a"));
  EXPECT_EQ(*a_lt_a, Boolean::falseObj());
  Handle<Object> a_lt_b(&scope, moduleAt(&runtime, main, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Boolean::trueObj());
  Handle<Object> b_lt_a(&scope, moduleAt(&runtime, main, "b_lt_a"));
  EXPECT_EQ(*b_lt_a, Boolean::falseObj());
  Handle<Object> b_lt_b(&scope, moduleAt(&runtime, main, "b_lt_b"));
  EXPECT_EQ(*b_lt_b, Boolean::falseObj());
}

TEST(FloatBuiltinsTest, CompareDoubleNe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1.0
b = 2.0
a_ne_b = a != b
a_ne_a = a != a
b_ne_b = b != b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_ne_b(&scope, moduleAt(&runtime, main, "a_ne_b"));
  EXPECT_EQ(*a_ne_b, Boolean::trueObj());
  Handle<Object> a_ne_a(&scope, moduleAt(&runtime, main, "a_ne_a"));
  EXPECT_EQ(*a_ne_a, Boolean::falseObj());
  Handle<Object> b_ne_b(&scope, moduleAt(&runtime, main, "b_ne_b"));
  EXPECT_EQ(*b_ne_b, Boolean::falseObj());
}

TEST(FloatBuiltinsTest, BinaryAddDouble) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 2.0
b = 1.5
c = a + b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(Float::cast(*c)->value(), 3.5);
}

TEST(FloatBuiltinsTest, BinaryAddSmallInteger) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 2.5
b = 1
c = a + b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(Float::cast(*c)->value(), 3.5);
}

TEST(FloatBuiltinsDeathTest, AddWithNonFloatSelfThrows) {
  const char* src = R"(
float.__add__(None, 1.0)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCString(src),
               "must be called with float instance as first argument");
}

TEST(FloatBuiltinsDeathTest, AddWithNonFloatOtherThrows) {
  const char* src = R"(
1.0 + None
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCString(src), "unimplemented");
}

TEST(FloatBuiltinsTest, BinarySubtractDouble) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 2.0
b = 1.5
c = a - b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(Float::cast(*c)->value(), 0.5);
}

TEST(FloatBuiltinsTest, BinarySubtractSmallInteger) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 2.5
b = 1
c = a - b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(c->isFloat());
  EXPECT_EQ(Float::cast(*c)->value(), 1.5);
}

TEST(FloatBuiltinsTest, DunderNewWithNoArgsReturnsZero) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = float.__new__(float)
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(Float::cast(*a)->value(), 0.0);
}

TEST(FloatBuiltinsTest, DunderNewWithFloatArgReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = float.__new__(float, 1.0)
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(Float::cast(*a)->value(), 1.0);
}

TEST(FloatBuiltinsDeathTest, SubWithNonFloatSelfThrows) {
  const char* src = R"(
float.__sub__(None, 1.0)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCString(src),
               "must be called with float instance as first argument");
}

TEST(FloatBuiltinsDeathTest, SubWithNonFloatOtherThrows) {
  const char* src = R"(
1.0 - None
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCString(src), "unimplemented");
}

}  // namespace python
