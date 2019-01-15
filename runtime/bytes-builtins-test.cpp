#include "gtest/gtest.h"

#include "bytes-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BytesBuiltinsTest, DunderLenWithZeroArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen));
  ASSERT_TRUE(len->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsDeathTest, DunderLenWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newBytes(1, 'a'));
  Object arg1(&scope, runtime.newBytes(1, 'b'));
  Object len(&scope, runBuiltin(BytesBuiltins::dunderLen, self, arg1));
  ASSERT_TRUE(len->isError());
  EXPECT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
}

TEST(BytesBuiltinsDeathTest, DunderLenWithNonBytesSelfRaisesTypeError) {
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
