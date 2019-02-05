#include "gtest/gtest.h"

#include "bytes-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BytesBuiltinsTest, AsBytesCallsDunderBytes) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self):
    return b'111'
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, asBytes(Thread::currentThread(), obj));
  ASSERT_TRUE(result->isBytes());
  Bytes expected(&scope, runtime.newBytes(3, '1'));
  EXPECT_EQ(RawBytes::cast(*result).compare(*expected), 0);
}

TEST(BytesBuiltinsTest, AsBytesWithNonBytesDunderBytesRaises) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self):
    return 1
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, asBytes(Thread::currentThread(), obj));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "__bytes__ returned non-bytes"));
}

TEST(BytesBuiltinsTest, AsBytesWithDunderBytesErrorRaises) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
  def __bytes__(self):
    raise ValueError("__bytes__() raised an error")
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, asBytes(Thread::currentThread(), obj));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "__bytes__() raised an error"));
}

TEST(BytesBuiltinsTest, AsBytesWithoutDunderBytesIterates) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
  def __iter__(self):
    return [97,98,99].__iter__()
obj = Foo()
)");
  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Object result(&scope, asBytes(Thread::currentThread(), obj));
  ASSERT_TRUE(result->isBytes());
  Bytes expected(&scope, runtime.newBytesWithAll({'a', 'b', 'c'}));
  EXPECT_EQ(RawBytes::cast(*result).compare(*expected), 0);
}

TEST(BytesBuiltinsTest, FromIterableWithListReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Object num(&scope, SmallInt::fromWord(42));
  runtime.listAdd(list, num);
  runtime.listAdd(list, num);
  runtime.listAdd(list, num);
  Object result(&scope, bytesFromIterable(Thread::currentThread(), list));
  ASSERT_TRUE(result->isBytes());
  Bytes expected(&scope, runtime.newBytesWithAll({42, 42, 42}));
  EXPECT_EQ(Bytes::cast(*result).compare(*expected), 0);
}

TEST(BytesBuiltinsTest, FromIterableWithTupleReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple->atPut(0, SmallInt::fromWord(42));
  tuple->atPut(1, SmallInt::fromWord(123));
  tuple->atPut(2, SmallInt::fromWord(0));
  Object result(&scope, bytesFromIterable(Thread::currentThread(), tuple));
  ASSERT_TRUE(result->isBytes());
  Bytes expected(&scope, runtime.newBytesWithAll({42, 123, 0}));
  EXPECT_EQ(Bytes::cast(*result).compare(*expected), 0);
}

TEST(BytesBuiltinsTest, FromIterableWithIterableReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  Range range(&scope, runtime.newRange('a', 'j', 2));
  Object result(&scope, bytesFromIterable(Thread::currentThread(), range));
  ASSERT_TRUE(result->isBytes());
  Bytes expected(&scope, runtime.newBytesWithAll({'a', 'c', 'e', 'g', 'i'}));
  EXPECT_EQ(Bytes::cast(*result).compare(*expected), 0);
}

TEST(BytesBuiltinsTest, FromIterableWithNonIterableRaises) {
  Runtime runtime;
  HandleScope scope;
  Int num(&scope, SmallInt::fromWord(0));
  Object result(&scope, bytesFromIterable(Thread::currentThread(), num));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, FromIterableWithStrRaises) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("hello"));
  Object result(&scope, bytesFromIterable(Thread::currentThread(), str));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, FromTupleWithSizeReturnsBytesMatchingSize) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple->atPut(0, SmallInt::fromWord(42));
  tuple->atPut(1, SmallInt::fromWord(123));
  Object result(&scope, bytesFromTuple(Thread::currentThread(), tuple, 2));
  ASSERT_TRUE(result->isBytes());
  Bytes bytes(&scope, *result);
  EXPECT_EQ(bytes->length(), 2);
  EXPECT_EQ(bytes->byteAt(0), 42);
  EXPECT_EQ(bytes->byteAt(1), 123);
}

TEST(BytesBuiltinsTest, FromTupleWithNonIndexRaises) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple->atPut(0, runtime.newFloat(1));
  Object result(&scope, bytesFromTuple(Thread::currentThread(), tuple, 1));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, FromTupleWithNegativeIntRaises) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple->atPut(0, SmallInt::fromWord(-1));
  Object result(&scope, bytesFromTuple(Thread::currentThread(), tuple, 1));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(BytesBuiltinsTest, FromTupleWithNonByteRaises) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple->atPut(0, SmallInt::fromWord(256));
  Object result(&scope, bytesFromTuple(Thread::currentThread(), tuple, 1));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(BytesBuiltinsTest, DunderAddWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__add__(b'')"), LayoutId::kTypeError,
      "TypeError: '__add__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderAddWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__add__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__add__' takes max 2 positional arguments but 3 given"));
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
  ASSERT_TRUE(sum->isBytes());
  Bytes result(&scope, *sum);
  EXPECT_EQ(result->length(), 3);
  EXPECT_EQ(result->byteAt(0), '1');
  EXPECT_EQ(result->byteAt(1), '2');
  EXPECT_EQ(result->byteAt(2), '2');
}

TEST(BytesBuiltinsTest, DunderEqWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__eq__(b'')"), LayoutId::kTypeError,
      "TypeError: '__eq__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderEqWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__eq__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__eq__' takes max 2 positional arguments but 3 given"));
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
  EXPECT_TRUE(eq->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderEqWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq->isBool());
  EXPECT_TRUE(RawBool::cast(*eq)->value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentLengthsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq->isBool());
  EXPECT_FALSE(RawBool::cast(*eq)->value());
}

TEST(BytesBuiltinsTest, DunderEqWithDifferentContentsReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object eq(&scope, runBuiltin(BytesBuiltins::dunderEq, self, other));
  ASSERT_TRUE(eq->isBool());
  EXPECT_FALSE(RawBool::cast(*eq)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__ge__(b'')"), LayoutId::kTypeError,
      "TypeError: '__ge__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderGeWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__ge__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__ge__' takes max 2 positional arguments but 3 given"));
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
  ASSERT_TRUE(ge->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderGeWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_TRUE(RawBool::cast(*ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_TRUE(RawBool::cast(*ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_FALSE(RawBool::cast(*ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_TRUE(RawBool::cast(*ge)->value());
}

TEST(BytesBuiltinsTest, DunderGeWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ge(&scope, runBuiltin(BytesBuiltins::dunderGe, self, other));
  ASSERT_TRUE(ge->isBool());
  EXPECT_FALSE(RawBool::cast(*ge)->value());
}

TEST(BytesBuiltinsTest, DunderGetItemWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__getitem__(b'')"), LayoutId::kTypeError,
      "TypeError: '__getitem__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderGetItemWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__getitem__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__getitem__' takes max 2 positional arguments but 3 given"));
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
  index->setStart(SmallInt::fromWord(0));
  index->setStop(SmallInt::fromWord(3));
  index->setStep(SmallInt::fromWord(1));
  Object item(&scope, runBuiltin(BytesBuiltins::dunderGetItem, self, index));
  ASSERT_TRUE(item->isBytes());
  Bytes result(&scope, *item);
  EXPECT_EQ(result->length(), 3);
  EXPECT_EQ(result->byteAt(0), 'h');
  EXPECT_EQ(result->byteAt(1), 'e');
  EXPECT_EQ(result->byteAt(2), 'l');
}

TEST(BytesBuiltinsTest, DunderGetItemWithSliceStepReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  byte hello[] = "hello world";
  Bytes self(&scope, runtime.newBytesWithAll(View<byte>(hello, 11)));
  Slice index(&scope, runtime.newSlice());
  index->setStart(SmallInt::fromWord(1));
  index->setStop(SmallInt::fromWord(6));
  index->setStep(SmallInt::fromWord(2));
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
  EXPECT_TRUE(raised(*item, LayoutId::kTypeError));
}

TEST(BytesBuiltinsTest, DunderGtWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__gt__(b'')"), LayoutId::kTypeError,
      "TypeError: '__gt__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderGtWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__gt__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__gt__' takes max 2 positional arguments but 3 given"));
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
  ASSERT_TRUE(gt->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderGtWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_FALSE(RawBool::cast(*gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithShorterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_TRUE(RawBool::cast(*gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithLongerOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_FALSE(RawBool::cast(*gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyEarlierOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_TRUE(RawBool::cast(*gt)->value());
}

TEST(BytesBuiltinsTest, DunderGtWithLexicographicallyLaterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object gt(&scope, runBuiltin(BytesBuiltins::dunderGt, self, other));
  ASSERT_TRUE(gt->isBool());
  EXPECT_FALSE(RawBool::cast(*gt)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__le__(b'')"), LayoutId::kTypeError,
      "TypeError: '__le__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderLeWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__le__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__le__' takes max 2 positional arguments but 3 given"));
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
  ASSERT_TRUE(le->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderLeWithEqualBytesReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_TRUE(RawBool::cast(*le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_FALSE(RawBool::cast(*le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_TRUE(RawBool::cast(*le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_FALSE(RawBool::cast(*le)->value());
}

TEST(BytesBuiltinsTest, DunderLeWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object le(&scope, runBuiltin(BytesBuiltins::dunderLe, self, other));
  ASSERT_TRUE(le->isBool());
  EXPECT_TRUE(RawBool::cast(*le)->value());
}

TEST(BytesBuiltinsTest, DunderLenWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__len__()"), LayoutId::kTypeError,
      "TypeError: '__len__' takes 1 arguments but 0 given"));
}

TEST(BytesBuiltinsTest, DunderLenWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__len__(b'', b'')"), LayoutId::kTypeError,
      "TypeError: '__len__' takes max 1 positional arguments but 2 given"));
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

TEST(BytesBuiltinsTest, DunderLtWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__lt__(b'')"), LayoutId::kTypeError,
      "TypeError: '__lt__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderLtWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__lt__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__lt__' takes max 2 positional arguments but 3 given"));
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
  ASSERT_TRUE(lt->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderLtWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_FALSE(RawBool::cast(*lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithShorterOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(2, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_FALSE(RawBool::cast(*lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_TRUE(RawBool::cast(*lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyEarlierOtherReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'b'));
  Object other(&scope, runtime.newBytes(3, 'a'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_FALSE(RawBool::cast(*lt)->value());
}

TEST(BytesBuiltinsTest, DunderLtWithLexicographicallyLaterOtherReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object lt(&scope, runBuiltin(BytesBuiltins::dunderLt, self, other));
  ASSERT_TRUE(lt->isBool());
  EXPECT_TRUE(RawBool::cast(*lt)->value());
}

TEST(BytesBuiltinsTest, DunderNeWithTooFewArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__ne__(b'')"), LayoutId::kTypeError,
      "TypeError: '__ne__' takes 2 arguments but 1 given"));
}

TEST(BytesBuiltinsTest, DunderNeWithTooManyArgsRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytes.__ne__(b'', b'', b'')"),
      LayoutId::kTypeError,
      "TypeError: '__ne__' takes max 2 positional arguments but 3 given"));
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
  EXPECT_TRUE(ne->isNotImplemented());
}

TEST(BytesBuiltinsTest, DunderNeWithEqualBytesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(5, 'a'));
  Object other(&scope, runtime.newBytes(5, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne->isBool());
  EXPECT_FALSE(RawBool::cast(*ne)->value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentLengthsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object other(&scope, runtime.newBytes(4, 'a'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne->isBool());
  EXPECT_TRUE(RawBool::cast(*ne)->value());
}

TEST(BytesBuiltinsTest, DunderNeWithDifferentContentsReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(3, 'a'));
  Object other(&scope, runtime.newBytes(3, 'b'));
  Object ne(&scope, runBuiltin(BytesBuiltins::dunderNe, self, other));
  ASSERT_TRUE(ne->isBool());
  EXPECT_TRUE(RawBool::cast(*ne)->value());
}

}  // namespace python
