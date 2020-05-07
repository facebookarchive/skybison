#include "bytearray-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using BytearrayBuiltinsTest = RuntimeFixture;

TEST_F(BytearrayBuiltinsTest, Add) {
  HandleScope scope(thread_);

  Bytearray array(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, array, 0);
  bytearrayAdd(thread_, runtime_, array, 1);
  bytearrayAdd(thread_, runtime_, array, 2);
  EXPECT_GE(array.capacity(), 3);
  EXPECT_EQ(array.numItems(), 3);
  EXPECT_EQ(array.byteAt(0), 0);
  EXPECT_EQ(array.byteAt(1), 1);
  EXPECT_EQ(array.byteAt(2), 2);
}

TEST_F(BytearrayBuiltinsTest, AsBytes) {
  HandleScope scope(thread_);

  Bytearray array(&scope, runtime_->newBytearray());
  Bytes bytes(&scope, bytearrayAsBytes(thread_, array));
  EXPECT_TRUE(isBytesEqualsBytes(bytes, View<byte>(nullptr, 0)));

  array.setItems(runtime_->mutableBytesWith(10, 0));
  array.setNumItems(3);
  bytes = bytearrayAsBytes(thread_, array);
  const byte expected_bytes[] = {0, 0, 0};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected_bytes));
}

TEST_F(BytearrayBuiltinsTest, ClearSetsLengthToZero) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
array = bytearray(b'foo')
array.clear()
)")
                   .isError());
  Bytearray array(&scope, mainModuleAt(runtime_, "array"));
  EXPECT_EQ(array.numItems(), 0);
}

TEST_F(BytearrayBuiltinsTest, DunderAddWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__add__(b'', b'')"),
      LayoutId::kTypeError,
      "'__add__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderAddWithNonBytesLikeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray(b'') + None"), LayoutId::kTypeError,
      "can only concatenate bytearray or bytes to bytearray"));
}

TEST_F(BytearrayBuiltinsTest, DunderAddWithBytearrayOtherReturnsNewBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->bytearrayExtend(thread_, other, byte_array);
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isBytearrayEqualsCStr(self, ""));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "123"));
}

TEST_F(BytearrayBuiltinsTest, DunderAddWithBytesOtherReturnsNewBytearray) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  Bytes other(&scope, runtime_->newBytes(4, '1'));
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isBytearrayEqualsCStr(self, ""));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "1111"));
}

TEST_F(BytearrayBuiltinsTest,
       DunderAddWithBytesSubclassOtherReturnsNewBytearray) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
other = Foo(b"1234")
)")
                   .isError());
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, mainModuleAt(runtime_, "other"));
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isBytearrayEqualsCStr(self, ""));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "1234"));
}

TEST_F(BytearrayBuiltinsTest, DunderAddReturnsConcatenatedBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte byte_array[] = {'f', 'o', 'o'};
  runtime_->bytearrayExtend(thread_, self, byte_array);
  Bytes other(&scope, runtime_->newBytes(1, 'd'));
  Object result(&scope, runBuiltin(METH(bytearray, __add__), self, other));
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "foo"));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "food"));
}

TEST_F(BytearrayBuiltinsTest, DunderEqWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__eq__(b'', bytearray())"),
      LayoutId::kTypeError,
      "'__eq__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderEqWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __eq__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(BytearrayBuiltinsTest, DunderEqWithEmptyBytearraysReturnsTrue) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, runtime_->newBytearray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderEqWithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderEqWithEqualBytearrayReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderEqWithDifferentLengthsReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderEqWithDifferentContentsReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __eq__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__ge__(b'', bytearray())"),
      LayoutId::kTypeError,
      "'__ge__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __ge__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithEmptyBytearraysReturnsTrue) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, runtime_->newBytearray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGeithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithEqualBytearrayReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithLongerOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, {bytes, 2});
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithShorterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithEarlierOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGeWithLaterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ge__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__gt__(b'', bytearray())"),
      LayoutId::kTypeError,
      "'__gt__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __gt__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithEmptyBytearraysReturnsFalse) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, runtime_->newBytearray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGtithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithEqualBytearrayReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithLongerOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, {bytes, 2});
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithShorterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithEarlierOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderGtWithLaterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __gt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderIaddWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__iadd__(b'', b'')"),
      LayoutId::kTypeError,
      "'__iadd__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderIaddWithNonBytesLikeRaisesTypeError) {
  const char* test = R"(
array = bytearray(b'')
array += None
)";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, test), LayoutId::kTypeError,
                    "can only concatenate bytearray or bytes to bytearray"));
}

TEST_F(BytearrayBuiltinsTest, DunderIaddWithBytearrayOtherConcatenatesToSelf) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  const byte bytes[] = {'1', '2', '3'};
  runtime_->bytearrayExtend(thread_, other, bytes);
  Object result(&scope, runBuiltin(METH(bytearray, __iadd__), self, other));
  EXPECT_TRUE(isBytearrayEqualsBytes(self, bytes));
  EXPECT_TRUE(isBytearrayEqualsBytes(result, bytes));
}

TEST_F(BytearrayBuiltinsTest, DunderIaddWithBytesOtherConcatenatesToSelf) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'1', '2', '3'};
  Bytes other(&scope, runtime_->newBytesWithAll(bytes));
  Object result(&scope, runBuiltin(METH(bytearray, __iadd__), self, other));
  EXPECT_TRUE(isBytearrayEqualsBytes(self, bytes));
  EXPECT_TRUE(isBytearrayEqualsBytes(result, bytes));
}

TEST_F(BytearrayBuiltinsTest,
       DunderIaddWithBytesSubclassOtherConcatenatesToSelf) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
other = Foo(b"1234")
)")
                   .isError());
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, mainModuleAt(runtime_, "other"));
  Object result(&scope, runBuiltin(METH(bytearray, __iadd__), self, other));
  const char* expected = "1234";
  EXPECT_TRUE(isBytearrayEqualsCStr(self, expected));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, expected));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithNonBytearrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__imul__(b'', 1)"), LayoutId::kTypeError,
      "'__imul__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithNonIntRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object count(&scope, runtime_->newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(bytearray, __imul__), self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithIntSubclassReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(int): pass
count = C(5)
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "aaaaa"));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithDunderIndexReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "aa"));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithBadDunderIndexRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __imul__), self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(BytearrayBuiltinsTest, DunderImulPropagatesDunderIndexError) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __imul__), self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithLargeIntRaisesOverflowError) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime_->newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __imul__), self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithOverflowRaisesMemoryError) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'a', 'b', 'c'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raised(runBuiltin(METH(bytearray, __imul__), self, count),
                     LayoutId::kMemoryError));
}

TEST_F(BytearrayBuiltinsTest,
       DunderImulWithEmptyBytearrayReturnsEmptyBytearray) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object count(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithNegativeReturnsEmptyBytearray) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  self.setItems(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithZeroReturnsEmptyBytearray) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  self.setItems(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(BytearrayBuiltinsTest, DunderImulWithOneReturnsSameBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'a', 'b'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsBytes(result, bytes));
}

TEST_F(BytearrayBuiltinsTest, DunderImulReturnsRepeatedBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'a', 'b'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(METH(bytearray, __imul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "ababab"));
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__le__(b'', bytearray())"),
      LayoutId::kTypeError,
      "'__le__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __le__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithEmptyBytearraysReturnsTrue) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, runtime_->newBytearray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLeithEqualBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithEqualBytearrayReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithLongerOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, {bytes, 2});
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithShorterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithEarlierOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLeWithLaterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __le__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLenWithNonBytearrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__len__(b'')"), LayoutId::kTypeError,
      "'__len__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderLenWithEmptyBytearrayReturnsZero) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  Object result(&scope, runBuiltin(METH(bytearray, __len__), self));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(BytearrayBuiltinsTest, DunderLenWithNonEmptyBytearrayReturnsPositive) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {1, 2, 3, 4, 5};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object result(&scope, runBuiltin(METH(bytearray, __len__), self));
  EXPECT_TRUE(isIntEqualsWord(*result, 5));

  const byte bytes2[] = {6, 7};
  runtime_->bytearrayExtend(thread_, self, bytes2);
  result = runBuiltin(METH(bytearray, __len__), self);
  EXPECT_TRUE(isIntEqualsWord(*result, 7));
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__lt__(b'', bytearray())"),
      LayoutId::kTypeError,
      "'__lt__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __lt__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithEmptyBytearraysReturnsFalse) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, runtime_->newBytearray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLtithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithEqualBytearrayReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, {bytes, 2});
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithShorterOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithEarlierOtherReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderLtWithLaterOtherReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'o', 'o', 'f'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'o'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __lt__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithNonBytearrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__mul__(b'', 1)"), LayoutId::kTypeError,
      "'__mul__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object count(&scope, runtime_->newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(bytearray, __mul__), self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithIntSubclassReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  const byte view[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, view);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(int): pass
count = C(3)
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "foofoofoo"));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedBytes) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "aa"));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __mul__), self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(BytearrayBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __mul__), self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime_->newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(bytearray, __mul__), self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithOverflowRaisesMemoryError) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'a', 'b', 'c'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raised(runBuiltin(METH(bytearray, __mul__), self, count),
                     LayoutId::kMemoryError));
}

TEST_F(BytearrayBuiltinsTest,
       DunderMulWithEmptyBytearrayReturnsEmptyBytearray) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object count(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithNegativeReturnsEmptyBytearray) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  self.setItems(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithZeroReturnsEmptyBytearray) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  self.setItems(runtime_->mutableBytesWith(8, 'a'));
  self.setNumItems(8);
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(BytearrayBuiltinsTest, DunderMulWithOneReturnsSameBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'a', 'b'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsBytes(result, bytes));
}

TEST_F(BytearrayBuiltinsTest, DunderMulReturnsRepeatedBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'a', 'b'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(METH(bytearray, __mul__), self, count));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "ababab"));
}

TEST_F(BytearrayBuiltinsTest, DunderNeWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__ne__(b'', bytearray())"),
      LayoutId::kTypeError,
      "'__ne__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderNeWithNonBytesOtherReturnsNotImplemented) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(bytearray, __ne__), self, other));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST_F(BytearrayBuiltinsTest, DunderNeWithEmptyBytearraysReturnsFalse) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object other(&scope, runtime_->newBytearray());
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderNeWithEqualBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderNeWithEqualBytearrayReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, bytes);
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::falseObj());
}

TEST_F(BytearrayBuiltinsTest, DunderNeWithDifferentLengthsReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  Bytearray other(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  runtime_->bytearrayExtend(thread_, other, {bytes, 2});
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderNeWithDifferentContentsReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {'f', 'o', 'o'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object other(&scope, runtime_->newBytes(3, 'f'));
  EXPECT_EQ(runBuiltin(METH(bytearray, __ne__), self, other), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray.__new__(3)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST_F(BytearrayBuiltinsTest, DunderNewWithNonBytearrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray.__new__(int)"),
                            LayoutId::kTypeError,
                            "not a subtype of bytearray"));
}

TEST_F(BytearrayBuiltinsTest, DunderNewReturnsEmptyBytearray) {
  HandleScope scope;
  Type cls(&scope, runtime_->typeAt(LayoutId::kBytearray));
  Object self(&scope, runBuiltin(METH(bytearray, __new__), cls));
  EXPECT_TRUE(isBytearrayEqualsCStr(self, ""));
}

TEST_F(BytearrayBuiltinsTest, NewBytearray) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "obj = bytearray(b'Hello world!')").isError());
  Bytearray self(&scope, mainModuleAt(runtime_, "obj"));
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "Hello world!"));
}

TEST_F(BytearrayBuiltinsTest, DunderReprWithNonBytearrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.__repr__(b'')"), LayoutId::kTypeError,
      "'__repr__' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, DunderReprWithEmptyBytearrayReturnsEmptyRepr) {
  HandleScope scope;
  Bytearray self(&scope, runtime_->newBytearray());
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "bytearray(b'')"));
}

TEST_F(BytearrayBuiltinsTest, DunderReprWithSimpleBytearrayReturnsRepr) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'f', 'o', 'o'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "bytearray(b'foo')"));
}

TEST_F(BytearrayBuiltinsTest,
       DunderReprWithDoubleQuoteUsesSingleQuoteDelimiters) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'_', '"', '_'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_'))"));
}

TEST_F(BytearrayBuiltinsTest,
       DunderReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'_', '\'', '_'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b"_\'_"))"));
}

TEST_F(BytearrayBuiltinsTest,
       DunderReprWithBothQuotesUsesSingleQuoteDelimiters) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'_', '"', '_', '\'', '_'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'_"_\'_'))"));
}

TEST_F(BytearrayBuiltinsTest, DunderReprWithSpeciaBytesUsesEscapeSequences) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {'\\', '\t', '\n', '\r'};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\\\t\n\r'))"));
}

TEST_F(BytearrayBuiltinsTest, DunderReprWithSmallAndLargeBytesUsesHex) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {0, 0x1f, 0x80, 0xff};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object repr(&scope, runBuiltin(METH(bytearray, __repr__), self));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"(bytearray(b'\x00\x1f\x80\xff'))"));
}

TEST_F(BytearrayBuiltinsTest, DunderRmulCallsDunderMul) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = 3 * bytearray(b'123')").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "123123123"));
}

TEST_F(BytearrayBuiltinsTest, HexWithNonBytearrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.hex(b'')"), LayoutId::kTypeError,
      "'hex' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, HexWithEmptyBytearrayReturnsEmptyString) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object result(&scope, runBuiltin(METH(bytearray, hex), self));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(BytearrayBuiltinsTest, HexWithNonEmptyBytesReturnsString) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  const byte bytes[] = {0x60, 0xe, 0x18, 0x21};
  runtime_->bytearrayExtend(thread_, self, bytes);
  Object result(&scope, runBuiltin(METH(bytearray, hex), self));
  EXPECT_TRUE(isStrEqualsCStr(*result, "600e1821"));
}

TEST_F(BytearrayBuiltinsTest, JoinWithNonIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray(b'').join(0)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(BytearrayBuiltinsTest, JoinWithMistypedIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray(b' ').join([1])"), LayoutId::kTypeError,
      "sequence item 0: expected a bytes-like object, 'int' found"));
}

TEST_F(BytearrayBuiltinsTest, JoinWithIterableReturnsBytearray) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __iter__(self):
    return [b'ab', b'c', b'def'].__iter__()
result = bytearray(b' ').join(Foo())
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "ab c def"));
}

TEST_F(BytearrayBuiltinsTest, MaketransWithNonBytesLikeFromRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.maketrans([1,2], b'ab')"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'list'"));
}

TEST_F(BytearrayBuiltinsTest, MaketransWithNonBytesLikeToRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.maketrans(b'1', 2)"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'int'"));
}

TEST_F(BytearrayBuiltinsTest, MaketransWithDifferentLengthsRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.maketrans(b'12', bytearray())"),
      LayoutId::kValueError, "maketrans arguments must have same length"));
}

TEST_F(BytearrayBuiltinsTest, MaketransWithEmptyReturnsDefaultBytes) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = bytearray.maketrans(bytearray(), b'')")
          .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  byte expected[256];
  for (word i = 0; i < 256; i++) {
    expected[i] = i;
  }
  EXPECT_TRUE(isBytesEqualsBytes(result, expected));
}

TEST_F(BytearrayBuiltinsTest, MaketransWithNonEmptyReturnsBytes) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_,
                  "result = bytearray.maketrans(bytearray(b'abc'), b'123')")
          .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isBytes());
  Bytes actual(&scope, *result);
  EXPECT_EQ(actual.byteAt('a'), '1');
  EXPECT_EQ(actual.byteAt('b'), '2');
  EXPECT_EQ(actual.byteAt('c'), '3');
}

TEST_F(BytearrayBuiltinsTest, TranslateWithNonBytearraySelfRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray.translate(b'', None)"),
      LayoutId::kTypeError,
      "'translate' requires a 'bytearray' object but received a 'bytes'"));
}

TEST_F(BytearrayBuiltinsTest, TranslateWithNonBytesLikeTableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray().translate(42)"),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'int'"));
}

TEST_F(BytearrayBuiltinsTest, TranslateWithNonBytesLikeDeleteRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "bytearray().translate(None, 42)"),
      LayoutId::kTypeError, "a bytes-like object is required, not 'int'"));
}

TEST_F(BytearrayBuiltinsTest, TranslateWithShortTableRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "bytearray().translate(b'')"),
                            LayoutId::kValueError,
                            "translation table must be 256 characters long"));
}

TEST_F(BytearrayBuiltinsTest, TranslateWithEmptyBytearrayReturnsNewBytearray) {
  HandleScope scope;
  Object self(&scope, runtime_->newBytearray());
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_->newBytearray());
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
  EXPECT_NE(result, self);
}

TEST_F(BytearrayBuiltinsTest, TranslateWithNonEmptySecondArgDeletesBytes) {
  HandleScope scope(thread_);
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, alabama);
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_->newBytesWithAll(abc));
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "Alm"));
}

TEST_F(BytearrayBuiltinsTest, TranslateWithTableTranslatesBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "table = bytes.maketrans(b'Aa', b'12')").isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, alabama);
  Object table(&scope, mainModuleAt(runtime_, "table"));
  Object del(&scope, runtime_->newBytearray());
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "1l2b2m2"));
}

TEST_F(BytearrayBuiltinsTest,
       TranslateWithTableAndDeleteTranslatesAndDeletesBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "table = bytes.maketrans(b'Aa', b'12')").isError());
  const byte alabama[] = {'A', 'l', 'a', 'b', 'a', 'm', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, alabama);
  Object table(&scope, mainModuleAt(runtime_, "table"));
  Object del(&scope, runtime_->newBytesWithAll(abc));
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "1lm"));
}

TEST_F(BytearrayBuiltinsTest, TranslateDeletesAllBytes) {
  HandleScope scope(thread_);
  const byte alabama[] = {'b', 'a', 'c', 'a', 'a', 'c', 'a'};
  const byte abc[] = {'a', 'b', 'c'};
  Bytearray self(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, self, alabama);
  Object table(&scope, NoneType::object());
  Object del(&scope, runtime_->newBytesWithAll(abc));
  Object result(&scope,
                runBuiltin(METH(bytearray, translate), self, table, del));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(BytearrayBuiltinsTest, DunderIterReturnsBytearrayIterator) {
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = type(bytearray().__iter__())").isError());
  HandleScope scope;
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, runtime_->typeAt(LayoutId::kBytearrayIterator));
}

TEST_F(BytearrayBuiltinsTest, IteratorDunderNextReturnsNextElement) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ba = bytearray(b'abc')
it = iter(ba)
r0 = it.__next__()
r1 = it.__next__()
r2 = it.__next__()
try:
  it.__next__()
  r3 = False
except StopIteration:
  r3 = True
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "r0"), 'a'));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "r1"), 'b'));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "r2"), 'c'));
  EXPECT_EQ(mainModuleAt(runtime_, "r3"), Bool::trueObj());
}

TEST_F(BytearrayBuiltinsTest,
       IteratorDunderNextStopsIterationWhenBytearrayIsShrunk) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ba = bytearray(b'abc')
it = iter(ba)
)")
                   .isError());
  HandleScope scope(thread_);
  Bytearray ba(&scope, mainModuleAt(runtime_, "ba"));
  BytearrayIterator it(&scope, mainModuleAt(runtime_, "it"));
  ba.setNumItems(0);
  EXPECT_TRUE(raised(thread_->invokeMethod1(it, ID(__next__)),
                     LayoutId::kStopIteration));
}

}  // namespace testing
}  // namespace py
