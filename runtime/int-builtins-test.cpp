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
  runFromCStr(&runtime, R"(
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
  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, src);

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

  runFromCStr(&runtime, src);

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

  runFromCStr(&runtime, src);

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
  EXPECT_DEBUG_ONLY_DEATH(runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

TEST(IntBuiltinsTest, DunderAndWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), 0x10);  // 0b10000
}

TEST(IntBuiltinsTest, DunderAndWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {0x0f, 0x30, 0x1}));
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xf0, 0x2, 7}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  ASSERT_TRUE(result->isLargeInt());
  Int expected(&scope, newIntWithDigits(&runtime, {0x03, 0x30}));
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);
}

TEST(IntBuiltinsTest, DunderAndWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(result->isNotImplemented());
}

TEST(IntBuiltinsTest, DunderAndWithInvalidArgumentLeftThrowsException) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  LargeInt right(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderAndWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderAndWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, i, i, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderLshift) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = 0b1101
b = a << 3
)");
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*b)->value(), 0x68);  // 0b1101000
}

TEST(IntBuiltinsTest, DunderLshiftWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "a = int.__lshift__(10, '')");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a->isNotImplemented());
}

TEST(IntBuiltinsTest, DunderLshiftWithInvalidArgumentThrowsException) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "a = 10 << ''"),
                            LayoutId::kTypeError,
                            "'__lshift__' is not supported"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "a = int.__lshift__('', 3)"),
                            LayoutId::kTypeError,
                            "'__lshift__' requires a 'int' object"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "a = 10 << -3"),
                            LayoutId::kValueError, "negative shift count"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "a = 10 << (1 << 100)"),
                            LayoutId::kOverflowError, "shift count too large"));
}

TEST(IntBuiltinsTest, DunderOrWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), 0x3D);  // 0b111101
}

TEST(IntBuiltinsTest, DunderOrWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {0x0C, 0xB0, 0xCAFE}));
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xD0}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  ASSERT_TRUE(result->isLargeInt());
  Int expected(&scope, newIntWithDigits(&runtime, {0x0F, 0xF0, 0xCAFE}));
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);
}

TEST(IntBuiltinsTest, DunderOrWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(result->isNotImplemented());
}

TEST(IntBuiltinsTest, DunderOrWithInvalidArgumentLeftThrowsException) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  LargeInt right(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderOrWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderOrWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, i, i, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, BinaryAddSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
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

  uword digits[] = {0, kMaxInt32};
  num = runtime.newIntWithDigits(digits);
  Object bit_length7(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length7->isSmallInt());
  // 31 bits for kMaxInt32 + 64 bits
  EXPECT_EQ(RawSmallInt::cast(*bit_length7)->value(), 95);

  // (kMinInt64 * 4).bit_length() == 66
  uword digits2[] = {0, kMaxUword - 1};  // kMaxUword - 1 == -2
  num = runtime.newIntWithDigits(digits2);
  Object bit_length8(&scope, runBuiltin(IntBuiltins::bitLength, num));
  ASSERT_TRUE(bit_length8->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*bit_length8)->value(), 66);

  // (kMinInt64 * 4 + 3).bit_length() == 65
  uword digits3[] = {3, kMaxUword - 1};  // kMaxUword - 1 == -2
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

TEST(IntBuiltinsTest, DunderFloatWithBoolReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, Bool::trueObj());
  Object a_float(&scope, runBuiltin(IntBuiltins::dunderFloat, a));
  ASSERT_TRUE(a_float->isFloat());
  EXPECT_EQ(RawFloat::cast(*a_float)->value(), 1.0);

  Object b(&scope, Bool::falseObj());
  Object b_float(&scope, runBuiltin(IntBuiltins::dunderFloat, b));
  ASSERT_TRUE(b_float->isFloat());
  EXPECT_EQ(RawFloat::cast(*b_float)->value(), 0.0);
}

TEST(IntBuiltinsTest, DunderFloatWithIntLiteralReturnsSameValue) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, "a = (7).__float__()");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isFloat());
  EXPECT_EQ(RawFloat::cast(*a)->value(), 7.0);
}

TEST(IntBuiltinsTest, DunderFloatFromIntClassReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Int b_int(&scope, runtime.newInt(7));
  Object b(&scope, runBuiltin(IntBuiltins::dunderFloat, b_int));
  ASSERT_TRUE(b->isFloat());
  EXPECT_EQ(RawFloat::cast(*b)->value(), 7.0);
}

TEST(IntBuiltinsTest, DunderFloatWithNonIntReturnsError) {
  Runtime runtime;
  HandleScope scope;

  Str str(&scope, runtime.newStrFromCStr("python"));
  Object str_res(&scope, runBuiltin(IntBuiltins::dunderInt, str));
  EXPECT_TRUE(str_res->isError());
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));

  Float flt(&scope, runtime.newFloat(1.0));
  Object flt_res(&scope, runBuiltin(IntBuiltins::dunderInt, flt));
  EXPECT_TRUE(flt_res->isError());
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
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
  EXPECT_EQ(large_e->digitAt(0), uword{1} << (kBitsPerWord - 1));
  EXPECT_EQ(large_e->digitAt(1), uword{0});
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

  runFromCStr(&runtime, R"(
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

  runFromCStr(&runtime, R"(
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

TEST(IntBuiltinsTest, FromBytesWithLittleEndianReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({0xca, 0xfe}));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  EXPECT_EQ(result->asWord(), 0xfeca);
}

TEST(IntBuiltinsTest, FromBytesWithLittleEndianReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope,
              runtime.newBytesWithAll({0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23, 0x45,
                                       0x67, 0x89, 0xab, 0xcd}));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  ASSERT_EQ(result->numDigits(), 2);
  EXPECT_EQ(result->digitAt(0), 0x67452301bebafecaU);
  EXPECT_EQ(result->digitAt(1), 0xcdab89U);
}

TEST(IntBuiltinsTest, FromBytesWithBigEndianReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({0xca, 0xfe}));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  EXPECT_EQ(result->asWord(), 0xcafe);
}

TEST(IntBuiltinsTest, FromBytesWithBytesConvertibleReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class X:
  def __bytes__(self):
    return b'*'
x = X()
)");
  Object x(&scope, moduleAt(&runtime, "__main__", "x"));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope, runBuiltin(IntBuiltins::fromBytes, x, byteorder));
  ASSERT_TRUE(result->isInt());
  EXPECT_EQ(RawInt::cast(*result)->asWord(), 42);
}

TEST(IntBuiltinsTest, FromBytesWithBigEndianReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope,
              runtime.newBytesWithAll({0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23, 0x45,
                                       0x67, 0x89, 0xab, 0xcd}));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  ASSERT_EQ(result->numDigits(), 2);
  EXPECT_EQ(result->digitAt(0), 0xbe0123456789abcdU);
  EXPECT_EQ(result->digitAt(1), 0xcafebaU);
}

TEST(IntBuiltinsTest, FromBytesWithEmptyBytes) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({}));
  Str bo_little(&scope, runtime.newStrFromCStr("little"));
  Int result_little(&scope,
                    runBuiltin(IntBuiltins::fromBytes, bytes, bo_little));
  EXPECT_EQ(result_little->asWord(), 0);

  Str bo_big(&scope, runtime.newStrFromCStr("big"));
  Int result_big(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, bo_big));
  EXPECT_EQ(result_big->asWord(), 0);
}

TEST(IntBuiltinsTest, FromBytesWithNumberWithDigitHighBitSet) {
  Runtime runtime;
  HandleScope scope;

  // Test special case where a positive number having a high bit set at the end
  // of a "digit" needs an extra digit in the LargeInt representation.
  Bytes bytes(&scope, runtime.newBytes(kWordSize, 0xff));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  Int expected(&scope, newIntWithDigits(&runtime, {kMaxUword, 0}));
  EXPECT_EQ(result->compare(expected), 0);
}

TEST(IntBuiltinsTest, FromBytesWithNegativeNumberReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime,
              "result = int.from_bytes(b'\\xff', 'little', signed=True)");
  Int result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result->asWord(), -1);
}

TEST(IntBuiltinsTest, FromBytesWithNegativeNumberReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
result = int.from_bytes(b'\xca\xfe\xba\xbe\x01\x23\x45\x67\x89\xab\xcd', 'big',
                        signed=True)
)");
  Int result(&scope, moduleAt(&runtime, "__main__", "result"));
  Int expected(&scope, newIntWithDigits(
                           &runtime, {0xbe0123456789abcd, 0xffffffffffcafeba}));
  EXPECT_EQ(result->compare(expected), 0);
}

TEST(IntBuiltinsTest, FromBytesWithKwArgumentsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
result = int.from_bytes(byteorder='big', bytes=b'\xbe\xef')
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isInt());
  EXPECT_EQ(RawInt::cast(*result)->asWord(), 0xbeef);
}

TEST(IntBuiltinsTest, FromBytesWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Bytes bytes(&scope, runtime.newBytesWithAll({0}));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Bool extra_arg(&scope, Bool::trueObj());
  Object result(
      &scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder, extra_arg));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, FromBytesWithInvalidBytesRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Str not_bytes(&scope, runtime.newStrFromCStr("not a bytes object"));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Object result(&scope,
                runBuiltin(IntBuiltins::fromBytes, not_bytes, byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, FromBytesWithInvalidByteorderStringRaisesValueError) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({0}));
  Str invalid_byteorder(&scope, runtime.newStrFromCStr("Not a byteorder"));
  Object result(&scope,
                runBuiltin(IntBuiltins::fromBytes, bytes, invalid_byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(IntBuiltinsTest, FromBytesWithInvalidByteorderRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({0}));
  Int not_a_byteorder(&scope, SmallInt::fromWord(42));
  Object result(&scope,
                runBuiltin(IntBuiltins::fromBytes, bytes, not_a_byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, FromBytesKwInvalidKeywordRaises) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "int.from_bytes(bytes=b'')"), LayoutId::kTypeError,
      "from_bytes() missing required argument 'byteorder' (pos 2)"));
  thread->clearPendingException();

  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "int.from_bytes(byteorder='little')"),
                    LayoutId::kTypeError,
                    "from_bytes() missing required argument 'bytes' (pos 1)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "int.from_bytes(b'', 'little', bytes=b'')"),
      LayoutId::kTypeError,
      "argument for from_bytes() given by name ('bytes') and position "
      "(1)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime,
                  "int.from_bytes(b'', 'little', byteorder='little')"),
      LayoutId::kTypeError,
      "argument for from_bytes() given by name ('byteorder') "
      "and position (2)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "int.from_bytes(b'', 'little', not_valid=True)"),
      LayoutId::kTypeError,
      "from_bytes() called with invalid keyword arguments"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime,
                  "int.from_bytes(b'', 'little', True, byteorder='little')"),
      LayoutId::kTypeError,
      "from_bytes() takes at most 2 positional arguments (3 given)"));
}

TEST(IntBuiltinsTest, SmallIntDunderRepr) {
  Runtime runtime;
  HandleScope scope;

  Int minint(&scope, SmallInt::fromWord(RawSmallInt::kMinValue));
  Object str(&scope, runBuiltin(SmallIntBuiltins::dunderRepr, minint));
  EXPECT_TRUE(isStrEqualsCStr(*str, "-4611686018427387904"));

  Int maxint(&scope, SmallInt::fromWord(RawSmallInt::kMaxValue));
  str = runBuiltin(SmallIntBuiltins::dunderRepr, maxint);
  EXPECT_TRUE(isStrEqualsCStr(*str, "4611686018427387903"));

  Int zero(&scope, SmallInt::fromWord(0));
  str = runBuiltin(SmallIntBuiltins::dunderRepr, zero);
  EXPECT_TRUE(isStrEqualsCStr(*str, "0"));

  Int num(&scope, SmallInt::fromWord(0xdeadbeef));
  str = runBuiltin(SmallIntBuiltins::dunderRepr, num);
  EXPECT_TRUE(isStrEqualsCStr(*str, "3735928559"));
}

TEST(IntBuiltinsTest, DunderXorWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(result)->value(), 0x2D);  // 0b101101
}

TEST(IntBuiltinsTest, DunderXorWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {0x0f, 0x30, 0xCAFE}));
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xf0}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  ASSERT_TRUE(result->isLargeInt());
  Int expected(&scope, newIntWithDigits(&runtime, {0x0C, 0xC0, 0xCAFE}));
  EXPECT_EQ(expected->compare(Int::cast(result)), 0);
}

TEST(IntBuiltinsTest, DunderXorWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(result->isNotImplemented());
}

TEST(IntBuiltinsTest, DunderXorWithInvalidArgumentLeftThrowsException) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  LargeInt right(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderXorWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderXorWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, i, i, i));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, ToBytesWithByteorderLittleEndianReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(3));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result->isBytes());
  Bytes bytes(&scope, *result);
  ASSERT_EQ(bytes->length(), 3);
  EXPECT_EQ(bytes->byteAt(0), 42);
  EXPECT_EQ(bytes->byteAt(1), 0);
  EXPECT_EQ(bytes->byteAt(2), 0);
}

TEST(IntBuiltinsTest, ToBytesWithByteorderBigEndianReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(2));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result->isBytes());
  Bytes bytes(&scope, *result);
  ASSERT_EQ(bytes->length(), 2);
  EXPECT_EQ(bytes->byteAt(0), 0);
  EXPECT_EQ(bytes->byteAt(1), 42);
}

TEST(IntBuiltinsTest, ToBytesKwReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
x0 = (0x1234).to_bytes(2, 'little')
x1 = (0x1234).to_bytes(2, 'little', signed=False)
x2 = (0x1234).to_bytes(2, 'little', signed=True)
x3 = (0x1234).to_bytes(2, byteorder='little')
x4 = (0x1234).to_bytes(length=2, byteorder='little')
x5 = (0x1234).to_bytes(2, byteorder='little', signed=False)
x6 = (0x1234).to_bytes(signed=False, byteorder='little', length=2)
)");
  for (const char* name : {"x0", "x1", "x2", "x3", "x4", "x5", "x6"}) {
    Object x(&scope, moduleAt(&runtime, "__main__", name));
    ASSERT_TRUE(x->isBytes()) << name;
    Bytes x_bytes(&scope, *x);
    ASSERT_EQ(x_bytes->length(), 2) << name;
    EXPECT_EQ(x_bytes->byteAt(0), 0x34) << name;
    EXPECT_EQ(x_bytes->byteAt(1), 0x12) << name;
  }
}

TEST(IntBuiltinsTest, ToBytesKwWithNegativeNumberReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
x0 = (-777).to_bytes(4, 'little', signed=True)
)");
  Object x(&scope, moduleAt(&runtime, "__main__", "x0"));
  ASSERT_TRUE(x->isBytes());
  Bytes x_bytes(&scope, *x);
  ASSERT_EQ(x_bytes->length(), 4);
  EXPECT_EQ(x_bytes->byteAt(0), 0xf7);
  EXPECT_EQ(x_bytes->byteAt(1), 0xfc);
  EXPECT_EQ(x_bytes->byteAt(2), 0xff);
  EXPECT_EQ(x_bytes->byteAt(3), 0xff);
}

TEST(IntBuiltinsTest, ToBytesWithSignedFalseReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  Str byteorder(&scope, runtime.newStrFromCStr("little"));

  // Test that the following numbers work fine with `signed=False` (they are the
  // same numbers that are expected to overflow with `signed=True` in
  // ToBytesWithSignedTrueOverflowRaisesOverflowError)
  Int length_1(&scope, SmallInt::fromWord(1));
  Int num_128(&scope, SmallInt::fromWord(128));
  Object result_128(
      &scope, runBuiltin(IntBuiltins::toBytes, num_128, length_1, byteorder));
  ASSERT_TRUE(result_128->isBytes());
  Bytes bytes_128(&scope, *result_128);
  ASSERT_EQ(bytes_128->length(), 1);
  EXPECT_EQ(bytes_128->byteAt(0), 0x80);

  Int length_2(&scope, SmallInt::fromWord(2));
  Int num_32768(&scope, SmallInt::fromWord(32768));
  Object result_32768(
      &scope, runBuiltin(IntBuiltins::toBytes, num_32768, length_2, byteorder));
  EXPECT_TRUE(result_32768->isBytes());
  Bytes bytes_32768(&scope, *result_32768);
  ASSERT_EQ(bytes_32768->length(), 2);
  EXPECT_EQ(bytes_32768->byteAt(0), 0);
  EXPECT_EQ(bytes_32768->byteAt(1), 0x80);

  Int length_8(&scope, SmallInt::fromWord(8));
  Int num_min_word(&scope, newIntWithDigits(&runtime, {0x8000000000000000, 0}));
  Object result_min_word(&scope, runBuiltin(IntBuiltins::toBytes, num_min_word,
                                            length_8, byteorder));
  EXPECT_TRUE(result_min_word->isBytes());
  Bytes bytes_min_word(&scope, *result_min_word);
  ASSERT_EQ(bytes_min_word->length(), 8);
  EXPECT_EQ(bytes_min_word->byteAt(0), 0);
  EXPECT_EQ(bytes_min_word->byteAt(1), 0);
  EXPECT_EQ(bytes_min_word->byteAt(2), 0);
  EXPECT_EQ(bytes_min_word->byteAt(3), 0);
  EXPECT_EQ(bytes_min_word->byteAt(4), 0);
  EXPECT_EQ(bytes_min_word->byteAt(5), 0);
  EXPECT_EQ(bytes_min_word->byteAt(6), 0);
  EXPECT_EQ(bytes_min_word->byteAt(7), 0x80);
}

TEST(IntBuiltinsTest, ToBytesWithLargeBufferByteorderBigEndianReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  // Test sign extension with zero when the buffer is larger than necessary.
  Int num(&scope, SmallInt::fromWord(0xcafebabe));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result->isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes->length(), 10);
  EXPECT_EQ(result_bytes->byteAt(0), 0);
  EXPECT_EQ(result_bytes->byteAt(1), 0);
  EXPECT_EQ(result_bytes->byteAt(2), 0);
  EXPECT_EQ(result_bytes->byteAt(3), 0);
  EXPECT_EQ(result_bytes->byteAt(4), 0);
  EXPECT_EQ(result_bytes->byteAt(5), 0);
  EXPECT_EQ(result_bytes->byteAt(6), 0xca);
  EXPECT_EQ(result_bytes->byteAt(7), 0xfe);
  EXPECT_EQ(result_bytes->byteAt(8), 0xba);
  EXPECT_EQ(result_bytes->byteAt(9), 0xbe);
}

TEST(IntBuiltinsTest, ToBytesWithLargeBufferByteorderLittleEndianReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  // Test sign extension with zero when the buffer is larger than necessary.
  Int num(&scope, SmallInt::fromWord(0xcafebabe));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result->isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes->length(), 10);
  EXPECT_EQ(result_bytes->byteAt(0), 0xbe);
  EXPECT_EQ(result_bytes->byteAt(1), 0xba);
  EXPECT_EQ(result_bytes->byteAt(2), 0xfe);
  EXPECT_EQ(result_bytes->byteAt(3), 0xca);
  EXPECT_EQ(result_bytes->byteAt(4), 0);
  EXPECT_EQ(result_bytes->byteAt(5), 0);
  EXPECT_EQ(result_bytes->byteAt(6), 0);
  EXPECT_EQ(result_bytes->byteAt(7), 0);
  EXPECT_EQ(result_bytes->byteAt(8), 0);
  EXPECT_EQ(result_bytes->byteAt(9), 0);
}

TEST(IntBuiltinsTest, ToBytesWithSignedTrueReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
result = (0x7fffffffffffffff).to_bytes(8, 'little', signed=True)
)");
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj->isBytes());
  Bytes result(&scope, *result_obj);
  EXPECT_EQ(result->length(), 8);
  EXPECT_EQ(result->byteAt(0), 0xff);
  EXPECT_EQ(result->byteAt(1), 0xff);
  EXPECT_EQ(result->byteAt(2), 0xff);
  EXPECT_EQ(result->byteAt(3), 0xff);
  EXPECT_EQ(result->byteAt(4), 0xff);
  EXPECT_EQ(result->byteAt(5), 0xff);
  EXPECT_EQ(result->byteAt(6), 0xff);
  EXPECT_EQ(result->byteAt(7), 0x7f);

  runFromCStr(&runtime, R"(
result_n_128 = (-128).to_bytes(1, 'little', signed=True)
)");
  Object result_n_128(&scope, moduleAt(&runtime, "__main__", "result_n_128"));
  ASSERT_TRUE(result_n_128->isBytes());
  Bytes bytes_n_128(&scope, *result_n_128);
  ASSERT_EQ(bytes_n_128->length(), 1);
  EXPECT_EQ(bytes_n_128->byteAt(0), 0x80);

  runFromCStr(&runtime, R"(
result_n_32768 = (-32768).to_bytes(2, 'little', signed=True)
)");
  Object result_n_32768(&scope,
                        moduleAt(&runtime, "__main__", "result_n_32768"));
  ASSERT_TRUE(result_n_32768->isBytes());
  Bytes bytes_n_32768(&scope, *result_n_32768);
  ASSERT_EQ(bytes_n_32768->length(), 2);
  EXPECT_EQ(bytes_n_32768->byteAt(0), 0x00);
  EXPECT_EQ(bytes_n_32768->byteAt(1), 0x80);

  runFromCStr(&runtime, R"(
result_n_min_word = (-9223372036854775808).to_bytes(8, 'little', signed=True)
)");
  Object result_n_min_word(&scope,
                           moduleAt(&runtime, "__main__", "result_n_min_word"));
  ASSERT_TRUE(result_n_min_word->isBytes());
  Bytes bytes_n_min_word(&scope, *result_n_min_word);
  ASSERT_EQ(bytes_n_min_word->length(), 8);
  EXPECT_EQ(bytes_n_min_word->byteAt(0), 0x00);
  EXPECT_EQ(bytes_n_min_word->byteAt(1), 0x00);
  EXPECT_EQ(bytes_n_min_word->byteAt(2), 0x00);
  EXPECT_EQ(bytes_n_min_word->byteAt(3), 0x00);
  EXPECT_EQ(bytes_n_min_word->byteAt(4), 0x00);
  EXPECT_EQ(bytes_n_min_word->byteAt(5), 0x00);
  EXPECT_EQ(bytes_n_min_word->byteAt(6), 0x00);
  EXPECT_EQ(bytes_n_min_word->byteAt(7), 0x80);
}

TEST(IntBuiltinsTest,
     ToBytesWithNegativeNumberLargeBufferBigEndianReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  // test sign extension for negative number when buffer is larger than
  // necessary.
  Int num(&scope, SmallInt::fromWord(-1024));
  Int length(&scope, SmallInt::fromWord(7));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  runFromCStr(&runtime, R"(
result = (-1024).to_bytes(7, 'big', signed=True)
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes->length(), 7);
  EXPECT_EQ(result_bytes->byteAt(0), 0xff);
  EXPECT_EQ(result_bytes->byteAt(1), 0xff);
  EXPECT_EQ(result_bytes->byteAt(2), 0xff);
  EXPECT_EQ(result_bytes->byteAt(3), 0xff);
  EXPECT_EQ(result_bytes->byteAt(4), 0xff);
  EXPECT_EQ(result_bytes->byteAt(5), 0xfc);
  EXPECT_EQ(result_bytes->byteAt(6), 0x00);
}

TEST(IntBuiltinsTest, ToBytesWithZeroLengthBigEndianReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(0));
  Int length(&scope, SmallInt::fromWord(0));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result->isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes->length(), 0);
}

TEST(IntBuiltinsTest, ToBytesWithZeroLengthLittleEndianReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(0));
  Int length(&scope, SmallInt::fromWord(0));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result->isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes->length(), 0);
}

TEST(IntBuiltinsTest, ToBytesWithSignedFalseRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(256));
  Int length(&scope, SmallInt::fromWord(1));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST(IntBuiltinsTest, ToBytesWithBigOverflowRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, newIntWithDigits(&runtime, {1, 2, 3}));
  Int length(&scope, SmallInt::fromWord(13));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST(IntBuiltinsTest, ToBytesWithSignedTrueRaisesOverflowError) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope;

  // Now check that signed=True with the same inputs triggers an error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
result = (128).to_bytes(1, 'little', signed=True)
)"),
                            LayoutId::kOverflowError,
                            "int too big to convert"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
result = (32768).to_bytes(2, 'little', signed=True)
)"),
                            LayoutId::kOverflowError,
                            "int too big to convert"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
result = (0x8000000000000000).to_bytes(8, 'little', signed=True)
)"),
                            LayoutId::kOverflowError,
                            "int too big to convert"));
}

TEST(IntBuiltinsTest, ToBytesWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, ToBytesWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Bool f(&scope, Bool::trueObj());
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder, f));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, ToBytesWithNonIntRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Str str(&scope, runtime.newStrFromCStr("not an int"));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, str, length, byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, ToBytesWithInvalidLengthArgRaises) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Str not_a_length(&scope, runtime.newStrFromCStr("not a length"));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result0(
      &scope, runBuiltin(IntBuiltins::toBytes, num, not_a_length, byteorder));
  EXPECT_TRUE(raised(*result0, LayoutId::kTypeError));
  Thread::currentThread()->clearPendingException();

  Int negative_length(&scope, SmallInt::fromWord(-3));
  Object result1(&scope, runBuiltin(IntBuiltins::toBytes, num, negative_length,
                                    byteorder));
  EXPECT_TRUE(raised(*result1, LayoutId::kValueError));
  Thread::currentThread()->clearPendingException();

  Int huge_length(&scope, testing::newIntWithDigits(&runtime, {0, 1024}));
  Object result2(&scope,
                 runBuiltin(IntBuiltins::toBytes, num, huge_length, byteorder));
  EXPECT_TRUE(raised(*result2, LayoutId::kOverflowError));
}

TEST(IntBuiltinsTest, ToBytesWithNegativeNumberRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(-1));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST(IntBuiltinsTest, ToBytesWithInvalidByteorderStringRaisesValueError) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(3));
  Str invalid_byteorder(&scope, runtime.newStrFromCStr("hello"));
  Object result(
      &scope, runBuiltin(IntBuiltins::toBytes, num, length, invalid_byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(IntBuiltinsTest, ToBytesWithInvalidByteorderTypeRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, num));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, ToBytesKwInvalidKeywordRaises) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "(4).to_bytes(signed=False)"), LayoutId::kTypeError,
      "to_bytes() missing required argument 'length' (pos 1)"));
  thread->clearPendingException();

  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "(4).to_bytes(byteorder='little')"),
                    LayoutId::kTypeError,
                    "to_bytes() missing required argument 'length' (pos 1)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "(4).to_bytes(2, signed=False)"),
      LayoutId::kTypeError,
      "to_bytes() missing required argument 'byteorder' (pos 2)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "(4).to_bytes(length=2, signed=False)"),
      LayoutId::kTypeError,
      "to_bytes() missing required argument 'byteorder' (pos 2)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "(4).to_bytes(2, 'little', not_valid=True)"),
      LayoutId::kTypeError,
      "to_bytes() called with invalid keyword arguments"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "(4).to_bytes(2, 'little', True, signed=True)"),
      LayoutId::kTypeError,
      "to_bytes() takes at most 2 positional arguments (3 given)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "(4).to_bytes(2, 'little', length=2)"),
      LayoutId::kTypeError,
      "argument for to_bytes() given by name ('length') and "
      "position (1)"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "(4).to_bytes(2, 'little', byteorder='little')"),
      LayoutId::kTypeError,
      "argument for to_bytes() given by name ('byteorder') and "
      "position (2)"));
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
  runFromCStr(&runtime, R"(
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

TEST(SmallIntBuiltinsTest, DunderModZeroDivision) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = 0.0
a % b
)"),
                            LayoutId::kZeroDivisionError, "float modulo"));

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = False
a % b
)"),
                            LayoutId::kZeroDivisionError,
                            "integer division or modulo by zero"));

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = 0
a % b
)"),
                            LayoutId::kZeroDivisionError,
                            "integer division or modulo by zero"));
}

TEST(SmallIntBuiltinsTest, DunderFloorDivZeroDivision) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = 0.0
a // b
)"),
                            LayoutId::kZeroDivisionError, "float divmod()"));

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = False
a // b
)"),
                            LayoutId::kZeroDivisionError,
                            "integer division or modulo by zero"));

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = 0
a // b
)"),
                            LayoutId::kZeroDivisionError,
                            "integer division or modulo by zero"));
}

TEST(SmallIntBuiltinsTest, DunderTrueDivZeroDivision) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = 0.0
a / b
)"),
                            LayoutId::kZeroDivisionError,
                            "float division by zero"));

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = False
a / b
)"),
                            LayoutId::kZeroDivisionError, "division by zero"));

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 10
b = 0
a / b
)"),
                            LayoutId::kZeroDivisionError, "division by zero"));
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
