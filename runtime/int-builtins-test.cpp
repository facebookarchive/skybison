#include "gtest/gtest.h"

#include <cmath>

#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(IntBuiltinsTest, BuiltinBases) {
  Runtime runtime;
  HandleScope scope;

  Type integer(&scope, runtime.typeAt(LayoutId::kInt));
  EXPECT_EQ(integer->builtinBase(), LayoutId::kInt);

  Type small_int(&scope, runtime.typeAt(LayoutId::kSmallInt));
  EXPECT_EQ(small_int->builtinBase(), LayoutId::kInt);

  Type large_int(&scope, runtime.typeAt(LayoutId::kLargeInt));
  EXPECT_EQ(large_int->builtinBase(), LayoutId::kInt);

  Type boolean(&scope, runtime.typeAt(LayoutId::kBool));
  EXPECT_EQ(boolean->builtinBase(), LayoutId::kInt);
}

TEST(IntBuiltinsTest, NewWithStringReturnsInt) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = int("123")
b = int("-987")
)");
  HandleScope scope;
  Int a(&scope, moduleAt(&runtime, "__main__", "a"));
  Int b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_EQ(a->asWord(), 123);
  EXPECT_EQ(b->asWord(), -987);
}

TEST(IntBuiltinsTest, NewWithStringAndIntBaseReturnsInt) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = int("23", 8)
b = int("abc", 16)
c = int("023", 0)
d = int("0xabc", 0)
)");
  HandleScope scope;
  Int a(&scope, moduleAt(&runtime, "__main__", "a"));
  Int b(&scope, moduleAt(&runtime, "__main__", "b"));
  Int c(&scope, moduleAt(&runtime, "__main__", "c"));
  Int d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_EQ(a->asWord(), 19);
  EXPECT_EQ(b->asWord(), 2748);
  EXPECT_EQ(c->asWord(), 19);
  EXPECT_EQ(d->asWord(), 2748);
}

TEST(IntBuiltinsTest, CompareSmallIntEq) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntGe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntGt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntLe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntLt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareSmallIntNe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
b = 2
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

TEST(IntBuiltinsTest, CompareOpSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
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

  Object a_lt_b(&scope, moduleAt(&runtime, "__main__", "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Bool::trueObj());
  Object a_le_b(&scope, moduleAt(&runtime, "__main__", "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());
  Object a_eq_b(&scope, moduleAt(&runtime, "__main__", "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Bool::falseObj());
  Object a_ge_b(&scope, moduleAt(&runtime, "__main__", "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Bool::falseObj());
  Object a_gt_b(&scope, moduleAt(&runtime, "__main__", "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Bool::falseObj());
  Object a_is_c(&scope, moduleAt(&runtime, "__main__", "a_is_c"));
  EXPECT_EQ(*a_is_c, Bool::trueObj());
  Object a_is_not_c(&scope, moduleAt(&runtime, "__main__", "a_is_not_c"));
  EXPECT_EQ(*a_is_not_c, Bool::falseObj());
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

  runtime.runFromCStr(src);

  Object invert_pos(&scope, moduleAt(&runtime, "__main__", "invert_pos"));
  ASSERT_TRUE(invert_pos->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*invert_pos)->value(), -124);

  Object invert_neg(&scope, moduleAt(&runtime, "__main__", "invert_neg"));
  ASSERT_TRUE(invert_neg->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*invert_neg)->value(), 455);
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

  runtime.runFromCStr(src);

  Object plus_pos(&scope, moduleAt(&runtime, "__main__", "plus_pos"));
  ASSERT_TRUE(plus_pos->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*plus_pos)->value(), 123);

  Object plus_neg(&scope, moduleAt(&runtime, "__main__", "plus_neg"));
  ASSERT_TRUE(plus_neg->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*plus_neg)->value(), -123);
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

  runtime.runFromCStr(src);

  Object minus_pos(&scope, moduleAt(&runtime, "__main__", "minus_pos"));
  ASSERT_TRUE(minus_pos->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*minus_pos)->value(), -123);

  Object minus_neg(&scope, moduleAt(&runtime, "__main__", "minus_neg"));
  ASSERT_TRUE(minus_neg->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*minus_neg)->value(), 123);
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
  EXPECT_DEBUG_ONLY_DEATH(runtime.runFromCStr(R"(
a = 268435456
a = a * a * a
)"),
                          "small integer overflow");
}

TEST(IntBuiltinsTest, BinaryAddOverflowCheck) {
  Runtime runtime;
  HandleScope scope;

  Int maxint(&scope, SmallInt::fromWord(RawSmallInt::kMaxValue));
  Object result(&scope,
                runBuiltin(SmallIntBuiltins::dunderAdd, maxint, maxint));
  ASSERT_TRUE(result->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*result)->asWord(), RawSmallInt::kMaxValue * 2);
}

TEST(IntBuiltinsTest, InplaceAdd) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
a += 0
b = a
a += 2
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 1);
}

TEST(IntBuiltinsTest, InplaceMultiply) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 5
a *= 1
b = a
a *= 2
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 10);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceFloorDiv) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 5
a //= 1
b = a
a //= 2
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 2);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceModulo) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 10
a %= 7
b = a
a %= 2
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 1);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 3);
}

TEST(IntBuiltinsTest, InplaceSub) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 10
a -= 0
b = a
a -= 7
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 10);
}

TEST(IntBuiltinsTest, InplaceXor) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 0xFE
a ^= 0
b = a
a ^= 0x03
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 0xFD);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 0xFE);
}

TEST(IntBuiltinsDeathTest, DunderOr) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = 0b010101
b = 0b111000
c = a | b
)");
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*c)->value(), 0b111101);
}

TEST(IntBuiltinsDeathTest, DunderOrWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr("a = int.__or__(10, '')");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a->isNotImplemented());
}

TEST(IntBuiltinsDeathTest, DunderOrWithInvalidArgumentThrowsException) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("a = 10 | ''"), "Cannot do binary op");
  EXPECT_DEATH(runtime.runFromCStr("a = int.__or__('', 3)"),
               "descriptor '__or__' requires a 'int' object");
}

TEST(IntBuiltinsDeathTest, DunderLshift) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = 0b1101
b = a << 3
)");
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 0b1101000);
}

TEST(IntBuiltinsDeathTest, DunderLshiftWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr("a = int.__lshift__(10, '')");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a->isNotImplemented());
}

TEST(IntBuiltinsDeathTest, DunderLshiftWithInvalidArgumentThrowsException) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("a = 10 << ''"), "Cannot do binary op");
  EXPECT_DEATH(runtime.runFromCStr("a = int.__lshift__('', 3)"),
               "'__lshift__' requires a 'int' object");
  EXPECT_DEATH(runtime.runFromCStr("a = 10 << -3"), "negative shift count");
  EXPECT_DEATH(runtime.runFromCStr("a = 10 << (1 << 100)"),
               "shift count too large");
}

TEST(IntBuiltinsTest, BinaryAddSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 2
b = 1
c = a + b
)");

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*c)->value(), 3);
}

TEST(IntBuiltinsTest, BinaryAddSmallIntOverflow) {
  Runtime runtime;
  HandleScope scope;

  Int int1(&scope, SmallInt::fromWord(RawSmallInt::kMaxValue - 1));
  Int int2(&scope, SmallInt::fromWord(2));
  Object c(&scope, runBuiltin(SmallIntBuiltins::dunderAdd, int1, int2));

  ASSERT_TRUE(c->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*c)->asWord(), RawSmallInt::kMaxValue + 1);
}

TEST(IntBuiltinsTest, BitLength) {
  Runtime runtime;
  HandleScope scope;

  // (0).bit_length() == 0
  Object num(&scope, SmallInt::fromWord(0));
  Object bit_length(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length)->value(), 0);

  // (1).bit_length() == 1
  num = SmallInt::fromWord(1);
  Object bit_length1(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length1)->value(), 1);

  // (-1).bit_length() == 1
  num = SmallInt::fromWord(1);
  Object bit_length2(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length2)->value(), 1);

  // (SmallInt::kMaxValue).bit_length() == 62
  num = SmallInt::fromWord(RawSmallInt::kMaxValue);
  Object bit_length3(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length3)->value(), 62);

  // (SmallInt::kMinValue).bit_length() == 63
  num = SmallInt::fromWord(RawSmallInt::kMinValue);
  Object bit_length4(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length4->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length4)->value(), 63);

  // (kMaxInt64).bit_length() == 63
  num = runtime.newInt(kMaxInt64);
  Object bit_length5(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length5->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length5)->value(), 63);

  // (kMinInt64).bit_length() == 64
  num = runtime.newInt(kMinInt64);
  Object bit_length6(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length6->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length6)->value(), 64);

  word digits[] = {0, kMaxInt32};
  num = runtime.newIntWithDigits(digits);
  Object bit_length7(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length7->isSmallInt());
  // 31 bits for kMaxInt32 + 64 bits
  EXPECT_EQ(RawSmallInt::cast(*bit_length7)->value(), 95);

  // (kMinInt64 * 4).bit_length() == 66
  word digits2[] = {0, -2};
  num = runtime.newIntWithDigits(digits2);
  Object bit_length8(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length8->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length8)->value(), 66);

  // (kMinInt64 * 4 + 3).bit_length() == 65
  word digits3[] = {3, -2};
  num = runtime.newIntWithDigits(digits3);
  Object bit_length9(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length9->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length9)->value(), 65);
}

TEST(IntBuiltinsTest, CompareLargeIntEq) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderEq, a, b));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderEq, a, zero));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderEq, a, a));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderEq, b, a));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderEq, b, zero));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderEq, b, b));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareLargeIntNe) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderNe, a, b));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderNe, a, zero));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderNe, a, a));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderNe, b, a));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderNe, b, zero));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderNe, b, b));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST(LargeIntBuiltinsTest, UnaryPositive) {
  Runtime runtime;
  HandleScope scope;

  Object smallint_max(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  Object a(&scope, runBuiltin(IntBuiltins::dunderPos, smallint_max));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(),
            static_cast<word>(RawSmallInt::kMaxValue));

  Object smallint_max1(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runBuiltin(IntBuiltins::dunderPos, smallint_max1));
  ASSERT_TRUE(b->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*b)->asWord(), RawSmallInt::kMaxValue + 1);

  Object smallint_min(&scope, runtime.newInt(RawSmallInt::kMinValue));
  Object c(&scope, runBuiltin(IntBuiltins::dunderPos, smallint_min));
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*c)->value(),
            static_cast<word>(RawSmallInt::kMinValue));

  Object smallint_min1(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object d(&scope, runBuiltin(IntBuiltins::dunderPos, smallint_min1));
  ASSERT_TRUE(d->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*d)->asWord(), RawSmallInt::kMinValue - 1);
}

TEST(LargeIntBuiltinsTest, UnaryNegateTest) {
  Runtime runtime;
  HandleScope scope;

  Object smallint_max(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  Object a(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_max));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), -RawSmallInt::kMaxValue);

  Object smallint_max1(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_max1));
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), RawSmallInt::kMinValue);

  Object smallint_min(&scope, runtime.newInt(RawSmallInt::kMinValue));
  Object c(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_min));
  ASSERT_TRUE(c->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*c)->asWord(), -RawSmallInt::kMinValue);

  Object smallint_min1(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object d(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_min1));
  ASSERT_TRUE(d->isLargeInt());
  EXPECT_EQ(RawLargeInt::cast(*d)->asWord(), -(RawSmallInt::kMinValue - 1));

  Int min_word(&scope, runtime.newInt(kMinWord));
  Object e(&scope, runBuiltin(IntBuiltins::dunderNeg, min_word));
  ASSERT_TRUE(e->isLargeInt());
  LargeInt large_e(&scope, *e);
  EXPECT_TRUE(large_e->isPositive());
  Int max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_EQ(RawInt::cast(*large_e)->compare(*max_word), 1);
  EXPECT_EQ(large_e->numDigits(), 2);
  EXPECT_EQ(large_e->digitAt(0), 1ULL << (kBitsPerWord - 1));
  EXPECT_EQ(large_e->digitAt(1), 0);
}

TEST(LargeIntBuiltinsTest, TruthyLargeInt) {
  const char* src = R"(
a = 4611686018427387903 + 1
if a:
  print("true")
else:
  print("false")
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "true\n");
}

TEST(IntBuiltinsTest, CompareLargeIntGe) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderGe, a, b));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderGe, a, zero));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderGe, a, a));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderGe, b, a));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderGe, b, zero));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderGe, b, b));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareLargeIntLe) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderLe, a, b));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderLe, a, zero));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderLe, a, a));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderLe, b, a));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderLe, b, zero));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderLe, b, b));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST(IntBuiltinsTest, CompareLargeIntGt) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderGt, a, b));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderGt, a, zero));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderGt, a, a));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderGt, b, a));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderGt, b, zero));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderGt, b, b));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST(IntBuiltinsTest, CompareLargeIntLt) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a->isLargeInt());
  ASSERT_TRUE(b->isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderLt, a, b));
  ASSERT_TRUE(cmp_1->isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderLt, a, zero));
  ASSERT_TRUE(cmp_2->isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderLt, a, a));
  ASSERT_TRUE(cmp_3->isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderLt, b, a));
  ASSERT_TRUE(cmp_4->isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderLt, b, zero));
  ASSERT_TRUE(cmp_5->isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderLt, b, b));
  ASSERT_TRUE(cmp_6->isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST(IntBuiltinsTest, StringToIntDPos) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Object str_d0(&scope, runtime.newStrFromCStr("0"));
  SmallInt int_d0(&scope, IntBuiltins::intFromString(thread, *str_d0, 10));
  EXPECT_EQ(int_d0->value(), 0);

  Object str_d123(&scope, runtime.newStrFromCStr("123"));
  SmallInt int_d123(&scope, IntBuiltins::intFromString(thread, *str_d123, 10));
  EXPECT_EQ(int_d123->value(), 123);

  Object str_d987n(&scope, runtime.newStrFromCStr("-987"));
  SmallInt int_d987n(&scope,
                     IntBuiltins::intFromString(thread, *str_d987n, 10));
  EXPECT_EQ(int_d987n->value(), -987);
}

TEST(IntBuiltinsTest, StringToIntDNeg) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Object str1(&scope, runtime.newStrFromCStr(""));
  Object res1(&scope, IntBuiltins::intFromString(thread, *str1, 10));
  EXPECT_TRUE(res1->isError());

  Object str2(&scope, runtime.newStrFromCStr("12ab"));
  Object res2(&scope, IntBuiltins::intFromString(thread, *str2, 10));
  EXPECT_TRUE(res2->isError());
}

TEST(IntBuiltinsTest, DunderIndexReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  runtime.runFromCStr(R"(
a = (7).__index__()
b = int.__index__(7)
)");

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(7, RawSmallInt::cast(*a)->value());
  EXPECT_EQ(7, RawSmallInt::cast(*b)->value());
}

TEST(IntBuiltinsTest, DunderIntReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  runtime.runFromCStr(R"(
a = (7).__int__()
b = int.__int__(7)
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a->isSmallInt());
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(7, RawSmallInt::cast(*a)->value());
  EXPECT_EQ(7, RawSmallInt::cast(*b)->value());

  Str str(&scope, runtime.newStrFromCStr("python"));
  Object res(&scope, runBuiltin(IntBuiltins::dunderInt, str));
  EXPECT_TRUE(res->isError());
}

TEST(IntBuiltinsTest, DunderIntOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  Object a(&scope, runBuiltin(IntBuiltins::dunderInt, true_obj));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(1, RawSmallInt::cast(*a)->value());

  Object false_obj(&scope, Bool::falseObj());
  Object b(&scope, runBuiltin(IntBuiltins::dunderInt, false_obj));
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(0, RawSmallInt::cast(*b)->value());
}

TEST(IntBuiltinsTest, DunderBoolOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderBool, true_obj), Bool::trueObj());

  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderBool, false_obj), Bool::falseObj());
}

TEST(IntBuiltinsTest, DunderEqOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderEq, true_obj, true_obj),
            Bool::trueObj());

  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderEq, true_obj, false_obj),
            Bool::falseObj());

  Object zero(&scope, SmallInt::fromWord(0));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderEq, true_obj, zero),
            Bool::falseObj());

  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderEq, true_obj, one), Bool::trueObj());
}

TEST(IntBuiltinsTest, DunderNeOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderNe, true_obj, true_obj),
            Bool::falseObj());

  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderNe, true_obj, false_obj),
            Bool::trueObj());

  Object zero(&scope, SmallInt::fromWord(0));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderNe, true_obj, zero), Bool::trueObj());

  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderNe, true_obj, one), Bool::falseObj());
}

TEST(IntBuiltinsTest, DunderNegOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderNeg, true_obj),
            SmallInt::fromWord(-1));

  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderNeg, false_obj),
            SmallInt::fromWord(0));
}

TEST(IntBuiltinsTest, DunderPosOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderPos, true_obj),
            SmallInt::fromWord(1));

  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderPos, false_obj),
            SmallInt::fromWord(0));
}

TEST(IntBuiltinsTest, DunderLtOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLt, true_obj, false_obj),
            Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLt, false_obj, true_obj),
            Bool::trueObj());

  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLt, false_obj, one), Bool::trueObj());

  Object minus_one(&scope, SmallInt::fromWord(-1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLt, false_obj, minus_one),
            Bool::falseObj());
}

TEST(IntBuiltinsTest, DunderGeOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGe, true_obj, false_obj),
            Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGe, false_obj, true_obj),
            Bool::falseObj());

  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGe, false_obj, one),
            Bool::falseObj());

  Object minus_one(&scope, SmallInt::fromWord(-1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGe, false_obj, minus_one),
            Bool::trueObj());
}

TEST(IntBuiltinsTest, DunderGtOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGt, true_obj, false_obj),
            Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGt, false_obj, true_obj),
            Bool::falseObj());

  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGt, false_obj, one),
            Bool::falseObj());

  Object minus_one(&scope, SmallInt::fromWord(-1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderGt, false_obj, minus_one),
            Bool::trueObj());
}

TEST(IntBuiltinsTest, DunderLeOnBool) {
  Runtime runtime;
  HandleScope scope;

  Object true_obj(&scope, Bool::trueObj());
  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLe, true_obj, false_obj),
            Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLe, false_obj, true_obj),
            Bool::trueObj());

  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLe, false_obj, one), Bool::trueObj());

  Object minus_one(&scope, SmallInt::fromWord(-1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderLe, false_obj, minus_one),
            Bool::falseObj());
}

TEST(IntBuiltinsTest, SmallIntDunderRepr) {
  Runtime runtime;
  HandleScope scope;

  Int minint(&scope, SmallInt::fromWord(RawSmallInt::kMinValue));
  Str str(&scope, runBuiltin(SmallIntBuiltins::dunderRepr, minint));
  EXPECT_PYSTRING_EQ(*str, "-4611686018427387904");

  Int maxint(&scope, SmallInt::fromWord(RawSmallInt::kMaxValue));
  str = runBuiltin(SmallIntBuiltins::dunderRepr, maxint);
  EXPECT_PYSTRING_EQ(*str, "4611686018427387903");

  Int zero(&scope, SmallInt::fromWord(0));
  str = runBuiltin(SmallIntBuiltins::dunderRepr, zero);
  EXPECT_PYSTRING_EQ(*str, "0");

  Int num(&scope, SmallInt::fromWord(0xdeadbeef));
  str = runBuiltin(SmallIntBuiltins::dunderRepr, num);
  EXPECT_PYSTRING_EQ(*str, "3735928559");
}

TEST(BoolBuiltinsTest, NewFromNonZeroIntegerReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Int num(&scope, SmallInt::fromWord(2));

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, num));
  EXPECT_TRUE(result->value());
}

TEST(BoolBuiltinsTest, NewFromZerorReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Int num(&scope, SmallInt::fromWord(0));

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, num));
  EXPECT_FALSE(result->value());
}

TEST(BoolBuiltinsTest, NewFromTrueReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Object true_obj(&scope, Bool::trueObj());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, true_obj));
  EXPECT_TRUE(result->value());
}

TEST(BoolBuiltinsTest, NewFromFalseReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Object false_obj(&scope, Bool::falseObj());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, false_obj));
  EXPECT_FALSE(result->value());
}

TEST(BoolBuiltinsTest, NewFromNoneIsFalse) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Object none(&scope, NoneType::object());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, none));
  EXPECT_FALSE(result->value());
}

TEST(BoolBuiltinsTest, NewFromUserDefinedType) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
  def __bool__(self):
    return True

class Bar:
  def __bool__(self):
    return False

foo = Foo()
bar = Bar()
)");
  HandleScope scope;
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object bar(&scope, moduleAt(&runtime, "__main__", "bar"));

  {
    Type type(&scope, runtime.typeAt(LayoutId::kBool));
    Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, foo));
    EXPECT_TRUE(result->value());
  }
  {
    Type type(&scope, runtime.typeAt(LayoutId::kBool));
    Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, bar));
    EXPECT_FALSE(result->value());
  }
}

TEST(SmallIntBuiltinsDeathTest, DunderModZeroDivision) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0.0
a % b
)"),
               "float modulo");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = False
a % b
)"),
               "integer division or modulo by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0
a % b
)"),
               "integer division or modulo by zero");
}

TEST(SmallIntBuiltinsDeathTest, DunderFloorDivZeroDivision) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0.0
a // b
)"),
               "float divmod()");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = False
a // b
)"),
               "integer division or modulo by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0
a // b
)"),
               "integer division or modulo by zero");
}

TEST(SmallIntBuiltinsDeathTest, DunderTrueDivZeroDivision) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0.0
a / b
)"),
               "float division by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = False
a / b
)"),
               "division by zero");

  EXPECT_DEATH(runtime.runFromCStr(R"(
a = 10
b = 0
a / b
)"),
               "division by zero");
}

TEST(SmallIntBuiltinsTest, DunderModWithFloat) {
  Runtime runtime;
  HandleScope scope;
  Int hundred(&scope, SmallInt::fromWord(100));

  // Test positive smallint mod positive float
  Float float1(&scope, runtime.newFloat(1.5));
  Float result(&scope,
               runBuiltin(SmallIntBuiltins::dunderMod, hundred, float1));
  EXPECT_NEAR(result->value(), 1.0, DBL_EPSILON);

  // Test positive smallint mod negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  Float result1(&scope,
                runBuiltin(SmallIntBuiltins::dunderMod, hundred, float2));
  EXPECT_NEAR(result1->value(), -0.5, DBL_EPSILON);

  // Test positive smallint mod infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  Float result2(&scope,
                runBuiltin(SmallIntBuiltins::dunderMod, hundred, float_inf));
  ASSERT_TRUE(result2->isFloat());
  EXPECT_NEAR(result2->value(), 100.0, DBL_EPSILON);

  // Test positive smallint mod negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  Float result3(
      &scope, runBuiltin(SmallIntBuiltins::dunderMod, hundred, neg_float_inf));
  EXPECT_EQ(result3->value(), -INFINITY);

  // Test negative smallint mod infinity
  Int minus_hundred(&scope, SmallInt::fromWord(-100));
  Float result4(&scope, runBuiltin(SmallIntBuiltins::dunderMod, minus_hundred,
                                   float_inf));
  EXPECT_EQ(result4->value(), INFINITY);

  // Test negative smallint mod negative infinity
  Float result5(&scope, runBuiltin(SmallIntBuiltins::dunderMod, minus_hundred,
                                   neg_float_inf));
  EXPECT_NEAR(result5->value(), -100.0, DBL_EPSILON);

  // Test negative smallint mod nan
  Float nan(&scope, runtime.newFloat(NAN));
  Float result6(&scope,
                runBuiltin(SmallIntBuiltins::dunderMod, minus_hundred, nan));
  EXPECT_TRUE(std::isnan(result6->value()));
}

TEST(SmallIntBuiltinsTest, DunderFloorDivWithFloat) {
  Runtime runtime;
  HandleScope scope;
  Int hundred(&scope, SmallInt::fromWord(100));

  // Test dividing a positive smallint by a positive float
  Float float1(&scope, runtime.newFloat(1.5));
  Float result(&scope,
               runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred, float1));
  EXPECT_NEAR(result->value(), 66.0, DBL_EPSILON);

  // Test dividing a positive smallint by a negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  Float result1(&scope,
                runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred, float2));
  EXPECT_NEAR(result1->value(), -67.0, DBL_EPSILON);

  // Test dividing a positive smallint by infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  Float result2(
      &scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred, float_inf));
  EXPECT_NEAR(result2->value(), 0.0, DBL_EPSILON);

  // Test dividing a positive smallint by negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  Float result3(&scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred,
                                   neg_float_inf));
  EXPECT_NEAR(result3->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by infinity
  SmallInt minus_hundred(&scope, SmallInt::fromWord(-100));
  Float result4(&scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv,
                                   minus_hundred, float_inf));
  EXPECT_NEAR(result4->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by negative infinity
  Float result5(&scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv,
                                   minus_hundred, neg_float_inf));
  EXPECT_NEAR(result5->value(), 0.0, DBL_EPSILON);

  // Test dividing negative smallint by nan
  Float nan(&scope, runtime.newFloat(NAN));
  Float result6(
      &scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv, minus_hundred, nan));
  EXPECT_TRUE(std::isnan(result6->value()));
}

TEST(SmallIntBuiltinsTest, DunderTrueDivWithFloat) {
  Runtime runtime;
  HandleScope scope;
  Int hundred(&scope, SmallInt::fromWord(100));

  // Test dividing a positive smallint by a positive float
  Float float1(&scope, runtime.newFloat(1.5));
  Float result(&scope,
               runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred, float1));
  EXPECT_NEAR(result->value(), 66.66666666666667, DBL_EPSILON);

  // Test dividing a positive smallint by a negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  Float result1(&scope,
                runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred, float2));
  EXPECT_NEAR(result1->value(), -66.66666666666667, DBL_EPSILON);

  // Test dividing a positive smallint by infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  Float result2(
      &scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred, float_inf));
  EXPECT_NEAR(result2->value(), 0.0, DBL_EPSILON);

  // Test dividing a positive smallint by negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  Float result3(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred,
                                   neg_float_inf));
  EXPECT_NEAR(result3->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by infinity
  Int minus_hundred(&scope, SmallInt::fromWord(-100));
  Float result4(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv,
                                   minus_hundred, float_inf));
  EXPECT_NEAR(result4->value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by negative infinity
  Float result5(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv,
                                   minus_hundred, neg_float_inf));
  EXPECT_NEAR(result5->value(), 0.0, DBL_EPSILON);

  // Test dividing negative smallint by nan
  Float nan(&scope, runtime.newFloat(NAN));
  Float result6(
      &scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, minus_hundred, nan));
  EXPECT_TRUE(std::isnan(result6->value()));
}

TEST(SmallIntBuiltinsTest, DunderTrueDivWithSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Object num1(&scope, SmallInt::fromWord(6));
  Object num2(&scope, SmallInt::fromWord(3));
  Float result(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, num1, num2));
  EXPECT_NEAR(result->value(), 2.0, DBL_EPSILON);

  num1 = SmallInt::fromWord(7);
  num2 = SmallInt::fromWord(3);
  result = runBuiltin(SmallIntBuiltins::dunderTrueDiv, num1, num2);
  EXPECT_NEAR(result->value(), 2.3333333333333335, DBL_EPSILON);
}

}  // namespace python
