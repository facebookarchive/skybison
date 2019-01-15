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

}  // namespace python
