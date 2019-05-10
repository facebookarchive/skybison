#include "gtest/gtest.h"

#include "bytes-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BytesBuiltinsTest, BuiltinBaseIsBytes) {
  Runtime runtime;
  HandleScope scope;
  Type bytes_type(&scope, runtime.typeAt(LayoutId::kBytes));
  EXPECT_EQ(bytes_type.builtinBase(), LayoutId::kBytes);
}

TEST(BytesBuiltinsTest, DunderAddWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__add__(b'')"), LayoutId::kTypeError,
      "TypeError: 'bytes.__add__' takes 2 positional arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderAddWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__add__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__add__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderAddWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, '1'));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(raised(*sum, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderAddWithNonBytesOtherRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, '1'));
  Object other(&scope, SmallInt::fromWord(2));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(raised(*sum, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderAddWithTwoBytesReturnsConcatenatedBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, '1'));
  Object other(&scope, runtime.newBytes(2, '2'));
  Object sum(&scope, runBuiltin(BytesBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(isBytesEqualsCStr(sum, "122"));
}

TEST(BytesBuiltinsTest, DunderEqWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__eq__(b'')"), LayoutId::kTypeError,
      "TypeError: 'bytes.__eq__' takes 2 positional arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderEqWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__eq__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__eq__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderEqWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  EXPECT_TRUE(raised(*eq, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderEqWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  EXPECT_TRUE(eq.isNotImplementedType());
}

TEST(BytesBuiltinsTest, DunderEqWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_TRUE(Bool::cast(*eq).value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentLengthsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_FALSE(Bool::cast(*eq).value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentContentsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_FALSE(Bool::cast(*eq).value());
}

TEST(BytesBuiltinsTest, DunderGeWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__ge__(b'')"), LayoutId::kTypeError,
      "TypeError: 'bytes.__ge__' takes 2 positional arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderGeWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__ge__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__ge__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderGeWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  EXPECT_TRUE(raised(*ge, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGeWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isNotImplementedType());
}

TEST(BytesBuiltinsTest, DunderGeWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(Bool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(Bool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_FALSE(Bool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(Bool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_FALSE(Bool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGetItemWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes.__getitem__(b'')"),
                            LayoutId::kTypeError,
                            "TypeError: 'bytes.__getitem__' takes 2 positional "
                            "arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__getitem__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__getitem__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__getitem__(0, 1)"), LayoutId::kTypeError,
      "'__getitem__' requires a 'bytes' object but received a 'int'"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithLargeIntRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b''[2**64]"),
                            LayoutId::kIndexError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithIntGreaterOrEqualLenRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b'abc'[3]"),
                            LayoutId::kIndexError, "index out of range"));
}

TEST(BytesBuiltinsTest,
     DunderGetItemWithNegativeIntGreaterThanLenRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b'abc'[-4]"),
                            LayoutId::kIndexError, "index out of range"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithNegativeIntIndexesFromEnd) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = b'hello'[-5]").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 'h'));
}

TEST(BytesBuiltinsTest, DunderGetItemIndexesFromBeginning) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = b'hello'[0]").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 'h'));
}

TEST(BytesBuiltinsTest, DunderGetItemWithSliceReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = b'hello world'[:3]").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "hel"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithSliceStepReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = b'hello world'[1:6:2]").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "el "));
}

TEST(BytesBuiltinsTest, DunderGetItemWithNonIndexOtherRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "b''[1.5]"), LayoutId::kTypeError,
                    "byte indices must be integers or slice, not float"));
}

TEST(BytesBuiltinsTest, DunderGtWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__gt__(b'')"), LayoutId::kTypeError,
      "TypeError: 'bytes.__gt__' takes 2 positional arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderGtWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__gt__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__gt__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderGtWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  EXPECT_TRUE(raised(*gt, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGtWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isNotImplementedType());
}

TEST(BytesBuiltinsTest, DunderGtWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(Bool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_TRUE(Bool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(Bool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_TRUE(Bool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(Bool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderHashReturnsSmallInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte bytes[] = {'h', 'e', 'l', 'l', 'o', '\0'};
  Bytes bytes_obj(&scope, runtime.newBytesWithAll(bytes));
  EXPECT_TRUE(runBuiltin(BytesBuiltins::dunderHash, bytes_obj).isSmallInt());
}

TEST(BytesBuiltinsTest, DunderHashSmallBytesReturnsSmallInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte bytes[] = {'h'};
  Bytes bytes_obj(&scope, runtime.newBytesWithAll(bytes));
  EXPECT_TRUE(runBuiltin(BytesBuiltins::dunderHash, bytes_obj).isSmallInt());
}

TEST(BytesBuiltinsTest, DunderHashWithEquivalentBytesReturnsSameHash) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  const byte bytes[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', '\0'};
  Bytes bytes_obj1(&scope, runtime.newBytesWithAll(bytes));
  Bytes bytes_obj2(&scope, runtime.newBytesWithAll(bytes));
  EXPECT_NE(*bytes_obj1, *bytes_obj2);
  Object result1(&scope, runBuiltin(BytesBuiltins::dunderHash, bytes_obj1));
  Object result2(&scope, runBuiltin(BytesBuiltins::dunderHash, bytes_obj2));
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_TRUE(result2.isSmallInt());
  EXPECT_EQ(*result1, *result2);
}

TEST(BytesBuiltinsTest, DunderLeWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__le__(b'')"), LayoutId::kTypeError,
      "TypeError: 'bytes.__le__' takes 2 positional arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderLeWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__le__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__le__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderLeWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  EXPECT_TRUE(raised(*le, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLeWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isNotImplementedType());
}

TEST(BytesBuiltinsTest, DunderLeWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(Bool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_FALSE(Bool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(Bool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_FALSE(Bool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(Bool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLenWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__len__()"), LayoutId::kTypeError,
      "TypeError: 'bytes.__len__' takes 1 positional arguments but 0 given"));
}

TEST(BytesBuiltinsTest, DunderLenWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes.__len__(b'', b'')"),
                            LayoutId::kTypeError,
                            "TypeError: 'bytes.__len__' takes max 1 positional "
                            "arguments but 2 given"));
}

TEST(BytesBuiltinsTest, DunderLenWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self));
  EXPECT_TRUE(raised(*len, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLenWithEmptyBytesReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytesWithAll(View<byte>(nullptr, 0)));
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

TEST(BytesBuiltinsTest, DunderLtWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__lt__(b'')"), LayoutId::kTypeError,
      "TypeError: 'bytes.__lt__' takes 2 positional arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderLtWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__lt__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__lt__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderLtWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  EXPECT_TRUE(raised(*lt, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderLtWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isNotImplementedType());
}

TEST(BytesBuiltinsTest, DunderLtWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(Bool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(Bool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_TRUE(Bool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(Bool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_TRUE(Bool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderMulWithNonBytesRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__mul__(0, 1)"), LayoutId::kTypeError,
      "'__mul__' requires a 'bytes' object but got 'int'"));
}

TEST(BytesBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, Bytes::empty());
  Object count(&scope, runtime.newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BytesBuiltins::dunderMul, self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST(BytesBuiltinsTest, DunderMulWithIntSubclassReturnsRepeatedBytes) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'a', 'b', 'c'};
  Object self(&scope, runtime.newBytesWithAll(view));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(int): pass
count = C(4)
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "abcabcabcabc"));
}

TEST(BytesBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "aa"));
}

TEST(BytesBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST(BytesBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST(BytesBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, Bytes::empty());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime.newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST(BytesBuiltinsTest, DunderMulWithOverflowRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "repeated bytes are too long"));
}

TEST(BytesBuiltinsTest, DunderMulWithEmptyBytesReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, Bytes::empty());
  Object count(&scope, runtime.newInt(10));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST(BytesBuiltinsTest, DunderMulWithNegativeReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(4, 'a'));
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST(BytesBuiltinsTest, DunderMulWithZeroReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(4, 'a'));
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST(BytesBuiltinsTest, DunderMulWithOneReturnsSameBytes) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes_array[] = {'a', 'b'};
  Object self(&scope, runtime.newBytesWithAll(bytes_array));
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ab"));
}

TEST(BytesBuiltinsTest, DunderMulReturnsRepeatedBytes) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes_array[] = {'a', 'b'};
  Object self(&scope, runtime.newBytesWithAll(bytes_array));
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ababab"));
}

TEST(BytesBuiltinsTest, DunderNeWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__ne__(b'')"), LayoutId::kTypeError,
      "TypeError: 'bytes.__ne__' takes 2 positional arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderNeWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "bytes.__ne__(b'', b'', b'')"),
                    LayoutId::kTypeError,
                    "TypeError: 'bytes.__ne__' takes max 2 positional "
                    "arguments but 3 given"));
}

TEST(BytesBuiltinsTest, DunderNeWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object other(&scope, runtime.newBytes(1, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  EXPECT_TRUE(raised(*ne, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderNeWithNonBytesOtherReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, SmallInt::fromWord(0));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  EXPECT_TRUE(ne.isNotImplementedType());
}

TEST(BytesBuiltinsTest, DunderNeWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_FALSE(Bool::cast(*ne).value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentLengthsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_TRUE(Bool::cast(*ne).value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentContentsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_TRUE(Bool::cast(*ne).value());
}

TEST(BytesBuiltinsTest, DunderNewWithoutSourceWithEncodingRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes(encoding='ascii')"),
                            LayoutId::kTypeError,
                            "encoding or errors without sequence argument"));
}

TEST(BytesBuiltinsTest, DunderNewWithoutSourceWithErrorsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes(errors='strict')"),
                            LayoutId::kTypeError,
                            "encoding or errors without sequence argument"));
}

TEST(BytesBuiltinsTest, DunderNewWithoutArgsReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "obj = bytes()").isError());
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  EXPECT_TRUE(isBytesEqualsCStr(obj, ""));
}

TEST(BytesBuiltinsTest,
     DunderNewWithNonStringSourceWithEncodingRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes(1, 'ascii')"),
                            LayoutId::kTypeError,
                            "encoding without a string argument"));
}

TEST(BytesBuiltinsTest,
     DunderNewWithoutEncodingWithErrorsAndStringSourceRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes('', errors='strict')"),
                            LayoutId::kTypeError,
                            "string argument without an encoding"));
}

TEST(BytesBuiltinsTest,
     DunderNewWithoutEncodingWithErrorsAndNonStringSourceRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes(1, errors='strict')"),
                            LayoutId::kTypeError,
                            "errors without a string argument"));
}

TEST(BytesBuiltinsTest, DunderNewWithMistypedDunderBytesRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self): return 1
bytes(Foo())
)"),
                            LayoutId::kTypeError,
                            "__bytes__ returned non-bytes (type int)"));
}

TEST(BytesBuiltinsTest, DunderNewPropagatesDunderBytesError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self): raise SystemError("foo")
bytes(Foo())
)"),
                            LayoutId::kSystemError, "foo"));
}

TEST(BytesBuiltinsTest, DunderNewWithDunderBytesReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self): return b'foo'
result = bytes(Foo())
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "foo"));
}

TEST(BytesBuiltinsTest, DunderNewWithNegativeIntegerSourceRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "result = bytes(-1)"),
                            LayoutId::kValueError, "negative count"));
}

TEST(BytesBuiltinsTest, DunderNewWithLargeIntegerSourceRaisesOverflowError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "result = bytes(2**63)"),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST(BytesBuiltinsTest, DunderNewWithIntegerSourceReturnsZeroFilledBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = bytes(10)").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  const byte bytes[10] = {};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST(BytesBuiltinsTest, DunderNewWithBytesReturnsSameBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = bytes(b'123')").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  const byte bytes[] = {'1', '2', '3'};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST(BytesBuiltinsTest, DunderNewWithByteArrayReturnsBytesCopy) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = bytes(bytearray(b'123'))").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  const byte bytes[] = {'1', '2', '3'};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST(BytesBuiltinsTest, DunderNewWithListReturnsNewBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = bytes([6, 28])").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  const byte bytes[] = {6, 28};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST(BytesBuiltinsTest, DunderNewWithTupleReturnsNewBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = bytes((6, 28))").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  const byte bytes[] = {6, 28};
  EXPECT_TRUE(isBytesEqualsBytes(result, bytes));
}

TEST(BytesBuiltinsTest, DunderNewWithNegativeRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "result = bytes([-1])"),
                            LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST(BytesBuiltinsTest, DunderNewWithGreaterThanByteRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "result = bytes([256])"),
                            LayoutId::kValueError,
                            "bytes must be in range(0, 256)"));
}

TEST(BytesBuiltinsTest, DunderNewWithIterableReturnsNewBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __iter__(self):
    return [1, 2, 3].__iter__()
result = bytes(Foo())
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  const byte expected[] = {1, 2, 3};
  EXPECT_TRUE(isBytesEqualsBytes(result, expected));
}

TEST(BytesBuiltinsTest, DunderReprWithNonBytesRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__repr__(bytearray())"),
      LayoutId::kTypeError,
      "'__repr__' requires a 'bytes' object but got 'bytearray'"));
}

TEST(BytesBuiltinsTest, DunderReprWithEmptyBytesReturnsEmptyRepr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, Bytes::empty());
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "b''"));
}

TEST(BytesBuiltinsTest, DunderReprWithSimpleBytesReturnsRepr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(10, '*'));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "b'**********'"));
}

TEST(BytesBuiltinsTest, DunderReprWithDoubleQuoteUsesSingleQuoteDelimiters) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'_', '"', '_'};
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'_"_')"));
}

TEST(BytesBuiltinsTest, DunderReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'_', '\'', '_'};
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b"_'_")"));
}

TEST(BytesBuiltinsTest, DunderReprWithBothQuotesUsesSingleQuoteDelimiters) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'_', '"', '_', '\'', '_'};
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'_"_\'_')"));
}

TEST(BytesBuiltinsTest, DunderReprWithSpeciaBytesUsesEscapeSequences) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {'\\', '\t', '\n', '\r'};
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'\\\t\n\r')"));
}

TEST(BytesBuiltinsTest, DunderReprWithSmallAndLargeBytesUsesHex) {
  Runtime runtime;
  HandleScope scope;
  const byte view[] = {0, 0x1f, 0x80, 0xff};
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'\x00\x1f\x80\xff')"));
}

TEST(BytesBuiltinsTest, DunderRmulCallsDunderMul) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = 3 * b'1'").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "111"));
}

TEST(BytesBuiltinsTest, DecodeWithASCIIReturnsString) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = b'hello'.decode('ascii')").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello"));
}

TEST(BytesBuiltinsTest, HexWithNonBytesRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes.hex(1)"),
                            LayoutId::kTypeError,
                            "'hex' requires a 'bytes' object but got 'int'"));
}

TEST(BytesBuiltinsTest, HexWithEmptyBytesReturnsEmptyString) {
  Runtime runtime;
  HandleScope scope;
  Bytes self(&scope, Bytes::empty());
  Object result(&scope, runBuiltin(BytesBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST(BytesBuiltinsTest, HexWithNonEmptyBytesReturnsString) {
  Runtime runtime;
  HandleScope scope;
  const byte bytes_array[] = {0x12, 0x34, 0xfe, 0x5b};
  Bytes self(&scope, runtime.newBytesWithAll(bytes_array));
  Object result(&scope, runBuiltin(BytesBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*result, "1234fe5b"));
}

TEST(BytesBuiltinsTest, JoinWithNonBytesRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes.join(1, [])"),
                            LayoutId::kTypeError,
                            "'join' requires a 'bytes' object"));
}

TEST(BytesBuiltinsTest, JoinWithNonIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b''.join(0)"),
                            LayoutId::kTypeError, "object is not iterable"));
}

TEST(BytesBuiltinsTest, JoinWithEmptyIterableReturnsEmptyByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime.newBytes(3, 'a'));
  Object iter(&scope, runtime.newTuple(0));
  Object result(&scope, runBuiltin(BytesBuiltins::join, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST(BytesBuiltinsTest, JoinWithEmptySeparatorReturnsBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, Bytes::empty());
  Tuple iter(&scope, runtime.newTuple(3));
  iter.atPut(0, runtime.newBytes(1, 'A'));
  iter.atPut(1, runtime.newBytes(2, 'B'));
  iter.atPut(2, runtime.newBytes(1, 'A'));
  Object result(&scope, runBuiltin(BytesBuiltins::join, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ABBA"));
}

TEST(BytesBuiltinsTest, JoinWithNonEmptyListReturnsBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime.newBytes(1, ' '));
  List iter(&scope, runtime.newList());
  Bytes value(&scope, runtime.newBytes(1, '*'));
  runtime.listAdd(thread, iter, value);
  runtime.listAdd(thread, iter, value);
  runtime.listAdd(thread, iter, value);
  Object result(&scope, runBuiltin(BytesBuiltins::join, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "* * *"));
}

TEST(BytesBuiltinsTest, JoinWithMistypedIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "b' '.join([1])"), LayoutId::kTypeError,
      "sequence item 0: expected a bytes-like object, int found"));
}

TEST(BytesBuiltinsTest, JoinWithIterableReturnsBytes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __iter__(self):
    return [b'ab', b'c', b'def'].__iter__()
result = b' '.join(Foo())
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ab c def"));
}

TEST(BytesBuiltinsTest, MaketransWithNonBytesLikeFromRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.maketrans([1,2], b'ab')"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'list'"));
}

TEST(BytesBuiltinsTest, MaketransWithNonBytesLikeToRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes.maketrans(b'1', 2)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST(BytesBuiltinsTest, MaketransWithDifferentLengthsRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.maketrans(b'12', bytearray())"),
      LayoutId::kValueError, "maketrans arguments must have same length"));
}

TEST(BytesBuiltinsTest, MaketransWithEmptyReturnsDefaultBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = bytes.maketrans(bytearray(), b'')")
          .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  byte expected[256];
  for (word i = 0; i < 256; i++) {
    expected[i] = i;
  }
  EXPECT_TRUE(isBytesEqualsBytes(result, expected));
}

TEST(BytesBuiltinsTest, MaketransWithNonEmptyReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime,
                  "result = bytes.maketrans(bytearray(b'abc'), b'123')")
          .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isBytes());
  Bytes actual(&scope, *result);
  EXPECT_EQ(actual.byteAt('a'), '1');
  EXPECT_EQ(actual.byteAt('b'), '2');
  EXPECT_EQ(actual.byteAt('c'), '3');
}

TEST(BytesBuiltinsTest, TranslateWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.translate(bytearray(), None)"),
      LayoutId::kTypeError,
      "'translate' requires a 'bytes' object but got 'bytearray'"));
}

TEST(BytesBuiltinsTest, TranslateWithNonBytesLikeTableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b''.translate(42)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST(BytesBuiltinsTest, TranslateWithNonBytesLikeDeleteRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b''.translate(None, 42)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST(BytesBuiltinsTest, TranslateWithShortTableRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b''.translate(b'')"),
                            LayoutId::kValueError,
                            "translation table must be 256 characters long"));
}

TEST(BytesBuiltinsTest, TranslateWithEmptyBytesReturnsEmptyBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, Bytes::empty());
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime.newByteArray());
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_EQ(result, Bytes::empty());
}

TEST(BytesBuiltinsTest, TranslateWithNonEmptySecondArgDeletesBytes) {
  Runtime runtime;
  HandleScope scope;
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Object self(&scope, runtime.newBytesWithAll(alabama));
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime.newBytesWithAll(abc));
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_TRUE(isBytesEqualsCStr(result, "Alm"));
}

TEST(BytesBuiltinsTest, TranslateWithTableTranslatesBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "table = bytes.maketrans(b'Aa', b'12')").isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  Object self(&scope, runtime.newBytesWithAll(alabama));
  Object table(&scope, moduleAt(&runtime, "__main__", "table"));
  Object del(&scope, Bytes::empty());
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_TRUE(isBytesEqualsCStr(result, "1l2b2m2"));
}

TEST(BytesBuiltinsTest, TranslateWithTableAndDeleteTranslatesAndDeletesBytes) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "table = bytes.maketrans(b'Aa', b'12')").isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Object self(&scope, runtime.newBytesWithAll(alabama));
  Object table(&scope, moduleAt(&runtime, "__main__", "table"));
  Object del(&scope, runtime.newBytesWithAll(abc));
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_TRUE(isBytesEqualsCStr(result, "1lm"));
}

TEST(BytesBuiltinsTest, TranslateDeletesAllBytes) {
  Runtime runtime;
  HandleScope scope;
  const byte alabama[] = {'b', 'a', 'c', 'a', 'a', 'c', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Object self(&scope, runtime.newBytesWithAll(alabama));
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime.newBytesWithAll(abc));
  Object result(&scope, runBuiltin(BytesBuiltins::translate, self, table, del));
  EXPECT_EQ(result, Bytes::empty());
}

}  // namespace python
