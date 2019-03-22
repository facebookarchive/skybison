#include "gtest/gtest.h"

#include "bytearray-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ByteArrayBuiltinsTest, Add) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, array, 0);
  byteArrayAdd(thread, &runtime, array, 1);
  byteArrayAdd(thread, &runtime, array, 2);
  EXPECT_GE(array.capacity(), 3);
  EXPECT_EQ(array.numItems(), 3);
  EXPECT_EQ(array.byteAt(0), 0);
  EXPECT_EQ(array.byteAt(1), 1);
  EXPECT_EQ(array.byteAt(2), 2);
}

TEST(ByteArrayBuiltinsTest, AsBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  Bytes bytes(&scope, byteArrayAsBytes(thread, &runtime, array));
  EXPECT_TRUE(isBytesEqualsBytes(bytes, {}));

  array.setBytes(runtime.newBytes(10, 0));
  array.setNumItems(3);
  bytes = byteArrayAsBytes(thread, &runtime, array);
  EXPECT_TRUE(isBytesEqualsBytes(bytes, {0, 0, 0}));
}

TEST(ByteArrayBuiltinsTest, DunderAddWithNonByteArraySelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray.__add__(b'', b'')"),
      LayoutId::kTypeError, "'__add__' requires a 'bytearray' object"));
}

TEST(ByteArrayBuiltinsTest, DunderAddWithNonBytesLikeRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray(b'') + None"), LayoutId::kTypeError,
      "can only concatenate bytearray or bytes to bytearray"));
}

TEST(ByteArrayBuiltinsTest, DunderAddWithByteArrayOtherReturnsNewByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  ByteArray other(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, other, {'1', '2', '3'});
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, ""));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "123"));
}

TEST(ByteArrayBuiltinsTest, DunderAddWithBytesOtherReturnsNewByteArray) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  Bytes other(&scope, runtime.newBytes(4, '1'));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, ""));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "1111"));
}

TEST(ByteArrayBuiltinsTest, DunderAddReturnsConcatenatedByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'f', 'o', 'o'});
  Bytes other(&scope, runtime.newBytes(1, 'd'));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderAdd, self, other));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "foo"));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "food"));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemWithNonBytesSelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray.__getitem__(1, 2)"),
      LayoutId::kTypeError, "'__getitem__' requires a 'bytearray' object"));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemWithNonIndexOtherRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray(5)[1.5]"), LayoutId::kTypeError,
      "bytearray indices must either be slice or provide '__index__'"));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemWithLargeIntRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray(5)[2 ** 63]"),
                            LayoutId::kIndexError,
                            "cannot fit index into an index-sized integer"));
}

TEST(ByteArrayBuiltinsTest,
     DunderGetItemWithIntGreaterOrEqualLenRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray(5)[5]"),
                            LayoutId::kIndexError, "index out of range"));
}

TEST(ByteArrayBuiltinsTest,
     DunderGetItemWithNegativeIntGreaterThanLenRaisesIndexError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray(5)[-6]"),
                            LayoutId::kIndexError, "index out of range"));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemWithNegativeIntIndexesFromEnd) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'h', 'e', 'l', 'l', 'o'});
  Object index(&scope, runtime.newInt(-5));
  Object item(&scope,
              runBuiltin(ByteArrayBuiltins::dunderGetItem, self, index));
  EXPECT_EQ(*item, SmallInt::fromWord('h'));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemIndexesFromBeginning) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'h', 'e', 'l', 'l', 'o'});
  Object index(&scope, SmallInt::fromWord(0));
  Object item(&scope,
              runBuiltin(ByteArrayBuiltins::dunderGetItem, self, index));
  EXPECT_EQ(*item, SmallInt::fromWord('h'));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemWithSliceReturnsByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'h', 'e', 'l', 'l', 'o'});
  Slice index(&scope, runtime.newSlice());
  index.setStop(SmallInt::fromWord(3));
  Object result(&scope,
                runBuiltin(ByteArrayBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "hel"));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemWithSliceStepReturnsByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(
      thread, self, {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'});
  Slice index(&scope, runtime.newSlice());
  index.setStart(SmallInt::fromWord(8));
  index.setStop(SmallInt::fromWord(3));
  index.setStep(SmallInt::fromWord(-2));
  Object result(&scope,
                runBuiltin(ByteArrayBuiltins::dunderGetItem, self, index));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "rwo"));
}

TEST(ByteArrayBuiltinsTest, DunderIaddWithNonByteArraySelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray.__iadd__(b'', b'')"),
      LayoutId::kTypeError, "'__iadd__' requires a 'bytearray' object"));
}

TEST(ByteArrayBuiltinsTest, DunderIaddWithNonBytesLikeRaisesTypeError) {
  Runtime runtime;
  const char* test = R"(
array = bytearray(b'')
array += None
)";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, test), LayoutId::kTypeError,
                    "can only concatenate bytearray or bytes to bytearray"));
}

TEST(ByteArrayBuiltinsTest, DunderIaddWithByteArrayOtherConcatenatesToSelf) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  ByteArray other(&scope, runtime.newByteArray());
  View<byte> bytes{'1', '2', '3'};
  runtime.byteArrayExtend(thread, other, bytes);
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderIadd, self, other));
  EXPECT_TRUE(isByteArrayEqualsBytes(self, bytes));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST(ByteArrayBuiltinsTest, DunderIaddWithBytesOtherConcatenatesToSelf) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  View<byte> bytes{'1', '2', '3'};
  Bytes other(&scope, runtime.newBytesWithAll(bytes));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderIadd, self, other));
  EXPECT_TRUE(isByteArrayEqualsBytes(self, bytes));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, SmallInt::fromWord(0));
  Object count(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ByteArrayBuiltins::dunderImul, self, count),
      LayoutId::kTypeError, "'__imul__' requires a 'bytearray' instance"));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithNonIntRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  Object count(&scope, runtime.newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ByteArrayBuiltins::dunderImul, self, count),
      LayoutId::kTypeError, "object cannot be interpreted as an integer"));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithDunderIndexReturnsRepeatedBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, self, 'a');
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return 2
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderImul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "aa"));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithBadDunderIndexRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(ByteArrayBuiltins::dunderImul, self, count),
                    LayoutId::kTypeError, "__index__ returned non-int"));
}

TEST(ByteArrayBuiltinsTest, DunderImulPropagatesDunderIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(ByteArrayBuiltins::dunderImul, self, count),
                    LayoutId::kArithmeticError, "called __index__"));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithLargeIntRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  Object count(&scope, runtime.newIntWithDigits({1, 1}));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(ByteArrayBuiltins::dunderImul, self, count),
                    LayoutId::kOverflowError,
                    "cannot fit count into an index-sized integer"));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithOverflowRaisesMemoryError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'a', 'b', 'c'});
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raised(runBuiltin(ByteArrayBuiltins::dunderImul, self, count),
                     LayoutId::kMemoryError));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithEmptyByteArrayReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  Object count(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderImul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithNegativeReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  self.setBytes(runtime.newBytes(4, 'a'));
  self.setNumItems(4);
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderImul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithZeroReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  self.setBytes(runtime.newBytes(4, 'a'));
  self.setNumItems(4);
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderImul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST(ByteArrayBuiltinsTest, DunderImulWithOneReturnsSameByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  View<byte> bytes{'a', 'b'};
  runtime.byteArrayExtend(thread, self, bytes);
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderImul, self, count));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST(ByteArrayBuiltinsTest, DunderImulReturnsRepeatedByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'a', 'b'});
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderImul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ababab"));
}

TEST(ByteArrayBuiltinsTest, DunderInitNoArgsClearsArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  runFromCStr(&runtime, R"(
array = bytearray(b'123')
result = array.__init__()
)");
  Object array(&scope, moduleAt(&runtime, "__main__", "array"));
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(array, ""));
}

TEST(ByteArrayBuiltinsTest, DunderInitWithNonByteArraySelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.__init__(3)"),
                            LayoutId::kTypeError,
                            "'__init__' requires a 'bytearray' object"));
}

TEST(ByteArrayBuiltinsTest,
     DunderInitWithoutSourceWithEncodingRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray(encoding='utf-8')"),
      LayoutId::kTypeError, "encoding or errors without sequence argument"));
}

TEST(ByteArrayBuiltinsTest, DunderInitWithoutSourceWithErrorsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray(errors='strict')"),
                            LayoutId::kTypeError,
                            "encoding or errors without sequence argument"));
}

TEST(ByteArrayBuiltinsTest,
     DunderInitWithStringWithoutEncodingRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(bytearray(""))"),
                            LayoutId::kTypeError,
                            "string argument without an encoding"));
}

TEST(ByteArrayBuiltinsTest,
     DunderInitWithNonStrSourceWithEncodingRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray(3, encoding='utf-8')"),
      LayoutId::kTypeError, "encoding or errors without a string argument"));
}

TEST(ByteArrayBuiltinsTest,
     DunderInitWithNonStrSourceWithErrorsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray(3, errors='strict')"),
      LayoutId::kTypeError, "encoding or errors without a string argument"));
}

TEST(ByteArrayBuiltinsTest,
     DunderInitWithSmallIntReturnsByteArrayWithNullBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  Int source(&scope, runtime.newInt(3));
  Object unbound(&scope, runtime.unboundValue());
  Object init(&scope, runBuiltin(ByteArrayBuiltins::dunderInit, self, source,
                                 unbound, unbound));
  ASSERT_TRUE(init.isNoneType());
  EXPECT_TRUE(isByteArrayEqualsBytes(self, {0, 0, 0}));
}

TEST(ByteArrayBuiltinsTest, DunderInitWithLargeIntRaisesOverflowError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray(2 ** 63)"),
                            LayoutId::kOverflowError,
                            "cannot fit count into an index-sized integer"));
}

TEST(ByteArrayBuiltinsTest, DunderInitWithNegativeIntRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray(-1)"),
                            LayoutId::kValueError, "negative count"));
}

TEST(ByteArrayBuiltinsTest, DunderInitWithBytesCopiesBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  View<byte> bytes{'f', 'o', 'o', 'b', 'a', 'r'};
  Bytes source(&scope, runtime.newBytesWithAll(bytes));
  Object unbound(&scope, runtime.unboundValue());
  Object init(&scope, runBuiltin(ByteArrayBuiltins::dunderInit, self, source,
                                 unbound, unbound));
  ASSERT_TRUE(init.isNoneType());
  EXPECT_TRUE(isByteArrayEqualsBytes(self, bytes));
}

TEST(ByteArrayBuiltinsTest, DunderInitWithByteArrayCopiesBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  ByteArray source(&scope, runtime.newByteArray());
  View<byte> bytes{'f', 'o', 'o', 'b', 'a', 'r'};
  runtime.byteArrayExtend(thread, source, bytes);
  Object unbound(&scope, runtime.unboundValue());
  Object init(&scope, runBuiltin(ByteArrayBuiltins::dunderInit, self, source,
                                 unbound, unbound));
  ASSERT_TRUE(init.isNoneType());
  EXPECT_TRUE(isByteArrayEqualsBytes(self, bytes));
  EXPECT_TRUE(isByteArrayEqualsBytes(source, bytes));
}

TEST(ByteArrayBuiltinsTest, DunderInitWithIterableCopiesBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  List source(&scope, runtime.newList());
  Object one(&scope, runtime.newInt(1));
  Object two(&scope, runtime.newInt(2));
  Object six(&scope, runtime.newInt(6));
  runtime.listAdd(source, one);
  runtime.listAdd(source, two);
  runtime.listAdd(source, six);
  Object unbound(&scope, runtime.unboundValue());
  Object init(&scope, runBuiltin(ByteArrayBuiltins::dunderInit, self, source,
                                 unbound, unbound));
  ASSERT_TRUE(init.isNoneType());
  EXPECT_TRUE(isByteArrayEqualsBytes(self, {1, 2, 6}));
}

TEST(ByteArrayBuiltinsTest, DunderLenWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.__len__(b'')"),
                            LayoutId::kTypeError,
                            "'__len__' requires a 'bytearray' instance"));
}

TEST(ByteArrayBuiltinsTest, DunderLenWithEmptyByteArrayReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderLen, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(ByteArrayBuiltinsTest, DunderLenWithNonEmptyByteArrayReturnsPositive) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {1, 2, 3, 4, 5});
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderLen, self));
  EXPECT_TRUE(isIntEqualsWord(*result, 5));

  runtime.byteArrayExtend(thread, self, {6, 7});
  result = runBuiltin(ByteArrayBuiltins::dunderLen, self);
  EXPECT_TRUE(isIntEqualsWord(*result, 7));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(0, 0));
  Object count(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ByteArrayBuiltins::dunderMul, self, count),
      LayoutId::kTypeError, "'__mul__' requires a 'bytearray' instance"));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  Object count(&scope, runtime.newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ByteArrayBuiltins::dunderMul, self, count),
      LayoutId::kTypeError, "object cannot be interpreted as an integer"));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedBytes) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, self, 'a');
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return 2
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "aa"));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(ByteArrayBuiltins::dunderMul, self, count),
                    LayoutId::kTypeError, "__index__ returned non-int"));
}

TEST(ByteArrayBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)");
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(ByteArrayBuiltins::dunderMul, self, count),
                    LayoutId::kArithmeticError, "called __index__"));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  Object count(&scope, runtime.newIntWithDigits({1, 1}));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(ByteArrayBuiltins::dunderMul, self, count),
                    LayoutId::kOverflowError,
                    "cannot fit count into an index-sized integer"));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithOverflowRaisesMemoryError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'a', 'b', 'c'});
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raised(runBuiltin(ByteArrayBuiltins::dunderMul, self, count),
                     LayoutId::kMemoryError));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithEmptyByteArrayReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  Object count(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithNegativeReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  self.setBytes(runtime.newBytes(4, 'a'));
  self.setNumItems(4);
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithZeroReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  self.setBytes(runtime.newBytes(4, 'a'));
  self.setNumItems(4);
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST(ByteArrayBuiltinsTest, DunderMulWithOneReturnsSameByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  View<byte> bytes{'a', 'b'};
  runtime.byteArrayExtend(thread, self, bytes);
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isByteArrayEqualsBytes(result, bytes));
}

TEST(ByteArrayBuiltinsTest, DunderMulReturnsRepeatedByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'a', 'b'});
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ababab"));
}

TEST(ByteArrayBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.__new__(3)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST(ByteArrayBuiltinsTest, DunderNewWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.__new__(int)"),
                            LayoutId::kTypeError,
                            "not a subtype of bytearray"));
}

TEST(ByteArrayBuiltinsTest, DunderNewReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  Type cls(&scope, runtime.typeAt(LayoutId::kByteArray));
  Object self(&scope, runBuiltin(ByteArrayBuiltins::dunderNew, cls));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, ""));
}

TEST(ByteArrayBuiltinsTest, NewByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  runFromCStr(&runtime, "obj = bytearray(b'Hello world!')");
  ByteArray self(&scope, moduleAt(&runtime, "__main__", "obj"));
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "Hello world!"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(0, 0));
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(raisedWithStr(*repr, LayoutId::kTypeError,
                            "'__repr__' requires a 'bytearray' object"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithOverlongByteArrayRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  self.setNumItems(kMaxWord / 4);
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(raisedWithStr(*repr, LayoutId::kOverflowError,
                            "bytearray object is too large to make repr"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithEmptyByteArrayReturnsEmptyRepr) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "bytearray(b'')"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithSimpleByteArrayReturnsRepr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'f', 'o', 'o'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "bytearray(b'foo')"));
}

TEST(ByteArrayBuiltinsTest,
     DunderReprWithDoubleQuoteUsesSingleQuoteDelimiters) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'_', '"', '_'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_'))"));
}

TEST(ByteArrayBuiltinsTest,
     DunderReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'_', '\'', '_'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b"_'_"))"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithBothQuotesUsesSingleQuoteDelimiters) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'_', '"', '_', '\'', '_'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_\'_'))"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithSpeciaBytesUsesEscapeSequences) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'\\', '\t', '\n', '\r'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\\\t\n\r'))"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithSmallAndLargeBytesUsesHex) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {0, 0x1f, 0x80, 0xff});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\x00\x1f\x80\xff'))"));
}

TEST(ByteArrayBuiltinsTest, DunderRmulCallsDunderMul) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = 3 * bytearray(b'123')");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "123123123"));
}

TEST(ByteArrayBuiltinsTest, HexWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.hex(1)"),
                            LayoutId::kTypeError,
                            "'hex' requires a 'bytearray' object"));
}

TEST(ByteArrayBuiltinsTest, HexWithEmptyByteArrayReturnsEmptyString) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newByteArray());
  Object result(&scope, runBuiltin(ByteArrayBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST(ByteArrayBuiltinsTest, HexWithNonEmptyBytesReturnsString) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {0x60, 0xe, 0x18, 0x21});
  Object result(&scope, runBuiltin(ByteArrayBuiltins::hex, self));
  EXPECT_TRUE(isStrEqualsCStr(*result, "600e1821"));
}

TEST(ByteArrayBuiltinsTest, JoinWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.join(b'', [])"),
                            LayoutId::kTypeError,
                            "'join' requires a 'bytearray' object"));
}

TEST(ByteArrayBuiltinsTest, JoinWithNonIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray(b'').join(0)"),
                            LayoutId::kTypeError, "object is not iterable"));
}

TEST(ByteArrayBuiltinsTest, JoinWithEmptyIterableReturnsEmptyByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, self, 'a');
  Object iter(&scope, runtime.newTuple(0));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::join, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST(ByteArrayBuiltinsTest, JoinWithEmptySeparatorReturnsByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  Tuple iter(&scope, runtime.newTuple(3));
  iter.atPut(0, runtime.newBytes(1, 'A'));
  iter.atPut(1, runtime.newBytes(2, 'B'));
  iter.atPut(2, runtime.newBytes(1, 'A'));
  Object result(&scope, runBuiltin(ByteArrayBuiltins::join, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ABBA"));
}

TEST(ByteArrayBuiltinsTest, JoinWithNonEmptyReturnsByteArray) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, self, ' ');
  List iter(&scope, runtime.newList());
  Bytes value(&scope, runtime.newBytes(1, '*'));
  runtime.listAdd(iter, value);
  runtime.listAdd(iter, value);
  runtime.listAdd(iter, value);
  Object result(&scope, runBuiltin(ByteArrayBuiltins::join, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "* * *"));
}

TEST(ByteArrayBuiltinsTest, JoinWithMistypedIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "bytearray(b' ').join([1])"), LayoutId::kTypeError,
      "sequence item 0: expected a bytes-like object, smallint found"));
}

TEST(ByteArrayBuiltinsTest, JoinWithIterableReturnsByteArray) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __iter__(self):
    return [b'ab', b'c', b'def'].__iter__()
result = bytearray(b' ').join(Foo())
)");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ab c def"));
}

}  // namespace python
