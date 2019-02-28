#include "gtest/gtest.h"

#include "bytearray-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ByteArrayBuiltinsTest, Add) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  ByteArray other(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, other, {'1', '2', '3'});
  ByteArray result(&scope,
                   runBuiltin(ByteArrayBuiltins::dunderAdd, self, other));
  EXPECT_EQ(self.numItems(), 0);
  EXPECT_EQ(result.numItems(), 3);
}

TEST(ByteArrayBuiltinsTest, DunderAddWithBytesOtherReturnsNewByteArray) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  Bytes other(&scope, runtime.newBytes(2, '1'));
  ByteArray result(&scope,
                   runBuiltin(ByteArrayBuiltins::dunderAdd, self, other));
  EXPECT_EQ(self.numItems(), 0);
  EXPECT_EQ(result.numItems(), 2);
}

TEST(ByteArrayBuiltinsTest, DunderAddReturnsConcatenatedByteArray) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'f', 'o', 'o'});
  Bytes other(&scope, runtime.newBytes(1, 'd'));
  ByteArray result(&scope,
                   runBuiltin(ByteArrayBuiltins::dunderAdd, self, other));
  EXPECT_EQ(self.numItems(), 3);
  EXPECT_EQ(result.numItems(), 4);
  EXPECT_EQ(result.byteAt(3), 'd');
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
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'h', 'e', 'l', 'l', 'o'});
  Slice index(&scope, runtime.newSlice());
  index.setStop(SmallInt::fromWord(3));
  ByteArray item(&scope,
                 runBuiltin(ByteArrayBuiltins::dunderGetItem, self, index));
  Bytes result(&scope, byteArrayAsBytes(thread, &runtime, item));
  EXPECT_TRUE(isBytesEqualsCStr(result, "hel"));
}

TEST(ByteArrayBuiltinsTest, DunderGetItemWithSliceStepReturnsByteArray) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(
      thread, self, {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'});
  Slice index(&scope, runtime.newSlice());
  index.setStart(SmallInt::fromWord(8));
  index.setStop(SmallInt::fromWord(3));
  index.setStep(SmallInt::fromWord(-2));
  ByteArray item(&scope,
                 runBuiltin(ByteArrayBuiltins::dunderGetItem, self, index));
  Bytes result(&scope, byteArrayAsBytes(thread, &runtime, item));
  EXPECT_TRUE(isBytesEqualsCStr(result, "rwo"));
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  ByteArray other(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, other, {'1', '2', '3'});
  ByteArray result(&scope,
                   runBuiltin(ByteArrayBuiltins::dunderIadd, self, other));
  ASSERT_EQ(self, result);
  EXPECT_EQ(result.numItems(), 3);
}

TEST(ByteArrayBuiltinsTest, DunderIaddWithBytesOtherConcatenatesToSelf) {
  Runtime runtime;
  HandleScope scope;
  ByteArray self(&scope, runtime.newByteArray());
  Bytes other(&scope, runtime.newBytes(2, '1'));
  ByteArray result(&scope,
                   runBuiltin(ByteArrayBuiltins::dunderIadd, self, other));
  ASSERT_EQ(self, result);
  EXPECT_EQ(result.numItems(), 2);
}

TEST(ByteArrayBuiltinsTest, DunderInitNoArgsClearsArray) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  runFromCStr(&runtime, R"(
array = bytearray(b'123')
result = array.__init__()
)");
  ByteArray array(&scope, moduleAt(&runtime, "__main__", "array"));
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isNoneType());
  EXPECT_EQ(array.numItems(), 0);
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  Int source(&scope, runtime.newInt(3));
  Object unbound(&scope, runtime.unboundValue());
  Object init(&scope, runBuiltin(ByteArrayBuiltins::dunderInit, self, source,
                                 unbound, unbound));
  ASSERT_TRUE(init.isNoneType());
  Bytes result(&scope, byteArrayAsBytes(thread, &runtime, self));
  EXPECT_TRUE(isBytesEqualsBytes(result, {0, 0, 0}));
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  Bytes source(&scope, runtime.newBytes(6, '!'));
  Object unbound(&scope, runtime.unboundValue());
  Object init(&scope, runBuiltin(ByteArrayBuiltins::dunderInit, self, source,
                                 unbound, unbound));
  ASSERT_TRUE(init.isNoneType());
  Bytes result(&scope, byteArrayAsBytes(thread, &runtime, self));
  EXPECT_EQ(result.compare(*source), 0);
}

TEST(ByteArrayBuiltinsTest, DunderInitWithByteArrayCopiesBytes) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  ByteArray source(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, source, {'h', 'i'});
  Object unbound(&scope, runtime.unboundValue());
  Object init(&scope, runBuiltin(ByteArrayBuiltins::dunderInit, self, source,
                                 unbound, unbound));
  ASSERT_TRUE(init.isNoneType());
  Bytes result(&scope, byteArrayAsBytes(thread, &runtime, self));
  Bytes expected(&scope, byteArrayAsBytes(thread, &runtime, source));
  EXPECT_EQ(result.compare(*expected), 0);
}

TEST(ByteArrayBuiltinsTest, DunderInitWithIterableCopiesBytes) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
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
  Bytes result(&scope, byteArrayAsBytes(thread, &runtime, self));
  EXPECT_TRUE(isBytesEqualsBytes(result, {1, 2, 6}));
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
  ByteArray self(&scope, runBuiltin(ByteArrayBuiltins::dunderNew, cls));
  EXPECT_EQ(self.numItems(), 0);
}

TEST(ByteArrayBuiltinsTest, NewByteArray) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  runFromCStr(&runtime, "obj = bytearray(b'Hello world!')");
  ByteArray self(&scope, moduleAt(&runtime, "__main__", "obj"));
  Bytes result(&scope, byteArrayAsBytes(thread, &runtime, self));
  EXPECT_TRUE(isBytesEqualsCStr(result, "Hello world!"));
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'f', 'o', 'o'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "bytearray(b'foo')"));
}

TEST(ByteArrayBuiltinsTest,
     DunderReprWithDoubleQuoteUsesSingleQuoteDelimiters) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'_', '"', '_'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_'))"));
}

TEST(ByteArrayBuiltinsTest,
     DunderReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'_', '\'', '_'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b"_'_"))"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithBothQuotesUsesSingleQuoteDelimiters) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'_', '"', '_', '\'', '_'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_\'_'))"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithSpeciaBytesUsesEscapeSequences) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {'\\', '\t', '\n', '\r'});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\\\t\n\r'))"));
}

TEST(ByteArrayBuiltinsTest, DunderReprWithSmallAndLargeBytesUsesHex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  runtime.byteArrayExtend(thread, self, {0, 0x1f, 0x80, 0xff});
  Object repr(&scope, runBuiltin(ByteArrayBuiltins::dunderRepr, self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\x00\x1f\x80\xff'))"));
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, self, 'a');
  Object iter(&scope, runtime.newTuple(0));
  ByteArray result(&scope, runBuiltin(ByteArrayBuiltins::join, self, iter));
  EXPECT_EQ(result.numItems(), 0);
}

TEST(ByteArrayBuiltinsTest, JoinWithEmptySeparatorReturnsByteArray) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  Tuple iter(&scope, runtime.newTuple(3));
  iter.atPut(0, runtime.newBytes(1, 'A'));
  iter.atPut(1, runtime.newBytes(2, 'B'));
  iter.atPut(2, runtime.newBytes(1, 'A'));
  ByteArray result(&scope, runBuiltin(ByteArrayBuiltins::join, self, iter));
  Bytes actual(&scope, byteArrayAsBytes(thread, &runtime, result));
  EXPECT_TRUE(isBytesEqualsCStr(actual, "ABBA"));
}

TEST(ByteArrayBuiltinsTest, JoinWithNonEmptyReturnsByteArray) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray self(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, self, ' ');
  List iter(&scope, runtime.newList());
  Bytes value(&scope, runtime.newBytes(1, '*'));
  runtime.listAdd(iter, value);
  runtime.listAdd(iter, value);
  runtime.listAdd(iter, value);
  ByteArray result(&scope, runBuiltin(ByteArrayBuiltins::join, self, iter));
  Bytes actual(&scope, byteArrayAsBytes(thread, &runtime, result));
  EXPECT_TRUE(isBytesEqualsCStr(actual, "* * *"));
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ByteArray result(&scope, moduleAt(&runtime, "__main__", "result"));
  Bytes actual(&scope, byteArrayAsBytes(thread, &runtime, result));
  EXPECT_TRUE(isBytesEqualsCStr(actual, "ab c def"));
}

}  // namespace python
