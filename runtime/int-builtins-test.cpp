#include "gtest/gtest.h"

#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(IntBuiltinsTest, NewWithStringReturnsInteger) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = int("123")
b = int("-987")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Integer> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Integer> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(a->asWord(), 123);
  EXPECT_EQ(b->asWord(), -987);
}

TEST(IntBuiltinsTest, NewWithStringAndIntegerBaseReturnsInteger) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = int("23", 8)
b = int("abc", 16)
c = int("023", 0)
d = int("0xabc", 0)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Integer> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Integer> b(&scope, moduleAt(&runtime, main, "b"));
  Handle<Integer> c(&scope, moduleAt(&runtime, main, "c"));
  Handle<Integer> d(&scope, moduleAt(&runtime, main, "d"));
  EXPECT_EQ(a->asWord(), 19);
  EXPECT_EQ(b->asWord(), 2748);
  EXPECT_EQ(c->asWord(), 19);
  EXPECT_EQ(d->asWord(), 2748);
}

TEST(IntBuiltinsTest, CompareSmallIntEq) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntGe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntGt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntLe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntLt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntNe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareOpSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
c = 1
a_lt_b = a < b
a_le_b = a <= b
a_eq_b = a == b
a_ge_b = a >= b
a_gt_b = a > b
a_is_c = a is c
a_is_not_c = a is not c
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_lt_b(&scope, moduleAt(&runtime, main, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Boolean::trueObj());
  Handle<Object> a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Boolean::trueObj());
  Handle<Object> a_eq_b(&scope, moduleAt(&runtime, main, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Boolean::falseObj());
  Handle<Object> a_ge_b(&scope, moduleAt(&runtime, main, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Boolean::falseObj());
  Handle<Object> a_gt_b(&scope, moduleAt(&runtime, main, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Boolean::falseObj());
  Handle<Object> a_is_c(&scope, moduleAt(&runtime, main, "a_is_c"));
  EXPECT_EQ(*a_is_c, Boolean::trueObj());
  Handle<Object> a_is_not_c(&scope, moduleAt(&runtime, main, "a_is_not_c"));
  EXPECT_EQ(*a_is_not_c, Boolean::falseObj());
}

TEST(IntBuiltinsTest, UnaryInvertSmallInt) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
invert_pos = ~pos
neg = -456
invert_neg = ~neg
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> invert_pos(&scope, moduleAt(&runtime, main, "invert_pos"));
  ASSERT_TRUE(invert_pos->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*invert_pos)->value(), -124);

  Handle<Object> invert_neg(&scope, moduleAt(&runtime, main, "invert_neg"));
  ASSERT_TRUE(invert_neg->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*invert_neg)->value(), 455);
}

TEST(IntBuiltinsTest, UnaryPositiveSmallInt) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
plus_pos = +pos
neg = -123
plus_neg = +neg
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> plus_pos(&scope, moduleAt(&runtime, main, "plus_pos"));
  ASSERT_TRUE(plus_pos->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*plus_pos)->value(), 123);

  Handle<Object> plus_neg(&scope, moduleAt(&runtime, main, "plus_neg"));
  ASSERT_TRUE(plus_neg->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*plus_neg)->value(), -123);
}

TEST(IntBuiltinsTest, UnaryNegateSmallInt) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
minus_pos = -pos
neg = -123
minus_neg = -neg
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> minus_pos(&scope, moduleAt(&runtime, main, "minus_pos"));
  ASSERT_TRUE(minus_pos->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*minus_pos)->value(), -123);

  Handle<Object> minus_neg(&scope, moduleAt(&runtime, main, "minus_neg"));
  ASSERT_TRUE(minus_neg->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*minus_neg)->value(), 123);
}

TEST(IntBuiltinsTest, TruthyIntPos) {
  const char* src = R"(
if 1:
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "foo\n");
}

TEST(IntBuiltinsTest, TruthyIntNeg) {
  const char* src = R"(
if 0:
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "bar\n");
}

TEST(IntBuiltinsTest, BinaryOps) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
a = 2
b = 3
c = 6
d = 7
print('a & b ==', a & b)
print('a ^ b ==', a ^ b)
print('a + b ==', a + b)

print('c // b ==', c // b)
print('d // b ==', d // b)

print('d % a ==', d % a)
print('d % b ==', d % b)

print('d * b ==', d * b)
print('c * b ==', c * b)

print('c - b ==', c - b)
print('b - c ==', b - c)

print('d * 0 ==', d * 0)
print('0 * d ==', 0 * d)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, R"(a & b == 2
a ^ b == 1
a + b == 5
c // b == 2
d // b == 2
d % a == 1
d % b == 1
d * b == 21
c * b == 18
c - b == 3
b - c == -3
d * 0 == 0
0 * d == 0
)");
}

TEST(IntBuiltinsTest, BinaryMulOverflowCheck) {
  Runtime runtime;

  // Overflows in the multiplication itself.
  EXPECT_DEBUG_ONLY_DEATH(runtime.runFromCString(R"(
a = 268435456
a = a * a * a
)"),
                          "small integer overflow");
}

TEST(IntBuiltinsTest, BinaryAddOverflowCheck) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, SmallInt::fromWord(SmallInt::kMaxValue));
  frame->setLocal(1, SmallInt::fromWord(SmallInt::kMaxValue));
  Handle<Object> result(&scope, SmallIntBuiltins::dunderAdd(thread, frame, 2));
  ASSERT_TRUE(result->isLargeInt());
  EXPECT_EQ(LargeInt::cast(*result)->asWord(), SmallInt::kMaxValue * 2);
}

TEST(IntBuiltinsTest, InplaceAdd) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
a += 0
b = a
a += 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 1);
}

TEST(IntBuiltinsTest, InplaceMultiply) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 5
a *= 1
b = a
a *= 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*a)->value(), 10);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceFloorDiv) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 5
a //= 1
b = a
a //= 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*a)->value(), 2);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceModulo) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 10
a %= 7
b = a
a %= 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*a)->value(), 1);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 3);
}

TEST(IntBuiltinsTest, InplaceSub) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 10
a -= 0
b = a
a -= 7
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 10);
}

TEST(IntBuiltinsTest, InplaceXor) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 0xFE
a ^= 0
b = a
a ^= 0x03
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*a)->value(), 0xFD);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 0xFE);
}

TEST(IntBuiltinsTest, BitLength) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  // (0).bit_length() == 0
  frame->setLocal(0, SmallInt::fromWord(0));
  Handle<Object> bit_length(&scope,
                            SmallIntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*bit_length)->value(), 0);

  // (1).bit_length() == 1
  frame->setLocal(0, SmallInt::fromWord(1));
  Handle<Object> bit_length1(&scope,
                             SmallIntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length1->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*bit_length1)->value(), 1);

  // (-1).bit_length() == 1
  frame->setLocal(0, SmallInt::fromWord(1));
  Handle<Object> bit_length2(&scope,
                             SmallIntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length2->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*bit_length2)->value(), 1);

  // (SmallInt::kMaxValue).bit_length() == 62
  frame->setLocal(0, SmallInt::fromWord(SmallInt::kMaxValue));
  Handle<Object> bit_length3(&scope,
                             SmallIntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length3->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*bit_length3)->value(), 62);

  // (SmallInt::kMinValue).bit_length() == 63
  frame->setLocal(0, SmallInt::fromWord(SmallInt::kMinValue));
  Handle<Object> bit_length4(&scope,
                             SmallIntBuiltins::bitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length4->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*bit_length4)->value(), 63);
}

TEST(IntBuiltinsTest, StringToIntDPos) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<Object> str_d0(&scope, runtime.newStringFromCString("0"));
  Handle<SmallInt> int_d0(&scope,
                          IntegerBuiltins::intFromString(thread, *str_d0, 10));
  EXPECT_EQ(int_d0->value(), 0);

  Handle<Object> str_d123(&scope, runtime.newStringFromCString("123"));
  Handle<SmallInt> int_d123(
      &scope, IntegerBuiltins::intFromString(thread, *str_d123, 10));
  EXPECT_EQ(int_d123->value(), 123);

  Handle<Object> str_d987n(&scope, runtime.newStringFromCString("-987"));
  Handle<SmallInt> int_d987n(
      &scope, IntegerBuiltins::intFromString(thread, *str_d987n, 10));
  EXPECT_EQ(int_d987n->value(), -987);
}

TEST(IntBuiltinsTest, StringToIntDNeg) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<Object> str1(&scope, runtime.newStringFromCString(""));
  Handle<Object> res1(&scope,
                      IntegerBuiltins::intFromString(thread, *str1, 10));
  EXPECT_TRUE(res1->isError());

  Handle<Object> str2(&scope, runtime.newStringFromCString("12ab"));
  Handle<Object> res2(&scope,
                      IntegerBuiltins::intFromString(thread, *str2, 10));
  EXPECT_TRUE(res2->isError());
}

}  // namespace python
