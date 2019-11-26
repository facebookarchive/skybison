#include "gtest/gtest.h"

#include <cmath>
#include <limits>

#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "type-builtins.h"

namespace py {

using namespace testing;

using BoolBuiltinsTest = RuntimeFixture;
using IntBuiltinsTest = RuntimeFixture;
using LargeIntBuiltinsTest = RuntimeFixture;

TEST_F(IntBuiltinsTest, BuiltinBases) {
  HandleScope scope(thread_);

  Type integer(&scope, runtime_.typeAt(LayoutId::kInt));
  EXPECT_EQ(integer.builtinBase(), LayoutId::kInt);

  Type small_int(&scope, runtime_.typeAt(LayoutId::kSmallInt));
  EXPECT_EQ(small_int.builtinBase(), LayoutId::kInt);

  Type large_int(&scope, runtime_.typeAt(LayoutId::kLargeInt));
  EXPECT_EQ(large_int.builtinBase(), LayoutId::kInt);

  Type boolean(&scope, runtime_.typeAt(LayoutId::kBool));
  EXPECT_EQ(boolean.builtinBase(), LayoutId::kInt);
}

TEST_F(IntBuiltinsTest, CompareSmallIntEq) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
a_eq_b = a == b
a_eq_a = a == a
b_eq_b = b == b
)")
                   .isError());

  Object a_eq_b(&scope, mainModuleAt(&runtime_, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Bool::falseObj());
  Object a_eq_a(&scope, mainModuleAt(&runtime_, "a_eq_a"));
  EXPECT_EQ(*a_eq_a, Bool::trueObj());
  Object b_eq_b(&scope, mainModuleAt(&runtime_, "b_eq_b"));
  EXPECT_EQ(*b_eq_b, Bool::trueObj());
}

TEST_F(IntBuiltinsTest, CompareSmallIntGe) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
a_ge_a = a >= a
a_ge_b = a >= b
b_ge_a = b >= a
b_ge_b = b >= b
)")
                   .isError());

  Object a_ge_a(&scope, mainModuleAt(&runtime_, "a_ge_a"));
  EXPECT_EQ(*a_ge_a, Bool::trueObj());
  Object a_ge_b(&scope, mainModuleAt(&runtime_, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Bool::falseObj());
  Object b_ge_a(&scope, mainModuleAt(&runtime_, "b_ge_a"));
  EXPECT_EQ(*b_ge_a, Bool::trueObj());
  Object b_ge_b(&scope, mainModuleAt(&runtime_, "b_ge_b"));
  EXPECT_EQ(*b_ge_b, Bool::trueObj());
}

TEST_F(IntBuiltinsTest, CompareSmallIntGt) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
a_gt_a = a > a
a_gt_b = a > b
b_gt_a = b > a
b_gt_b = b > b
)")
                   .isError());

  Object a_gt_a(&scope, mainModuleAt(&runtime_, "a_gt_a"));
  EXPECT_EQ(*a_gt_a, Bool::falseObj());
  Object a_gt_b(&scope, mainModuleAt(&runtime_, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Bool::falseObj());
  Object b_gt_a(&scope, mainModuleAt(&runtime_, "b_gt_a"));
  EXPECT_EQ(*b_gt_a, Bool::trueObj());
  Object b_gt_b(&scope, mainModuleAt(&runtime_, "b_gt_b"));
  EXPECT_EQ(*b_gt_b, Bool::falseObj());
}

TEST_F(IntBuiltinsTest, CompareSmallIntLe) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
a_le_a = a <= a
a_le_b = a <= b
b_le_a = b <= a
b_le_b = b <= b
)")
                   .isError());

  Object a_le_a(&scope, mainModuleAt(&runtime_, "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
  Object a_le_b(&scope, mainModuleAt(&runtime_, "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());
  Object b_le_a(&scope, mainModuleAt(&runtime_, "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());
  Object b_le_b(&scope, mainModuleAt(&runtime_, "b_le_b"));
  EXPECT_EQ(*b_le_b, Bool::trueObj());
}

TEST_F(IntBuiltinsTest, CompareSmallIntLt) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
a_lt_a = a < a
a_lt_b = a < b
b_lt_a = b < a
b_lt_b = b < b
)")
                   .isError());

  Object a_lt_a(&scope, mainModuleAt(&runtime_, "a_lt_a"));
  EXPECT_EQ(*a_lt_a, Bool::falseObj());
  Object a_lt_b(&scope, mainModuleAt(&runtime_, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Bool::trueObj());
  Object b_lt_a(&scope, mainModuleAt(&runtime_, "b_lt_a"));
  EXPECT_EQ(*b_lt_a, Bool::falseObj());
  Object b_lt_b(&scope, mainModuleAt(&runtime_, "b_lt_b"));
  EXPECT_EQ(*b_lt_b, Bool::falseObj());
}

TEST_F(IntBuiltinsTest, CompareSmallIntNe) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
b = 2
a_ne_b = a != b
a_ne_a = a != a
b_ne_b = b != b
)")
                   .isError());

  Object a_ne_b(&scope, mainModuleAt(&runtime_, "a_ne_b"));
  EXPECT_EQ(*a_ne_b, Bool::trueObj());
  Object a_ne_a(&scope, mainModuleAt(&runtime_, "a_ne_a"));
  EXPECT_EQ(*a_ne_a, Bool::falseObj());
  Object b_ne_b(&scope, mainModuleAt(&runtime_, "b_ne_b"));
  EXPECT_EQ(*b_ne_b, Bool::falseObj());
}

TEST_F(IntBuiltinsTest, CompareOpSmallInt) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
)")
                   .isError());

  Object a_lt_b(&scope, mainModuleAt(&runtime_, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Bool::trueObj());
  Object a_le_b(&scope, mainModuleAt(&runtime_, "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());
  Object a_eq_b(&scope, mainModuleAt(&runtime_, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Bool::falseObj());
  Object a_ge_b(&scope, mainModuleAt(&runtime_, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Bool::falseObj());
  Object a_gt_b(&scope, mainModuleAt(&runtime_, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Bool::falseObj());
  Object a_is_c(&scope, mainModuleAt(&runtime_, "a_is_c"));
  EXPECT_EQ(*a_is_c, Bool::trueObj());
  Object a_is_not_c(&scope, mainModuleAt(&runtime_, "a_is_not_c"));
  EXPECT_EQ(*a_is_not_c, Bool::falseObj());
}

TEST_F(IntBuiltinsTest, UnaryPositiveSmallInt) {
  HandleScope scope(thread_);

  const char* src = R"(
pos = 123
plus_pos = +pos
neg = -123
plus_neg = +neg
)";

  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object plus_pos(&scope, mainModuleAt(&runtime_, "plus_pos"));
  EXPECT_TRUE(isIntEqualsWord(*plus_pos, 123));

  Object plus_neg(&scope, mainModuleAt(&runtime_, "plus_neg"));
  EXPECT_TRUE(isIntEqualsWord(*plus_neg, -123));
}

TEST_F(IntBuiltinsTest, UnaryNegateSmallInt) {
  HandleScope scope(thread_);

  const char* src = R"(
pos = 123
minus_pos = -pos
neg = -123
minus_neg = -neg
)";

  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object minus_pos(&scope, mainModuleAt(&runtime_, "minus_pos"));
  EXPECT_TRUE(isIntEqualsWord(*minus_pos, -123));

  Object minus_neg(&scope, mainModuleAt(&runtime_, "minus_neg"));
  EXPECT_TRUE(isIntEqualsWord(*minus_neg, 123));
}

TEST_F(IntBuiltinsTest, TruthyIntPos) {
  HandleScope scope(thread_);
  Int one(&scope, SmallInt::fromWord(1));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderBool, one), Bool::trueObj());
}

TEST_F(IntBuiltinsTest, TruthyIntZero) {
  HandleScope scope(thread_);
  Int zero(&scope, SmallInt::fromWord(0));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderBool, zero), Bool::falseObj());
}

TEST_F(IntBuiltinsTest, InplaceAdd) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1
a += 0
b = a
a += 2
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  EXPECT_TRUE(isIntEqualsWord(*b, 1));
}

TEST_F(IntBuiltinsTest, InplaceMultiply) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 5
a *= 1
b = a
a *= 2
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 10));
  EXPECT_TRUE(isIntEqualsWord(*b, 5));
}

TEST_F(IntBuiltinsTest, InplaceFloordiv) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 5
a //= 1
b = a
a //= 2
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 2));
  EXPECT_TRUE(isIntEqualsWord(*b, 5));
}

TEST_F(IntBuiltinsTest, InplaceModulo) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 10
a %= 7
b = a
a %= 2
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 3));
}

TEST_F(IntBuiltinsTest, InplaceSub) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 10
a -= 0
b = a
a -= 7
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  EXPECT_TRUE(isIntEqualsWord(*b, 10));
}

TEST_F(IntBuiltinsTest, InplaceXor) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 0xFE
a ^= 0
b = a
a ^= 0x03
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 0xFD));
  EXPECT_TRUE(isIntEqualsWord(*b, 0xFE));
}

TEST_F(IntBuiltinsTest, DunderAbsWithBoolFalseReturnsZero) {
  HandleScope scope(thread_);
  Int self(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  EXPECT_EQ(result, SmallInt::fromWord(0));
}

TEST_F(IntBuiltinsTest, DunderAbsWithBoolTrueReturnsOne) {
  HandleScope scope(thread_);
  Int self(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  EXPECT_EQ(result, SmallInt::fromWord(1));
}

TEST_F(IntBuiltinsTest, DunderAbsWithPositiveIntReturnsInt) {
  HandleScope scope(thread_);
  Int self(&scope, runtime_.newInt(1234));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(IntBuiltinsTest, DunderAbsWithNegativeIntReturnsInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0x154a0071b091fb7e, 0x9661bb54b4e68c59};
  Int self(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  const uword expected_digits[] = {0xeab5ff8e4f6e0482, 0x699e44ab4b1973a6};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderAbsWithIntSubclassReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
neg = X(-42)
pos = X(42)
zero = X()
)")
                   .isError());
  Object neg(&scope, mainModuleAt(&runtime_, "neg"));
  Object pos(&scope, mainModuleAt(&runtime_, "pos"));
  Object zero(&scope, mainModuleAt(&runtime_, "zero"));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderAbs, neg), SmallInt::fromWord(42));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderAbs, pos), SmallInt::fromWord(42));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderAbs, zero), SmallInt::fromWord(0));
}

TEST_F(IntBuiltinsTest, DunderAddWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);

  Int left(&scope, SmallInt::fromWord(42));
  Int right(&scope, SmallInt::fromWord(-7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 35));
}

TEST_F(IntBuiltinsTest, DunderAddWithSmallIntsOverflowReturnsLargeInt) {
  HandleScope scope(thread_);

  Int max_small_int(&scope, SmallInt::fromWord(RawSmallInt::kMaxValue));
  Int one(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, max_small_int, one));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMaxValue + 1));
}

TEST_F(IntBuiltinsTest, DunderAddWithLargeInts) {
  HandleScope scope(thread_);

  const uword digits_left[] = {0xfedcba0987654321, 0x1234567890abcdef};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x9876543210abcdef, 0xfedcba0123456789};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  const uword expected_digits[] = {0x97530e3b98111110, 0x11111079b3f13579};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderAddWithPositiveLargeIntsCarrying) {
  HandleScope scope(thread_);

  const uword digits_left[] = {kMaxUword, kMaxUword, 0};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {1};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  const uword expected_digits[] = {0, 0, 1};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderAddWithNegativeLargeIntsCarrying) {
  HandleScope scope(thread_);

  const uword digits_left[] = {kMaxUword};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));  // == -1.
  // The smallest negative number representable with 2 digits.
  const uword digits_right[] = {0, static_cast<uword>(kMinWord)};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  const uword expected_digits[] = {kMaxUword, kMaxWord, kMaxUword};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderAndWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x10));  // 0b10000
}

TEST_F(IntBuiltinsTest, DunderAndWithLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0x0f, 0x30, 0x1};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x03, 0xf0, 0x2, 7};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  const uword expected_digits[] = {0x03, 0x30};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderAndWithNonIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  const uword digits[] = {1, 2};
  Int left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderAndWithInvalidArgumentLeftRaisesException) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  const uword digits[] = {1, 2};
  LargeInt right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderAndWithIntSubclassReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
left = X(0b0011)
right = X(0b0101)
)")
                   .isError());
  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_EQ(result, SmallInt::fromWord(1));  // 0b0001
}

TEST_F(IntBuiltinsTest, DunderCeilAliasesDunderInt) {
  HandleScope scope(thread_);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object ceil_name(&scope, Runtime::internStrFromCStr(thread_, "__ceil__"));
  Object ceil_obj(&scope, typeAt(type, ceil_name));
  ASSERT_TRUE(ceil_obj.isFunction());
  Function ceil(&scope, *ceil_obj);
  Object dint_name(&scope, Runtime::internStrFromCStr(thread_, "__int__"));
  Object dint_obj(&scope, typeAt(type, dint_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *ceil_obj);
  EXPECT_EQ(Code::cast(ceil.code()).code(), Code::cast(dint.code()).code());
  EXPECT_EQ(ceil.entry(), dint.entry());
  EXPECT_EQ(ceil.entryKw(), dint.entryKw());
  EXPECT_EQ(ceil.entryEx(), dint.entryEx());
}

TEST_F(IntBuiltinsTest, DunderFloorAliasesDunderInt) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object floor_name(&scope, Runtime::internStrFromCStr(thread_, "__floor__"));
  Object floor_obj(&scope, typeAt(type, floor_name));
  ASSERT_TRUE(floor_obj.isFunction());
  Function floor(&scope, *floor_obj);
  Object dint_name(&scope, Runtime::internStrFromCStr(thread_, "__int__"));
  Object dint_obj(&scope, typeAt(type, dint_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *floor_obj);
  EXPECT_EQ(Code::cast(floor.code()).code(), Code::cast(dint.code()).code());
  EXPECT_EQ(floor.entry(), dint.entry());
  EXPECT_EQ(floor.entryKw(), dint.entryKw());
  EXPECT_EQ(floor.entryEx(), dint.entryEx());
}

TEST_F(IntBuiltinsTest, DunderLshiftWithBoolsTrueFalseReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, Bool::trueObj());
  Object right(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithBoolsFalseTrueReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, Bool::falseObj());
  Object right(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithBoolSmallIntReturnsLargeInt) {
  HandleScope scope(thread_);
  Object left(&scope, Bool::trueObj());
  Object right(&scope, runtime_.newInt(kBitsPerWord));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  const uword expected_digits[] = {0, 1};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(0xd));  // 0b1101
  Object right(&scope, runtime_.newInt(3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x68));  // 0b1101000
}

TEST_F(IntBuiltinsTest, DunderLshiftWithNegativeSmallIntReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(-2));
  Object right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -4));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithZeroReturnsZero) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(0));
  const uword digits[] = {1, 2, 3, 4};
  Object right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithBigSmallIntReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(RawSmallInt::kMaxValue >> 1));
  Object right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMaxValue - 1));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithBigNegativeSmallIntReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(RawSmallInt::kMinValue >> 1));
  Object right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMinValue));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithSmallIntsReturnsLargeInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(4));
  Object right(&scope, runtime_.newInt(kBitsPerWord - 4));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  ASSERT_TRUE(result_obj.isLargeInt());
  LargeInt result(&scope, *result_obj);
  EXPECT_EQ(result.numDigits(), 1);
  EXPECT_EQ(result.digitAt(0), uword{1} << (kBitsPerWord - 2));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithSmallIntsNegativeReturnsLargeInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(-4));
  Object right(&scope, runtime_.newInt(kBitsPerWord - 3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  const uword expected_digits[] = {static_cast<uword>(-4)
                                   << (kBitsPerWord - 3)};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithSmallIntOverflowReturnsLargeInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(4));
  Object right(&scope, runtime_.newInt(kBitsPerWord - 3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  const uword expected_digits[] = {kHighbitUword, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest,
       DunderLshiftWithNegativeSmallIntOverflowReturnsLargeInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(-4));
  Object right(&scope, runtime_.newInt(kBitsPerWord - 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  const uword expected_digits[] = {0, kMaxUword};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithLargeIntReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {1, 1};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(2 * kBitsPerWord + 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  const uword expected_digits[] = {0, 0, 4, 4};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithNegativeLargeIntReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {kMaxUword - 1, kMaxUword - 1};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(2 * kBitsPerWord + 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  const uword expected_digits[] = {0, 0, kMaxUword - 7, kMaxUword - 4};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithLargeIntWholeWordReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0xfe84754526de453c, 0x47e8218b97f94763};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(kBitsPerWord * 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  const uword expected_digits[] = {0, 0, 0xfe84754526de453c,
                                   0x47e8218b97f94763};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithNegativeShiftAmountRaiseValueError) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(0));
  Object right(&scope, runtime_.newInt(-1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kValueError, "negative shift count"));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithNonIntSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  Object right(&scope, runtime_.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderLshiftWithNonIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(0));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderLshiftWithIntSubclassReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
left = X(0b1101)
right = X(3)
)")
                   .isError());
  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_EQ(result, SmallInt::fromWord(0x68));  // 0b1101000
}

TEST_F(IntBuiltinsTest, DunderModWithSmallIntReturnsInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(-9876));
  Object right(&scope, runtime_.newInt(123));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMod, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 87));
}

TEST_F(IntBuiltinsTest, DunderModWithZeroRaisesZeroDivisionError) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(2));
  Object right(&scope, runtime_.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMod, left, right));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kZeroDivisionError,
                            "integer division or modulo by zero"));
}

TEST_F(IntBuiltinsTest, DunderModWithNonIntSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  Object right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMod, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderModWithNontIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(1));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderMod, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderMulWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);

  Int left(&scope, runtime_.newInt(13));
  Int right(&scope, runtime_.newInt(-3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -39));
}

TEST_F(IntBuiltinsTest, DunderMulWithSmallIntsReturnsSingleDigitLargeInt) {
  HandleScope scope(thread_);

  Int left(&scope, RawSmallInt::fromWord(RawSmallInt::kMaxValue));
  Int right(&scope, RawSmallInt::fromWord(2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMaxValue * 2));
}

TEST_F(IntBuiltinsTest, DunderMulWithSmallIntsReturnsLargeInt) {
  HandleScope scope(thread_);

  Int num(&scope, RawSmallInt::fromWord(RawSmallInt::kMaxValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, num, num));
  const uword expected_digits[] = {0x8000000000000001, 0xfffffffffffffff};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderMulWithSmallIntLargeIntReturnsLargeInt) {
  HandleScope scope(thread_);

  Int left(&scope, RawSmallInt::fromWord(-3));
  const uword digits[] = {0xa1b2c3d4e5f67890, 0xaabbccddeeff};
  Int right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  const uword expected_digits[] = {0x1ae7b4814e1c9650, 0xfffdffcc99663301};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderMulWithZeroReturnsSmallInt) {
  HandleScope scope(thread_);

  const uword digits[] = {0, 1};
  Int left(&scope, newIntWithDigits(&runtime_, digits));
  Int right(&scope, RawSmallInt::fromWord(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, DunderMulWithPositiveLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);

  const uword digits_left[] = {0xfedcba0987654321, 0x1234567890abcdef};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x0123456789abcdef, 0xfedcba9876543210, 0};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  const uword expected_digits[] = {0x2236d928fe5618cf, 0xaa6c87569f0ec6a4,
                                   0x213cff7595234949, 0x121fa00acd77d743};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderMulWithMaxPositiveLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);

  const uword digits[] = {kMaxUword, 0};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, num, num));
  const uword expected_digits[] = {1, kMaxUword - 1, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderMulWithNegativeLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);

  // Smallest negative number representable with 2 digits.
  const uword digits[] = {0, static_cast<uword>(kMinWord)};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, num, num));
  const uword expected_digits[] = {0, 0, 0, static_cast<uword>(kMinWord) >> 1};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderMulWithNegativePositiveLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);

  const uword digits_left[] = {0xada6d35d8ef7c790};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x3ff2ca02c44fbb1c, 0x5873a2744317c09a};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  const uword expected_digits[] = {0x6d80780b775003c0, 0xb46184fc0839baa0,
                                   0xe38c265747f0661f};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderMulWithPositiveNegativeLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);

  const uword digits_left[] = {0x3ff2ca02c44fbb1c, 0x5873a2744317c09a};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0xada6d35d8ef7c790};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  const uword expected_digits[] = {0x6d80780b775003c0, 0xb46184fc0839baa0,
                                   0xe38c265747f0661f};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderMulWithNonIntSelfRaisesTypeError) {
  HandleScope scope(thread_);

  Str str(&scope, Str::empty());
  Int right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, str, right));
  ASSERT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderMulWithNonIntRightReturnsNotImplemented) {
  HandleScope scope(thread_);

  Int left(&scope, runtime_.newInt(1));
  Str str(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, str));
  ASSERT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderOrWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x3D));  // 0b111101
}

TEST_F(IntBuiltinsTest, DunderOrWithLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0x0C, 0xB0, 0xCAFE};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x03, 0xD0};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  const uword expected_digits[] = {0x0F, 0xF0, 0xCAFE};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderOrWithNonIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  const uword digits[] = {1, 2};
  Int left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderOrWithInvalidArgumentLeftRaisesException) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  const uword digits[] = {1, 2};
  LargeInt right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderOrWithIntSubclassReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
left = X(0b0011)
right = X(0b0101)
)")
                   .isError());
  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_EQ(result, SmallInt::fromWord(7));  // 0b0111
}

TEST_F(IntBuiltinsTest, BinaryAddSmallInt) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 2
b = 1
c = a + b
)")
                   .isError());

  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));
}

TEST_F(IntBuiltinsTest, BitLength) {
  HandleScope scope(thread_);

  // (0).bit_length() == 0
  Object num(&scope, SmallInt::fromWord(0));
  Object bit_length(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length, 0));

  // (1).bit_length() == 1
  num = SmallInt::fromWord(1);
  Object bit_length1(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length1, 1));

  // (-1).bit_length() == 1
  num = SmallInt::fromWord(1);
  Object bit_length2(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length2, 1));

  // (SmallInt::kMaxValue).bit_length() == 62
  num = SmallInt::fromWord(RawSmallInt::kMaxValue);
  Object bit_length3(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length3, 62));

  // (SmallInt::kMinValue).bit_length() == 63
  num = SmallInt::fromWord(RawSmallInt::kMinValue);
  Object bit_length4(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length4, 63));

  // (kMaxInt64).bit_length() == 63
  num = runtime_.newInt(kMaxInt64);
  Object bit_length5(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length5, 63));

  // (kMinInt64).bit_length() == 64
  num = runtime_.newInt(kMinInt64);
  Object bit_length6(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length6, 64));

  uword digits[] = {0, kMaxInt32};
  num = runtime_.newIntWithDigits(digits);
  Object bit_length7(&scope, runBuiltin(IntBuiltins::bitLength, num));
  // 31 bits for kMaxInt32 + 64 bits
  EXPECT_TRUE(isIntEqualsWord(*bit_length7, 95));

  // (kMinInt64 * 4).bit_length() == 66
  uword digits2[] = {0, kMaxUword - 1};  // kMaxUword - 1 == -2
  num = runtime_.newIntWithDigits(digits2);
  Object bit_length8(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length8, 66));

  // (kMinInt64 * 4 + 3).bit_length() == 65
  uword digits3[] = {3, kMaxUword - 1};  // kMaxUword - 1 == -2
  num = runtime_.newIntWithDigits(digits3);
  Object bit_length9(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length9, 65));
}

TEST_F(IntBuiltinsTest, CompareLargeIntEq) {
  HandleScope scope(thread_);

  Object a(&scope, runtime_.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime_.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a.isLargeInt());
  ASSERT_TRUE(b.isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderEq, a, b));
  ASSERT_TRUE(cmp_1.isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderEq, a, zero));
  ASSERT_TRUE(cmp_2.isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderEq, a, a));
  ASSERT_TRUE(cmp_3.isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderEq, b, a));
  ASSERT_TRUE(cmp_4.isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderEq, b, zero));
  ASSERT_TRUE(cmp_5.isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderEq, b, b));
  ASSERT_TRUE(cmp_6.isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST_F(IntBuiltinsTest, CompareLargeIntNe) {
  HandleScope scope(thread_);

  Object a(&scope, runtime_.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime_.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a.isLargeInt());
  ASSERT_TRUE(b.isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderNe, a, b));
  ASSERT_TRUE(cmp_1.isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderNe, a, zero));
  ASSERT_TRUE(cmp_2.isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderNe, a, a));
  ASSERT_TRUE(cmp_3.isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderNe, b, a));
  ASSERT_TRUE(cmp_4.isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderNe, b, zero));
  ASSERT_TRUE(cmp_5.isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderNe, b, b));
  ASSERT_TRUE(cmp_6.isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST_F(IntBuiltinsTest, DunderFloatWithBoolReturnsFloat) {
  HandleScope scope(thread_);

  Object a(&scope, Bool::trueObj());
  Object a_float(&scope, runBuiltin(IntBuiltins::dunderFloat, a));
  ASSERT_TRUE(a_float.isFloat());
  EXPECT_EQ(Float::cast(*a_float).value(), 1.0);

  Object b(&scope, Bool::falseObj());
  Object b_float(&scope, runBuiltin(IntBuiltins::dunderFloat, b));
  ASSERT_TRUE(b_float.isFloat());
  EXPECT_EQ(Float::cast(*b_float).value(), 0.0);
}

TEST_F(IntBuiltinsTest, DunderFloatWithSmallIntReturnsFloat) {
  HandleScope scope(thread_);

  Int num(&scope, RawSmallInt::fromWord(-7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), -7.0);
}

TEST_F(IntBuiltinsTest, DunderFloatWithOneDigitLargeIntReturnsFloat) {
  HandleScope scope(thread_);

  const uword digits[] = {static_cast<uword>(kMinWord)};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), double{kMinWord});
}

TEST_F(IntBuiltinsTest, DunderFloatWithLargeIntReturnsFloat) {
  HandleScope scope(thread_);

  const uword digits[] = {0x85b3f6fb0496ac6f, 0x129ef6};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtod("0x1.29ef685b3f6fbp+84", nullptr));
}

TEST_F(IntBuiltinsTest, DunderFloatWithNegativeLargeIntReturnsFloat) {
  HandleScope scope(thread_);

  const uword digits[] = {0x937822557f9bad3f, 0xb31911a86c86a071};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtod("-0x1.339bb95e4de58p+126", nullptr));
}

TEST_F(IntBuiltinsTest,
       DunderFloatWithNegativeLargeIntMagnitudeComputationCarriesReturnsFloat) {
  HandleScope scope(thread_);

  const uword digits[] = {1, 0, 0, 0xfffedcc000000000};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), std::strtod("-0x1.234p240", nullptr));
}

TEST_F(IntBuiltinsTest, DunderFloatWithLargeIntRoundedDownReturnsFloat) {
  HandleScope scope(thread_);

  // Produce a 1 so that all of the mantissa lies in the high digit but the bit
  // triggering the rounding is in the low digit.
  uword mantissa_high_bit = uword{1} << kDoubleMantissaBits;
  const uword digits[] = {0, mantissa_high_bit};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), std::strtod("0x1.p116", nullptr));
}

TEST_F(IntBuiltinsTest, DunderFloatWithLargeIntRoundedDownToEvenReturnsFloat) {
  HandleScope scope(thread_);

  const uword digits[] = {uword{1} << (kBitsPerWord - kDoubleMantissaBits - 1),
                          1};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), std::strtod("0x1.p64", nullptr));
}

TEST_F(IntBuiltinsTest, DunderFloatWithLargeIntRoundedUpToEvenReturnsFloat) {
  HandleScope scope(thread_);

  uword mantissa_high_bit_plus_one = (uword{1} << kDoubleMantissaBits) + 1;
  const uword digits[] = {kHighbitUword, mantissa_high_bit_plus_one};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtod("0x1.0000000000002p116", nullptr));
}

TEST_F(IntBuiltinsTest,
       DunderFloatWithNegativeLargeIntRoundedDownToEvenReturnsFloat) {
  HandleScope scope(thread_);

  uword mantissa_high_bit = uword{1} << kDoubleMantissaBits;
  const uword digits[] = {0, kHighbitUword, ~mantissa_high_bit};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), std::strtod("-0x1.p180", nullptr));
}

TEST_F(IntBuiltinsTest,
       DunderFloatWithNegativeLargeIntRoundedUpToEvenReturnsFloat) {
  HandleScope scope(thread_);

  uword mantissa_high_bit_plus_one = (uword{1} << kDoubleMantissaBits) | 1;
  const uword digits[] = {0, kHighbitUword, ~mantissa_high_bit_plus_one};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(),
            std::strtod("-0x1.0000000000002p180", nullptr));
}

TEST_F(IntBuiltinsTest,
       DunderFloatWithLargeIntRoundedUpIncreasingExponentReturnsFloat) {
  HandleScope scope(thread_);

  uword mantissa_all_one = (uword{1} << (kDoubleMantissaBits + 1)) - 1;
  const uword digits[] = {kHighbitUword, mantissa_all_one};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), std::strtod("0x1.p117", nullptr));
}

static RawObject largestIntBeforeFloatOverflow(Runtime* runtime) {
  int exponent_bits = kBitsPerDouble - kDoubleMantissaBits - 1;
  word max_unbiased_exponent = (1 << (exponent_bits - 1)) - 1;
  CHECK((max_unbiased_exponent + 1) % kBitsPerWord == 0,
        "assuming max exponent position matches highest bit in digit");
  // Note: Need an extra digit for the sign.
  word num_digits = (max_unbiased_exponent + 1) / kBitsPerWord + 1;
  std::unique_ptr<uword[]> digits(new uword[num_digits]);
  for (word i = 0; i < num_digits - 1; i++) {
    digits[i] = kMaxUword;
  }
  // Set the bit immediately below the mantissa to zero to avoid rounding up.
  digits[num_digits - 2] &= ~(1 << (kBitsPerWord - kDoubleMantissaBits - 2));
  digits[num_digits - 1] = 0;
  return runtime->newIntWithDigits(View<uword>(digits.get(), num_digits));
}

TEST_F(IntBuiltinsTest,
       DunderFloatLargestPossibleLargeIntBeforeOverflowReturnsFloat) {
  HandleScope scope(thread_);

  Int num(&scope, largestIntBeforeFloatOverflow(&runtime_));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(Float::cast(*result).value(), std::numeric_limits<double>::max());
}

TEST_F(IntBuiltinsTest, DunderFloatOverflowRaisesOverflowError) {
  HandleScope scope(thread_);

  // Add 1 to the largest number that is still convertible to float.
  Int num0(&scope, largestIntBeforeFloatOverflow(&runtime_));
  Int one(&scope, runtime_.newInt(1));
  Int num1(&scope, runBuiltin(IntBuiltins::dunderAdd, num0, one));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num1));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST_F(IntBuiltinsTest, DunderFloatWithNonIntReturnsError) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInt, none));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderFloordivWithSmallIntReturnsInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(42));
  Object right(&scope, runtime_.newInt(9));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloordiv, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 4));
}

TEST_F(IntBuiltinsTest, DunderFloordivWithZeroRaisesZeroDivisionError) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(2));
  Object right(&scope, runtime_.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloordiv, left, right));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kZeroDivisionError,
                            "integer division or modulo by zero"));
}

TEST_F(IntBuiltinsTest, DunderFloordivWithNonIntSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  Object right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloordiv, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderFloordivWithNontIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(1));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloordiv, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(LargeIntBuiltinsTest, TruthyLargeInt) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
value = 46116860184273879030000000
)")
                   .isError());
  HandleScope scope(thread_);
  Object value(&scope, mainModuleAt(&runtime_, "value"));
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderBool, value), Bool::trueObj());
}

TEST_F(IntBuiltinsTest, CompareLargeIntGe) {
  HandleScope scope(thread_);

  Object a(&scope, runtime_.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime_.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a.isLargeInt());
  ASSERT_TRUE(b.isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderGe, a, b));
  ASSERT_TRUE(cmp_1.isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderGe, a, zero));
  ASSERT_TRUE(cmp_2.isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderGe, a, a));
  ASSERT_TRUE(cmp_3.isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderGe, b, a));
  ASSERT_TRUE(cmp_4.isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderGe, b, zero));
  ASSERT_TRUE(cmp_5.isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderGe, b, b));
  ASSERT_TRUE(cmp_6.isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST_F(IntBuiltinsTest, CompareLargeIntLe) {
  HandleScope scope(thread_);

  Object a(&scope, runtime_.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime_.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a.isLargeInt());
  ASSERT_TRUE(b.isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderLe, a, b));
  ASSERT_TRUE(cmp_1.isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderLe, a, zero));
  ASSERT_TRUE(cmp_2.isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderLe, a, a));
  ASSERT_TRUE(cmp_3.isBool());
  EXPECT_EQ(*cmp_3, Bool::trueObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderLe, b, a));
  ASSERT_TRUE(cmp_4.isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderLe, b, zero));
  ASSERT_TRUE(cmp_5.isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderLe, b, b));
  ASSERT_TRUE(cmp_6.isBool());
  EXPECT_EQ(*cmp_6, Bool::trueObj());
}

TEST_F(IntBuiltinsTest, CompareLargeIntGt) {
  HandleScope scope(thread_);

  Object a(&scope, runtime_.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime_.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a.isLargeInt());
  ASSERT_TRUE(b.isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderGt, a, b));
  ASSERT_TRUE(cmp_1.isBool());
  EXPECT_EQ(*cmp_1, Bool::trueObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderGt, a, zero));
  ASSERT_TRUE(cmp_2.isBool());
  EXPECT_EQ(*cmp_2, Bool::trueObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderGt, a, a));
  ASSERT_TRUE(cmp_3.isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderGt, b, a));
  ASSERT_TRUE(cmp_4.isBool());
  EXPECT_EQ(*cmp_4, Bool::falseObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderGt, b, zero));
  ASSERT_TRUE(cmp_5.isBool());
  EXPECT_EQ(*cmp_5, Bool::falseObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderGt, b, b));
  ASSERT_TRUE(cmp_6.isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST_F(IntBuiltinsTest, CompareLargeIntLt) {
  HandleScope scope(thread_);

  Object a(&scope, runtime_.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime_.newInt(RawSmallInt::kMinValue - 1));
  Object zero(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(a.isLargeInt());
  ASSERT_TRUE(b.isLargeInt());

  Object cmp_1(&scope, runBuiltin(IntBuiltins::dunderLt, a, b));
  ASSERT_TRUE(cmp_1.isBool());
  EXPECT_EQ(*cmp_1, Bool::falseObj());

  Object cmp_2(&scope, runBuiltin(IntBuiltins::dunderLt, a, zero));
  ASSERT_TRUE(cmp_2.isBool());
  EXPECT_EQ(*cmp_2, Bool::falseObj());

  Object cmp_3(&scope, runBuiltin(IntBuiltins::dunderLt, a, a));
  ASSERT_TRUE(cmp_3.isBool());
  EXPECT_EQ(*cmp_3, Bool::falseObj());

  Object cmp_4(&scope, runBuiltin(IntBuiltins::dunderLt, b, a));
  ASSERT_TRUE(cmp_4.isBool());
  EXPECT_EQ(*cmp_4, Bool::trueObj());

  Object cmp_5(&scope, runBuiltin(IntBuiltins::dunderLt, b, zero));
  ASSERT_TRUE(cmp_5.isBool());
  EXPECT_EQ(*cmp_5, Bool::trueObj());

  Object cmp_6(&scope, runBuiltin(IntBuiltins::dunderLt, b, b));
  ASSERT_TRUE(cmp_6.isBool());
  EXPECT_EQ(*cmp_6, Bool::falseObj());
}

TEST_F(IntBuiltinsTest, DunderIndexAliasesDunderInt) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object index_name(&scope, Runtime::internStrFromCStr(thread_, "__index__"));
  Object index_obj(&scope, typeAt(type, index_name));
  ASSERT_TRUE(index_obj.isFunction());
  Function index(&scope, *index_obj);
  Object dint_name(&scope, Runtime::internStrFromCStr(thread_, "__int__"));
  Object dint_obj(&scope, typeAt(type, dint_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *index_obj);
  EXPECT_EQ(Code::cast(index.code()).code(), Code::cast(dint.code()).code());
  EXPECT_EQ(index.entry(), dint.entry());
  EXPECT_EQ(index.entryKw(), dint.entryKw());
  EXPECT_EQ(index.entryEx(), dint.entryEx());
}

TEST_F(IntBuiltinsTest, DunderIntWithBoolFalseReturnsZero) {
  HandleScope scope(thread_);

  Object self(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInt, self));
  EXPECT_EQ(result, SmallInt::fromWord(0));
}

TEST_F(IntBuiltinsTest, DunderIntWithBoolTrueReturnsOne) {
  HandleScope scope(thread_);

  Object self(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInt, self));
  EXPECT_EQ(result, SmallInt::fromWord(1));
}

TEST_F(IntBuiltinsTest, DunderIntWithSmallIntReturnsSame) {
  HandleScope scope(thread_);

  Object self(&scope, RawSmallInt::fromWord(7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderInt, self));
  EXPECT_EQ(self, result);
}

TEST_F(IntBuiltinsTest, DunderIntReturnsSameValue) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = (7).__int__()
b = int.__int__(7)
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 7));
  EXPECT_TRUE(isIntEqualsWord(*b, 7));

  Str str(&scope, runtime_.newStrFromCStr("python"));
  Object res(&scope, runBuiltin(IntBuiltins::dunderInt, str));
  EXPECT_TRUE(res.isError());
}

TEST_F(IntBuiltinsTest, DunderInvertWithBoolTrueReturnsSmallInt) {
  HandleScope scope(thread_);
  Object num(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(*result, -2));
}

TEST_F(IntBuiltinsTest, DunderInvertWithBoolFalseReturnsSmallInt) {
  HandleScope scope(thread_);
  Object num(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST_F(IntBuiltinsTest, DunderInvertWithSmallIntReturnsSmallInt) {
  HandleScope scope(thread_);
  Object num(&scope, SmallInt::fromWord(-224466));
  Object result(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(*result, 224465));
}

TEST_F(IntBuiltinsTest, DunderInvertWithLargeIntReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword num_digits[] = {0x6c5bfcb426758496, 0xda8bdbe69c009bc5, 0};
  Object num(&scope, newIntWithDigits(&runtime_, num_digits));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result_obj.isLargeInt());
  Int result(&scope, *result_obj);
  const uword expected_digits[] = {0x93a4034bd98a7b69, 0x2574241963ff643a,
                                   kMaxUword};
  Int expected(&scope, newIntWithDigits(&runtime_, expected_digits));
  EXPECT_EQ(expected.compare(*result), 0);
}

TEST_F(IntBuiltinsTest, DunderBoolOnBool) {
  HandleScope scope(thread_);

  Object true_obj(&scope, Bool::trueObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderBool, true_obj), Bool::trueObj());

  Object false_obj(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(IntBuiltins::dunderBool, false_obj), Bool::falseObj());
}

TEST_F(IntBuiltinsTest, DunderDivmodWithBoolsReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, Bool::trueObj());
  Object right(&scope, Bool::trueObj());
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithSmallIntReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, RawSmallInt::fromWord(4321));
  Object right(&scope, RawSmallInt::fromWord(17));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 254));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSmallIntNegativeDividendNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, RawSmallInt::fromWord(-987654321));
  Object right(&scope, RawSmallInt::fromWord(-654));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1510174));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), -525));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithSmallIntNegativeDividendReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, RawSmallInt::fromWord(-123456789));
  Object right(&scope, RawSmallInt::fromWord(456));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), -270739));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 195));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSmallIntNegativeDividendNoRemainderReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, RawSmallInt::fromWord(-94222222181));
  Object right(&scope, RawSmallInt::fromWord(53));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), -1777777777));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithSmallIntNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, RawSmallInt::fromWord(111222333));
  Object right(&scope, RawSmallInt::fromWord(-444));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), -250501));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), -111));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSmallIntNegativeDivisorNoRemainderReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, RawSmallInt::fromWord(94222222181));
  Object right(&scope, RawSmallInt::fromWord(-53));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), -1777777777));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSmallIntAndDivisorMinusOneReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(kMinWord));
  Object right(&scope, runtime_.newInt(-1));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {kHighbitUword, 0};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithLargeIntAndDivisorMinusOneReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0, kHighbitUword};
  Object left(&scope, runtime_.newIntWithDigits(digits));
  Object right(&scope, runtime_.newInt(-1));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0, kHighbitUword, 0};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithSingleDigitDivisorReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0x4a23475557e990d0, 0x56c1275a8b41bed9};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(77));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0x79cb7c896c08a31, 0x1206e39b2042db3};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 19));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithBoolDivisorReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0x4a23475557e990d0, 0x56c1275a8b41bed9};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, Bool::trueObj());
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0x4a23475557e990d0, 0x56c1275a8b41bed9};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSingleDigitNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0x6d73444a30629c55, 0x2c4ab2d4de16e2ef};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(-87654));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0x334af489352d60f6, 0xffffdee26dff7ad9};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), -7591));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSingleDigitNegativeDivisorNoRemainderReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0x6d73444a30629c55, 0x2c4ab2d4de16e2ef};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(-5));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0x83b5bf245cb913ef, 0xf72442a239fb6c36};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSingleDigitDivisorNegativeDividendReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0x94472249c23c1189, 0xffe0519aab10d602};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(12345));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0x5b96544c9be595f3, 0xffffff57d046e6d2};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 7790));
}

TEST_F(
    IntBuiltinsTest,
    DunderDivmodWithSingleDigitDivisorNegativeDividendNoRemainderReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0x94472249c23c1189, 0xffe0519aab10d602};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(5));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0x50db06db8d3f36b5, 0xfff9a9ebbbd02acd};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithSingleDigitNegativeDivisorNegativeDividendReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0x91a950df92c04492, 0xd60eebbadb89de2f};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(-1117392329));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0x6aaebd022be4f5c, 0xa1368e9f};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), -108249138));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithJustNotASingleDigitDivisorReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {0xaaa, 3};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(-0x100000000));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), -12884901889));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), -4294964566));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithBiggerDivisorReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0x575420c5052ae9c6};
  Object left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x383b89d9e2bb74f5, 0x1234};
  Object right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 0));
  // The algorithm should take a shortcut and return the dividend unchanged.
  EXPECT_EQ(result.at(1), left);
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithNegativeDividendBiggerDivisorReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(-55));
  const uword digits[] = {0, 1};
  Object right(&scope, newIntWithDigits(&runtime_, digits));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), -1));
  const uword expected_digits[] = {~uword{54}, 0};
  EXPECT_TRUE(isIntEqualsDigits(result.at(1), expected_digits));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithDividendBiggerNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(55));
  const uword digits[] = {0, kHighbitUword};
  Object right(&scope, newIntWithDigits(&runtime_, digits));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), -1));
  const uword expected_digits[] = {55, kHighbitUword};
  EXPECT_TRUE(isIntEqualsDigits(result.at(1), expected_digits));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithNegativeDividendBiggerNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(-55));
  const uword digits[] = {0, kMaxUword};
  Object right(&scope, newIntWithDigits(&runtime_, digits));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 0));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), -55));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithLargeIntReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0x383b89d9e2bb74f5, 0x410f8dceb8660505,
                               0x383b1ab8d7938f4b, 0x87108b9b45b43d};
  Object left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x975420c5052ae9c6, 0x3bcd71afac71b2e4};
  Object right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits1[] = {0x4015dc39ddfb7863, 0x2422dc41b36a89e};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits1));
  const uword expected_digits2[] = {0x58023143a26c3d63, 0x290c5dcb84cbb46f};
  EXPECT_TRUE(isIntEqualsDigits(result.at(1), expected_digits2));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithLargeIntPowerOfTwoReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0xee31ba892c71000e, 0x7175d128f7c2574a};
  Object left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0, 1};
  Object right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 0x7175d128f7c2574a));
  const uword expected_digits[] = {0xee31ba892c71000e, 0};
  EXPECT_TRUE(isIntEqualsDigits(result.at(1), expected_digits));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithZeroDividendBiggerNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(0));
  const uword digits[] = {0, 1};
  Object right(&scope, newIntWithDigits(&runtime_, digits));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 0));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithLargeIntNegativeDividendReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0x72a8be6d697d55c0, 0x9d95978dc878d9ae,
                               0xae86bef7900edb79};
  Object left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x9893b50147995ab1, 0x73537a3bc36c3a0e};
  Object right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits1[] = {0x4b2538374030ad53, 0xffffffffffffffff};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits1));
  const uword expected_digits2[] = {0x2f13a2c4f4b515d, 0x38ab976c676089ea};
  EXPECT_TRUE(isIntEqualsDigits(result.at(1), expected_digits2));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithLargeIntNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0x11000235a5b61b48, 0x54cb34ee1cde8d78,
                               0x2ac801d0ae5dcf65};
  Object left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0xfb2879c8be1e7dda, 0xf8101cf6608d0f6a};
  Object right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits1[] = {0x9c248b8175e4f19f, 0xfffffffffffffffa};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits1));
  const uword expected_digits2[] = {0xdc2e58062423b6e2, 0xfa5dd4db30c9589e};
  EXPECT_TRUE(isIntEqualsDigits(result.at(1), expected_digits2));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithLargeIntNegativeDividendNegativeDivisorReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0xc4b749b3bc2eb7e0, 0x74e4cc72dc8a2e9b,
                               0x46bb00bd468a1799, 0xc29ae4e0ae05134};
  Object left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x839c30dba1685693, 0xad0140cf78eaee70,
                                0xd77ec3cef0613585};
  Object right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits1[] = {0xb320ce53675ba5b0};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits1));
  const uword expected_digits2[] = {0xfbf66d17996573d0, 0xfb57b237e188be27,
                                    0xe9d7473ac0f6b873};
  EXPECT_TRUE(isIntEqualsDigits(result.at(1), expected_digits2));
}

TEST_F(IntBuiltinsTest,
       DunderDivmodWithLargeIntTriggeringNegateBugReturnsTuple) {
  HandleScope scope(thread_);
  const uword digits[] = {1, 0, 1};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(-5));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  const uword expected_digits[] = {0xcccccccccccccccc, 0xcccccccccccccccc};
  EXPECT_TRUE(isIntEqualsDigits(result.at(0), expected_digits));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), -3));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithZeroRaisesZeroDivisionError) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(2));
  Object right(&scope, runtime_.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kZeroDivisionError,
                            "integer division or modulo by zero"));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithNonIntSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  Object right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderDivmodWithNontIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(1));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderDivmod, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderEqOnBool) {
  HandleScope scope(thread_);

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

TEST_F(IntBuiltinsTest, DunderNeOnBool) {
  HandleScope scope(thread_);

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

TEST_F(IntBuiltinsTest, DunderNegWithSmallIntReturnsSmallInt) {
  HandleScope scope(thread_);
  Object num(&scope, runtime_.newInt(42));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(isIntEqualsWord(*result, -42));
}

TEST_F(IntBuiltinsTest, DunderNegWithSmallIntReturnsLargeInt) {
  HandleScope scope(thread_);
  Object num(&scope, runtime_.newInt(RawSmallInt::kMinValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(isIntEqualsWord(*result, -RawSmallInt::kMinValue));
}

TEST_F(IntBuiltinsTest, DunderNegWithBoolFalseReturnsSmallInt) {
  HandleScope scope(thread_);
  Object value(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, value));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, DunderNegWithBoolTrueReturnsSmallInt) {
  HandleScope scope(thread_);
  Object value(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, value));
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST_F(IntBuiltinsTest, DunderNegWithLargeIntReturnsSmallInt) {
  HandleScope scope(thread_);
  Object num(&scope, runtime_.newInt(-RawSmallInt::kMinValue));
  EXPECT_TRUE(num.isLargeInt());
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMinValue));
}

TEST_F(IntBuiltinsTest, DunderNegWithLargeIntReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0xad7721b1763aff22, 0x2afce48517f151b2};
  Object num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  const uword expected_digits[] = {0x5288de4e89c500de, 0xd5031b7ae80eae4d};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderNegWithLargeIntCarriesReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0, 0xfffffff000000000};
  Object num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  const uword expected_digits[] = {0, 0x1000000000};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderNegWithLargeIntOverflowsReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0, kHighbitUword};
  Object num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  const uword expected_digits[] = {0, kHighbitUword, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderNegWithLargeIntShrinksReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {kHighbitUword, 0};
  Object num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  const uword expected_digits[] = {kHighbitUword};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderNegWithLargeIntNoShrinksReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {1, kHighbitUword, 0};
  Object num(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  const uword expected_digits[] = {kMaxUword, kHighbitUword - 1, kMaxUword};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderPosAliasesDunderInt) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object pos_name(&scope, Runtime::internStrFromCStr(thread_, "__pos__"));
  Object pos_obj(&scope, typeAt(type, pos_name));
  ASSERT_TRUE(pos_obj.isFunction());
  Function pos(&scope, *pos_obj);
  Object dint_name(&scope, Runtime::internStrFromCStr(thread_, "__int__"));
  Object dint_obj(&scope, typeAt(type, dint_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *pos_obj);
  EXPECT_EQ(Code::cast(pos.code()).code(), Code::cast(dint.code()).code());
  EXPECT_EQ(pos.entry(), dint.entry());
  EXPECT_EQ(pos.entryKw(), dint.entryKw());
  EXPECT_EQ(pos.entryEx(), dint.entryEx());
}

TEST_F(IntBuiltinsTest, DunderLtOnBool) {
  HandleScope scope(thread_);

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

TEST_F(IntBuiltinsTest, DunderGeOnBool) {
  HandleScope scope(thread_);

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

TEST_F(IntBuiltinsTest, DunderGtOnBool) {
  HandleScope scope(thread_);

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

TEST_F(IntBuiltinsTest, DunderLeOnBool) {
  HandleScope scope(thread_);

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

TEST_F(IntBuiltinsTest, DunderRoundAliasesDunderInt) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object round_name(&scope, Runtime::internStrFromCStr(thread_, "__round__"));
  Object round_obj(&scope, typeAt(type, round_name));
  ASSERT_TRUE(round_obj.isFunction());
  Function round(&scope, *round_obj);
  Object dint_name(&scope, Runtime::internStrFromCStr(thread_, "__int__"));
  Object dint_obj(&scope, typeAt(type, dint_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *round_obj);
  EXPECT_EQ(Code::cast(round.code()).code(), Code::cast(dint.code()).code());
  EXPECT_EQ(round.entry(), dint.entry());
  EXPECT_EQ(round.entryKw(), dint.entryKw());
  EXPECT_EQ(round.entryEx(), dint.entryEx());
}

TEST_F(IntBuiltinsTest, DunderTruncAliasesDunderInt) {
  HandleScope scope(thread_);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object trunc_name(&scope, Runtime::internStrFromCStr(thread_, "__trunc__"));
  Object trunc_obj(&scope, typeAt(type, trunc_name));
  ASSERT_TRUE(trunc_obj.isFunction());
  Function trunc(&scope, *trunc_obj);
  Object dint_name(&scope, Runtime::internStrFromCStr(thread_, "__int__"));
  Object dint_obj(&scope, typeAt(type, dint_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *trunc_obj);
  EXPECT_EQ(Code::cast(trunc.code()).code(), Code::cast(dint.code()).code());
  EXPECT_EQ(trunc.entry(), dint.entry());
  EXPECT_EQ(trunc.entryKw(), dint.entryKw());
  EXPECT_EQ(trunc.entryEx(), dint.entryEx());
}

TEST_F(IntBuiltinsTest, FromBytesWithLittleEndianReturnsLargeInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__radd__(True, 41)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST_F(IntBuiltinsTest, DunderRandWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__rand__(0x123456789, 0x987654321)")
          .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x103454301));
}

TEST_F(IntBuiltinsTest, DunderReprWithZeroReturnsStr) {
  HandleScope scope(thread_);
  Object num(&scope, runtime_.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "0"));
}

TEST_F(IntBuiltinsTest, DunderReprWithSmallIntReturnsStr) {
  HandleScope scope(thread_);
  Object num(&scope, runtime_.newInt(0xdeadbeef));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "3735928559"));
}

TEST_F(IntBuiltinsTest, DunderReprWithSmallIntMaxReturnsStr) {
  HandleScope scope(thread_);
  Object num(&scope, runtime_.newInt(RawSmallInt::kMaxValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "4611686018427387903"));
}

TEST_F(IntBuiltinsTest, DunderReprWithSmallIntMinReturnsStr) {
  HandleScope scope(thread_);
  Object num(&scope, runtime_.newInt(RawSmallInt::kMinValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "-4611686018427387904"));
}

TEST_F(IntBuiltinsTest, DunderReprWithBoolFalseReturnsStr) {
  HandleScope scope(thread_);
  Object num(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "0"));
}

TEST_F(IntBuiltinsTest, DunderReprWithBoolTrueReturnsStr) {
  HandleScope scope(thread_);
  Object num(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1"));
}

TEST_F(IntBuiltinsTest, DunderReprWithLargeIntOneDigitReturnsStr) {
  HandleScope scope(thread_);
  const uword digits[] = {0x7ab65f95e6775822};
  Object num(&scope, runtime_.newIntWithDigits(digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "8842360015809894434"));
}

TEST_F(IntBuiltinsTest, DunderReprWithLargeIntOneDigitMinReturnsStr) {
  HandleScope scope(thread_);
  const uword digits[] = {0x8000000000000000};
  Object num(&scope, runtime_.newIntWithDigits(digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "-9223372036854775808"));
}

TEST_F(IntBuiltinsTest, DunderReprWithLargeIntOneDigitMaxReturnsStr) {
  HandleScope scope(thread_);
  const uword digits[] = {0x7fffffffffffffff};
  Object num(&scope, runtime_.newIntWithDigits(digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "9223372036854775807"));
}

TEST_F(IntBuiltinsTest, DunderReprWithLargeIntReturnsStr) {
  HandleScope scope(thread_);

  const uword digits[] = {0x68ccbb7f61087fb7, 0x4081e2972fe52778};
  Object num(&scope, runtime_.newIntWithDigits(digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(
      isStrEqualsCStr(*result, "85744993827831399429103580491677204407"));
}

TEST_F(IntBuiltinsTest, DunderReprWithNegativeLargeIntReturnsStr) {
  HandleScope scope(thread_);

  const uword digits[] = {0x49618108301eff93, 0xc70a0c6e0731da35,
                          0x438a2278e8762294, 0xccf89b106c9b714d,
                          0xfa694d4cbdf0b0ba};
  Object num(&scope, runtime_.newIntWithDigits(digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(
      isStrEqualsCStr(*result,
                      "-4663013431296140509759060231428418933437027788588076073"
                      "3669209802197774863968523736917349564525"));
}

TEST_F(IntBuiltinsTest, DunderReprWithLargeIntManyZerosReturnsStr) {
  HandleScope scope(thread_);

  const uword digits[] = {0x6ea69b2000000000, 0xf374ff2873cd99de, 0x375c24};
  Object num(&scope, runtime_.newIntWithDigits(digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(
      *result, "1234567890000000000000000000000000000000000000"));
}

TEST_F(IntBuiltinsTest, DunderReprWithLargeIntCarriesReturnsStr) {
  HandleScope scope(thread_);
  const uword digits[] = {kMaxUword, uword{kMaxWord}, kMaxUword};
  Object num(&scope, runtime_.newIntWithDigits(digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(
      isStrEqualsCStr(*result, "-170141183460469231731687303715884105729"));
}

TEST_F(IntBuiltinsTest, DunderReprWithIntSubclassReturnsStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
num = X(0xdeadbeef)
)")
                   .isError());
  Object num(&scope, mainModuleAt(&runtime_, "num"));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "3735928559"));
}

TEST_F(IntBuiltinsTest, DunderRdivmodWithSmallIntsReturnsTuple) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__rdivmod__(3, 11)").isError());
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
}

TEST_F(IntBuiltinsTest, DunderRfloordivWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__rfloordiv__(3, 11)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(IntBuiltinsTest, DunderRlshiftWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__rlshift__(3, -7)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, -56));
}

TEST_F(IntBuiltinsTest, DunderRmodWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__rmod__(3, 11)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(IntBuiltinsTest, DunderRmulWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__rmul__(-321, 123)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, -39483));
}

TEST_F(IntBuiltinsTest, DunderRorWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__ror__(0x123456789, 0x987654321)")
          .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x9a76567a9));
}

TEST_F(IntBuiltinsTest, DunderRpowWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = int.__rpow__(8, 2)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 256));
}

TEST_F(IntBuiltinsTest, DunderRrshiftWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = int.__rrshift__(16, 0xf00ddead)")
          .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xf00d));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithBoolsReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, Bool::trueObj());
  Object right(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(-1234));
  Object right(&scope, runtime_.newInt(3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -155));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithOversizedAmountSmallIntReturnsZero) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(1));
  Object right(&scope, runtime_.newInt(kBitsPerWord));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithOversizedAmountLargeIntReturnsZero) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(1));
  const uword digits[] = {1, 2};
  Object right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithLargeIntOversizedAmountReturnsZero) {
  HandleScope scope(thread_);
  const uword digits[] = {1, 2};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(kBitsPerWord * 3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0x188518dcaaa656f7, 0x7459da1092edebab,
                          0x692e3b38af8dcfbe};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(83));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  const uword expected_digits[] = {0xb9f7ce8b3b42125d, 0xd25c76715f1};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithLargeIntWholeWordReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0x1c386fefbb1baf3d, 0x379bcaa886c98c13,
                          0xe0f6379843f98b29, 0};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(kBitsPerWord * 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  const uword expected_digits[] = {0xe0f6379843f98b29, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithLargeIntNegativeReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits[] = {0x3190ff6fa83269bc, 0xe7a1689a33ca9ae6};
  Object left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, runtime_.newInt(13));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  const uword expected_digits[] = {0xd7318c87fb7d4193, 0xffff3d0b44d19e54};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithNegativeShiftAmountRaisesValueError) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(1));
  Object right(&scope, runtime_.newInt(-4));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kValueError, "negative shift count"));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithNonIntSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  Object right(&scope, runtime_.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderRshiftWithNonIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object left(&scope, runtime_.newInt(0));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderRshiftWithIntSubclassReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
left = X(-1234)
right = X(3)
)")
                   .isError());
  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_EQ(result, SmallInt::fromWord(-155));
}

TEST_F(IntBuiltinsTest, DunderStrAliasesDunderRepr) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object str_name(&scope, Runtime::internStrFromCStr(thread_, "__str__"));
  Object str_obj(&scope, typeAt(type, str_name));
  ASSERT_TRUE(str_obj.isFunction());
  Function str(&scope, *str_obj);
  Object repr_name(&scope, Runtime::internStrFromCStr(thread_, "__repr__"));
  Object repr_obj(&scope, typeAt(type, repr_name));
  ASSERT_TRUE(repr_obj.isFunction());
  Function repr(&scope, *str_obj);
  EXPECT_EQ(Code::cast(str.code()).code(), Code::cast(repr.code()).code());
  EXPECT_EQ(str.entry(), repr.entry());
  EXPECT_EQ(str.entryKw(), repr.entryKw());
  EXPECT_EQ(str.entryEx(), repr.entryEx());
}

TEST_F(IntBuiltinsTest, DunderSubWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);

  Int left(&scope, SmallInt::fromWord(42));
  Int right(&scope, SmallInt::fromWord(-7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 49));
}

TEST_F(IntBuiltinsTest, DunderSubWithSmallIntsOverflowReturnsLargeInt) {
  HandleScope scope(thread_);

  Int min_small_int(&scope, SmallInt::fromWord(RawSmallInt::kMinValue));
  Int one(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, min_small_int, one));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMinValue - 1));
}

TEST_F(IntBuiltinsTest, DunderSubWithLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);

  const uword digits_left[] = {0xfedcba0987654321, 0x1234567890abcdef};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x9876543210abcdef, 0xfedcba0123456789};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  const uword expected_digits[] = {0x666665d776b97532, 0x13579c776d666666};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest,
       DunderSubWithPositiveLargeIntsBorrowingReturnsLargeInt) {
  HandleScope scope(thread_);

  const uword digits_left[] = {1};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {kMaxUword, kMaxUword, 0};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  const uword expected_digits[] = {2, 0, kMaxUword};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest,
       DunderSubWithNegativeLargeIntsBorrowingReturnsLargeInt) {
  HandleScope scope(thread_);

  // The smallest negative number representable with 2 digits.
  const uword digits_left[] = {0, static_cast<uword>(kMinWord)};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {1};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  const uword expected_digits[] = {kMaxUword, uword{kMaxWord}, kMaxUword};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderSubWithNonIntSelfRaisesTypeError) {
  HandleScope scope(thread_);

  Str str(&scope, Str::empty());
  Int right(&scope, runtime_.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, str, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderSubWithNonIntRightReturnsNotImplemented) {
  HandleScope scope(thread_);

  Int left(&scope, runtime_.newInt(1));
  Str str(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, str));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderXorWithSmallIntsReturnsSmallInt) {
  HandleScope scope(thread_);
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x2D));  // 0b101101
}

TEST_F(IntBuiltinsTest, DunderXorWithLargeIntsReturnsLargeInt) {
  HandleScope scope(thread_);
  const uword digits_left[] = {0x0f, 0x30, 0xCAFE};
  Int left(&scope, newIntWithDigits(&runtime_, digits_left));
  const uword digits_right[] = {0x03, 0xf0};
  Int right(&scope, newIntWithDigits(&runtime_, digits_right));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  const uword expected_digits[] = {0x0C, 0xC0, 0xCAFE};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(IntBuiltinsTest, DunderXorWithNonIntReturnsNotImplemented) {
  HandleScope scope(thread_);
  const uword digits[] = {1, 2};
  Int left(&scope, newIntWithDigits(&runtime_, digits));
  Object right(&scope, Str::empty());
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(IntBuiltinsTest, DunderXorWithInvalidArgumentLeftRaisesException) {
  HandleScope scope(thread_);
  Object left(&scope, Str::empty());
  const uword digits[] = {1, 2};
  LargeInt right(&scope, newIntWithDigits(&runtime_, digits));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, DunderXorWithIntSubclassReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
left = X(0b0011)
right = X(0b0101)
)")
                   .isError());
  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_EQ(result, SmallInt::fromWord(6));  // 0b0110
}

TEST_F(IntBuiltinsTest, ToBytesWithByteorderLittleEndianReturnsBytes) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(3));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));

  const byte bytes[] = {42, 0, 0};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(IntBuiltinsTest, ToBytesWithIntSubclassReturnsBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class X(int): pass
num = X(42)
length = X(3)
)")
                   .isError());
  Object num(&scope, mainModuleAt(&runtime_, "num"));
  Object length(&scope, mainModuleAt(&runtime_, "length"));
  Object byteorder(&scope, runtime_.newStrFromCStr("little"));
  Object signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  const byte bytes[] = {42, 0, 0};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(IntBuiltinsTest, ToBytesWithByteorderBigEndianReturnsBytes) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(2));
  Str byteorder(&scope, runtime_.newStrFromCStr("big"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  const byte bytes[] = {0, 42};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(IntBuiltinsTest, ToBytesKwReturnsBytes) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
x0 = (0x1234).to_bytes(2, 'little')
x1 = (0x1234).to_bytes(2, 'little', signed=False)
x2 = (0x1234).to_bytes(2, 'little', signed=True)
x3 = (0x1234).to_bytes(2, byteorder='little')
x4 = (0x1234).to_bytes(length=2, byteorder='little')
x5 = (0x1234).to_bytes(2, byteorder='little', signed=False)
x6 = (0x1234).to_bytes(signed=False, byteorder='little', length=2)
)")
                   .isError());
  const byte bytes[] = {0x34, 0x12};
  for (const char* name : {"x0", "x1", "x2", "x3", "x4", "x5", "x6"}) {
    Object x(&scope, mainModuleAt(&runtime_, name));
    EXPECT_TRUE(isBytesEqualsBytes(x, bytes)) << name;
  }
}

TEST_F(IntBuiltinsTest, ToBytesKwWithNegativeNumberReturnsBytes) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
x0 = (-777).to_bytes(4, 'little', signed=True)
)")
                   .isError());
  Object x(&scope, mainModuleAt(&runtime_, "x0"));
  const byte bytes[] = {0xf7, 0xfc, 0xff, 0xff};
  EXPECT_TRUE(isBytesEqualsBytes(x, bytes));
}

TEST_F(IntBuiltinsTest, ToBytesWithSignedFalseReturnsBytes) {
  HandleScope scope(thread_);

  Str byteorder(&scope, runtime_.newStrFromCStr("little"));

  // Test that the following numbers work fine with `signed=False` (they are the
  // same numbers that are expected to overflow with `signed=True` in
  // ToBytesWithSignedTrueOverflowRaisesOverflowError)
  Int length_1(&scope, SmallInt::fromWord(1));
  Int num_128(&scope, SmallInt::fromWord(128));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result_128(&scope, runBuiltin(IntBuiltins::toBytes, num_128, length_1,
                                       byteorder, signed_obj));
  const byte bytes[] = {0x80};
  EXPECT_TRUE(isBytesEqualsBytes(result_128, bytes));

  Int length_2(&scope, SmallInt::fromWord(2));
  Int num_32768(&scope, SmallInt::fromWord(32768));
  Object result_32768(&scope, runBuiltin(IntBuiltins::toBytes, num_32768,
                                         length_2, byteorder, signed_obj));
  const byte bytes2[] = {0, 0x80};
  EXPECT_TRUE(isBytesEqualsBytes(result_32768, bytes2));

  Int length_8(&scope, SmallInt::fromWord(8));
  const uword digits[] = {0x8000000000000000, 0};
  Int num_min_word(&scope, newIntWithDigits(&runtime_, digits));
  Object result_min_word(&scope, runBuiltin(IntBuiltins::toBytes, num_min_word,
                                            length_8, byteorder, signed_obj));
  const byte bytes3[] = {0, 0, 0, 0, 0, 0, 0, 0x80};
  EXPECT_TRUE(isBytesEqualsBytes(result_min_word, bytes3));
}

TEST_F(IntBuiltinsTest, ToBytesWithLargeBufferByteorderBigEndianReturnsBytes) {
  HandleScope scope(thread_);

  // Test sign extension with zero when the buffer is larger than necessary.
  Int num(&scope, SmallInt::fromWord(0xcafebabe));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime_.newStrFromCStr("big"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  const byte bytes[] = {0, 0, 0, 0, 0, 0, 0xca, 0xfe, 0xba, 0xbe};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(IntBuiltinsTest,
       ToBytesWithLargeBufferByteorderLittleEndianReturnsBytes) {
  HandleScope scope(thread_);

  // Test sign extension with zero when the buffer is larger than necessary.
  Int num(&scope, SmallInt::fromWord(0xcafebabe));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  const byte bytes[] = {0xbe, 0xba, 0xfe, 0xca, 0, 0, 0, 0, 0, 0};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(IntBuiltinsTest, ToBytesWithSignedTrueReturnsBytes) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = (0x7fffffffffffffff).to_bytes(8, 'little', signed=True)
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte bytes[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result_n_128 = (-128).to_bytes(1, 'little', signed=True)
)")
                   .isError());
  Object result_n_128(&scope, mainModuleAt(&runtime_, "result_n_128"));
  const byte bytes2[] = {0x80};
  EXPECT_TRUE(isBytesEqualsBytes(result_n_128, bytes2));

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result_n_32768 = (-32768).to_bytes(2, 'little', signed=True)
)")
                   .isError());
  Object result_n_32768(&scope, mainModuleAt(&runtime_, "result_n_32768"));
  const byte bytes3[] = {0, 0x80};
  EXPECT_TRUE(isBytesEqualsBytes(result_n_32768, bytes3));

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result_n_min_word = (-9223372036854775808).to_bytes(8, 'little', signed=True)
)")
                   .isError());
  Object result_n_min_word(&scope,
                           mainModuleAt(&runtime_, "result_n_min_word"));
  const byte bytes4[] = {0, 0, 0, 0, 0, 0, 0, 0x80};
  EXPECT_TRUE(isBytesEqualsBytes(result_n_min_word, bytes4));
}

TEST_F(IntBuiltinsTest,
       ToBytesWithNegativeNumberLargeBufferBigEndianReturnsBytes) {
  HandleScope scope(thread_);

  // test sign extension for negative number when buffer is larger than
  // necessary.
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = (-1024).to_bytes(7, 'big', signed=True)
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte bytes[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(IntBuiltinsTest, ToBytesWithZeroLengthBigEndianReturnsEmptyBytes) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(0));
  Int length(&scope, SmallInt::fromWord(0));
  Str byteorder(&scope, runtime_.newStrFromCStr("big"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  ASSERT_TRUE(isBytesEqualsBytes(result, View<byte>(nullptr, 0)));
}

TEST_F(IntBuiltinsTest, ToBytesWithZeroLengthLittleEndianReturnsEmptyBytes) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(0));
  Int length(&scope, SmallInt::fromWord(0));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  ASSERT_TRUE(isBytesEqualsBytes(result, View<byte>(nullptr, 0)));
}

TEST_F(IntBuiltinsTest, ToBytesWithSignedFalseRaisesOverflowError) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(256));
  Int length(&scope, SmallInt::fromWord(1));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST_F(IntBuiltinsTest, ToBytesWithBigOverflowRaisesOverflowError) {
  HandleScope scope(thread_);

  const uword digits[] = {1, 2, 3};
  Int num(&scope, newIntWithDigits(&runtime_, digits));
  Int length(&scope, SmallInt::fromWord(13));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST_F(IntBuiltinsTest, ToBytesWithSignedTrueRaisesOverflowError) {
  Thread* thread = Thread::current();

  // Now check that signed=True with the same inputs triggers an error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
result = (128).to_bytes(1, 'little', signed=True)
)"),
                            LayoutId::kOverflowError,
                            "int too big to convert"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
result = (32768).to_bytes(2, 'little', signed=True)
)"),
                            LayoutId::kOverflowError,
                            "int too big to convert"));
  thread->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
result = (0x8000000000000000).to_bytes(8, 'little', signed=True)
)"),
                            LayoutId::kOverflowError,
                            "int too big to convert"));
}

TEST_F(IntBuiltinsTest, ToBytesWithNonIntRaisesTypeError) {
  HandleScope scope(thread_);

  Str str(&scope, runtime_.newStrFromCStr("not an int"));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, str, length, byteorder,
                                   signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, ToBytesWithInvalidLengthArgRaisesTypeError) {
  HandleScope scope(thread_);
  Int num(&scope, SmallInt::fromWord(42));
  Str not_a_length(&scope, runtime_.newStrFromCStr("not a length"));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, not_a_length,
                                   byteorder, signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(IntBuiltinsTest, ToBytesWithInvalidLengthArgRaisesValueError) {
  HandleScope scope(thread_);
  Int num(&scope, SmallInt::fromWord(42));
  Int negative_length(&scope, SmallInt::fromWord(-3));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, negative_length,
                                   byteorder, signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST_F(IntBuiltinsTest, ToBytesWithInvalidLengthArgRaisesOverflowError) {
  HandleScope scope(thread_);
  Int num(&scope, SmallInt::fromWord(42));
  const uword digits[] = {0, 1024};
  Int huge_length(&scope, newIntWithDigits(&runtime_, digits));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, huge_length,
                                   byteorder, signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST_F(IntBuiltinsTest, ToBytesWithNegativeNumberRaisesOverflowError) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(-1));
  Int length(&scope, SmallInt::fromWord(10));
  Str byteorder(&scope, runtime_.newStrFromCStr("little"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length, byteorder,
                                   signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST_F(IntBuiltinsTest, ToBytesWithInvalidByteorderStringRaisesValueError) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(3));
  Str invalid_byteorder(&scope, runtime_.newStrFromCStr("hello"));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, length,
                                   invalid_byteorder, signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST_F(IntBuiltinsTest, ToBytesWithInvalidByteorderTypeRaisesTypeError) {
  HandleScope scope(thread_);

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(3));
  Bool signed_obj(&scope, Bool::falseObj());
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, num, signed_obj));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(BoolBuiltinsTest, NewFromNonZeroIntegerReturnsTrue) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kBool));
  Int num(&scope, SmallInt::fromWord(2));

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, num));
  EXPECT_TRUE(result.value());
}

TEST_F(BoolBuiltinsTest, NewFromZerorReturnsFalse) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kBool));
  Int num(&scope, SmallInt::fromWord(0));

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, num));
  EXPECT_FALSE(result.value());
}

TEST_F(BoolBuiltinsTest, NewFromTrueReturnsTrue) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kBool));
  Object true_obj(&scope, Bool::trueObj());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, true_obj));
  EXPECT_TRUE(result.value());
}

TEST_F(BoolBuiltinsTest, NewFromFalseReturnsTrue) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kBool));
  Object false_obj(&scope, Bool::falseObj());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, false_obj));
  EXPECT_FALSE(result.value());
}

TEST_F(BoolBuiltinsTest, NewFromNoneIsFalse) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kBool));
  Object none(&scope, NoneType::object());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, none));
  EXPECT_FALSE(result.value());
}

TEST_F(BoolBuiltinsTest, NewFromUserDefinedType) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __bool__(self):
    return True

class Bar:
  def __bool__(self):
    return False

foo = Foo()
bar = Bar()
)")
                   .isError());
  HandleScope scope(thread_);
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  Object bar(&scope, mainModuleAt(&runtime_, "bar"));

  {
    Type type(&scope, runtime_.typeAt(LayoutId::kBool));
    Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, foo));
    EXPECT_TRUE(result.value());
  }
  {
    Type type(&scope, runtime_.typeAt(LayoutId::kBool));
    Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, bar));
    EXPECT_FALSE(result.value());
  }
}

TEST_F(IntBuiltinsTest, DunderTrueDivWithZeroLeftReturnsZero) {
  HandleScope scope(thread_);
  Int left(&scope, SmallInt::fromWord(0));
  Int right(&scope, SmallInt::fromWord(17));
  Object result(&scope, runBuiltin(IntBuiltins::dunderTrueDiv, left, right));
  ASSERT_TRUE(result.isFloat());
  Float flt(&scope, *result);
  EXPECT_EQ(flt.value(), 0.0);
}

TEST_F(IntBuiltinsTest, DunderTrueDivWithBoolFalseRaisesZeroDivisionError) {
  HandleScope scope(thread_);
  Object numerator(&scope, SmallInt::fromWord(10));
  Object denominator(&scope, Bool::falseObj());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(IntBuiltins::dunderTrueDiv, numerator, denominator),
      LayoutId::kZeroDivisionError, "division by zero"));
}

TEST_F(IntBuiltinsTest, DunderTrueDivWithIntZeroRaisesZeroDivisionError) {
  HandleScope scope(thread_);
  Object numerator(&scope, SmallInt::fromWord(10));
  Object denominator(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(IntBuiltins::dunderTrueDiv, numerator, denominator),
      LayoutId::kZeroDivisionError, "division by zero"));
}

TEST_F(IntBuiltinsTest, DunderTrueDivWithNonIntLeftRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "int.__truediv__(1.0, 2)"), LayoutId::kTypeError,
      "'__truediv__' requires a 'int' object but got 'float'"));
}

TEST_F(IntBuiltinsTest, DunderTrueDivWithFloatRightReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object left(&scope, SmallInt::fromWord(100));
  Object right(&scope, runtime_.newFloat(1.5));
  Object result(&scope, runBuiltin(IntBuiltins::dunderTrueDiv, left, right));
  EXPECT_EQ(result, NotImplementedType::object());
}

TEST_F(IntBuiltinsTest, DunderTrueDivWithSmallIntsReturnsFloat) {
  HandleScope scope(thread_);

  Object num1(&scope, SmallInt::fromWord(6));
  Object num2(&scope, SmallInt::fromWord(3));
  Float result(&scope, runBuiltin(IntBuiltins::dunderTrueDiv, num1, num2));
  EXPECT_NEAR(result.value(), 2.0, DBL_EPSILON);

  num1 = SmallInt::fromWord(7);
  num2 = SmallInt::fromWord(3);
  result = runBuiltin(IntBuiltins::dunderTrueDiv, num1, num2);
  EXPECT_NEAR(result.value(), 2.3333333333333335, DBL_EPSILON);
}

TEST_F(IntBuiltinsTest, ConjugateAliasesDunderInt) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, moduleAtByCStr(&runtime_, "builtins", "int"));
  Object conjugate_name(&scope,
                        Runtime::internStrFromCStr(thread_, "conjugate"));
  Object conjugate_obj(&scope, typeAt(type, conjugate_name));
  ASSERT_TRUE(conjugate_obj.isFunction());
  Function conjugate(&scope, *conjugate_obj);
  Object dint_name(&scope, Runtime::internStrFromCStr(thread_, "__int__"));
  Object dint_obj(&scope, typeAt(type, dint_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *conjugate_obj);
  EXPECT_EQ(Code::cast(conjugate.code()).code(),
            Code::cast(dint.code()).code());
  EXPECT_EQ(conjugate.entry(), dint.entry());
  EXPECT_EQ(conjugate.entryKw(), dint.entryKw());
  EXPECT_EQ(conjugate.entryEx(), dint.entryEx());
}

TEST_F(IntBuiltinsTest, DenominatorReturnsOne) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = (44).denominator").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(IntBuiltinsTest, ImagReturnsZero) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = (44).imag").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(IntBuiltinsTest, NumeratorReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = (44).numerator").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 44));
}

TEST_F(IntBuiltinsTest, RealReturnsInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = (44).real").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 44));
}

TEST_F(IntBuiltinsTest, CompareWithBigNegativeNumber) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "a = -46116860184273879030000").isError());
  HandleScope scope(thread_);
  Int a(&scope, mainModuleAt(&runtime_, "a"));
  Int b(&scope, SmallInt::fromWord(SmallInt::kMinValue));
  EXPECT_LT(a.compare(*b), 0);
  EXPECT_GT(b.compare(*a), 0);
}

}  // namespace py
