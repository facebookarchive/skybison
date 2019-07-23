#include "gtest/gtest.h"

#include "frame.h"
#include "objects.h"
#include "range-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using LongRangeIteratorBuiltinsTest = RuntimeFixture;
using RangeBuiltinsTest = RuntimeFixture;
using RangeIteratorBuiltinsTest = RuntimeFixture;

TEST_F(LongRangeIteratorBuiltinsTest, DunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, runtime_.newIntFromUnsigned(kMaxUword));
  Int step(&scope, SmallInt::fromWord(1));
  Object iter(&scope, runtime_.newLongRangeIterator(start, stop, step));
  Object result(&scope,
                runBuiltin(LongRangeIteratorBuiltins::dunderIter, iter));
  EXPECT_EQ(result, iter);
}

TEST_F(LongRangeIteratorBuiltinsTest, DunderLengthHintReturnsPendingLength) {
  HandleScope scope(thread_);
  Int zero(&scope, SmallInt::fromWord(0));
  Int six(&scope, SmallInt::fromWord(6));
  Int neg(&scope, SmallInt::fromWord(-6));
  Int max(&scope, runtime_.newIntFromUnsigned(kMaxUword));
  Object empty(&scope, runtime_.newLongRangeIterator(max, zero, six));
  Object forwards(&scope, runtime_.newLongRangeIterator(zero, max, six));
  Object backwards(&scope, runtime_.newLongRangeIterator(max, zero, neg));
  word expected = static_cast<word>(kMaxUword / 6 + 1);

  Object length_hint1(
      &scope, runBuiltin(LongRangeIteratorBuiltins::dunderLengthHint, empty));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 0));

  Object length_hint2(
      &scope,
      runBuiltin(LongRangeIteratorBuiltins::dunderLengthHint, forwards));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, expected));

  Object length_hint3(
      &scope,
      runBuiltin(LongRangeIteratorBuiltins::dunderLengthHint, backwards));
  EXPECT_TRUE(isIntEqualsWord(*length_hint3, expected));
}

TEST_F(LongRangeIteratorBuiltinsTest, DunderNextWithEmptyRaisesStopIteration) {
  HandleScope scope(thread_);
  Int start(&scope, runtime_.newIntFromUnsigned(kMaxUword));
  Int stop(&scope, SmallInt::fromWord(0));
  Int step(&scope, SmallInt::fromWord(1));
  Object iter(&scope, runtime_.newLongRangeIterator(start, stop, step));
  EXPECT_TRUE(raised(runBuiltin(LongRangeIteratorBuiltins::dunderNext, iter),
                     LayoutId::kStopIteration));
}

TEST_F(LongRangeIteratorBuiltinsTest, DunderNextWithNonEmptyReturnsInts) {
  HandleScope scope(thread_);
  Int start(&scope, runtime_.newIntFromUnsigned(kMaxUword - 15));
  Int stop(&scope, runtime_.newIntFromUnsigned(kMaxUword));
  Int step(&scope, SmallInt::fromWord(10));
  Object iter(&scope, runtime_.newLongRangeIterator(start, stop, step));

  Object item1(&scope, runBuiltin(LongRangeIteratorBuiltins::dunderNext, iter));
  const uword result1[] = {kMaxUword - 15, 0};
  EXPECT_TRUE(isIntEqualsDigits(*item1, result1));

  Object item2(&scope, runBuiltin(LongRangeIteratorBuiltins::dunderNext, iter));
  const uword result2[] = {kMaxUword - 5, 0};
  EXPECT_TRUE(isIntEqualsDigits(*item2, result2));

  EXPECT_TRUE(raised(runBuiltin(LongRangeIteratorBuiltins::dunderNext, iter),
                     LayoutId::kStopIteration));
}

TEST_F(LongRangeIteratorBuiltinsTest, DunderNextModifiesPendingLength) {
  HandleScope scope(thread_);
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, runtime_.newIntFromUnsigned(kMaxUword));
  Int step(&scope, SmallInt::fromWord(4));
  LongRangeIterator iter(&scope,
                         runtime_.newLongRangeIterator(start, stop, step));
  Object length_hint(
      &scope, runBuiltin(LongRangeIteratorBuiltins::dunderLengthHint, iter));
  word result1 = static_cast<word>(kMaxUword / 4 + 1);
  EXPECT_TRUE(isIntEqualsWord(*length_hint, result1));

  ASSERT_FALSE(
      runBuiltin(LongRangeIteratorBuiltins::dunderNext, iter).isError());
  length_hint = runBuiltin(LongRangeIteratorBuiltins::dunderLengthHint, iter);
  word result2 = result1 - 1;
  EXPECT_TRUE(isIntEqualsWord(*length_hint, result2));
}

TEST_F(RangeBuiltinsTest, DunderIterReturnsRangeIter) {
  HandleScope scope(thread_);
  Int zero(&scope, SmallInt::fromWord(0));
  Int one(&scope, SmallInt::fromWord(1));
  Object range(&scope, runtime_.newRange(zero, one, one));
  Object iter(&scope, runBuiltin(RangeBuiltins::dunderIter, range));
  EXPECT_TRUE(iter.isRangeIterator());
}

TEST_F(RangeIteratorBuiltinsTest, DunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Object iter(&scope, runtime_.newRangeIterator(0, 2, 13));
  Object result(&scope, runBuiltin(RangeIteratorBuiltins::dunderIter, iter));
  EXPECT_EQ(result, iter);
}

TEST_F(RangeIteratorBuiltinsTest, DunderLengthHintReturnsPendingLength) {
  HandleScope scope(thread_);
  Object empty(&scope, runtime_.newRangeIterator(-12, 10, 0));
  Object forwards(&scope, runtime_.newRangeIterator(3, 15, 4));
  Object backwards(&scope, runtime_.newRangeIterator(0, -3, 2));

  Object length_hint1(
      &scope, runBuiltin(RangeIteratorBuiltins::dunderLengthHint, empty));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 0));

  Object length_hint2(
      &scope, runBuiltin(RangeIteratorBuiltins::dunderLengthHint, forwards));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, 4));

  Object length_hint3(
      &scope, runBuiltin(RangeIteratorBuiltins::dunderLengthHint, backwards));
  EXPECT_TRUE(isIntEqualsWord(*length_hint3, 2));
}

TEST_F(RangeIteratorBuiltinsTest, DunderNextWithEmptyRaisesStopIteration) {
  HandleScope scope(thread_);
  Object iter(&scope, runtime_.newRangeIterator(23, 5, 0));
  EXPECT_TRUE(raised(runBuiltin(RangeIteratorBuiltins::dunderNext, iter),
                     LayoutId::kStopIteration));
}

TEST_F(RangeIteratorBuiltinsTest, DunderNextWithNonEmptyReturnsInts) {
  HandleScope scope(thread_);
  Object iter(&scope, runtime_.newRangeIterator(0, 1, 2));

  Object item1(&scope, runBuiltin(RangeIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object item2(&scope, runBuiltin(RangeIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item2, 1));

  EXPECT_TRUE(raised(runBuiltin(RangeIteratorBuiltins::dunderNext, iter),
                     LayoutId::kStopIteration));
}

TEST_F(RangeIteratorBuiltinsTest, DunderNextModifiesPendingLength) {
  HandleScope scope(thread_);
  RangeIterator iter(&scope, runtime_.newRangeIterator(0, 1, 2));

  ASSERT_FALSE(runBuiltin(RangeIteratorBuiltins::dunderNext, iter).isError());
  Object length_hint(&scope,
                     runBuiltin(RangeIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 1));

  ASSERT_FALSE(runBuiltin(RangeIteratorBuiltins::dunderNext, iter).isError());
  length_hint = runBuiltin(RangeIteratorBuiltins::dunderLengthHint, iter);
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

}  // namespace python
