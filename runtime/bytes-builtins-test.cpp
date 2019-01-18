#include "gtest/gtest.h"

#include "bytes-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BytesBuiltinsTest, DunderAddWithZeroArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd));
  ASSERT_TRUE(sum->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderAddWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, '1'));
  Object arg1(&scope, runtime.newBytes(2, '2'));
  Object arg2(&scope, runtime.newBytes(3, '3'));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, arg1, arg2));
  ASSERT_TRUE(sum->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderAddWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self));
  ASSERT_TRUE(sum->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderAddWithNonBytesOtherRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, '1'));
  Object other(&scope, SmallInt::fromWord(2));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd));
  ASSERT_TRUE(sum->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderAddWithTwoBytesReturnsConcatenatedBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, '1'));
  Object other(&scope, runtime.newBytes(2, '2'));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  ASSERT_TRUE(sum->isBytes());
  Bytes result(&scope, *sum);
  EXPECT_EQ(result->length(), 3);
  EXPECT_EQ(result->byteAt(0), '1');
  EXPECT_EQ(result->byteAt(1), '2');
  EXPECT_EQ(result->byteAt(2), '2');
}

TEST(BytesBuiltinsTest, DunderEqWithWrongNumberOfArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq));
  ASSERT_TRUE(eq->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderEqWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderEqWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  EXPECT_TRUE(eq->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderEqWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq->isBool());
  EXPECT_TRUE(RawBool::cast(eq)->value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentLengthsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq->isBool());
  EXPECT_FALSE(RawBool::cast(eq)->value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentContentsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq->isBool());
  EXPECT_FALSE(RawBool::cast(eq)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithWrongNumberOfArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe));
  ASSERT_TRUE(ge->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGeWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self));
  ASSERT_TRUE(ge->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGeWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderGeWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_TRUE(RawBool::cast(ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_TRUE(RawBool::cast(ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_FALSE(RawBool::cast(ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_TRUE(RawBool::cast(ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_FALSE(RawBool::cast(ge)->value());
}

TEST(BytesBuiltinsTest, DunderGetItemWithWrongNumberOfArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem));
  ASSERT_TRUE(item->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGetItemWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self));
  ASSERT_TRUE(item->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGetItemWithLargeIntRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  uword idx[] = {1, 1};
  Object index(&scope, runtime.newIntWithDigits(View<uword>(idx, 2)));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  ASSERT_TRUE(item->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kIndexError));
}

TEST(BytesBuiltinsTest, DunderGetItemWithIntGreaterOrEqualLenRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object index(&scope, RawSmallInt::fromWord(4));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  ASSERT_TRUE(item->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kIndexError));
}

TEST(BytesBuiltinsTest,
     DunderGetItemWithNegativeIntGreaterThanLenRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object index(&scope, runtime.newInt(-4));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  ASSERT_TRUE(item->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kIndexError));
}

TEST(BytesBuiltinsTest, DunderGetItemWithNegativeIntIndexesFromEnd) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello";
  Object self(&scope, runtime.newBytesWithAll(View<byte>(hello, 5)));
  Object index(&scope, runtime.newInt(-5));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  ASSERT_TRUE(item->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*item), RawSmallInt::fromWord('h'));
}

TEST(BytesBuiltinsTest, DunderGetItemIndexesFromBeginning) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello";
  Object self(&scope, runtime.newBytesWithAll(View<byte>(hello, 5)));
  Object index(&scope, RawSmallInt::fromWord(0));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  ASSERT_TRUE(item->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*item), RawSmallInt::fromWord('h'));
}

TEST(BytesBuiltinsTest, DunderGetItemWithSliceReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello world";
  Bytes self(&scope, runtime.newBytesWithAll(View<byte>(hello, 11)));
  Object one(&scope, RawSmallInt::fromWord(1));
  Object two(&scope, RawSmallInt::fromWord(2));
  Object six(&scope, RawSmallInt::fromWord(6));
  Object index(&scope, runtime.newSlice(one, six, two));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  ASSERT_TRUE(item->isBytes());
  Bytes result(&scope, *item);
  EXPECT_EQ(result->length(), 3);
  EXPECT_EQ(result->byteAt(0), 'e');
  EXPECT_EQ(result->byteAt(1), 'l');
  EXPECT_EQ(result->byteAt(2), ' ');
}

TEST(BytesBuiltinsTest, DunderGetItemWithNonIndexOtherRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newFloat(1.5));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, other));
  ASSERT_TRUE(item->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGtWithWrongNumberOfArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt));
  ASSERT_TRUE(gt->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGtWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self));
  ASSERT_TRUE(gt->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGtWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderGtWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_FALSE(RawBool::cast(gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_TRUE(RawBool::cast(gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_FALSE(RawBool::cast(gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_TRUE(RawBool::cast(gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_FALSE(RawBool::cast(gt)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithWrongNumberOfArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe));
  ASSERT_TRUE(le->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLeWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self));
  ASSERT_TRUE(le->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLeWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderLeWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_TRUE(RawBool::cast(le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_FALSE(RawBool::cast(le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_TRUE(RawBool::cast(le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_FALSE(RawBool::cast(le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_TRUE(RawBool::cast(le)->value());
}

TEST(BytesBuiltinsTest, DunderLenWithZeroArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen));
  ASSERT_TRUE(len->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLenWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object arg1(&scope, runtime.newBytes(1, 'b'));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self, arg1));
  ASSERT_TRUE(len->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLenWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  ASSERT_TRUE(len->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLenWithEmptyBytesReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytesWithAll({nullptr, 0}));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  EXPECT_EQ(len, SmallInt::fromWord(0));
}

TEST(BytesBuiltinsTest, DunderLenWithNonEmptyBytesReturnsLength) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(4, 'a'));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  EXPECT_EQ(len, SmallInt::fromWord(4));
}

TEST(BytesBuiltinsTest, DunderLtWithWrongNumberOfArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt));
  ASSERT_TRUE(lt->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLtWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self));
  ASSERT_TRUE(lt->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLtWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderLtWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_FALSE(RawBool::cast(lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_FALSE(RawBool::cast(lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_TRUE(RawBool::cast(lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_FALSE(RawBool::cast(lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_TRUE(RawBool::cast(lt)->value());
}

TEST(BytesBuiltinsTest, DunderNeWithWrongNumberOfArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe));
  ASSERT_TRUE(ne->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderNeWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderNeWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  EXPECT_TRUE(ne->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderNeWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne->isBool());
  EXPECT_FALSE(RawBool::cast(ne)->value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentLengthsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne->isBool());
  EXPECT_TRUE(RawBool::cast(ne)->value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentContentsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne->isBool());
  EXPECT_TRUE(RawBool::cast(ne)->value());
}

}  // namespace python
