#include "gtest/gtest.h"

#include <cstdint>

#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ByteArrayTest, DownsizeMaintainsCapacity) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  ByteArray array(&scope, runtime.newByteArray());
  const byte byte_array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  runtime.byteArrayExtend(thread, array, byte_array);
  ASSERT_EQ(array.numItems(), 9);
  word capacity = array.capacity();
  array.downsize(5);
  EXPECT_EQ(array.numItems(), 5);
  EXPECT_EQ(array.capacity(), capacity);
}

TEST(DoubleTest, DoubleTest) {
  Runtime runtime;
  RawObject o = runtime.newFloat(3.14);
  ASSERT_TRUE(o.isFloat());
  RawFloat d = Float::cast(o);
  EXPECT_EQ(d.value(), 3.14);
}

TEST(ComplexTest, ComplexTest) {
  Runtime runtime;
  RawObject o = runtime.newComplex(1.0, 2.0);
  ASSERT_TRUE(o.isComplex());
  RawComplex c = Complex::cast(o);
  EXPECT_EQ(c.real(), 1.0);
  EXPECT_EQ(c.imag(), 2.0);
}

TEST(IntTest, IntTest) {
  Runtime runtime;
  HandleScope scope;

  Object o1(&scope, runtime.newInt(42));
  EXPECT_TRUE(isIntEqualsWord(*o1, 42));

  Object o2(&scope, runtime.newInt(9223372036854775807L));
  EXPECT_TRUE(isIntEqualsWord(*o2, 9223372036854775807L));

  int stack_val = 123;
  Int o3(&scope, runtime.newIntFromCPtr(&stack_val));
  EXPECT_EQ(*static_cast<int*>(o3.asCPtr()), 123);

  Object o4(&scope, runtime.newInt(kMinWord));
  EXPECT_TRUE(isIntEqualsWord(*o4, kMinWord));

  uword digits[] = {kMaxUword, 0};
  Int o5(&scope, runtime.newIntWithDigits(digits));
  EXPECT_TRUE(o5.isLargeInt());
  EXPECT_EQ(o5.bitLength(), kBitsPerWord);

  uword digits2[] = {kMaxUword, 1};
  Int o6(&scope, runtime.newIntWithDigits(digits2));
  EXPECT_TRUE(o6.isLargeInt());
  EXPECT_EQ(o6.bitLength(), kBitsPerWord + 1);
}

TEST(IntTest, LargeIntValid) {
  Runtime runtime;
  HandleScope scope;

  LargeInt i(&scope, runtime.heap()->createLargeInt(2));
  i.digitAtPut(0, -1234);
  i.digitAtPut(1, -1);
  // Redundant sign-extension
  EXPECT_FALSE(i.isValid());

  i.digitAtPut(1, -2);
  EXPECT_TRUE(i.isValid());

  i.digitAtPut(0, 1234);
  i.digitAtPut(1, 0);
  // Redundant zero-extension
  EXPECT_FALSE(i.isValid());

  i.digitAtPut(1, 1);
  EXPECT_TRUE(i.isValid());
}

TEST(IntTest, IsPositive) {
  Runtime runtime;
  HandleScope scope;

  Int zero(&scope, runtime.newInt(0));
  EXPECT_FALSE(zero.isPositive());

  Int one(&scope, runtime.newInt(1));
  EXPECT_TRUE(one.isPositive());

  Int neg_one(&scope, runtime.newInt(-1));
  EXPECT_FALSE(neg_one.isPositive());

  Int max_small_int(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  EXPECT_TRUE(max_small_int.isPositive());

  Int min_small_int(&scope, runtime.newInt(RawSmallInt::kMinValue));
  EXPECT_FALSE(min_small_int.isPositive());

  Int max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_TRUE(max_word.isPositive());

  Int min_word(&scope, runtime.newInt(kMinWord));
  EXPECT_FALSE(min_word.isPositive());
}

TEST(IntTest, IsNegative) {
  Runtime runtime;
  HandleScope scope;

  Int zero(&scope, runtime.newInt(0));
  EXPECT_FALSE(zero.isNegative());

  Int one(&scope, runtime.newInt(1));
  EXPECT_FALSE(one.isNegative());

  Int neg_one(&scope, runtime.newInt(-1));
  EXPECT_TRUE(neg_one.isNegative());

  Int max_small_int(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  EXPECT_FALSE(max_small_int.isNegative());

  Int min_small_int(&scope, runtime.newInt(RawSmallInt::kMinValue));
  EXPECT_TRUE(min_small_int.isNegative());

  Int max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_FALSE(max_word.isNegative());

  Int min_word(&scope, runtime.newInt(kMinWord));
  EXPECT_TRUE(min_word.isNegative());
}

TEST(IntTest, IsZero) {
  Runtime runtime;
  HandleScope scope;

  Int zero(&scope, runtime.newInt(0));
  EXPECT_TRUE(zero.isZero());

  Int one(&scope, runtime.newInt(1));
  EXPECT_FALSE(one.isZero());

  Int neg_one(&scope, runtime.newInt(-1));
  EXPECT_FALSE(neg_one.isZero());

  Int max_small_int(&scope, runtime.newInt(RawSmallInt::kMaxValue));
  EXPECT_FALSE(max_small_int.isZero());

  Int min_small_int(&scope, runtime.newInt(RawSmallInt::kMinValue));
  EXPECT_FALSE(min_small_int.isZero());

  Int max_word(&scope, runtime.newInt(kMaxWord));
  EXPECT_FALSE(max_word.isZero());

  Int min_word(&scope, runtime.newInt(kMinWord));
  EXPECT_FALSE(min_word.isZero());
}

TEST(IntTest, Compare) {
  Runtime runtime;
  HandleScope scope;

  Int zero(&scope, runtime.newInt(0));
  Int one(&scope, runtime.newInt(1));
  Int neg_one(&scope, runtime.newInt(-1));

  EXPECT_EQ(zero.compare(*zero), 0);
  EXPECT_GE(one.compare(*neg_one), 1);
  EXPECT_LE(neg_one.compare(*one), -1);

  Int min_small_int(&scope, runtime.newInt(RawSmallInt::kMinValue));
  Int max_small_int(&scope, runtime.newInt(RawSmallInt::kMaxValue));

  EXPECT_GE(max_small_int.compare(*min_small_int), 1);
  EXPECT_LE(min_small_int.compare(*max_small_int), -1);
  EXPECT_EQ(min_small_int.compare(*min_small_int), 0);
  EXPECT_EQ(max_small_int.compare(*max_small_int), 0);

  Int min_word(&scope, runtime.newInt(kMinWord));
  Int max_word(&scope, runtime.newInt(kMaxWord));

  EXPECT_GE(max_word.compare(*min_word), 1);
  EXPECT_LE(min_word.compare(*max_word), -1);
  EXPECT_EQ(min_word.compare(*min_word), 0);
  EXPECT_EQ(max_word.compare(*max_word), 0);

  EXPECT_GE(max_word.compare(*max_small_int), 1);
  EXPECT_LE(min_word.compare(*min_small_int), -1);
}

TEST(IntTest, LargeIntCompare) {
  Runtime runtime;
  HandleScope scope;
  const uword digits_great[] = {1, 1};
  Int great(&scope, newIntWithDigits(&runtime, digits_great));
  const uword digits_small[] = {0, 0, kMaxUword};
  Int small(&scope, newIntWithDigits(&runtime, digits_small));
  EXPECT_EQ(great.compare(*small), 1);
  EXPECT_EQ(small.compare(*great), -1);

  const uword digits_great2[] = {1, 1, 1};
  const uword digits_small2[] = {1, 1};
  great = newIntWithDigits(&runtime, digits_great2);
  small = newIntWithDigits(&runtime, digits_small2);
  EXPECT_EQ(great.compare(*small), 1);
  EXPECT_EQ(small.compare(*great), -1);

  const uword digits_great3[] = {kMaxUword - 1, 1};
  const uword digits_small3[] = {2, 1};
  great = newIntWithDigits(&runtime, digits_great3);
  small = newIntWithDigits(&runtime, digits_small3);
  EXPECT_EQ(great.compare(*small), 1);
  EXPECT_EQ(small.compare(*great), -1);

  const uword digits_great4[] = {kMaxUword - 1, kMaxUword - 1};
  const uword digits_small4[] = {2, kMaxUword - 1};
  great = newIntWithDigits(&runtime, digits_great4);
  small = newIntWithDigits(&runtime, digits_small4);
  EXPECT_EQ(great.compare(*small), 1);
  EXPECT_EQ(small.compare(*great), -1);
}

#define EXPECT_VALID(expr, expected_value)                                     \
  {                                                                            \
    auto const result = (expr);                                                \
    EXPECT_EQ(result.error, CastError::None);                                  \
    EXPECT_EQ(result.value, expected_value);                                   \
  }

TEST(IntTest, AsInt) {
  Runtime runtime;
  HandleScope scope;

  Int zero(&scope, runtime.newInt(0));
  EXPECT_VALID(zero.asInt<int>(), 0);
  EXPECT_VALID(zero.asInt<unsigned>(), 0U);
  EXPECT_VALID(zero.asInt<unsigned long>(), 0UL);
  EXPECT_VALID(zero.asInt<unsigned long long>(), 0ULL);

  Int num(&scope, runtime.newInt(1234));
  EXPECT_EQ(num.asInt<byte>().error, CastError::Overflow);
  EXPECT_EQ(num.asInt<int8_t>().error, CastError::Overflow);
  EXPECT_VALID(num.asInt<int>(), 1234);
  EXPECT_VALID(num.asInt<long>(), 1234);
  EXPECT_VALID(num.asInt<unsigned>(), 1234U);
  EXPECT_VALID(num.asInt<unsigned long>(), 1234UL);

  Int neg_num(&scope, runtime.newInt(-4567));
  EXPECT_EQ(neg_num.asInt<unsigned>().error, CastError::Underflow);
  EXPECT_EQ(neg_num.asInt<int8_t>().error, CastError::Underflow);
  EXPECT_VALID(neg_num.asInt<int16_t>(), -4567);

  Int neg_one(&scope, runtime.newInt(-1));
  EXPECT_VALID(neg_one.asInt<int>(), -1);
  EXPECT_EQ(neg_one.asInt<unsigned>().error, CastError::Underflow);

  Int int_max(&scope, runtime.newInt(kMaxInt32));
  EXPECT_VALID(int_max.asInt<int32_t>(), kMaxInt32);
  EXPECT_EQ(int_max.asInt<int16_t>().error, CastError::Overflow);

  Int uword_max(&scope, runtime.newIntFromUnsigned(kMaxUword));
  EXPECT_VALID(uword_max.asInt<uword>(), kMaxUword);
  EXPECT_EQ(uword_max.asInt<word>().error, CastError::Overflow);

  Int word_max(&scope, runtime.newInt(kMaxWord));
  EXPECT_VALID(word_max.asInt<word>(), kMaxWord);
  EXPECT_VALID(word_max.asInt<uword>(), uword{kMaxWord});
  EXPECT_EQ(word_max.asInt<int32_t>().error, CastError::Overflow);

  Int word_min(&scope, runtime.newInt(kMinWord));
  EXPECT_VALID(word_min.asInt<word>(), kMinWord);
  EXPECT_EQ(word_min.asInt<uword>().error, CastError::Underflow);
  EXPECT_EQ(word_min.asInt<int32_t>().error, CastError::Overflow);

  uword digits[] = {0, kMaxUword};
  Int negative(&scope, runtime.newIntWithDigits(digits));
  EXPECT_EQ(negative.asInt<word>().error, CastError::Underflow);
  EXPECT_EQ(negative.asInt<uword>().error, CastError::Underflow);
}

#undef EXPECT_VALID

TEST(IntTest, SmallIntFromWordTruncatedWithSmallNegativeNumberReturnsSelf) {
  EXPECT_EQ(SmallInt::fromWord(-1), SmallInt::fromWordTruncated(-1));
}

TEST(ModulesTest, TestCreate) {
  Runtime runtime;
  HandleScope scope;
  Object name(&scope, runtime.newStrFromCStr("mymodule"));
  Module module(&scope, runtime.newModule(name));
  EXPECT_EQ(module.name(), *name);
  EXPECT_TRUE(module.dict().isDict());
}

TEST(SliceTest, adjustIndices) {
  // Test: 0:10:1 on len: 10
  word length = 10;
  word start = 0;
  word stop = 10;
  word step = 1;
  word new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 10);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 10);

  // Test: 2:10:1 on len: 10
  start = 2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 8);
  ASSERT_EQ(start, 2);
  ASSERT_EQ(stop, 10);

  // Test: -4:10:1 on len: 10
  start = -4;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 4);
  ASSERT_EQ(start, 6);
  ASSERT_EQ(stop, 10);

  // Test: 0:2:1 on len: 10
  start = 0;
  stop = 2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 2);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 2);

  // Test: 0:-2:1 on len: 10
  start = 0;
  stop = -2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 8);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 8);

  // Test: 0:10:2 on len: 10
  start = 0;
  stop = 10;
  step = 2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 5);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 10);

  // Test: 0:10:-2 on len: 10
  start = 0;
  stop = 10;
  step = -2;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 9);
}

TEST(SliceTest, adjustIndicesOutOfBounds) {
  // Test: 10:5:1 on len: 5
  word length = 5;
  word start = 10;
  word stop = 5;
  word step = 1;
  word new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 5);
  ASSERT_EQ(stop, 5);

  // Test: -10:5:1 on len: 5
  start = -10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 5);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 5);

  // Test: 0:10:1 on len: 5
  start = 0;
  stop = 10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 5);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 5);

  // Test: 0:-10:1 on len: 5
  stop = -10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 0);

  // Test: 0:5:10 on len: 5
  stop = 5;
  step = 10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 1);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 5);

  // Test: 0:5:-10 on len: 5
  step = -10;
  new_length = Slice::adjustIndices(length, &start, &stop, step);
  ASSERT_EQ(new_length, 0);
  ASSERT_EQ(start, 0);
  ASSERT_EQ(stop, 4);
}

TEST(SliceTest, LengthWithNegativeStepAndStopLessThanStartReturnsLength) {
  EXPECT_EQ(Slice::length(5, 2, -1), 3);
}

TEST(SliceTest, LengthWithNegativeStepAndStartLessThanStopReturnsZero) {
  EXPECT_EQ(Slice::length(2, 5, -1), 0);
}

TEST(SliceTest, LengthWithNegativeStepAndStartEqualsStopReturnsZero) {
  EXPECT_EQ(Slice::length(2, 2, -1), 0);
}

TEST(SliceTest, LengthWithPositiveStepAndStartLessThanStopReturnsLength) {
  EXPECT_EQ(Slice::length(2, 5, 1), 3);
}

TEST(SliceTest, LengthWithPositiveStepAndStopLessThanStartReturnsZero) {
  EXPECT_EQ(Slice::length(5, 2, 1), 0);
}

TEST(SliceTest, LengthWithPositiveStepAndStartEqualsStopReturnsZero) {
  EXPECT_EQ(Slice::length(2, 2, 1), 0);
}

TEST(LargeStrTest, CopyTo) {
  Runtime runtime;

  RawObject obj = runtime.newStrFromCStr("hello world!");
  ASSERT_TRUE(obj.isLargeStr());
  RawStr str = Str::cast(obj);

  byte array[5];
  memset(array, 'a', ARRAYSIZE(array));
  str.copyTo(array, 0);
  EXPECT_EQ(array[0], 'a');
  EXPECT_EQ(array[1], 'a');
  EXPECT_EQ(array[2], 'a');
  EXPECT_EQ(array[3], 'a');
  EXPECT_EQ(array[4], 'a');

  memset(array, 'b', ARRAYSIZE(array));
  str.copyTo(array, 1);
  EXPECT_EQ(array[0], 'h');
  EXPECT_EQ(array[1], 'b');
  EXPECT_EQ(array[2], 'b');
  EXPECT_EQ(array[3], 'b');
  EXPECT_EQ(array[4], 'b');

  memset(array, 'c', ARRAYSIZE(array));
  str.copyTo(array, 5);
  EXPECT_EQ(array[0], 'h');
  EXPECT_EQ(array[1], 'e');
  EXPECT_EQ(array[2], 'l');
  EXPECT_EQ(array[3], 'l');
  EXPECT_EQ(array[4], 'o');
}

TEST(StringTest, CompareSmallStrCStrASCII) {
  Runtime runtime;
  HandleScope scope;

  Str small_ascii(&scope, runtime.newStrFromCStr("sm"));
  ASSERT_TRUE(small_ascii.isSmallStr());

  // Equal
  EXPECT_EQ(small_ascii.compareCStr("sm"), 0);

  // Less
  EXPECT_EQ(small_ascii.compareCStr("sma"), -1);
  EXPECT_EQ(small_ascii.compareCStr("sn"), -1);

  // Greater
  EXPECT_EQ(small_ascii.compareCStr("s"), 1);
  EXPECT_EQ(small_ascii.compareCStr("sl"), 1);
}

TEST(StringTest, CompareSmallStrWithNulCStrASCII) {
  Runtime runtime;
  HandleScope scope;

  const byte data[] = {'s', '\0', 'm'};
  Str small_ascii(&scope, runtime.newStrWithAll(data));
  ASSERT_TRUE(small_ascii.isSmallStr());

  // Less
  EXPECT_EQ(small_ascii.compareCStr("t"), -1);

  // Greater
  EXPECT_EQ(small_ascii.compareCStr("s"), 1);
  EXPECT_EQ(small_ascii.compareCStr("a\0m"), 1);
}

TEST(StringTest, CompareLargeStrWithNulCStrASCII) {
  Runtime runtime;
  HandleScope scope;

  const byte data[] = {'l', 'a', 'r', 'g', 'e', '\0', 's', 't'};
  Str large_ascii(&scope, runtime.newStrWithAll(data));
  ASSERT_TRUE(large_ascii.isLargeStr());

  // Less
  EXPECT_EQ(large_ascii.compareCStr("largz"), -1);

  // Greater
  EXPECT_EQ(large_ascii.compareCStr("large"), 1);
  EXPECT_EQ(large_ascii.compareCStr("larga\0st"), 1);
}

TEST(StringTest, CompareLargeStrCStrASCII) {
  Runtime runtime;
  HandleScope scope;

  Str large_ascii(&scope, runtime.newStrFromCStr("large string"));
  ASSERT_TRUE(large_ascii.isLargeStr());

  // Equal
  EXPECT_EQ(large_ascii.compareCStr("large string"), 0);

  // Less
  EXPECT_EQ(large_ascii.compareCStr("large strings"), -1);
  EXPECT_EQ(large_ascii.compareCStr("large tbigger"), -1);

  // Greater
  EXPECT_EQ(large_ascii.compareCStr("large strin"), 1);
  EXPECT_EQ(large_ascii.compareCStr("large smaller"), 1);
}

TEST(StringTest, CompareSmallStrCStrUTF8) {
  Runtime runtime;
  HandleScope scope;

  Str small_utf8(&scope, runtime.newStrFromCStr("\xC3\x87"));
  ASSERT_TRUE(small_utf8.isSmallStr());

  // Equal
  EXPECT_EQ(small_utf8.compareCStr("\xC3\x87"), 0);

  // Less
  EXPECT_EQ(small_utf8.compareCStr("\xC3\x87s"), -1);
  EXPECT_EQ(small_utf8.compareCStr("\xC3\x88"), -1);
  EXPECT_EQ(small_utf8.compareCStr("\xC3\xA7"), -1);

  // Greater
  EXPECT_EQ(small_utf8.compareCStr(""), 1);
  EXPECT_EQ(small_utf8.compareCStr("\xC3\x86"), 1);
  EXPECT_EQ(small_utf8.compareCStr("\xC3\x67"), 1);
}

TEST(StringTest, CompareLargeStrCStrUTF8) {
  Runtime runtime;
  HandleScope scope;

  Str large_utf8(&scope, runtime.newStrFromCStr("\xC3\x87 large"));
  ASSERT_TRUE(large_utf8.isLargeStr());

  // Equal
  EXPECT_EQ(large_utf8.compareCStr("\xC3\x87 large"), 0);

  // Less
  EXPECT_EQ(large_utf8.compareCStr("\xC3\x87 larges"), -1);
  EXPECT_EQ(large_utf8.compareCStr("\xC3\x88 large"), -1);
  EXPECT_EQ(large_utf8.compareCStr("\xC3\xA7 large"), -1);

  // Greater
  EXPECT_EQ(large_utf8.compareCStr("\xC3\x87"), 1);
  EXPECT_EQ(large_utf8.compareCStr("\xC3\x86 large"), 1);
  EXPECT_EQ(large_utf8.compareCStr("g large"), 1);
}

TEST(StringTest, CompareSmallStrCStrLatin1) {
  Runtime runtime;
  HandleScope scope;

  Str small_latin1(&scope, runtime.newStrFromCStr("\xDC"));
  ASSERT_TRUE(small_latin1.isSmallStr());

  // Equal
  EXPECT_EQ(small_latin1.compareCStr("\xDC"), 0);

  // Less
  EXPECT_EQ(small_latin1.compareCStr("\xDCs"), -1);
  EXPECT_EQ(small_latin1.compareCStr("\xDD"), -1);
  EXPECT_EQ(small_latin1.compareCStr("\xEC"), -1);

  // Greater
  EXPECT_EQ(small_latin1.compareCStr(""), 1);
  EXPECT_EQ(small_latin1.compareCStr("\xDB"), 1);
  EXPECT_EQ(small_latin1.compareCStr("\xAC"), 1);
}

TEST(StringTest, CompareLargeStrCStrLatin1) {
  Runtime runtime;
  HandleScope scope;

  Str large_latin1(&scope, runtime.newStrFromCStr("\xDClarge str"));
  ASSERT_TRUE(large_latin1.isLargeStr());

  // Equal
  EXPECT_EQ(large_latin1.compareCStr("\xDClarge str"), 0);

  // Less
  EXPECT_EQ(large_latin1.compareCStr("\xDClarge strs"), -1);
  EXPECT_EQ(large_latin1.compareCStr("\xDDlarge str"), -1);
  EXPECT_EQ(large_latin1.compareCStr("\xEClarge str"), -1);

  // Greater
  EXPECT_EQ(large_latin1.compareCStr("\xDC"), 1);
  EXPECT_EQ(large_latin1.compareCStr("\xDBlarge str"), 1);
  EXPECT_EQ(large_latin1.compareCStr("\xBClarge str"), 1);
}

TEST(SmallStrTest, Tests) {
  RawObject obj0 = SmallStr::fromCStr("AB");
  ASSERT_TRUE(obj0.isSmallStr());
  auto str0 = Str::cast(obj0);
  EXPECT_EQ(str0.length(), 2);
  EXPECT_EQ(str0.charAt(0), 'A');
  EXPECT_EQ(str0.charAt(1), 'B');
  byte array[3]{0, 0, 0};
  str0.copyTo(array, 2);
  EXPECT_EQ(array[0], 'A');
  EXPECT_EQ(array[1], 'B');
  EXPECT_EQ(array[2], 0);
}

TEST(SmallStrTest, CodePointLengthWithAsciiReturnsLength) {
  RawObject len0 = SmallStr::fromCStr("");
  ASSERT_TRUE(len0.isSmallStr());
  EXPECT_EQ(Str::cast(len0).length(), 0);
  EXPECT_EQ(Str::cast(len0).codePointLength(), 0);

  RawObject len1 = SmallStr::fromCStr("1");
  ASSERT_TRUE(len1.isSmallStr());
  EXPECT_EQ(Str::cast(len1).length(), 1);
  EXPECT_EQ(Str::cast(len1).codePointLength(), 1);

  RawObject len2 = SmallStr::fromCStr("12");
  ASSERT_TRUE(len2.isSmallStr());
  EXPECT_EQ(Str::cast(len2).length(), 2);
  EXPECT_EQ(Str::cast(len2).codePointLength(), 2);

  RawObject len3 = SmallStr::fromCStr("123");
  ASSERT_TRUE(len3.isSmallStr());
  EXPECT_EQ(Str::cast(len3).length(), 3);
  EXPECT_EQ(Str::cast(len3).codePointLength(), 3);
}

TEST(SmallStrTest, CodePointLengthWithOneCodePoint) {
  RawObject len1 = SmallStr::fromCStr("\x24");
  ASSERT_TRUE(len1.isSmallStr());
  EXPECT_EQ(Str::cast(len1).length(), 1);
  EXPECT_EQ(Str::cast(len1).codePointLength(), 1);

  RawObject len2 = SmallStr::fromCStr("\xC2\xA2");
  ASSERT_TRUE(len2.isSmallStr());
  EXPECT_EQ(Str::cast(len2).length(), 2);
  EXPECT_EQ(Str::cast(len2).codePointLength(), 1);

  RawObject len3 = SmallStr::fromCStr("\xE0\xA4\xB9");
  ASSERT_TRUE(len3.isSmallStr());
  EXPECT_EQ(Str::cast(len3).length(), 3);
  EXPECT_EQ(Str::cast(len3).codePointLength(), 1);

  RawObject len4 = SmallStr::fromCStr("\xF0\x90\x8D\x88");
  ASSERT_TRUE(len4.isSmallStr());
  EXPECT_EQ(Str::cast(len4).length(), 4);
  EXPECT_EQ(Str::cast(len4).codePointLength(), 1);
}

TEST(SmallStrTest, CodePointLengthWithTwoCodePoints) {
  RawObject len1 = SmallStr::fromCStr("\x24\x65");
  ASSERT_TRUE(len1.isSmallStr());
  EXPECT_EQ(Str::cast(len1).length(), 2);
  EXPECT_EQ(Str::cast(len1).codePointLength(), 2);

  RawObject len2 = SmallStr::fromCStr("\xC2\xA2\xC2\xA3");
  ASSERT_TRUE(len2.isSmallStr());
  EXPECT_EQ(Str::cast(len2).length(), 4);
  EXPECT_EQ(Str::cast(len2).codePointLength(), 2);

  RawObject len3 = SmallStr::fromCStr("\xE0\xA4\xB9\xC2\xA3");
  ASSERT_TRUE(len3.isSmallStr());
  EXPECT_EQ(Str::cast(len3).length(), 5);
  EXPECT_EQ(Str::cast(len3).codePointLength(), 2);

  RawObject len4 = SmallStr::fromCStr("\xF0\x90\x8D\x88\xC2\xA3");
  ASSERT_TRUE(len4.isSmallStr());
  EXPECT_EQ(Str::cast(len4).length(), 6);
  EXPECT_EQ(Str::cast(len4).codePointLength(), 2);
}

TEST(SmallStrTest, CodePointLengthWithThreeCodePoints) {
  RawObject len1 = SmallStr::fromCStr("\x24\x65\x66");
  ASSERT_TRUE(len1.isSmallStr());
  EXPECT_EQ(Str::cast(len1).length(), 3);
  EXPECT_EQ(Str::cast(len1).codePointLength(), 3);

  RawObject len2 = SmallStr::fromCStr("\xC2\xA2\xC2\xA3\xC2\xA4");
  ASSERT_TRUE(len2.isSmallStr());
  EXPECT_EQ(Str::cast(len2).length(), 6);
  EXPECT_EQ(Str::cast(len2).codePointLength(), 3);

  RawObject len3 = SmallStr::fromCStr("\xE0\xA4\xB9\xC2\xA3\xC2\xA4");
  ASSERT_TRUE(len3.isSmallStr());
  EXPECT_EQ(Str::cast(len3).length(), 7);
  EXPECT_EQ(Str::cast(len3).codePointLength(), 3);

  RawObject len4 = SmallStr::fromCStr("\xF0\x90\x8D\x88\x65\xC2\xA3");
  ASSERT_TRUE(len4.isSmallStr());
  EXPECT_EQ(Str::cast(len4).length(), 7);
  EXPECT_EQ(Str::cast(len4).codePointLength(), 3);
}

TEST(SmallStrTest, FromCodePointOneByte) {
  RawObject obj = SmallStr::fromCodePoint(0x24);
  ASSERT_TRUE(obj.isSmallStr());
  RawStr str = Str::cast(obj);
  ASSERT_EQ(str.length(), 1);
  EXPECT_EQ(str.charAt(0), 0x24);
}

TEST(SmallStrTest, FromCodePointTwoByte) {
  RawObject obj = SmallStr::fromCodePoint(0xA2);
  ASSERT_TRUE(obj.isSmallStr());
  RawStr str = Str::cast(obj);
  ASSERT_EQ(str.length(), 2);
  EXPECT_EQ(str.charAt(0), 0xC2);
  EXPECT_EQ(str.charAt(1), 0xA2);
}

TEST(SmallStrTest, FromCodePointThreeByte) {
  RawObject obj1 = SmallStr::fromCodePoint(0x0939);
  ASSERT_TRUE(obj1.isSmallStr());
  RawStr str1 = Str::cast(obj1);
  ASSERT_EQ(str1.length(), 3);
  EXPECT_EQ(str1.charAt(0), 0xE0);
  EXPECT_EQ(str1.charAt(1), 0xA4);
  EXPECT_EQ(str1.charAt(2), 0xB9);

  RawObject obj2 = SmallStr::fromCodePoint(0x20AC);
  ASSERT_TRUE(obj2.isSmallStr());
  RawStr str2 = Str::cast(obj2);
  ASSERT_EQ(str2.length(), 3);
  EXPECT_EQ(str2.charAt(0), 0xE2);
  EXPECT_EQ(str2.charAt(1), 0x82);
  EXPECT_EQ(str2.charAt(2), 0xAC);
}

TEST(SmallStrTest, FromCodePointFourByte) {
  RawObject obj = SmallStr::fromCodePoint(0x10348);
  ASSERT_TRUE(obj.isSmallStr());
  RawStr str = Str::cast(obj);
  ASSERT_EQ(str.length(), 4);
  EXPECT_EQ(str.charAt(0), 0xF0);
  EXPECT_EQ(str.charAt(1), 0x90);
  EXPECT_EQ(str.charAt(2), 0x8D);
  EXPECT_EQ(str.charAt(3), 0x88);
}

TEST(StrTest, OffsetByCodePoints) {
  Runtime runtime;
  HandleScope scope;

  Str empty(&scope, Str::empty());
  EXPECT_EQ(empty.length(), 0);
  EXPECT_EQ(empty.codePointLength(), 0);
  EXPECT_EQ(empty.offsetByCodePoints(0, 1), 0);
  EXPECT_EQ(empty.offsetByCodePoints(2, 0), 0);
  EXPECT_EQ(empty.offsetByCodePoints(2, 1), 0);

  Str ascii(&scope, runtime.newStrFromCStr("abcd"));
  EXPECT_EQ(ascii.length(), 4);
  EXPECT_EQ(ascii.codePointLength(), 4);

  // for ASCII, each code point is one byte wide
  EXPECT_EQ(ascii.offsetByCodePoints(0, 0), 0);
  EXPECT_EQ(ascii.offsetByCodePoints(0, 3), 3);
  EXPECT_EQ(ascii.offsetByCodePoints(1, 0), 1);
  EXPECT_EQ(ascii.offsetByCodePoints(2, 0), 2);
  EXPECT_EQ(ascii.offsetByCodePoints(2, 1), 3);
  EXPECT_EQ(ascii.offsetByCodePoints(3, 0), 3);

  // return the length once we reach the end of the string
  EXPECT_EQ(ascii.offsetByCodePoints(0, 4), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(0, 5), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(1, 3), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(1, 4), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(2, 2), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(2, 3), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(3, 1), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(3, 2), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(4, 0), 4);
  EXPECT_EQ(ascii.offsetByCodePoints(6, 0), 4);

  Str unicode(&scope,
              runtime.newStrFromCStr("\xd7\x90pq\xd7\x91\xd7\x92-\xd7\x93"));
  EXPECT_EQ(unicode.length(), 11);
  EXPECT_EQ(unicode.codePointLength(), 7);

  // for Unicode, code points may be more than one byte wide
  EXPECT_EQ(unicode.offsetByCodePoints(0, 0), 0);
  EXPECT_EQ(unicode.offsetByCodePoints(0, 1), 2);
  EXPECT_EQ(unicode.offsetByCodePoints(0, 2), 3);
  EXPECT_EQ(unicode.offsetByCodePoints(0, 3), 4);
  EXPECT_EQ(unicode.offsetByCodePoints(0, 4), 6);
  EXPECT_EQ(unicode.offsetByCodePoints(0, 5), 8);
  EXPECT_EQ(unicode.offsetByCodePoints(0, 6), 9);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 0), 2);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 1), 3);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 2), 4);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 3), 6);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 4), 8);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 5), 9);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 6), 11);
  EXPECT_EQ(unicode.offsetByCodePoints(4, 0), 4);
  EXPECT_EQ(unicode.offsetByCodePoints(4, 1), 6);
  EXPECT_EQ(unicode.offsetByCodePoints(6, 0), 6);

  // return the length once we reach the end of the string
  EXPECT_EQ(unicode.offsetByCodePoints(0, 7), 11);
  EXPECT_EQ(unicode.offsetByCodePoints(0, 9), 11);
  EXPECT_EQ(unicode.offsetByCodePoints(2, 7), 11);
  EXPECT_EQ(unicode.offsetByCodePoints(3, 6), 11);
  EXPECT_EQ(unicode.offsetByCodePoints(4, 5), 11);
  EXPECT_EQ(unicode.offsetByCodePoints(8, 3), 11);
  EXPECT_EQ(unicode.offsetByCodePoints(12, 0), 11);
}

TEST(LargeStrTest, CodePointLengthAscii) {
  Runtime runtime;
  HandleScope scope;

  const char* code_units = "01234567012345670";

  Str str(&scope, runtime.newStrFromCStr(code_units));
  EXPECT_TRUE(str.isLargeStr());
  EXPECT_EQ(str.length(), std::strlen(code_units));
  EXPECT_EQ(str.codePointLength(), 17);
}

TEST(LargeStrTest, CodePointLength) {
  Runtime runtime;
  HandleScope scope;

  const char* code_units =
      "\xd7\x99\xd7\xa9 \xd7\x9c\xd7\x99 \xd7\x94\xd7\xa8\xd7\x91\xd7\x94 "
      "\xd7\x90\xd7\x95\xd7\xaa\xd7\x99\xd7\x95\xd7\xaa "
      "\xd7\xa2\xd7\x9b\xd7\xa9\xd7\x99\xd7\x95";

  Str str(&scope, runtime.newStrFromCStr(code_units));
  EXPECT_TRUE(str.isLargeStr());
  EXPECT_EQ(str.length(), std::strlen(code_units));
  EXPECT_EQ(str.codePointLength(), 23);
}

TEST(StringTest, ToCString) {
  Runtime runtime;
  HandleScope scope;

  Str empty(&scope, Str::empty());
  char* c_empty = empty.toCStr();
  ASSERT_NE(c_empty, nullptr);
  EXPECT_STREQ(c_empty, "");
  std::free(c_empty);

  Str length1(&scope, runtime.newStrFromCStr("a"));
  char* c_length1 = length1.toCStr();
  ASSERT_NE(c_length1, nullptr);
  EXPECT_STREQ(c_length1, "a");
  std::free(c_length1);

  Str length2(&scope, runtime.newStrFromCStr("ab"));
  char* c_length2 = length2.toCStr();
  ASSERT_NE(c_length2, nullptr);
  EXPECT_STREQ(c_length2, "ab");
  std::free(c_length2);

  Str length10(&scope, runtime.newStrFromCStr("1234567890"));
  char* c_length10 = length10.toCStr();
  ASSERT_NE(c_length10, nullptr);
  EXPECT_STREQ(c_length10, "1234567890");
  std::free(c_length10);

  Str nulchar(&scope, runtime.newStrFromCStr("wx\0yz"));
  char* c_nulchar = nulchar.toCStr();
  ASSERT_NE(c_nulchar, nullptr);
  EXPECT_STREQ(c_nulchar, "wx");
  std::free(c_nulchar);
}

TEST(StringTest, CompareSmallStr) {
  Runtime runtime;
  HandleScope scope;

  Str small(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(small.isSmallStr());

  EXPECT_TRUE(small.equalsCStr("foo"));
  // This apparently stupid test is in response to a bug where we assumed
  // that the c-string passed to SmallStr::equalsCStr would always
  // be small itself.
  EXPECT_FALSE(small.equalsCStr("123456789"));
}

TEST(StringTest, CompareWithUnicode) {
  Runtime runtime;
  HandleScope scope;
  Str small(&scope, runtime.newStrFromCStr(u8"hello\u2028"));
  EXPECT_TRUE(small.equalsCStr("hello\u2028"));
}

TEST(WeakRefTest, EnqueueAndDequeue) {
  Runtime runtime;
  HandleScope scope;
  RawObject list = NoneType::object();
  Thread* thread = Thread::current();
  for (int i = 0; i < 3; i++) {
    Object obj(&scope, SmallInt::fromWord(i));
    Object none(&scope, NoneType::object());
    WeakRef weak(&scope, runtime.newWeakRef(thread, obj, none));
    WeakRef::enqueueReference(*weak, &list);
  }
  WeakRef weak(&scope, WeakRef::dequeueReference(&list));
  EXPECT_TRUE(isIntEqualsWord(weak.referent(), 0));

  weak = WeakRef::dequeueReference(&list);
  EXPECT_TRUE(isIntEqualsWord(weak.referent(), 1));

  weak = WeakRef::dequeueReference(&list);
  EXPECT_TRUE(isIntEqualsWord(weak.referent(), 2));

  EXPECT_EQ(list, NoneType::object());
}

TEST(WeakRefTest, SpliceQueue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  RawObject list1 = NoneType::object();
  RawObject list2 = NoneType::object();
  EXPECT_EQ(WeakRef::spliceQueue(list1, list2), NoneType::object());

  Object none(&scope, NoneType::object());
  RawObject list3 = runtime.newWeakRef(thread, none, none);
  WeakRef::cast(list3).setLink(list3);
  EXPECT_EQ(WeakRef::spliceQueue(list1, list3), list3);
  EXPECT_EQ(WeakRef::spliceQueue(list3, list2), list3);

  for (int i = 0; i < 2; i++) {
    Object obj1(&scope, SmallInt::fromWord(i));
    WeakRef weak1(&scope, runtime.newWeakRef(thread, obj1, none));
    weak1.setReferent(SmallInt::fromWord(i));
    WeakRef::enqueueReference(*weak1, &list1);

    Object obj2(&scope, SmallInt::fromWord(i + 2));
    WeakRef weak2(&scope, runtime.newWeakRef(thread, obj2, none));
    WeakRef::enqueueReference(*weak2, &list2);
  }
  RawObject list = WeakRef::spliceQueue(list1, list2);
  WeakRef weak(&scope, WeakRef::dequeueReference(&list));
  EXPECT_TRUE(isIntEqualsWord(weak.referent(), 0));

  weak = WeakRef::dequeueReference(&list);
  EXPECT_TRUE(isIntEqualsWord(weak.referent(), 1));

  weak = WeakRef::dequeueReference(&list);
  EXPECT_TRUE(isIntEqualsWord(weak.referent(), 2));

  weak = WeakRef::dequeueReference(&list);
  EXPECT_TRUE(isIntEqualsWord(weak.referent(), 3));

  EXPECT_EQ(list, NoneType::object());
}

TEST(TupleTest, NoneFillTupleFillsTupleWithNone) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple.atPut(0, SmallInt::fromWord(0));
  tuple.atPut(1, SmallInt::fromWord(1));
  tuple.atPut(2, SmallInt::fromWord(2));
  tuple.fill(NoneType::object());
  EXPECT_EQ(tuple.at(0), NoneType::object());
  EXPECT_EQ(tuple.at(1), NoneType::object());
  EXPECT_EQ(tuple.at(2), NoneType::object());
}

TEST(ErrorTest, ErrorIsError) {
  EXPECT_TRUE(Error::error().isError());

  EXPECT_TRUE(Error::exception().isError());
  EXPECT_TRUE(Error::exception().isErrorException());

  EXPECT_TRUE(Error::notFound().isError());
  EXPECT_TRUE(Error::notFound().isErrorNotFound());

  EXPECT_TRUE(Error::noMoreItems().isError());
  EXPECT_TRUE(Error::noMoreItems().isErrorNoMoreItems());

  EXPECT_TRUE(Error::outOfMemory().isError());
  EXPECT_TRUE(Error::outOfMemory().isErrorOutOfMemory());

  EXPECT_TRUE(Error::outOfBounds().isError());
  EXPECT_TRUE(Error::outOfBounds().isErrorOutOfBounds());
}

TEST(ErrorTest, ErrorHasCorrectKind) {
  EXPECT_EQ(Error::error().kind(), ErrorKind::kNone);
  EXPECT_EQ(Error::exception().kind(), ErrorKind::kException);
  EXPECT_EQ(Error::notFound().kind(), ErrorKind::kNotFound);
  EXPECT_EQ(Error::noMoreItems().kind(), ErrorKind::kNoMoreItems);
  EXPECT_EQ(Error::outOfMemory().kind(), ErrorKind::kOutOfMemory);
  EXPECT_EQ(Error::outOfBounds().kind(), ErrorKind::kOutOfBounds);
}

}  // namespace python
