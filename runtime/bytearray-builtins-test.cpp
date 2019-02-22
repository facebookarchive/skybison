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

}  // namespace python
