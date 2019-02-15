#include "gtest/gtest.h"

#include <cmath>
#include <limits>

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
  EXPECT_EQ(integer.builtinBase(), LayoutId::kInt);

  Type small_int(&scope, runtime.typeAt(LayoutId::kSmallInt));
  EXPECT_EQ(small_int.builtinBase(), LayoutId::kInt);

  Type large_int(&scope, runtime.typeAt(LayoutId::kLargeInt));
  EXPECT_EQ(large_int.builtinBase(), LayoutId::kInt);

  Type boolean(&scope, runtime.typeAt(LayoutId::kBool));
  EXPECT_EQ(boolean.builtinBase(), LayoutId::kInt);
}

TEST(IntBuiltinsTest, NewWithStringReturnsInt) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
a = int("123")
b = int("-987")
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 123));
  EXPECT_TRUE(isIntEqualsWord(*b, -987));
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
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(isIntEqualsWord(*a, 19));
  EXPECT_TRUE(isIntEqualsWord(*b, 2748));
  EXPECT_TRUE(isIntEqualsWord(*c, 19));
  EXPECT_TRUE(isIntEqualsWord(*d, 2748));
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
  EXPECT_TRUE(isIntEqualsWord(*plus_pos, 123));

  Object plus_neg(&scope, moduleAt(&runtime, "__main__", "plus_neg"));
  EXPECT_TRUE(isIntEqualsWord(*plus_neg, -123));
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
  EXPECT_TRUE(isIntEqualsWord(*minus_pos, -123));

  Object minus_neg(&scope, moduleAt(&runtime, "__main__", "minus_neg"));
  EXPECT_TRUE(isIntEqualsWord(*minus_neg, 123));
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
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  EXPECT_TRUE(isIntEqualsWord(*b, 1));
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
  EXPECT_TRUE(isIntEqualsWord(*a, 10));
  EXPECT_TRUE(isIntEqualsWord(*b, 5));
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
  EXPECT_TRUE(isIntEqualsWord(*a, 2));
  EXPECT_TRUE(isIntEqualsWord(*b, 5));
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
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 3));
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
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  EXPECT_TRUE(isIntEqualsWord(*b, 10));
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
  EXPECT_TRUE(isIntEqualsWord(*a, 0xFD));
  EXPECT_TRUE(isIntEqualsWord(*b, 0xFE));
}

TEST(IntBuiltinsTest, DunderAbsWithBoolFalseReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int self(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderAbsWithBoolTrueReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int self(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST(IntBuiltinsTest, DunderAbsWithPositiveIntReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Int self(&scope, runtime.newInt(1234));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST(IntBuiltinsTest, DunderAbsWithNegativeIntReturnsInt) {
  Runtime runtime;
  HandleScope scope;
  Int self(&scope, newIntWithDigits(&runtime,
                                    {0x154a0071b091fb7e, 0x9661bb54b4e68c59}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAbs, self));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0xeab5ff8e4f6e0482, 0x699e44ab4b1973a6}));
}

TEST(IntBuiltinsTest, DunderAddWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(42));
  Int right(&scope, SmallInt::fromWord(-7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 35));
}

TEST(IntBuiltinsTest, DunderAddWithSmallIntsOverflowReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int max_small_int(&scope, SmallInt::fromWord(RawSmallInt::kMaxValue));
  Int one(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, max_small_int, one));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMaxValue + 1));
}

TEST(IntBuiltinsTest, DunderAddWithLargeInts) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime,
                                    {0xfedcba0987654321, 0x1234567890abcdef}));
  Int right(&scope, newIntWithDigits(&runtime,
                                     {0x9876543210abcdef, 0xfedcba0123456789}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0x97530e3b98111110, 0x11111079b3f13579}));
}

TEST(IntBuiltinsTest, DunderAddWithPositiveLargeIntsCarrying) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime, {kMaxUword, kMaxUword, 0}));
  Int right(&scope, newIntWithDigits(&runtime, {1}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0, 0, 1}));
}

TEST(IntBuiltinsTest, DunderAddWithNegativeLargeIntsCarrying) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime, {kMaxUword}));  // == -1.
  // The smallest negative number representable with 2 digits.
  Int right(&scope,
            newIntWithDigits(&runtime, {0, static_cast<uword>(kMinWord)}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAdd, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {kMaxUword, kMaxWord, kMaxUword}));
}

TEST(IntBuiltinsTest, DunderAndWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x10));  // 0b10000
}

TEST(IntBuiltinsTest, DunderAndWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {0x0f, 0x30, 0x1}));
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xf0, 0x2, 7}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0x03, 0x30}));
}

TEST(IntBuiltinsTest, DunderAndWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(result.isNotImplemented());
}

TEST(IntBuiltinsTest, DunderAndWithInvalidArgumentLeftRaisesException) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  LargeInt right(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderAnd, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderCeilAliasesDunderInt) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object ceil_name(&scope, runtime.newStrFromCStr("__ceil__"));
  Object ceil_obj(&scope, runtime.typeDictAt(dict, ceil_name));
  ASSERT_TRUE(ceil_obj.isFunction());
  Function ceil(&scope, *ceil_obj);
  Object dint_name(&scope, runtime.newStrFromCStr("__int__"));
  Object dint_obj(&scope, runtime.typeDictAt(dict, ceil_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *ceil_obj);
  EXPECT_EQ(RawCode::cast(ceil.code()).code(),
            RawCode::cast(dint.code()).code());
  EXPECT_EQ(ceil.entry(), dint.entry());
  EXPECT_EQ(ceil.entryKw(), dint.entryKw());
  EXPECT_EQ(ceil.entryEx(), dint.entryEx());
}

TEST(IntBuiltinsTest, DunderFloorAliasesDunderInt) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object floor_name(&scope, runtime.newStrFromCStr("__floor__"));
  Object floor_obj(&scope, runtime.typeDictAt(dict, floor_name));
  ASSERT_TRUE(floor_obj.isFunction());
  Function floor(&scope, *floor_obj);
  Object dint_name(&scope, runtime.newStrFromCStr("__int__"));
  Object dint_obj(&scope, runtime.typeDictAt(dict, floor_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *floor_obj);
  EXPECT_EQ(RawCode::cast(floor.code()).code(),
            RawCode::cast(dint.code()).code());
  EXPECT_EQ(floor.entry(), dint.entry());
  EXPECT_EQ(floor.entryKw(), dint.entryKw());
  EXPECT_EQ(floor.entryEx(), dint.entryEx());
}

TEST(IntBuiltinsTest, DunderLshiftWithBoolsTrueFalseReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, Bool::trueObj());
  Object right(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST(IntBuiltinsTest, DunderLshiftWithBoolsFalseTrueReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, Bool::falseObj());
  Object right(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderLshiftWithBoolSmallIntReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, Bool::trueObj());
  Object right(&scope, runtime.newInt(kBitsPerWord));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0, 1}));
}

TEST(IntBuiltinsTest, DunderLshiftWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(0xd));  // 0b1101
  Object right(&scope, runtime.newInt(3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x68));  // 0b1101000
}

TEST(IntBuiltinsTest, DunderLshiftWithNegativeSmallIntReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(-2));
  Object right(&scope, runtime.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -4));
}

TEST(IntBuiltinsTest, DunderLshiftWithZeroReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(0));
  Object right(&scope, newIntWithDigits(&runtime, {1, 2, 3, 4}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderLshiftWithBigSmallIntReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(RawSmallInt::kMaxValue >> 1));
  Object right(&scope, runtime.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMaxValue - 1));
}

TEST(IntBuiltinsTest, DunderLshiftWithBigNegativeSmallIntReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(RawSmallInt::kMinValue >> 1));
  Object right(&scope, runtime.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMinValue));
}

TEST(IntBuiltinsTest, DunderLshiftWithSmallIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(4));
  Object right(&scope, runtime.newInt(kBitsPerWord - 4));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  ASSERT_TRUE(result_obj.isLargeInt());
  LargeInt result(&scope, *result_obj);
  EXPECT_EQ(result.numDigits(), 1);
  EXPECT_EQ(result.digitAt(0), uword{1} << (kBitsPerWord - 2));
}

TEST(IntBuiltinsTest, DunderLshiftWithSmallIntsNegativeReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(-4));
  Object right(&scope, runtime.newInt(kBitsPerWord - 3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(
      *result, {static_cast<uword>(-4) << (kBitsPerWord - 3)}));
}

TEST(IntBuiltinsTest, DunderLshiftWithSmallIntOverflowReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(4));
  Object right(&scope, runtime.newInt(kBitsPerWord - 3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {uword{1} << (kBitsPerWord - 1), 0}));
}

TEST(IntBuiltinsTest, DunderLshiftWithNegativeSmallIntOverflowReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(-4));
  Object right(&scope, runtime.newInt(kBitsPerWord - 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0, kMaxUword}));
}

TEST(IntBuiltinsTest, DunderLshiftWithLargeIntReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, newIntWithDigits(&runtime, {1, 1}));
  Object right(&scope, runtime.newInt(2 * kBitsPerWord + 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0, 0, 4, 4}));
}

TEST(IntBuiltinsTest, DunderLshiftWithNegativeLargeIntReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope,
              newIntWithDigits(&runtime, {kMaxUword - 1, kMaxUword - 1}));
  Object right(&scope, runtime.newInt(2 * kBitsPerWord + 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0, 0, kMaxUword - 7, kMaxUword - 4}));
}

TEST(IntBuiltinsTest, DunderLshiftWithLargeIntWholeWordReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, newIntWithDigits(
                          &runtime, {0xfe84754526de453c, 0x47e8218b97f94763}));
  Object right(&scope, runtime.newInt(kBitsPerWord * 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(
      *result, {0, 0, 0xfe84754526de453c, 0x47e8218b97f94763}));
}

TEST(IntBuiltinsTest, DunderLshiftWithNegativeShiftAmountRaiseValueError) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(0));
  Object right(&scope, runtime.newInt(-1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kValueError, "negative shift count"));
}

TEST(IntBuiltinsTest, DunderLshiftWithNonIntSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  Object right(&scope, runtime.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderLshiftWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(0));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderLshift, left, right));
  EXPECT_TRUE(result.isNotImplemented());
}

TEST(IntBuiltinsTest, DunderMulWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, runtime.newInt(13));
  Int right(&scope, runtime.newInt(-3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -39));
}

TEST(IntBuiltinsTest, DunderMulWithSmallIntsReturnsSingleDigitLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, RawSmallInt::fromWord(RawSmallInt::kMaxValue));
  Int right(&scope, RawSmallInt::fromWord(2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMaxValue * 2));
}

TEST(IntBuiltinsTest, DunderMulWithSmallIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, RawSmallInt::fromWord(RawSmallInt::kMaxValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, num, num));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0x8000000000000001, 0xfffffffffffffff}));
}

TEST(IntBuiltinsTest, DunderMulWithSmallIntLargeIntReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, RawSmallInt::fromWord(-3));
  Int right(&scope,
            newIntWithDigits(&runtime, {0xa1b2c3d4e5f67890, 0xaabbccddeeff}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0x1ae7b4814e1c9650, 0xfffdffcc99663301}));
}

TEST(IntBuiltinsTest, DunderMulWithZeroReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime, {0, 1}));
  Int right(&scope, RawSmallInt::fromWord(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderMulWithPositiveLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime,
                                    {0xfedcba0987654321, 0x1234567890abcdef}));
  Int right(&scope, newIntWithDigits(
                        &runtime, {0x0123456789abcdef, 0xfedcba9876543210, 0}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0x2236d928fe5618cf, 0xaa6c87569f0ec6a4,
                                  0x213cff7595234949, 0x121fa00acd77d743}));
}

TEST(IntBuiltinsTest, DunderMulWithMaxPositiveLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, newIntWithDigits(&runtime, {kMaxUword, 0}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, num, num));
  EXPECT_TRUE(isIntEqualsDigits(*result, {1, kMaxUword - 1, 0}));
}

TEST(IntBuiltinsTest, DunderMulWithNegativeLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  // Smallest negative number representable with 2 digits.
  Int num(&scope,
          newIntWithDigits(&runtime, {0, static_cast<uword>(kMinWord)}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, num, num));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0, 0, 0, static_cast<uword>(kMinWord) >> 1}));
}

TEST(IntBuiltinsTest, DunderMulWithNegativePositiveLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime, {0xada6d35d8ef7c790}));
  Int right(&scope, newIntWithDigits(&runtime,
                                     {0x3ff2ca02c44fbb1c, 0x5873a2744317c09a}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsDigits(
      *result, {0x6d80780b775003c0, 0xb46184fc0839baa0, 0xe38c265747f0661f}));
}

TEST(IntBuiltinsTest, DunderMulWithPositiveNegativeLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime,
                                    {0x3ff2ca02c44fbb1c, 0x5873a2744317c09a}));
  Int right(&scope, newIntWithDigits(&runtime, {0xada6d35d8ef7c790}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, right));
  EXPECT_TRUE(isIntEqualsDigits(
      *result, {0x6d80780b775003c0, 0xb46184fc0839baa0, 0xe38c265747f0661f}));
}

TEST(IntBuiltinsTest, DunderMulWithNonIntSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Str str(&scope, runtime.newStrFromCStr(""));
  Int right(&scope, runtime.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, str, right));
  ASSERT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderMulWithNonIntRightReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, runtime.newInt(1));
  Str str(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderMul, left, str));
  ASSERT_TRUE(result.isNotImplemented());
}

TEST(IntBuiltinsTest, DunderOrWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x3D));  // 0b111101
}

TEST(IntBuiltinsTest, DunderOrWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {0x0C, 0xB0, 0xCAFE}));
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xD0}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0x0F, 0xF0, 0xCAFE}));
}

TEST(IntBuiltinsTest, DunderOrWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
  EXPECT_TRUE(result.isNotImplemented());
}

TEST(IntBuiltinsTest, DunderOrWithInvalidArgumentLeftRaisesException) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  LargeInt right(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderOr, left, right));
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
  EXPECT_TRUE(isIntEqualsWord(*c, 3));
}

TEST(IntBuiltinsTest, BitLength) {
  Runtime runtime;
  HandleScope scope;

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
  num = runtime.newInt(kMaxInt64);
  Object bit_length5(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length5, 63));

  // (kMinInt64).bit_length() == 64
  num = runtime.newInt(kMinInt64);
  Object bit_length6(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length6, 64));

  uword digits[] = {0, kMaxInt32};
  num = runtime.newIntWithDigits(digits);
  Object bit_length7(&scope, runBuiltin(IntBuiltins::bitLength, num));
  // 31 bits for kMaxInt32 + 64 bits
  EXPECT_TRUE(isIntEqualsWord(*bit_length7, 95));

  // (kMinInt64 * 4).bit_length() == 66
  uword digits2[] = {0, kMaxUword - 1};  // kMaxUword - 1 == -2
  num = runtime.newIntWithDigits(digits2);
  Object bit_length8(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length8, 66));

  // (kMinInt64 * 4 + 3).bit_length() == 65
  uword digits3[] = {3, kMaxUword - 1};  // kMaxUword - 1 == -2
  num = runtime.newIntWithDigits(digits3);
  Object bit_length9(&scope, runBuiltin(IntBuiltins::bitLength, num));
  EXPECT_TRUE(isIntEqualsWord(*bit_length9, 65));
}

TEST(IntBuiltinsTest, CompareLargeIntEq) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
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

TEST(IntBuiltinsTest, CompareLargeIntNe) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
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

TEST(IntBuiltinsTest, DunderFloatWithBoolReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, Bool::trueObj());
  Object a_float(&scope, runBuiltin(IntBuiltins::dunderFloat, a));
  ASSERT_TRUE(a_float.isFloat());
  EXPECT_EQ(RawFloat::cast(*a_float)->value(), 1.0);

  Object b(&scope, Bool::falseObj());
  Object b_float(&scope, runBuiltin(IntBuiltins::dunderFloat, b));
  ASSERT_TRUE(b_float.isFloat());
  EXPECT_EQ(RawFloat::cast(*b_float)->value(), 0.0);
}

TEST(IntBuiltinsTest, DunderFloatWithSmallIntReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, RawSmallInt::fromWord(-7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(), -7.0);
}

TEST(IntBuiltinsTest, DunderFloatWithOneDigitLargeIntReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, newIntWithDigits(&runtime, {static_cast<uword>(kMinWord)}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(), static_cast<double>(kMinWord));
}

TEST(IntBuiltinsTest, DunderFloatWithLargeIntReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, newIntWithDigits(&runtime, {0x85b3f6fb0496ac6f, 0x129ef6}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(),
            std::strtod("0x1.29ef685b3f6fbp+84", nullptr));
}

TEST(IntBuiltinsTest, DunderFloatWithNegativeLargeIntReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope,
          newIntWithDigits(&runtime, {0x937822557f9bad3f, 0xb31911a86c86a071}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(),
            std::strtod("-0x1.339bb95e4de58p+126", nullptr));
}

TEST(IntBuiltinsTest,
     DunderFloatWithNegativeLargeIntMagnitudeComputationCarriesReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, newIntWithDigits(&runtime, {1, 0, 0, 0xfffedcc000000000}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(),
            std::strtod("-0x1.234p240", nullptr));
}

TEST(IntBuiltinsTest, DunderFloatWithLargeIntRoundedDownReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  // Produce a 1 so that all of the mantissa lies in the high digit but the bit
  // triggering the rounding is in the low digit.
  uword mantissa_high_bit = static_cast<uword>(1) << kDoubleMantissaBits;
  Int num(&scope, newIntWithDigits(&runtime, {0, mantissa_high_bit}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(), std::strtod("0x1.p116", nullptr));
}

TEST(IntBuiltinsTest, DunderFloatWithLargeIntRoundedDownToEvenReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  uword mantissa_high_bit = static_cast<uword>(1) << kDoubleMantissaBits;
  uword high_one = static_cast<uword>(1) << (kBitsPerWord - 1);
  Int num(&scope, newIntWithDigits(&runtime, {high_one, mantissa_high_bit}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(), std::strtod("0x1.p116", nullptr));
}

TEST(IntBuiltinsTest, DunderFloatWithLargeIntRoundedUpToEvenReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  uword mantissa_high_bit_plus_one =
      (static_cast<uword>(1) << kDoubleMantissaBits) + 1;
  uword high_one = static_cast<uword>(1) << (kBitsPerWord - 1);
  Int num(&scope,
          newIntWithDigits(&runtime, {high_one, mantissa_high_bit_plus_one}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(),
            std::strtod("0x1.0000000000002p116", nullptr));
}

TEST(IntBuiltinsTest,
     DunderFloatWithNegativeLargeIntRoundedDownToEvenReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  uword mantissa_high_bit = static_cast<uword>(1) << kDoubleMantissaBits;
  uword high_one = static_cast<uword>(1) << (kBitsPerWord - 1);
  Int num(&scope,
          newIntWithDigits(&runtime, {0, high_one, ~mantissa_high_bit}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(),
            std::strtod("-0x1.p180", nullptr));
}

TEST(IntBuiltinsTest,
     DunderFloatWithNegativeLargeIntRoundedUpToEvenReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  uword mantissa_high_bit_plus_one =
      (static_cast<uword>(1) << kDoubleMantissaBits) | 1;
  uword high_one = static_cast<uword>(1) << (kBitsPerWord - 1);
  Int num(&scope, newIntWithDigits(&runtime,
                                   {0, high_one, ~mantissa_high_bit_plus_one}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(),
            std::strtod("-0x1.0000000000002p180", nullptr));
}

TEST(IntBuiltinsTest,
     DunderFloatWithLargeIntRoundedUpIncreasingExponentReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  uword mantissa_all_one =
      (static_cast<uword>(1) << (kDoubleMantissaBits + 1)) - 1;
  uword high_one = static_cast<uword>(1) << (kBitsPerWord - 1);
  Int num(&scope, newIntWithDigits(&runtime, {high_one, mantissa_all_one}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(), std::strtod("0x1.p117", nullptr));
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

TEST(IntBuiltinsTest,
     DunderFloatLargestPossibleLargeIntBeforeOverflowReturnsFloat) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, largestIntBeforeFloatOverflow(&runtime));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num));
  ASSERT_TRUE(result.isFloat());
  EXPECT_EQ(RawFloat::cast(*result)->value(),
            std::numeric_limits<double>::max());
}

TEST(IntBuiltinsTest, DunderFloatOverflowRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;

  // Add 1 to the largest number that is still convertible to float.
  Int num0(&scope, largestIntBeforeFloatOverflow(&runtime));
  Int one(&scope, runtime.newInt(1));
  Int num1(&scope, runBuiltin(IntBuiltins::dunderAdd, num0, one));
  Object result(&scope, runBuiltin(IntBuiltins::dunderFloat, num1));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
}

TEST(IntBuiltinsTest, DunderFloatWithNonIntReturnsError) {
  Runtime runtime;
  HandleScope scope;

  Str str(&scope, runtime.newStrFromCStr("python"));
  Object str_res(&scope, runBuiltin(IntBuiltins::dunderInt, str));
  EXPECT_TRUE(str_res.isError());
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));

  Float flt(&scope, runtime.newFloat(1.0));
  Object flt_res(&scope, runBuiltin(IntBuiltins::dunderInt, flt));
  EXPECT_TRUE(flt_res.isError());
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
}

TEST(LargeIntBuiltinsTest, UnaryNegateTest) {
  Runtime runtime;
  HandleScope scope;

  Object smallint_max(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  Object a(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_max));
  EXPECT_TRUE(isIntEqualsWord(*a, -RawSmallInt::kMaxValue));

  Object smallint_max1(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_max1));
  EXPECT_TRUE(isIntEqualsWord(*b, RawSmallInt::kMinValue));

  Object smallint_min(&scope, runtime.newInt(RawSmallInt::kMinValue));
  Object c(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_min));
  EXPECT_TRUE(isIntEqualsWord(*c, -RawSmallInt::kMinValue));

  Object smallint_min1(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
  Object d(&scope, runBuiltin(IntBuiltins::dunderNeg, smallint_min1));
  EXPECT_TRUE(isIntEqualsWord(*d, -(RawSmallInt::kMinValue - 1)));

  Int min_word(&scope, runtime.newInt(kMinWord));
  Object e(&scope, runBuiltin(IntBuiltins::dunderNeg, min_word));
  ASSERT_TRUE(e.isLargeInt());
  LargeInt large_e(&scope, *e);
  EXPECT_TRUE(large_e.isPositive());
  Int max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_EQ(RawInt::cast(*large_e)->compare(*max_word), 1);
  EXPECT_EQ(large_e.numDigits(), 2);
  EXPECT_EQ(large_e.digitAt(0), uword{1} << (kBitsPerWord - 1));
  EXPECT_EQ(large_e.digitAt(1), uword{0});
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

TEST(IntBuiltinsTest, CompareLargeIntLe) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
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

TEST(IntBuiltinsTest, CompareLargeIntGt) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
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

TEST(IntBuiltinsTest, CompareLargeIntLt) {
  Runtime runtime;
  HandleScope scope;

  Object a(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object b(&scope, runtime.newInt(RawSmallInt::kMinValue - 1));
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

TEST(IntBuiltinsTest, StringToIntDPos) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Object str_d0(&scope, runtime.newStrFromCStr("0"));
  SmallInt int_d0(&scope, IntBuiltins::intFromString(thread, *str_d0, 10));
  EXPECT_EQ(int_d0.value(), 0);

  Object str_d123(&scope, runtime.newStrFromCStr("123"));
  SmallInt int_d123(&scope, IntBuiltins::intFromString(thread, *str_d123, 10));
  EXPECT_EQ(int_d123.value(), 123);

  Object str_d987n(&scope, runtime.newStrFromCStr("-987"));
  SmallInt int_d987n(&scope,
                     IntBuiltins::intFromString(thread, *str_d987n, 10));
  EXPECT_EQ(int_d987n.value(), -987);
}

TEST(IntBuiltinsTest, StringToIntDNeg) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Object str1(&scope, runtime.newStrFromCStr(""));
  Object res1(&scope, IntBuiltins::intFromString(thread, *str1, 10));
  EXPECT_TRUE(res1.isError());

  Object str2(&scope, runtime.newStrFromCStr("12ab"));
  Object res2(&scope, IntBuiltins::intFromString(thread, *str2, 10));
  EXPECT_TRUE(res2.isError());
}

TEST(IntBuiltinsTest, DunderIndexAliasesDunderInt) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object index_name(&scope, runtime.newStrFromCStr("__index__"));
  Object index_obj(&scope, runtime.typeDictAt(dict, index_name));
  ASSERT_TRUE(index_obj.isFunction());
  Function index(&scope, *index_obj);
  Object dint_name(&scope, runtime.newStrFromCStr("__int__"));
  Object dint_obj(&scope, runtime.typeDictAt(dict, index_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *index_obj);
  EXPECT_EQ(RawCode::cast(index.code()).code(),
            RawCode::cast(dint.code()).code());
  EXPECT_EQ(index.entry(), dint.entry());
  EXPECT_EQ(index.entryKw(), dint.entryKw());
  EXPECT_EQ(index.entryEx(), dint.entryEx());
}

TEST(IntBuiltinsTest, DunderIntWithBoolFalseReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Object self(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInt, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderIntWithBoolTrueReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Object self(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInt, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST(IntBuiltinsTest, DunderIntWithSmallIntReturnsSame) {
  Runtime runtime;
  HandleScope scope;

  Object self(&scope, RawSmallInt::fromWord(7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderInt, self));
  EXPECT_EQ(self, result);
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
  EXPECT_TRUE(isIntEqualsWord(*a, 7));
  EXPECT_TRUE(isIntEqualsWord(*b, 7));

  Str str(&scope, runtime.newStrFromCStr("python"));
  Object res(&scope, runBuiltin(IntBuiltins::dunderInt, str));
  EXPECT_TRUE(res.isError());
}

TEST(IntBuiltinsTest, DunderInvertWithBoolTrueReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(*result, -2));
}

TEST(IntBuiltinsTest, DunderInvertWithBoolFalseReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST(IntBuiltinsTest, DunderInvertWithSmallIntReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, SmallInt::fromWord(-224466));
  Object result(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(*result, 224465));
}

TEST(IntBuiltinsTest, DunderInvertWithLargeIntReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, newIntWithDigits(&runtime, {0x6c5bfcb426758496,
                                                 0xda8bdbe69c009bc5, 0}));
  Object result_obj(&scope, runBuiltin(IntBuiltins::dunderInvert, num));
  ASSERT_TRUE(result_obj.isLargeInt());
  Int result(&scope, *result_obj);
  Int expected(&scope,
               newIntWithDigits(&runtime, {0x93a4034bd98a7b69,
                                           0x2574241963ff643a, kMaxUword}));
  EXPECT_EQ(expected.compare(*result), 0);
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

TEST(IntBuiltinsTest, DunderNegWithSmallIntReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newInt(42));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(isIntEqualsWord(*result, -42));
}

TEST(IntBuiltinsTest, DunderNegWithSmallIntReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newInt(RawSmallInt::kMinValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(isIntEqualsWord(*result, -RawSmallInt::kMinValue));
}

TEST(IntBuiltinsTest, DunderNegWithBoolFalseReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object value(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, value));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderNegWithBoolTrueReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object value(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, value));
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST(IntBuiltinsTest, DunderNegWithLargeIntReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newInt(-RawSmallInt::kMinValue));
  EXPECT_TRUE(num.isLargeInt());
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMinValue));
}

TEST(IntBuiltinsTest, DunderNegWithLargeIntReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, newIntWithDigits(
                         &runtime, {0xad7721b1763aff22, 0x2afce48517f151b2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0x5288de4e89c500de, 0xd5031b7ae80eae4d}));
}

TEST(IntBuiltinsTest, DunderNegWithLargeIntCarriesReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, newIntWithDigits(&runtime, {0, 0xfffffff000000000}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0, 0x1000000000}));
}

TEST(IntBuiltinsTest, DunderNegWithLargeIntOverflowsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope,
             newIntWithDigits(&runtime, {0, uword{1} << (kBitsPerWord - 1)}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderNeg, num));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0, uword{1} << (kBitsPerWord - 1), 0}));
}

TEST(IntBuiltinsTest, DunderPosAliasesDunderInt) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object pos_name(&scope, runtime.newStrFromCStr("__pos__"));
  Object pos_obj(&scope, runtime.typeDictAt(dict, pos_name));
  ASSERT_TRUE(pos_obj.isFunction());
  Function pos(&scope, *pos_obj);
  Object dint_name(&scope, runtime.newStrFromCStr("__int__"));
  Object dint_obj(&scope, runtime.typeDictAt(dict, pos_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *pos_obj);
  EXPECT_EQ(RawCode::cast(pos.code()).code(),
            RawCode::cast(dint.code()).code());
  EXPECT_EQ(pos.entry(), dint.entry());
  EXPECT_EQ(pos.entryKw(), dint.entryKw());
  EXPECT_EQ(pos.entryEx(), dint.entryEx());
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

TEST(IntBuiltinsTest, DunderRoundAliasesDunderInt) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object round_name(&scope, runtime.newStrFromCStr("__round__"));
  Object round_obj(&scope, runtime.typeDictAt(dict, round_name));
  ASSERT_TRUE(round_obj.isFunction());
  Function round(&scope, *round_obj);
  Object dint_name(&scope, runtime.newStrFromCStr("__int__"));
  Object dint_obj(&scope, runtime.typeDictAt(dict, round_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *round_obj);
  EXPECT_EQ(RawCode::cast(round.code()).code(),
            RawCode::cast(dint.code()).code());
  EXPECT_EQ(round.entry(), dint.entry());
  EXPECT_EQ(round.entryKw(), dint.entryKw());
  EXPECT_EQ(round.entryEx(), dint.entryEx());
}

TEST(IntBuiltinsTest, DunderTruncAliasesDunderInt) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object trunc_name(&scope, runtime.newStrFromCStr("__trunc__"));
  Object trunc_obj(&scope, runtime.typeDictAt(dict, trunc_name));
  ASSERT_TRUE(trunc_obj.isFunction());
  Function trunc(&scope, *trunc_obj);
  Object dint_name(&scope, runtime.newStrFromCStr("__int__"));
  Object dint_obj(&scope, runtime.typeDictAt(dict, trunc_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *trunc_obj);
  EXPECT_EQ(RawCode::cast(trunc.code()).code(),
            RawCode::cast(dint.code()).code());
  EXPECT_EQ(trunc.entry(), dint.entry());
  EXPECT_EQ(trunc.entryKw(), dint.entryKw());
  EXPECT_EQ(trunc.entryEx(), dint.entryEx());
}

TEST(IntBuiltinsTest, FromBytesWithLittleEndianReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({0xca, 0xfe}));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xfeca));
}

TEST(IntBuiltinsTest, FromBytesWithLittleEndianReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope,
              runtime.newBytesWithAll({0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23, 0x45,
                                       0x67, 0x89, 0xab, 0xcd}));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  ASSERT_EQ(result.numDigits(), 2);
  EXPECT_EQ(result.digitAt(0), 0x67452301bebafecaU);
  EXPECT_EQ(result.digitAt(1), 0xcdab89U);
}

TEST(IntBuiltinsTest, FromBytesWithBigEndianReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({0xca, 0xfe}));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Object result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xcafe));
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
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST(IntBuiltinsTest, FromBytesWithBigEndianReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope,
              runtime.newBytesWithAll({0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23, 0x45,
                                       0x67, 0x89, 0xab, 0xcd}));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  ASSERT_EQ(result.numDigits(), 2);
  EXPECT_EQ(result.digitAt(0), 0xbe0123456789abcdU);
  EXPECT_EQ(result.digitAt(1), 0xcafebaU);
}

TEST(IntBuiltinsTest, FromBytesWithEmptyBytes) {
  Runtime runtime;
  HandleScope scope;

  Bytes bytes(&scope, runtime.newBytesWithAll({}));
  Str bo_little(&scope, runtime.newStrFromCStr("little"));
  Object result_little(&scope,
                       runBuiltin(IntBuiltins::fromBytes, bytes, bo_little));
  EXPECT_TRUE(isIntEqualsWord(*result_little, 0));

  Str bo_big(&scope, runtime.newStrFromCStr("big"));
  Object result_big(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, bo_big));
  EXPECT_TRUE(isIntEqualsWord(*result_big, 0));
}

TEST(IntBuiltinsTest, FromBytesWithNumberWithDigitHighBitSet) {
  Runtime runtime;
  HandleScope scope;

  // Test special case where a positive number having a high bit set at the end
  // of a "digit" needs an extra digit in the LargeInt representation.
  Bytes bytes(&scope, runtime.newBytes(kWordSize, 0xff));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Int result(&scope, runBuiltin(IntBuiltins::fromBytes, bytes, byteorder));
  EXPECT_TRUE(isIntEqualsDigits(*result, {kMaxUword, 0}));
}

TEST(IntBuiltinsTest, FromBytesWithNegativeNumberReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime,
              "result = int.from_bytes(b'\\xff', 'little', signed=True)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST(IntBuiltinsTest, FromBytesWithNegativeNumberReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
result = int.from_bytes(b'\xca\xfe\xba\xbe\x01\x23\x45\x67\x89\xab\xcd', 'big',
                        signed=True)
)");
  Int result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0xbe0123456789abcd, 0xffffffffffcafeba}));
}

TEST(IntBuiltinsTest, FromBytesWithKwArgumentsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
result = int.from_bytes(byteorder='big', bytes=b'\xbe\xef')
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xbeef));
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

TEST(IntBuiltinsTest, FromBytesKwInvalidKeywordRaisesTypeError) {
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

TEST(IntBuiltinsTest, DunderReprWithZeroReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "0"));
}

TEST(IntBuiltinsTest, DunderReprWithSmallIntReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newInt(0xdeadbeef));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "3735928559"));
}

TEST(IntBuiltinsTest, DunderReprWithSmallIntMaxReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "4611686018427387903"));
}

TEST(IntBuiltinsTest, DunderReprWithSmallIntMinReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newInt(RawSmallInt::kMinValue));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "-4611686018427387904"));
}

TEST(IntBuiltinsTest, DunderReprWithBoolFalseReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "0"));
}

TEST(IntBuiltinsTest, DunderReprWithBoolTrueReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1"));
}

TEST(IntBuiltinsTest, DunderReprWithLargeIntOneDigitReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newIntWithDigits({0x7ab65f95e6775822}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "8842360015809894434"));
}

TEST(IntBuiltinsTest, DunderReprWithLargeIntOneDigitMinReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newIntWithDigits({0x8000000000000000}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "-9223372036854775808"));
}

TEST(IntBuiltinsTest, DunderReprWithLargeIntOneDigitMaxReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, runtime.newIntWithDigits({0x7fffffffffffffff}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(*result, "9223372036854775807"));
}

TEST(IntBuiltinsTest, DunderReprWithLargeIntReturnsStr) {
  Runtime runtime;
  HandleScope scope;

  Object num(&scope, runtime.newIntWithDigits(
                         {0x68ccbb7f61087fb7, 0x4081e2972fe52778}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(
      isStrEqualsCStr(*result, "85744993827831399429103580491677204407"));
}

TEST(IntBuiltinsTest, DunderReprWithNegativeLargeIntReturnsStr) {
  Runtime runtime;
  HandleScope scope;

  Object num(&scope,
             runtime.newIntWithDigits({0x49618108301eff93, 0xc70a0c6e0731da35,
                                       0x438a2278e8762294, 0xccf89b106c9b714d,
                                       0xfa694d4cbdf0b0ba}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(
      isStrEqualsCStr(*result,
                      "-4663013431296140509759060231428418933437027788588076073"
                      "3669209802197774863968523736917349564525"));
}

TEST(IntBuiltinsTest, DunderReprWithLargeIntManyZerosReturnsStr) {
  Runtime runtime;
  HandleScope scope;

  Object num(&scope, runtime.newIntWithDigits(
                         {0x6ea69b2000000000, 0xf374ff2873cd99de, 0x375c24}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRepr, num));
  EXPECT_TRUE(isStrEqualsCStr(
      *result, "1234567890000000000000000000000000000000000000"));
}

TEST(IntBuiltinsTest, DunderRshiftWithBoolsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, Bool::trueObj());
  Object right(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST(IntBuiltinsTest, DunderRshiftWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(-1234));
  Object right(&scope, runtime.newInt(3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, -155));
}

TEST(IntBuiltinsTest, DunderRshiftWithOversizedAmountSmallIntReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(1));
  Object right(&scope, runtime.newInt(kBitsPerWord));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderRshiftWithOversizedAmountLargeIntReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(1));
  Object right(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderRshiftWithLargeIntOversizedAmountReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object right(&scope, runtime.newInt(kBitsPerWord * 3));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(IntBuiltinsTest, DunderRshiftWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, newIntWithDigits(&runtime,
                                       {0x188518dcaaa656f7, 0x7459da1092edebab,
                                        0x692e3b38af8dcfbe}));
  Object right(&scope, runtime.newInt(83));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0xb9f7ce8b3b42125d, 0xd25c76715f1}));
}

TEST(IntBuiltinsTest, DunderRshiftWithLargeIntWholeWordReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, newIntWithDigits(&runtime,
                                       {0x1c386fefbb1baf3d, 0x379bcaa886c98c13,
                                        0xe0f6379843f98b29, 0}));
  Object right(&scope, runtime.newInt(kBitsPerWord * 2));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0xe0f6379843f98b29, 0}));
}

TEST(IntBuiltinsTest, DunderRshiftWithLargeIntNegativeReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, newIntWithDigits(
                          &runtime, {0x3190ff6fa83269bc, 0xe7a1689a33ca9ae6}));
  Object right(&scope, runtime.newInt(13));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0xd7318c87fb7d4193, 0xffff3d0b44d19e54}));
}

TEST(IntBuiltinsTest, DunderRshiftWithNegativeShiftAmountRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(1));
  Object right(&scope, runtime.newInt(-4));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kValueError, "negative shift count"));
}

TEST(IntBuiltinsTest, DunderRshiftWithNonIntSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  Object right(&scope, runtime.newInt(0));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderRshiftWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(0));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderRshift, left, right));
  EXPECT_TRUE(result.isNotImplemented());
}

TEST(IntBuiltinsTest, DunderStrAliasesDunderRepr) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object str_name(&scope, runtime.newStrFromCStr("__str__"));
  Object str_obj(&scope, runtime.typeDictAt(dict, str_name));
  ASSERT_TRUE(str_obj.isFunction());
  Function str(&scope, *str_obj);
  Object repr_name(&scope, runtime.newStrFromCStr("__repr__"));
  Object repr_obj(&scope, runtime.typeDictAt(dict, str_name));
  ASSERT_TRUE(repr_obj.isFunction());
  Function repr(&scope, *str_obj);
  EXPECT_EQ(RawCode::cast(str.code()).code(),
            RawCode::cast(repr.code()).code());
  EXPECT_EQ(str.entry(), repr.entry());
  EXPECT_EQ(str.entryKw(), repr.entryKw());
  EXPECT_EQ(str.entryEx(), repr.entryEx());
}

TEST(IntBuiltinsTest, DunderSubWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, SmallInt::fromWord(42));
  Int right(&scope, SmallInt::fromWord(-7));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 49));
}

TEST(IntBuiltinsTest, DunderSubWithSmallIntsOverflowReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int min_small_int(&scope, SmallInt::fromWord(RawSmallInt::kMinValue));
  Int one(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, min_small_int, one));
  EXPECT_TRUE(isIntEqualsWord(*result, RawSmallInt::kMinValue - 1));
}

TEST(IntBuiltinsTest, DunderSubWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime,
                                    {0xfedcba0987654321, 0x1234567890abcdef}));
  Int right(&scope, newIntWithDigits(&runtime,
                                     {0x9876543210abcdef, 0xfedcba0123456789}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  EXPECT_TRUE(
      isIntEqualsDigits(*result, {0x666665d776b97532, 0x13579c776d666666}));
}

TEST(IntBuiltinsTest, DunderSubWithPositiveLargeIntsBorrowingReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, newIntWithDigits(&runtime, {1}));
  Int right(&scope, newIntWithDigits(&runtime, {kMaxUword, kMaxUword, 0}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {2, 0, kMaxUword}));
}

TEST(IntBuiltinsTest, DunderSubWithNegativeLargeIntsBorrowingReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;

  // The smallest negative number representable with 2 digits.
  Int left(&scope,
           newIntWithDigits(&runtime, {0, static_cast<uword>(kMinWord)}));
  Int right(&scope, newIntWithDigits(&runtime, {1}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, right));
  EXPECT_TRUE(isIntEqualsDigits(
      *result, {kMaxUword, static_cast<uword>(kMaxWord), kMaxUword}));
}

TEST(IntBuiltinsTest, DunderSubWithNonIntSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;

  Str str(&scope, runtime.newStrFromCStr(""));
  Int right(&scope, runtime.newInt(1));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, str, right));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, DunderSubWithNonIntRightReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;

  Int left(&scope, runtime.newInt(1));
  Str str(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderSub, left, str));
  EXPECT_TRUE(result.isNotImplemented());
}

TEST(IntBuiltinsTest, DunderXorWithSmallIntsReturnsSmallInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, SmallInt::fromWord(0x15));   // 0b010101
  Int right(&scope, SmallInt::fromWord(0x38));  // 0b111000
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x2D));  // 0b101101
}

TEST(IntBuiltinsTest, DunderXorWithLargeIntsReturnsLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {0x0f, 0x30, 0xCAFE}));
  Int right(&scope, newIntWithDigits(&runtime, {0x03, 0xf0}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(isIntEqualsDigits(*result, {0x0C, 0xC0, 0xCAFE}));
}

TEST(IntBuiltinsTest, DunderXorWithNonIntReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Int left(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object right(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
  EXPECT_TRUE(result.isNotImplemented());
}

TEST(IntBuiltinsTest, DunderXorWithInvalidArgumentLeftRaisesException) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newStrFromCStr(""));
  LargeInt right(&scope, newIntWithDigits(&runtime, {1, 2}));
  Object result(&scope, runBuiltin(IntBuiltins::dunderXor, left, right));
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
  ASSERT_TRUE(result.isBytes());
  Bytes bytes(&scope, *result);
  ASSERT_EQ(bytes.length(), 3);
  EXPECT_EQ(bytes.byteAt(0), 42);
  EXPECT_EQ(bytes.byteAt(1), 0);
  EXPECT_EQ(bytes.byteAt(2), 0);
}

TEST(IntBuiltinsTest, ToBytesWithByteorderBigEndianReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(42));
  Int length(&scope, SmallInt::fromWord(2));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result.isBytes());
  Bytes bytes(&scope, *result);
  ASSERT_EQ(bytes.length(), 2);
  EXPECT_EQ(bytes.byteAt(0), 0);
  EXPECT_EQ(bytes.byteAt(1), 42);
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
    ASSERT_TRUE(x.isBytes()) << name;
    Bytes x_bytes(&scope, *x);
    ASSERT_EQ(x_bytes.length(), 2) << name;
    EXPECT_EQ(x_bytes.byteAt(0), 0x34) << name;
    EXPECT_EQ(x_bytes.byteAt(1), 0x12) << name;
  }
}

TEST(IntBuiltinsTest, ToBytesKwWithNegativeNumberReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
x0 = (-777).to_bytes(4, 'little', signed=True)
)");
  Object x(&scope, moduleAt(&runtime, "__main__", "x0"));
  ASSERT_TRUE(x.isBytes());
  Bytes x_bytes(&scope, *x);
  ASSERT_EQ(x_bytes.length(), 4);
  EXPECT_EQ(x_bytes.byteAt(0), 0xf7);
  EXPECT_EQ(x_bytes.byteAt(1), 0xfc);
  EXPECT_EQ(x_bytes.byteAt(2), 0xff);
  EXPECT_EQ(x_bytes.byteAt(3), 0xff);
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
  ASSERT_TRUE(result_128.isBytes());
  Bytes bytes_128(&scope, *result_128);
  ASSERT_EQ(bytes_128.length(), 1);
  EXPECT_EQ(bytes_128.byteAt(0), 0x80);

  Int length_2(&scope, SmallInt::fromWord(2));
  Int num_32768(&scope, SmallInt::fromWord(32768));
  Object result_32768(
      &scope, runBuiltin(IntBuiltins::toBytes, num_32768, length_2, byteorder));
  EXPECT_TRUE(result_32768.isBytes());
  Bytes bytes_32768(&scope, *result_32768);
  ASSERT_EQ(bytes_32768.length(), 2);
  EXPECT_EQ(bytes_32768.byteAt(0), 0);
  EXPECT_EQ(bytes_32768.byteAt(1), 0x80);

  Int length_8(&scope, SmallInt::fromWord(8));
  Int num_min_word(&scope, newIntWithDigits(&runtime, {0x8000000000000000, 0}));
  Object result_min_word(&scope, runBuiltin(IntBuiltins::toBytes, num_min_word,
                                            length_8, byteorder));
  EXPECT_TRUE(result_min_word.isBytes());
  Bytes bytes_min_word(&scope, *result_min_word);
  ASSERT_EQ(bytes_min_word.length(), 8);
  EXPECT_EQ(bytes_min_word.byteAt(0), 0);
  EXPECT_EQ(bytes_min_word.byteAt(1), 0);
  EXPECT_EQ(bytes_min_word.byteAt(2), 0);
  EXPECT_EQ(bytes_min_word.byteAt(3), 0);
  EXPECT_EQ(bytes_min_word.byteAt(4), 0);
  EXPECT_EQ(bytes_min_word.byteAt(5), 0);
  EXPECT_EQ(bytes_min_word.byteAt(6), 0);
  EXPECT_EQ(bytes_min_word.byteAt(7), 0x80);
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
  ASSERT_TRUE(result.isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes.length(), 10);
  EXPECT_EQ(result_bytes.byteAt(0), 0);
  EXPECT_EQ(result_bytes.byteAt(1), 0);
  EXPECT_EQ(result_bytes.byteAt(2), 0);
  EXPECT_EQ(result_bytes.byteAt(3), 0);
  EXPECT_EQ(result_bytes.byteAt(4), 0);
  EXPECT_EQ(result_bytes.byteAt(5), 0);
  EXPECT_EQ(result_bytes.byteAt(6), 0xca);
  EXPECT_EQ(result_bytes.byteAt(7), 0xfe);
  EXPECT_EQ(result_bytes.byteAt(8), 0xba);
  EXPECT_EQ(result_bytes.byteAt(9), 0xbe);
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
  ASSERT_TRUE(result.isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes.length(), 10);
  EXPECT_EQ(result_bytes.byteAt(0), 0xbe);
  EXPECT_EQ(result_bytes.byteAt(1), 0xba);
  EXPECT_EQ(result_bytes.byteAt(2), 0xfe);
  EXPECT_EQ(result_bytes.byteAt(3), 0xca);
  EXPECT_EQ(result_bytes.byteAt(4), 0);
  EXPECT_EQ(result_bytes.byteAt(5), 0);
  EXPECT_EQ(result_bytes.byteAt(6), 0);
  EXPECT_EQ(result_bytes.byteAt(7), 0);
  EXPECT_EQ(result_bytes.byteAt(8), 0);
  EXPECT_EQ(result_bytes.byteAt(9), 0);
}

TEST(IntBuiltinsTest, ToBytesWithSignedTrueReturnsBytes) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
result = (0x7fffffffffffffff).to_bytes(8, 'little', signed=True)
)");
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isBytes());
  Bytes result(&scope, *result_obj);
  EXPECT_EQ(result.length(), 8);
  EXPECT_EQ(result.byteAt(0), 0xff);
  EXPECT_EQ(result.byteAt(1), 0xff);
  EXPECT_EQ(result.byteAt(2), 0xff);
  EXPECT_EQ(result.byteAt(3), 0xff);
  EXPECT_EQ(result.byteAt(4), 0xff);
  EXPECT_EQ(result.byteAt(5), 0xff);
  EXPECT_EQ(result.byteAt(6), 0xff);
  EXPECT_EQ(result.byteAt(7), 0x7f);

  runFromCStr(&runtime, R"(
result_n_128 = (-128).to_bytes(1, 'little', signed=True)
)");
  Object result_n_128(&scope, moduleAt(&runtime, "__main__", "result_n_128"));
  ASSERT_TRUE(result_n_128.isBytes());
  Bytes bytes_n_128(&scope, *result_n_128);
  ASSERT_EQ(bytes_n_128.length(), 1);
  EXPECT_EQ(bytes_n_128.byteAt(0), 0x80);

  runFromCStr(&runtime, R"(
result_n_32768 = (-32768).to_bytes(2, 'little', signed=True)
)");
  Object result_n_32768(&scope,
                        moduleAt(&runtime, "__main__", "result_n_32768"));
  ASSERT_TRUE(result_n_32768.isBytes());
  Bytes bytes_n_32768(&scope, *result_n_32768);
  ASSERT_EQ(bytes_n_32768.length(), 2);
  EXPECT_EQ(bytes_n_32768.byteAt(0), 0x00);
  EXPECT_EQ(bytes_n_32768.byteAt(1), 0x80);

  runFromCStr(&runtime, R"(
result_n_min_word = (-9223372036854775808).to_bytes(8, 'little', signed=True)
)");
  Object result_n_min_word(&scope,
                           moduleAt(&runtime, "__main__", "result_n_min_word"));
  ASSERT_TRUE(result_n_min_word.isBytes());
  Bytes bytes_n_min_word(&scope, *result_n_min_word);
  ASSERT_EQ(bytes_n_min_word.length(), 8);
  EXPECT_EQ(bytes_n_min_word.byteAt(0), 0x00);
  EXPECT_EQ(bytes_n_min_word.byteAt(1), 0x00);
  EXPECT_EQ(bytes_n_min_word.byteAt(2), 0x00);
  EXPECT_EQ(bytes_n_min_word.byteAt(3), 0x00);
  EXPECT_EQ(bytes_n_min_word.byteAt(4), 0x00);
  EXPECT_EQ(bytes_n_min_word.byteAt(5), 0x00);
  EXPECT_EQ(bytes_n_min_word.byteAt(6), 0x00);
  EXPECT_EQ(bytes_n_min_word.byteAt(7), 0x80);
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
  ASSERT_TRUE(result.isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes.length(), 7);
  EXPECT_EQ(result_bytes.byteAt(0), 0xff);
  EXPECT_EQ(result_bytes.byteAt(1), 0xff);
  EXPECT_EQ(result_bytes.byteAt(2), 0xff);
  EXPECT_EQ(result_bytes.byteAt(3), 0xff);
  EXPECT_EQ(result_bytes.byteAt(4), 0xff);
  EXPECT_EQ(result_bytes.byteAt(5), 0xfc);
  EXPECT_EQ(result_bytes.byteAt(6), 0x00);
}

TEST(IntBuiltinsTest, ToBytesWithZeroLengthBigEndianReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(0));
  Int length(&scope, SmallInt::fromWord(0));
  Str byteorder(&scope, runtime.newStrFromCStr("big"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result.isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes.length(), 0);
}

TEST(IntBuiltinsTest, ToBytesWithZeroLengthLittleEndianReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;

  Int num(&scope, SmallInt::fromWord(0));
  Int length(&scope, SmallInt::fromWord(0));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, length, byteorder));
  ASSERT_TRUE(result.isBytes());
  Bytes result_bytes(&scope, *result);
  ASSERT_EQ(result_bytes.length(), 0);
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

TEST(IntBuiltinsTest, ToBytesWithInvalidLengthArgRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int num(&scope, SmallInt::fromWord(42));
  Str not_a_length(&scope, runtime.newStrFromCStr("not a length"));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, not_a_length, byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(IntBuiltinsTest, ToBytesWithInvalidLengthArgRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  Int num(&scope, SmallInt::fromWord(42));
  Int negative_length(&scope, SmallInt::fromWord(-3));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope, runBuiltin(IntBuiltins::toBytes, num, negative_length,
                                   byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(IntBuiltinsTest, ToBytesWithInvalidLengthArgRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Int num(&scope, SmallInt::fromWord(42));
  Int huge_length(&scope, testing::newIntWithDigits(&runtime, {0, 1024}));
  Str byteorder(&scope, runtime.newStrFromCStr("little"));
  Object result(&scope,
                runBuiltin(IntBuiltins::toBytes, num, huge_length, byteorder));
  EXPECT_TRUE(raised(*result, LayoutId::kOverflowError));
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

TEST(IntBuiltinsTest, ToBytesKwInvalidKeywordRaisesTypeError) {
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
  EXPECT_TRUE(result.value());
}

TEST(BoolBuiltinsTest, NewFromZerorReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Int num(&scope, SmallInt::fromWord(0));

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, num));
  EXPECT_FALSE(result.value());
}

TEST(BoolBuiltinsTest, NewFromTrueReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Object true_obj(&scope, Bool::trueObj());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, true_obj));
  EXPECT_TRUE(result.value());
}

TEST(BoolBuiltinsTest, NewFromFalseReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Object false_obj(&scope, Bool::falseObj());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, false_obj));
  EXPECT_FALSE(result.value());
}

TEST(BoolBuiltinsTest, NewFromNoneIsFalse) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kBool));
  Object none(&scope, NoneType::object());

  Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, none));
  EXPECT_FALSE(result.value());
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
    EXPECT_TRUE(result.value());
  }
  {
    Type type(&scope, runtime.typeAt(LayoutId::kBool));
    Bool result(&scope, runBuiltin(BoolBuiltins::dunderNew, type, bar));
    EXPECT_FALSE(result.value());
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
  EXPECT_NEAR(result.value(), 1.0, DBL_EPSILON);

  // Test positive smallint mod negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  Float result1(&scope,
                runBuiltin(SmallIntBuiltins::dunderMod, hundred, float2));
  EXPECT_NEAR(result1.value(), -0.5, DBL_EPSILON);

  // Test positive smallint mod infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  Float result2(&scope,
                runBuiltin(SmallIntBuiltins::dunderMod, hundred, float_inf));
  ASSERT_TRUE(result2.isFloat());
  EXPECT_NEAR(result2.value(), 100.0, DBL_EPSILON);

  // Test positive smallint mod negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  Float result3(
      &scope, runBuiltin(SmallIntBuiltins::dunderMod, hundred, neg_float_inf));
  EXPECT_EQ(result3.value(), -INFINITY);

  // Test negative smallint mod infinity
  Int minus_hundred(&scope, SmallInt::fromWord(-100));
  Float result4(&scope, runBuiltin(SmallIntBuiltins::dunderMod, minus_hundred,
                                   float_inf));
  EXPECT_EQ(result4.value(), INFINITY);

  // Test negative smallint mod negative infinity
  Float result5(&scope, runBuiltin(SmallIntBuiltins::dunderMod, minus_hundred,
                                   neg_float_inf));
  EXPECT_NEAR(result5.value(), -100.0, DBL_EPSILON);

  // Test negative smallint mod nan
  Float nan(&scope, runtime.newFloat(NAN));
  Float result6(&scope,
                runBuiltin(SmallIntBuiltins::dunderMod, minus_hundred, nan));
  EXPECT_TRUE(std::isnan(result6.value()));
}

TEST(SmallIntBuiltinsTest, DunderFloorDivWithFloat) {
  Runtime runtime;
  HandleScope scope;
  Int hundred(&scope, SmallInt::fromWord(100));

  // Test dividing a positive smallint by a positive float
  Float float1(&scope, runtime.newFloat(1.5));
  Float result(&scope,
               runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred, float1));
  EXPECT_NEAR(result.value(), 66.0, DBL_EPSILON);

  // Test dividing a positive smallint by a negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  Float result1(&scope,
                runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred, float2));
  EXPECT_NEAR(result1.value(), -67.0, DBL_EPSILON);

  // Test dividing a positive smallint by infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  Float result2(
      &scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred, float_inf));
  EXPECT_NEAR(result2.value(), 0.0, DBL_EPSILON);

  // Test dividing a positive smallint by negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  Float result3(&scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv, hundred,
                                   neg_float_inf));
  EXPECT_NEAR(result3.value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by infinity
  SmallInt minus_hundred(&scope, SmallInt::fromWord(-100));
  Float result4(&scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv,
                                   minus_hundred, float_inf));
  EXPECT_NEAR(result4.value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by negative infinity
  Float result5(&scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv,
                                   minus_hundred, neg_float_inf));
  EXPECT_NEAR(result5.value(), 0.0, DBL_EPSILON);

  // Test dividing negative smallint by nan
  Float nan(&scope, runtime.newFloat(NAN));
  Float result6(
      &scope, runBuiltin(SmallIntBuiltins::dunderFloorDiv, minus_hundred, nan));
  EXPECT_TRUE(std::isnan(result6.value()));
}

TEST(SmallIntBuiltinsTest, DunderTrueDivWithFloat) {
  Runtime runtime;
  HandleScope scope;
  Int hundred(&scope, SmallInt::fromWord(100));

  // Test dividing a positive smallint by a positive float
  Float float1(&scope, runtime.newFloat(1.5));
  Float result(&scope,
               runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred, float1));
  EXPECT_NEAR(result.value(), 66.66666666666667, DBL_EPSILON);

  // Test dividing a positive smallint by a negative float
  Float float2(&scope, runtime.newFloat(-1.5));
  Float result1(&scope,
                runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred, float2));
  EXPECT_NEAR(result1.value(), -66.66666666666667, DBL_EPSILON);

  // Test dividing a positive smallint by infinity
  Float float_inf(&scope, runtime.newFloat(INFINITY));
  Float result2(
      &scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred, float_inf));
  EXPECT_NEAR(result2.value(), 0.0, DBL_EPSILON);

  // Test dividing a positive smallint by negative infinity
  Float neg_float_inf(&scope, runtime.newFloat(-INFINITY));
  Float result3(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, hundred,
                                   neg_float_inf));
  EXPECT_NEAR(result3.value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by infinity
  Int minus_hundred(&scope, SmallInt::fromWord(-100));
  Float result4(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv,
                                   minus_hundred, float_inf));
  EXPECT_NEAR(result4.value(), 0.0, DBL_EPSILON);

  // Test dividing a negative smallint by negative infinity
  Float result5(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv,
                                   minus_hundred, neg_float_inf));
  EXPECT_NEAR(result5.value(), 0.0, DBL_EPSILON);

  // Test dividing negative smallint by nan
  Float nan(&scope, runtime.newFloat(NAN));
  Float result6(
      &scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, minus_hundred, nan));
  EXPECT_TRUE(std::isnan(result6.value()));
}

TEST(SmallIntBuiltinsTest, DunderTrueDivWithSmallInt) {
  Runtime runtime;
  HandleScope scope;

  Object num1(&scope, SmallInt::fromWord(6));
  Object num2(&scope, SmallInt::fromWord(3));
  Float result(&scope, runBuiltin(SmallIntBuiltins::dunderTrueDiv, num1, num2));
  EXPECT_NEAR(result.value(), 2.0, DBL_EPSILON);

  num1 = SmallInt::fromWord(7);
  num2 = SmallInt::fromWord(3);
  result = runBuiltin(SmallIntBuiltins::dunderTrueDiv, num1, num2);
  EXPECT_NEAR(result.value(), 2.3333333333333335, DBL_EPSILON);
}

TEST(IntBuiltinsTest, ConjugateAliasesDunderInt) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, moduleAt(&runtime, "builtins", "int"));
  Dict dict(&scope, type.dict());
  Object conjugate_name(&scope, runtime.newStrFromCStr("conjugate"));
  Object conjugate_obj(&scope, runtime.typeDictAt(dict, conjugate_name));
  ASSERT_TRUE(conjugate_obj.isFunction());
  Function conjugate(&scope, *conjugate_obj);
  Object dint_name(&scope, runtime.newStrFromCStr("__int__"));
  Object dint_obj(&scope, runtime.typeDictAt(dict, conjugate_name));
  ASSERT_TRUE(dint_obj.isFunction());
  Function dint(&scope, *conjugate_obj);
  EXPECT_EQ(RawCode::cast(conjugate.code()).code(),
            RawCode::cast(dint.code()).code());
  EXPECT_EQ(conjugate.entry(), dint.entry());
  EXPECT_EQ(conjugate.entryKw(), dint.entryKw());
  EXPECT_EQ(conjugate.entryEx(), dint.entryEx());
}

}  // namespace python
