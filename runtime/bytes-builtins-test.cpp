#include "gtest/gtest.h"

#include "bytes-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BytesBuiltinsTest, CallDunderBytesCallsDunderBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self):
    return b'111'
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, callDunderBytes(thread, obj));
  EXPECT_TRUE(isBytesEqualsCStr(result, "111"));
}

TEST(BytesBuiltinsTest, CallDunderBytesWithNonBytesDunderBytesRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self):
    return 1
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, callDunderBytes(thread, obj));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "__bytes__ returned non-bytes"));
}

TEST(BytesBuiltinsTest, CallDunderBytesWithDunderBytesErrorRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self):
    raise ValueError("__bytes__() raised an error")
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, callDunderBytes(thread, obj));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "__bytes__() raised an error"));
}

TEST(BytesBuiltinsTest, CallDunderBytesWithoutDunderBytesReturnsNone) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  runFromCStr(&runtime, R"(
class Foo: pass
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, callDunderBytes(thread, obj));
  EXPECT_TRUE(result.isNoneType());
}

TEST(BytesBuiltinsTest, FromIterableWithListReturnsBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object num(&scope, SmallInt::fromWord('*'));
  runtime.listAdd(list, num);
  runtime.listAdd(list, num);
  runtime.listAdd(list, num);
  Object result(&scope, bytesFromIterable(thread, list));
  EXPECT_TRUE(isBytesEqualsCStr(result, "***"));
}

TEST(BytesBuiltinsTest, FromIterableWithTupleReturnsBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple.atPut(0, SmallInt::fromWord(42));
  tuple.atPut(1, SmallInt::fromWord(123));
  tuple.atPut(2, SmallInt::fromWord(0));
  Object result(&scope, bytesFromIterable(thread, tuple));
  EXPECT_TRUE(isBytesEqualsBytes(result, {42, 123, 0}));
}

TEST(BytesBuiltinsTest, FromIterableWithIterableReturnsBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Range range(&scope, runtime.newRange('a', 'j', 2));
  Object result(&scope, bytesFromIterable(thread, range));
  EXPECT_TRUE(isBytesEqualsCStr(result, "acegi"));
}

TEST(BytesBuiltinsTest, FromIterableReturnsBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  runFromCStr(&runtime, R"(
class Foo:
  def __iter__(self):
    return [97,98,99].__iter__()
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, bytesFromIterable(thread, obj));
  EXPECT_TRUE(isBytesEqualsBytes(result, {97, 98, 99}));
}

TEST(BytesBuiltinsTest, FromIterableWithNonIterableRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Int num(&scope, SmallInt::fromWord(0));
  Object result(&scope, bytesFromIterable(thread, num));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, FromIterableWithStrRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("hello"));
  Object result(&scope, bytesFromIterable(thread, str));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, FromTupleWithSizeReturnsBytesMatchingSize) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple.atPut(0, SmallInt::fromWord(42));
  tuple.atPut(1, SmallInt::fromWord(123));
  Object result(&scope, bytesFromTuple(thread, tuple, 2));
  EXPECT_TRUE(isBytesEqualsBytes(result, {42, 123}));
}

TEST(BytesBuiltinsTest, FromTupleWithNonIndexRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, runtime.newFloat(1));
  Object result(&scope, bytesFromTuple(thread, tuple, 1));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, FromTupleWithNegativeIntRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, SmallInt::fromWord(-1));
  Object result(&scope, bytesFromTuple(thread, tuple, 1));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(BytesBuiltinsTest, FromTupleWithNonByteRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, SmallInt::fromWord(256));
  Object result(&scope, bytesFromTuple(thread, tuple, 1));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
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
  EXPECT_TRUE(RawBool::cast(*eq).value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentLengthsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_FALSE(RawBool::cast(*eq).value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentContentsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq.isBool());
  EXPECT_FALSE(RawBool::cast(*eq).value());
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
  EXPECT_TRUE(RawBool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(RawBool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_FALSE(RawBool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_TRUE(RawBool::cast(*ge).value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge.isBool());
  EXPECT_FALSE(RawBool::cast(*ge).value());
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
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object index(&scope, SmallInt::fromWord(4));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(raised(*item, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGetItemWithLargeIntRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  uword idx[] = {1, 1};
  Object index(&scope, runtime.newIntWithDigits(View<uword>(idx, 2)));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(raised(*item, LayoutId::kIndexError));
}

TEST(BytesBuiltinsTest, DunderGetItemWithIntGreaterOrEqualLenRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object index(&scope, RawSmallInt::fromWord(4));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(raised(*item, LayoutId::kIndexError));
}

TEST(BytesBuiltinsTest,
     DunderGetItemWithNegativeIntGreaterThanLenRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object index(&scope, runtime.newInt(-4));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(raised(*item, LayoutId::kIndexError));
}

TEST(BytesBuiltinsTest, DunderGetItemWithNegativeIntIndexesFromEnd) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello";
  Object self(&scope, runtime.newBytesWithAll(View<byte>(hello, 5)));
  Object index(&scope, runtime.newInt(-5));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_EQ(*item, RawSmallInt::fromWord('h'));
}

TEST(BytesBuiltinsTest, DunderGetItemIndexesFromBeginning) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello";
  Object self(&scope, runtime.newBytesWithAll(View<byte>(hello, 5)));
  Object index(&scope, RawSmallInt::fromWord(0));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_EQ(*item, RawSmallInt::fromWord('h'));
}

TEST(BytesBuiltinsTest, DunderGetItemWithSliceReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello world";
  Bytes self(&scope, runtime.newBytesWithAll(View<byte>(hello, 11)));
  Slice index(&scope, runtime.newSlice());
  index.setStart(SmallInt::fromWord(0));
  index.setStop(SmallInt::fromWord(3));
  index.setStep(SmallInt::fromWord(1));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(isBytesEqualsCStr(item, "hel"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithSliceStepReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello world";
  Bytes self(&scope, runtime.newBytesWithAll(View<byte>(hello, 11)));
  Slice index(&scope, runtime.newSlice());
  index.setStart(SmallInt::fromWord(1));
  index.setStop(SmallInt::fromWord(6));
  index.setStep(SmallInt::fromWord(2));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(isBytesEqualsCStr(item, "el "));
}

TEST(BytesBuiltinsTest, DunderGetItemWithNonIndexOtherRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newFloat(1.5));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, other));
  EXPECT_TRUE(raised(*item, LayoutId::kTypeError));
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
  EXPECT_FALSE(RawBool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_TRUE(RawBool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(RawBool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_TRUE(RawBool::cast(*gt).value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt.isBool());
  EXPECT_FALSE(RawBool::cast(*gt).value());
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
  EXPECT_TRUE(RawBool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_FALSE(RawBool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(RawBool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_FALSE(RawBool::cast(*le).value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le.isBool());
  EXPECT_TRUE(RawBool::cast(*le).value());
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
  EXPECT_FALSE(RawBool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(RawBool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_TRUE(RawBool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_FALSE(RawBool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt.isBool());
  EXPECT_TRUE(RawBool::cast(*lt).value());
}

TEST(BytesBuiltinsTest, DunderMulWithNonBytesRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object count(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kTypeError,
                            "'__mul__' requires a 'bytes' instance"));
}

TEST(BytesBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(0, 0));
  Object count(&scope, runtime.newList());
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kTypeError,
                            "object cannot be interpreted as an integer"));
}

TEST(BytesBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return 2
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "aa"));
}

TEST(BytesBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int"));
}

TEST(BytesBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST(BytesBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(0, 0));
  Object count(&scope, runtime.newIntWithDigits({1, 1}));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BytesBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "cannot fit count into an index-sized integer"));
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
  Object self(&scope, runtime.newBytes(0, 0));
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
  Object self(&scope, runtime.newBytesWithAll({'a', 'b'}));
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(BytesBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ab"));
}

TEST(BytesBuiltinsTest, DunderMulReturnsRepeatedBytes) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytesWithAll({'a', 'b'}));
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
  EXPECT_FALSE(RawBool::cast(*ne).value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentLengthsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_TRUE(RawBool::cast(*ne).value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentContentsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne.isBool());
  EXPECT_TRUE(RawBool::cast(*ne).value());
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
  runFromCStr(&runtime, "obj = bytes()");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  EXPECT_TRUE(isBytesEqualsBytes(obj, {}));
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
                            "__bytes__ returned non-bytes"));
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
  runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self): return b'foo'
result = bytes(Foo())
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "foo"));
}

TEST(BytesBuiltinsTest, DunderNewWithIntegerSourceReturnsNullFilledBytes) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = bytes(10)");
  Bytes result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsBytes(result, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
}

TEST(BytesBuiltinsTest, DunderNewWithIterableReturnsNewBytes) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = bytes([6, 28])");
  Bytes result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsBytes(result, {6, 28}));
}

TEST(BytesBuiltinsTest, DunderReprWithNonBytesRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(raisedWithStr(*repr, LayoutId::kTypeError,
                            "'__repr__' requires a 'bytes' object"));
}

TEST(BytesBuiltinsTest, DunderReprWithEmptyBytesReturnsEmptyRepr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(0, 0));
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
  byte bytes[3] = {'_', '"', '_'};
  View<byte> view(bytes, sizeof(bytes));
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'_"_')"));
}

TEST(BytesBuiltinsTest, DunderReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  Runtime runtime;
  HandleScope scope;
  byte bytes[3] = {'_', '\'', '_'};
  View<byte> view(bytes, sizeof(bytes));
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b"_'_")"));
}

TEST(BytesBuiltinsTest, DunderReprWithBothQuotesUsesSingleQuoteDelimiters) {
  Runtime runtime;
  HandleScope scope;
  byte bytes[5] = {'_', '"', '_', '\'', '_'};
  View<byte> view(bytes, sizeof(bytes));
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'_"_\'_')"));
}

TEST(BytesBuiltinsTest, DunderReprWithSpeciaBytesUsesEscapeSequences) {
  Runtime runtime;
  HandleScope scope;
  byte bytes[4] = {'\\', '\t', '\n', '\r'};
  View<byte> view(bytes, sizeof(bytes));
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'\\\t\n\r')"));
}

TEST(BytesBuiltinsTest, DunderReprWithSmallAndLargeBytesUsesHex) {
  Runtime runtime;
  HandleScope scope;
  byte bytes[4] = {0, 0x1f, 0x80, 0xff};
  View<byte> view(bytes, sizeof(bytes));
  Object self(&scope, runtime.newBytesWithAll(view));
  Object repr(&scope, runBuiltin(BytesBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(b'\x00\x1f\x80\xff')"));
}

TEST(BytesBuiltinsTest, DunderRmulCallsDunderMul) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = 3 * b'1'");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "111"));
}

TEST(BytesBuiltinsTest, DecodeWithUnknownCodecReturnsNotImplemented) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "b'hello'.decode('unknown')"),
                            LayoutId::kNotImplementedError,
                            "Non-fastpass codecs are unimplemented"));
}

TEST(BytesBuiltinsTest, HexWithNonBytesRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytes.hex(1)"),
                            LayoutId::kTypeError,
                            "'hex' requires a 'bytes' object"));
}

TEST(BytesBuiltinsTest, HexWithEmptyBytesReturnsEmptyString) {
  Runtime runtime;
  HandleScope scope;
  Bytes self(&scope, runtime.newBytes(0, 0));
  Object result(&scope, runBuiltin(BytesBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST(BytesBuiltinsTest, HexWithNonEmptyBytesReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Bytes self(&scope, runtime.newBytesWithAll({0x12, 0x34, 0xfe, 0x5b}));
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
  Bytes self(&scope, runtime.newBytes(0, 0));
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
  runtime.listAdd(iter, value);
  runtime.listAdd(iter, value);
  runtime.listAdd(iter, value);
  Object result(&scope, runBuiltin(BytesBuiltins::join, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "* * *"));
}

TEST(BytesBuiltinsTest, JoinWithMistypedIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "b' '.join([1])"), LayoutId::kTypeError,
      "sequence item 0: expected a bytes-like object, smallint found"));
}

TEST(BytesBuiltinsTest, JoinWithIterableReturnsBytes) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __iter__(self):
    return [b'ab', b'c', b'def'].__iter__()
result = b' '.join(Foo())
)");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ab c def"));
}

}  // namespace python
