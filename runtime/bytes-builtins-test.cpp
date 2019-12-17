#include "bytes-builtins.h"

#include "gtest/gtest.h"

#include "test-utils.h"

namespace py {

using namespace testing;

using BytesBuiltinsTest = RuntimeFixture;
using BytesIteratorBuiltinsTest = RuntimeFixture;

TEST_F(BytesBuiltinsTest, BuiltinBaseIsBytes) {
  HandleScope scope(thread_);
  Type bytes_type(&scope, runtime_.typeAt(LayoutId::kBytes));
  EXPECT_EQ(bytes_type.builtinBase(), LayoutId::kBytes);
}

TEST_F(BytesBuiltinsTest, BytesIsValidUTF8ReturnsTrue) {
  EXPECT_TRUE(bytesIsValidUTF8(Bytes::empty()));

  HandleScope scope(thread_);
  const byte zeros_bytes[] = {0, 0, 0};
  Bytes zeros(&scope, runtime_.newBytesWithAll(zeros_bytes));
  EXPECT_TRUE(bytesIsValidUTF8(*zeros));

  const byte test0[] = {'S', ':', 'a'};
  EXPECT_TRUE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test0))));
  const byte test1[] = {'S', ':', 0xC3, 0xA4};
  EXPECT_TRUE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test1))));
  const byte test2[] = {'S', ':', 0xE2, 0x88, 0x91};
  EXPECT_TRUE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test2))));
  const byte test3[] = {'S', ':', 0xF0, 0x9F, 0x90, 0x8D};
  EXPECT_TRUE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test3))));

  const byte test_misc[] = {0xEC, 0x95, 0x88, 0xEB, 0x85, 0x95, ' ',
                            0xEC, 0x84, 0xB8, 0xEC, 0x83, 0x81, 0};
  EXPECT_TRUE(
      bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test_misc))));

  const byte max_unicode[] = {0xF4, 0x8F, 0xBF, 0xBF};
  EXPECT_TRUE(
      bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(max_unicode))));

  const byte min_surrogate_minus_one[] = {0xED, 0x9F, 0xBF};
  EXPECT_TRUE(bytesIsValidUTF8(
      Bytes::cast(runtime_.newBytesWithAll(min_surrogate_minus_one))));
  const byte max_surrogate_plus_one[] = {0xEE, 0x80, 0x80};
  EXPECT_TRUE(bytesIsValidUTF8(
      Bytes::cast(runtime_.newBytesWithAll(max_surrogate_plus_one))));
}

TEST_F(BytesBuiltinsTest,
       BytesIsValidUTF8WithContinuationAfterValidCharReturnsFalse) {
  const byte test0[] = {0x80};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test0))));
  const byte test1[] = {'a', 0xB3};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test1))));
  const byte test2[] = {0xC3, 0xA4, 0xB3};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test2))));
  const byte test3[] = {0xE2, 0x88, 0x91, 0xB3};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test3))));
  const byte test4[] = {0xF0, 0x9F, 0x90, 0x8D, 0xB3};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test4))));
}

TEST_F(BytesBuiltinsTest, BytesIsValidUTF8WithMissingContinuationReturnsFalse) {
  const byte test0[] = {0xC3};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test0))));
  const byte test1[] = {0xC3, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test1))));

  const byte test2[] = {0xE2};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test2))));
  const byte test3[] = {0xE2, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test3))));
  const byte test5[] = {0xE2, 0x88};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test5))));
  const byte test4[] = {0xE2, 0, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test4))));
  const byte test6[] = {0xE2, 0x88, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test6))));

  const byte test7[] = {0xF0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test7))));
  const byte test8[] = {0xF0, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test8))));
  const byte test9[] = {0xF0, 0x9F};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test9))));
  const byte test10[] = {0xF0, 0, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test10))));
  const byte test11[] = {0xF0, 0x9F, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test11))));
  const byte test12[] = {0xF0, 0x9F, 0x90};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test12))));
  const byte test13[] = {0xF0, 0, 0, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test13))));
  const byte test14[] = {0xF0, 0x9F, 0x90, 0};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test14))));
}

TEST_F(BytesBuiltinsTest,
       BytesIsValidUTF8WithValueTooSmallForEncodingReturnsFalse) {
  const byte test0[] = {0xC0, 0x84};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test0))));
  const byte test1[] = {0xE0, 0x80, 0x80};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test1))));
  const byte test2[] = {0xE0, 0x9F, 0xBF};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test2))));
  const byte test3[] = {0xF0, 0x80, 0x80, 0x80};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test3))));
  const byte test4[] = {0xF0, 0x80, 0x9F, 0xBF};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test4))));
  const byte test5[] = {0xF4, 0x90, 0x80, 0x80};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test5))));
  const byte test6[] = {0xF7, 0xBF, 0xBF, 0xBF};
  EXPECT_FALSE(bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(test6))));
}

TEST_F(BytesBuiltinsTest, BytesIsValidUTF8WithSurrogateReturnsFalse) {
  const byte min_surrogate[] = {0xED, 0xA0, 0x80};
  EXPECT_FALSE(
      bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(min_surrogate))));
  const byte max_surrogate[] = {0xED, 0xBF, 0xBF};
  EXPECT_FALSE(
      bytesIsValidUTF8(Bytes::cast(runtime_.newBytesWithAll(max_surrogate))));
}

TEST_F(BytesBuiltinsTest, BytesIsValidStrWithSurrogateReturnsTrue) {
  const byte min_surrogate[] = {0xED, 0xA0, 0x80};
  EXPECT_TRUE(
      bytesIsValidStr(Bytes::cast(runtime_.newBytesWithAll(min_surrogate))));
  const byte max_surrogate[] = {0xED, 0xBF, 0xBF};
  EXPECT_TRUE(
      bytesIsValidStr(Bytes::cast(runtime_.newBytesWithAll(max_surrogate))));
}

TEST_F(BytesBuiltinsTest, FindWithSameBytesReturnsZero) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  word start = 0;
  word end = haystack.length();
  word result = bytesFind(haystack, haystack.length(), haystack,
                          haystack.length(), start, end);
  EXPECT_EQ(result, 0);
}

TEST_F(BytesBuiltinsTest, FindWithWideBoundsReturnsIndex) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  const byte needle_bytes[] = {100, 74};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, runtime_.newBytesWithAll(needle_bytes));
  word start = -1000;
  word end = 123;
  word result = bytesFind(haystack, haystack.length(), needle, needle.length(),
                          start, end);
  EXPECT_EQ(result, 2);
}

TEST_F(BytesBuiltinsTest, FindWithNegativeBoundsReturnsIndex) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  const byte needle_bytes[] = {100, 74};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, runtime_.newBytesWithAll(needle_bytes));
  word start = -5;
  word end = -2;
  word result = bytesFind(haystack, haystack.length(), needle, needle.length(),
                          start, end);
  EXPECT_EQ(result, 2);
}

TEST_F(BytesBuiltinsTest, FindWithEmptyReturnsAdjustedStart) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, Bytes::empty());
  word start = -3;
  word end = -1;
  word result = bytesFind(haystack, haystack.length(), needle, needle.length(),
                          start, end);
  EXPECT_EQ(result, 3);
}

TEST_F(BytesBuiltinsTest, FindWithEndLessThanStartReturnsNegativeOne) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, Bytes::empty());
  word start = 3;
  word end = 2;
  word result = bytesFind(haystack, haystack.length(), needle, needle.length(),
                          start, end);
  EXPECT_EQ(result, -1);
}

TEST_F(BytesBuiltinsTest, FindWithSingleCharReturnsFirstIndexInRange) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {100, 55, 100, 74, 100, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, runtime_.newBytes(1, 100));
  word start = 1;
  word end = haystack.length();
  word result = bytesFind(haystack, haystack.length(), needle, needle.length(),
                          start, end);
  EXPECT_EQ(result, 2);
}

TEST_F(BytesBuiltinsTest, RFindWithSameBytesReturnsZero) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  word start = 0;
  word end = haystack.length();
  word result = bytesRFind(haystack, haystack.length(), haystack,
                           haystack.length(), start, end);
  EXPECT_EQ(result, 0);
}

TEST_F(BytesBuiltinsTest, RFindWithWideBoundsReturnsIndex) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  const byte needle_bytes[] = {100, 74};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, runtime_.newBytesWithAll(needle_bytes));
  word start = -1000;
  word end = 123;
  word result = bytesRFind(haystack, haystack.length(), needle, needle.length(),
                           start, end);
  EXPECT_EQ(result, 2);
}

TEST_F(BytesBuiltinsTest, RFindWithNegativeBoundsReturnsIndex) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  const byte needle_bytes[] = {100, 74};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, runtime_.newBytesWithAll(needle_bytes));
  word start = -5;
  word end = -2;
  word result = bytesRFind(haystack, haystack.length(), needle, needle.length(),
                           start, end);
  EXPECT_EQ(result, 2);
}

TEST_F(BytesBuiltinsTest, RFindWithEmptyReturnsAdjustedEnd) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, Bytes::empty());
  word start = -3;
  word end = -1;
  word result = bytesRFind(haystack, haystack.length(), needle, needle.length(),
                           start, end);
  EXPECT_EQ(result, 5);
}

TEST_F(BytesBuiltinsTest, RFindWithEndLessThanStartReturnsNegativeOne) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {102, 55, 100, 74, 91, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, Bytes::empty());
  word start = 3;
  word end = 2;
  word result = bytesRFind(haystack, haystack.length(), needle, needle.length(),
                           start, end);
  EXPECT_EQ(result, -1);
}

TEST_F(BytesBuiltinsTest, RFindWithSingleCharReturnsLastIndexInRange) {
  HandleScope scope(thread_);
  const byte haystack_bytes[] = {100, 55, 100, 74, 100, 118};
  Bytes haystack(&scope, runtime_.newBytesWithAll(haystack_bytes));
  Bytes needle(&scope, runtime_.newBytes(1, 100));
  word start = 0;
  word end = 4;
  word result = bytesRFind(haystack, haystack.length(), needle, needle.length(),
                           start, end);
  EXPECT_EQ(result, 2);
}

TEST_F(BytesBuiltinsTest, DunderAddWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__add__(b'')"), LayoutId::kTypeError,
      "'bytes.__add__' takes min 2 positional arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderAddWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__add__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "'bytes.__add__' takes max 2 positional arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderAddWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime_.newBytes(1, '1'));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(raised(*sum, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderAddWithNonBytesOtherRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, '1'));
  Object other(&scope, SmallInt::fromWord(2));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(raised(*sum, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderAddWithBytesLikeOtherReturnsBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object self(&scope, runtime_.newBytes(1, '1'));
  ByteArray other(&scope, runtime_.newByteArray());
  const byte buf[] = {'2', '3'};
  runtime_.byteArrayExtend(thread, other, buf);
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(isBytesEqualsCStr(sum, "123"));
}

TEST_F(BytesBuiltinsTest, DunderAddWithBytesSubclassReturnsBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b'abc')
other = Foo(b'123')
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object other(&scope, mainModuleAt(&runtime_, "other"));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(isBytesEqualsCStr(sum, "abc123"));
}

TEST_F(BytesBuiltinsTest, DunderAddWithTwoBytesReturnsConcatenatedBytes) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, '1'));
  Object other(&scope, runtime_.newBytes(2, '2'));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(isBytesEqualsCStr(sum, "122"));
}

TEST_F(BytesBuiltinsTest, DunderEqWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__eq__(b'')"), LayoutId::kTypeError,
      "'bytes.__eq__' takes min 2 positional arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderEqWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__eq__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "'bytes.__eq__' takes max 2 positional arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderEqWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime_.newBytes(1, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  EXPECT_TRUE(raised(*eq, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderEqWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  EXPECT_TRUE(eq.isNotImplementedType());
}

TEST_F(BytesBuiltinsTest, DunderEqWithBytesSubclassComparesBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b'123')
other = Foo(b'123')
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object other(&scope, mainModuleAt(&runtime_, "other"));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  EXPECT_EQ(eq, Bool::trueObj());
}

TEST_F(BytesBuiltinsTest, DunderEqWithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(5, 'a'));
  Object other(&scope, runtime_.newBytes(5, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_TRUE(Bool::cast(*eq).value());
}

TEST_F(BytesBuiltinsTest, DunderEqWithDifferentLengthsReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, runtime_.newBytes(4, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_FALSE(Bool::cast(*eq).value());
}

TEST_F(BytesBuiltinsTest, DunderEqWithDifferentContentsReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(3, 'b'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_FALSE(Bool::cast(*eq).value());
}

TEST_F(BytesBuiltinsTest, DunderGeWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__ge__(b'')"), LayoutId::kTypeError,
      "'bytes.__ge__' takes min 2 positional arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderGeWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__ge__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "'bytes.__ge__' takes max 2 positional arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderGeWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime_.newBytes(1, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  EXPECT_TRUE(raised(*ge, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderGeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isNotImplementedType());
}

TEST_F(BytesBuiltinsTest, DunderGeWithBytesSubclassComparesBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b'123')
other = Foo(b'123')
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object other(&scope, mainModuleAt(&runtime_, "other"));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  EXPECT_EQ(ge, Bool::trueObj());
}

TEST_F(BytesBuiltinsTest, DunderGeWithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(5, 'a'));
  Object other(&scope, runtime_.newBytes(5, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(Bool::cast(*ge).value());
}

TEST_F(BytesBuiltinsTest, DunderGeWithShorterOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(2, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(Bool::cast(*ge).value());
}

TEST_F(BytesBuiltinsTest, DunderGeWithLongerOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(4, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_FALSE(Bool::cast(*ge).value());
}

TEST_F(BytesBuiltinsTest,
       DunderGeWithLexicographicallyEarlierOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'b'));
  Object other(&scope, runtime_.newBytes(3, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(Bool::cast(*ge).value());
}

TEST_F(BytesBuiltinsTest, DunderGeWithLexicographicallyLaterOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(3, 'b'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_FALSE(Bool::cast(*ge).value());
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "bytes.__getitem__(b'')"),
                            LayoutId::kTypeError,
                            "'bytes.__getitem__' takes min 2 positional "
                            "arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, "bytes.__getitem__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "'bytes.__getitem__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithNonBytesSelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__getitem__(0, 1)"), LayoutId::kTypeError,
      "'__getitem__' requires a 'bytes' object but received a 'int'"));
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithLargeIntRaisesIndexError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "b''[2**64]"),
                            LayoutId::kIndexError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(BytesBuiltinsTest,
       DunderGetitemWithIntGreaterOrEqualLenRaisesIndexError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "b'abc'[3]"),
                            LayoutId::kIndexError, "index out of range"));
}

TEST_F(BytesBuiltinsTest,
       DunderGetitemWithNegativeIntGreaterThanLenRaisesIndexError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "b'abc'[-4]"),
                            LayoutId::kIndexError, "index out of range"));
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithNegativeIntIndexesFromEnd) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = b'hello'[-5]").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 'h'));
}

TEST_F(BytesBuiltinsTest, DunderGetitemIndexesFromBeginning) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = b'hello'[0]").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 'h'));
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithSliceReturnsBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = b'hello world'[:3]").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "hel"));
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithSliceStepReturnsBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = b'hello world'[1:6:2]").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "el "));
}

TEST_F(BytesBuiltinsTest, DunderGetitemWithNonIndexOtherRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, "b''[1.5]"), LayoutId::kTypeError,
                    "byte indices must be integers or slice, not float"));
}

TEST_F(BytesBuiltinsTest, DunderGtWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__gt__(b'')"), LayoutId::kTypeError,
      "'bytes.__gt__' takes min 2 positional arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderGtWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__gt__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "'bytes.__gt__' takes max 2 positional arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderGtWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime_.newBytes(1, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  EXPECT_TRUE(raised(*gt, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderGtWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isNotImplementedType());
}

TEST_F(BytesBuiltinsTest, DunderGtWithBytesSubclassComparesBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b'123')
other = Foo(b'123')
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object other(&scope, mainModuleAt(&runtime_, "other"));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  EXPECT_EQ(gt, Bool::falseObj());
}

TEST_F(BytesBuiltinsTest, DunderGtWithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(5, 'a'));
  Object other(&scope, runtime_.newBytes(5, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(Bool::cast(*gt).value());
}

TEST_F(BytesBuiltinsTest, DunderGtWithShorterOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(2, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_TRUE(Bool::cast(*gt).value());
}

TEST_F(BytesBuiltinsTest, DunderGtWithLongerOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(4, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(Bool::cast(*gt).value());
}

TEST_F(BytesBuiltinsTest,
       DunderGtWithLexicographicallyEarlierOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'b'));
  Object other(&scope, runtime_.newBytes(3, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_TRUE(Bool::cast(*gt).value());
}

TEST_F(BytesBuiltinsTest, DunderGtWithLexicographicallyLaterOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(3, 'b'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(Bool::cast(*gt).value());
}

TEST_F(BytesBuiltinsTest, DunderHashReturnsSmallInt) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte bytes[] = {'h', 'e', 'l', 'l', 'o', '\0'};
  Bytes bytes_obj(&scope, runtime_.newBytesWithAll(bytes));
  EXPECT_TRUE(runBuiltin(BytesBuiltins::dunderHash, bytes_obj).isSmallInt());
}

TEST_F(BytesBuiltinsTest, DunderHashSmallBytesReturnsSmallInt) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte bytes[] = {'h'};
  Bytes bytes_obj(&scope, runtime_.newBytesWithAll(bytes));
  EXPECT_TRUE(runBuiltin(BytesBuiltins::dunderHash, bytes_obj).isSmallInt());
}

TEST_F(BytesBuiltinsTest, DunderHashWithEquivalentBytesReturnsSameHash) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte bytes[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', '\0'};
  Bytes bytes_obj1(&scope, runtime_.newBytesWithAll(bytes));
  Bytes bytes_obj2(&scope, runtime_.newBytesWithAll(bytes));
  EXPECT_NE(*bytes_obj1, *bytes_obj2);
  Object result1(&scope, runBuiltin(BytesBuiltins::dunderHash, bytes_obj1));
  Object result2(&scope, runBuiltin(BytesBuiltins::dunderHash, bytes_obj2));
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_TRUE(result2.isSmallInt());
  EXPECT_EQ(*result1, *result2);
}

TEST_F(BytesBuiltinsTest, DunderHashWithBytesSubclassReturnsSameHash) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(bytes): pass
i0 = C(b"abc")
i1 = b"abc"
)")
                   .isError());
  Object i0(&scope, mainModuleAt(&runtime_, "i0"));
  Object i1(&scope, mainModuleAt(&runtime_, "i1"));

  Object result0(&scope, runBuiltin(BytesBuiltins::dunderHash, i0));
  Object result1(&scope, runBuiltin(BytesBuiltins::dunderHash, i1));
  EXPECT_TRUE(result0.isSmallInt());
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_EQ(result0, result1);
}

TEST_F(BytesBuiltinsTest, DunderIterReturnsBytesIterator) {
  HandleScope scope(thread_);
  Object self(&scope, Bytes::empty());
  Object result(&scope, runBuiltin(BytesBuiltins::dunderIter, self));
  EXPECT_TRUE(result.isBytesIterator());
}

TEST_F(BytesBuiltinsTest, DunderLeWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__le__(b'')"), LayoutId::kTypeError,
      "'bytes.__le__' takes min 2 positional arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderLeWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__le__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "'bytes.__le__' takes max 2 positional arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderLeWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime_.newBytes(1, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  EXPECT_TRUE(raised(*le, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderLeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isNotImplementedType());
}

TEST_F(BytesBuiltinsTest, DunderLeWithBytesSubclassComparesBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b'123')
other = Foo(b'123')
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object other(&scope, mainModuleAt(&runtime_, "other"));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  EXPECT_EQ(le, Bool::trueObj());
}

TEST_F(BytesBuiltinsTest, DunderLeWithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(5, 'a'));
  Object other(&scope, runtime_.newBytes(5, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(Bool::cast(*le).value());
}

TEST_F(BytesBuiltinsTest, DunderLeWithShorterOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(2, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_FALSE(Bool::cast(*le).value());
}

TEST_F(BytesBuiltinsTest, DunderLeWithLongerOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(4, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(Bool::cast(*le).value());
}

TEST_F(BytesBuiltinsTest,
       DunderLeWithLexicographicallyEarlierOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'b'));
  Object other(&scope, runtime_.newBytes(3, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_FALSE(Bool::cast(*le).value());
}

TEST_F(BytesBuiltinsTest, DunderLeWithLexicographicallyLaterOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(3, 'b'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(Bool::cast(*le).value());
}

TEST_F(BytesBuiltinsTest, DunderLenWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__len__()"), LayoutId::kTypeError,
      "'bytes.__len__' takes min 1 positional arguments but 0 given"));
}

TEST_F(BytesBuiltinsTest, DunderLenWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__len__(b'', b'')"), LayoutId::kTypeError,
      "'bytes.__len__' takes max 1 positional arguments but 2 given"));
}

TEST_F(BytesBuiltinsTest, DunderLenWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  EXPECT_TRUE(raised(*len, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderLenWithEmptyBytesReturnsZero) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytesWithAll(View<byte>(nullptr, 0)));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  EXPECT_EQ(len, SmallInt::fromWord(0));
}

TEST_F(BytesBuiltinsTest, DunderLenWithNonEmptyBytesReturnsLength) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(4, 'a'));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  EXPECT_EQ(len, SmallInt::fromWord(4));
}

TEST_F(BytesBuiltinsTest, DunderLenWithBytesSubclassReturnsLength) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b"1234567890")
  )")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  EXPECT_EQ(len, SmallInt::fromWord(10));
}

TEST_F(BytesBuiltinsTest, DunderLtWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__lt__(b'')"), LayoutId::kTypeError,
      "'bytes.__lt__' takes min 2 positional arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderLtWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__lt__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "'bytes.__lt__' takes max 2 positional arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderLtWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime_.newBytes(1, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  EXPECT_TRUE(raised(*lt, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderLtWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isNotImplementedType());
}

TEST_F(BytesBuiltinsTest, DunderLtWithBytesSubclassComparesBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b'123')
other = Foo(b'123')
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object other(&scope, mainModuleAt(&runtime_, "other"));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  EXPECT_EQ(lt, Bool::falseObj());
}

TEST_F(BytesBuiltinsTest, DunderLtWithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(5, 'a'));
  Object other(&scope, runtime_.newBytes(5, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(Bool::cast(*lt).value());
}

TEST_F(BytesBuiltinsTest, DunderLtWithShorterOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(2, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(Bool::cast(*lt).value());
}

TEST_F(BytesBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(4, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_TRUE(Bool::cast(*lt).value());
}

TEST_F(BytesBuiltinsTest,
       DunderLtWithLexicographicallyEarlierOtherReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'b'));
  Object other(&scope, runtime_.newBytes(3, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(Bool::cast(*lt).value());
}

TEST_F(BytesBuiltinsTest, DunderLtWithLexicographicallyLaterOtherReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(3, 'b'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_TRUE(Bool::cast(*lt).value());
}

TEST_F(BytesBuiltinsTest, DunderMulWithNonBytesRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__mul__(0, 1)"), LayoutId::kTypeError,
      "'__mul__' requires a 'bytes' object but got 'int'"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, Bytes::empty());
  Object count(&scope, runtime_.newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BytesBuiltins::dunderMul, self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithIntSubclassReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  const byte view[] = {'a', 'b', 'c'};
  Object self(&scope, runtime_.newBytesWithAll(view));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(int): pass
count = C(4)
)")
                   .isError());
  Object count(&scope, mainModuleAt(&runtime_, "count"));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "abcabcabcabc"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(&runtime_, "count"));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "aa"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(&runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(BytesBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(&runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  HandleScope scope(thread_);
  Object self(&scope, Bytes::empty());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime_.newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithOverflowRaisesOverflowError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "repeated bytes are too long"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithEmptyBytesReturnsEmptyBytes) {
  HandleScope scope(thread_);
  Object self(&scope, Bytes::empty());
  Object count(&scope, runtime_.newInt(10));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST_F(BytesBuiltinsTest, DunderMulWithNegativeReturnsEmptyBytes) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(4, 'a'));
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST_F(BytesBuiltinsTest, DunderMulWithZeroReturnsEmptyBytes) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(4, 'a'));
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST_F(BytesBuiltinsTest, DunderMulWithOneReturnsSameBytes) {
  HandleScope scope(thread_);
  const byte bytes_array[] = {'a', 'b'};
  Object self(&scope, runtime_.newBytesWithAll(bytes_array));
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ab"));
}

TEST_F(BytesBuiltinsTest, DunderMulReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  const byte bytes_array[] = {'a', 'b'};
  Object self(&scope, runtime_.newBytesWithAll(bytes_array));
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ababab"));
}

TEST_F(BytesBuiltinsTest, DunderMulWithBytesSubclassReturnsRepeatedBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b"ab")
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ababab"));
}

TEST_F(BytesBuiltinsTest, DunderNeWithTooFewArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__ne__(b'')"), LayoutId::kTypeError,
      "'bytes.__ne__' takes min 2 positional arguments but 1 given"));
}

TEST_F(BytesBuiltinsTest, DunderNeWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__ne__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "'bytes.__ne__' takes max 2 positional arguments but 3 given"));
}

TEST_F(BytesBuiltinsTest, DunderNeWithNonBytesSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime_.newBytes(1, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  EXPECT_TRUE(raised(*ne, LayoutId::kTypeError));
}

TEST_F(BytesBuiltinsTest, DunderNeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  EXPECT_TRUE(ne.isNotImplementedType());
}

TEST_F(BytesBuiltinsTest, DunderNeWithBytesSubclassComparesBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b'123')
other = Foo(b'123')
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object other(&scope, mainModuleAt(&runtime_, "other"));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  EXPECT_EQ(ne, Bool::falseObj());
}

TEST_F(BytesBuiltinsTest, DunderNeWithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(5, 'a'));
  Object other(&scope, runtime_.newBytes(5, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_FALSE(Bool::cast(*ne).value());
}

TEST_F(BytesBuiltinsTest, DunderNeWithDifferentLengthsReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(1, 'a'));
  Object other(&scope, runtime_.newBytes(4, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_TRUE(Bool::cast(*ne).value());
}

TEST_F(BytesBuiltinsTest, DunderNeWithDifferentContentsReturnsTrue) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(3, 'a'));
  Object other(&scope, runtime_.newBytes(3, 'b'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_TRUE(Bool::cast(*ne).value());
}

TEST_F(BytesBuiltinsTest, DunderNewWithoutSourceWithEncodingRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "bytes(encoding='ascii')"),
                            LayoutId::kTypeError,
                            "encoding or errors without sequence argument"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithoutSourceWithErrorsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "bytes(errors='strict')"),
                            LayoutId::kTypeError,
                            "encoding or errors without sequence argument"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithoutArgsReturnsEmptyBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "obj = bytes()").isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_TRUE(isBytesEqualsCStr(obj, ""));
}

TEST_F(BytesBuiltinsTest,
       DunderNewWithNonStringSourceWithEncodingRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "bytes(1, 'ascii')"),
                            LayoutId::kTypeError,
                            "encoding without a string argument"));
}

TEST_F(BytesBuiltinsTest,
       DunderNewWithoutEncodingWithErrorsAndStringSourceRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes('', errors='strict')"),
      LayoutId::kTypeError, "string argument without an encoding"));
}

TEST_F(BytesBuiltinsTest,
       DunderNewWithoutEncodingWithErrorsAndNonStringSourceRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "bytes(1, errors='strict')"),
                            LayoutId::kTypeError,
                            "errors without a string argument"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithMistypedDunderBytesRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Foo:
  def __bytes__(self): return 1
bytes(Foo())
)"),
                            LayoutId::kTypeError,
                            "__bytes__ returned non-bytes (type int)"));
}

TEST_F(BytesBuiltinsTest, DunderNewPropagatesDunderBytesError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Foo:
  def __bytes__(self): raise SystemError("foo")
bytes(Foo())
)"),
                            LayoutId::kSystemError, "foo"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithDunderBytesReturnsBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __bytes__(self): return b'foo'
result = bytes(Foo())
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "foo"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithNegativeIntegerSourceRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "result = bytes(-1)"),
                            LayoutId::kValueError, "negative count"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithLargeIntegerSourceRaisesOverflowError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "result = bytes(2**63)"),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithIntegerSourceReturnsZeroFilledBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = bytes(10)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte bytes[10] = {};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(BytesBuiltinsTest, DunderNewWithBytesReturnsSameBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = bytes(b'123')").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte bytes[] = {'1', '2', '3'};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(BytesBuiltinsTest, DunderNewWithByteArrayReturnsBytesCopy) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = bytes(bytearray(b'123'))").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte bytes[] = {'1', '2', '3'};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(BytesBuiltinsTest, DunderNewWithListReturnsNewBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = bytes([6, 28])").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte bytes[] = {6, 28};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(BytesBuiltinsTest, DunderNewWithTupleReturnsNewBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = bytes((6, 28))").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte bytes[] = {6, 28};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST_F(BytesBuiltinsTest, DunderNewWithNegativeRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "result = bytes([-1])"),
                            LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithGreaterThanByteRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "result = bytes([256])"),
                            LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST_F(BytesBuiltinsTest, DunderNewWithIterableReturnsNewBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __iter__(self):
    return [1, 2, 3].__iter__()
result = bytes(Foo())
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  const byte expected[] = {1, 2, 3};
  EXPECT_TRUE(isBytesEqualsBytes(result, expected));
}

TEST_F(BytesBuiltinsTest, DunderReprWithNonBytesRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.__repr__(bytearray())"),
      LayoutId::kTypeError,
      "'__repr__' requires a 'bytes' object but got 'bytearray'"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithEmptyBytesReturnsEmptyRepr) {
  HandleScope scope(thread_);
  Object self(&scope, Bytes::empty());
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "b''"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithSimpleBytesReturnsRepr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newBytes(10, '*'));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "b'**********'"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithBytesSubclassReturnsStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b"*****")
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "b'*****'"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithDoubleQuoteUsesSingleQuoteDelimiters) {
  HandleScope scope(thread_);
  const byte view[] = {'_', '"', '_'};
  Object self(&scope, runtime_.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'_"_')"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  HandleScope scope(thread_);
  const byte view[] = {'_', '\'', '_'};
  Object self(&scope, runtime_.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b"_'_")"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithBothQuotesUsesSingleQuoteDelimiters) {
  HandleScope scope(thread_);
  const byte view[] = {'_', '"', '_', '\'', '_'};
  Object self(&scope, runtime_.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'_"_\'_')"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithSpeciaBytesUsesEscapeSequences) {
  HandleScope scope(thread_);
  const byte view[] = {'\\', '\t', '\n', '\r'};
  Object self(&scope, runtime_.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'\\\t\n\r')"));
}

TEST_F(BytesBuiltinsTest, DunderReprWithSmallAndLargeBytesUsesHex) {
  HandleScope scope(thread_);
  const byte view[] = {0, 0x1f, 0x80, 0xff};
  Object self(&scope, runtime_.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'\x00\x1f\x80\xff')"));
}

TEST_F(BytesBuiltinsTest, DunderRmulCallsDunderMul) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = 3 * b'1'").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "111"));
}

TEST_F(BytesBuiltinsTest, DecodeWithASCIIReturnsString) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = b'hello'.decode('ascii')").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello"));
}

TEST_F(BytesBuiltinsTest, HexWithNonBytesRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "bytes.hex(1)"),
                            LayoutId::kTypeError,
                            "'hex' requires a 'bytes' object but got 'int'"));
}

TEST_F(BytesBuiltinsTest, HexWithEmptyBytesReturnsEmptyString) {
  HandleScope scope(thread_);
  Bytes self(&scope, Bytes::empty());
  Object result(&scope, runBuiltin(BytesBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(BytesBuiltinsTest, HexWithNonEmptyBytesReturnsString) {
  HandleScope scope(thread_);
  const byte bytes_array[] = {0x12, 0x34, 0xfe, 0x5b};
  Bytes self(&scope, runtime_.newBytesWithAll(bytes_array));
  Object result(&scope, runBuiltin(BytesBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1234fe5b"));
}

TEST_F(BytesBuiltinsTest, HexWithBytesSubclassReturnsStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
self = Foo(b"*\x01a\x92")
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(&runtime_, "self"));
  Object repr(&scope, runBuiltin(BytesBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "2a016192"));
}

TEST_F(BytesBuiltinsTest, JoinWithNonIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "b''.join(0)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(BytesBuiltinsTest, JoinWithMistypedIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "b' '.join([1])"), LayoutId::kTypeError,
      "sequence item 0: expected a bytes-like object, int found"));
}

TEST_F(BytesBuiltinsTest, JoinWithIterableReturnsBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __iter__(self):
    return [b'ab', b'c', b'def'].__iter__()
result = b' '.join(Foo())
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ab c def"));
}

TEST_F(BytesBuiltinsTest, MaketransWithNonBytesLikeFromRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.maketrans([1,2], b'ab')"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'list'"));
}

TEST_F(BytesBuiltinsTest, MaketransWithNonBytesLikeToRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "bytes.maketrans(b'1', 2)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST_F(BytesBuiltinsTest, MaketransWithDifferentLengthsRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.maketrans(b'12', bytearray())"),
      LayoutId::kValueError, "maketrans arguments must have same length"));
}

TEST_F(BytesBuiltinsTest, MaketransWithEmptyReturnsDefaultBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = bytes.maketrans(bytearray(), b'')")
          .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  byte expected[256];
  for (word i = 0; i < 256; i++) {
    expected[i] = i;
  }
  EXPECT_TRUE(isBytesEqualsBytes(result, expected));
}

TEST_F(BytesBuiltinsTest, MaketransWithNonEmptyReturnsBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_,
                  "result = bytes.maketrans(bytearray(b'abc'), b'123')")
          .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isBytes());
  Bytes actual(&scope, *result);
  EXPECT_EQ(actual.byteAt('a'), '1');
  EXPECT_EQ(actual.byteAt('b'), '2');
  EXPECT_EQ(actual.byteAt('c'), '3');
}

TEST_F(BytesBuiltinsTest, TranslateWithNonBytesSelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "bytes.translate(bytearray(), None)"),
      LayoutId::kTypeError,
      "'translate' requires a 'bytes' object but got 'bytearray'"));
}

TEST_F(BytesBuiltinsTest, TranslateWithNonBytesLikeTableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "b''.translate(42)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST_F(BytesBuiltinsTest, TranslateWithNonBytesLikeDeleteRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "b''.translate(None, 42)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST_F(BytesBuiltinsTest, TranslateWithShortTableRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "b''.translate(b'')"),
                            LayoutId::kValueError,
                            "translation table must be 256 characters long"));
}

TEST_F(BytesBuiltinsTest, TranslateWithEmptyBytesReturnsEmptyBytes) {
  HandleScope scope(thread_);
  Object self(&scope, Bytes::empty());
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_.newByteArray());
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_EQ(result, Bytes::empty());
}

TEST_F(BytesBuiltinsTest, TranslateWithNonEmptySecondArgDeletesBytes) {
  HandleScope scope(thread_);
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Object self(&scope, runtime_.newBytesWithAll(alabama));
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_.newBytesWithAll(abc));
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_TRUE(isBytesEqualsCStr(result, "Alm"));
}

TEST_F(BytesBuiltinsTest, TranslateWithTableTranslatesBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "table = bytes.maketrans(b'Aa', b'12')")
                   .isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  Object self(&scope, runtime_.newBytesWithAll(alabama));
  Object table(&scope, mainModuleAt(&runtime_, "table"));
  Object del(&scope, Bytes::empty());
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_TRUE(isBytesEqualsCStr(result, "1l2b2m2"));
}

TEST_F(BytesBuiltinsTest,
       TranslateWithTableAndDeleteTranslatesAndDeletesBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "table = bytes.maketrans(b'Aa', b'12')")
                   .isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Object self(&scope, runtime_.newBytesWithAll(alabama));
  Object table(&scope, mainModuleAt(&runtime_, "table"));
  Object del(&scope, runtime_.newBytesWithAll(abc));
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_TRUE(isBytesEqualsCStr(result, "1lm"));
}

TEST_F(BytesBuiltinsTest, TranslateDeletesAllBytes) {
  HandleScope scope(thread_);
  const byte alabama[] = {'b', 'a', 'c', 'a', 'a', 'c', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Object self(&scope, runtime_.newBytesWithAll(alabama));
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_.newBytesWithAll(abc));
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_EQ(result, Bytes::empty());
}

TEST_F(BytesIteratorBuiltinsTest, DunderLengthHintReturnsRemainingCount) {
  HandleScope scope(thread_);
  const byte data[] = {100, 0, 37};
  Bytes bytes(&scope, SmallBytes::fromBytes(data));
  Object iter(&scope, runtime_.newBytesIterator(thread_, bytes));
  Object result(&scope,
                runBuiltin(BytesIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
  ASSERT_TRUE(!runBuiltin(BytesIteratorBuiltins::dunderNext, iter).isError());
  result = runBuiltin(BytesIteratorBuiltins::dunderLengthHint, iter);
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
  ASSERT_TRUE(!runBuiltin(BytesIteratorBuiltins::dunderNext, iter).isError());
  result = runBuiltin(BytesIteratorBuiltins::dunderLengthHint, iter);
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
  ASSERT_TRUE(!runBuiltin(BytesIteratorBuiltins::dunderNext, iter).isError());
  result = runBuiltin(BytesIteratorBuiltins::dunderLengthHint, iter);
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
  EXPECT_TRUE(raised(runBuiltin(BytesIteratorBuiltins::dunderNext, iter),
                     LayoutId::kStopIteration));
}

TEST_F(BytesIteratorBuiltinsTest, DunderIterReturnsSelf) {
  HandleScope scope(thread_);
  const byte data[] = {100, 0, 37};
  Bytes bytes(&scope, SmallBytes::fromBytes(data));
  Object iter(&scope, runtime_.newBytesIterator(thread_, bytes));
  Object result(&scope, runBuiltin(BytesIteratorBuiltins::dunderIter, iter));
  EXPECT_EQ(result, iter);
}

TEST_F(BytesIteratorBuiltinsTest, DunderNextReturnsNextElement) {
  HandleScope scope(thread_);
  const byte data[] = {100, 0, 37};
  Bytes bytes(&scope, SmallBytes::fromBytes(data));
  Object iter(&scope, runtime_.newBytesIterator(thread_, bytes));
  Object result(&scope, runBuiltin(BytesIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
  result = runBuiltin(BytesIteratorBuiltins::dunderNext, iter);
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
  result = runBuiltin(BytesIteratorBuiltins::dunderNext, iter);
  EXPECT_TRUE(isIntEqualsWord(*result, 37));
  EXPECT_TRUE(raised(runBuiltin(BytesIteratorBuiltins::dunderNext, iter),
                     LayoutId::kStopIteration));
}

}  // namespace py
