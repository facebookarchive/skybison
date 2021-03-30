#include <cmath>

#include "gtest/gtest.h"

#include "attributedict.h"
#include "builtins-module.h"
#include "builtins.h"
#include "bytearray-builtins.h"
#include "dict-builtins.h"
#include "int-builtins.h"
#include "memoryview-builtins.h"
#include "module-builtins.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"
#include "type-builtins.h"

namespace py {
namespace testing {

using UnderBuiltinsModuleTest = RuntimeFixture;
using UnderBuiltinsModuleDeathTest = RuntimeFixture;

TEST_F(UnderBuiltinsModuleTest, UnderAnysetCheckWithNonSetReturnsFalse) {
  HandleScope scope(thread_);
  Object obj(&scope, newTupleWithNone(1));
  Object result(&scope, runBuiltin(FUNC(_builtins, _anyset_check), obj));
  EXPECT_EQ(result, Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest, UnderAnysetCheckWithSetReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newSet());
  Object result(&scope, runBuiltin(FUNC(_builtins, _anyset_check), obj));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest, UnderAnysetCheckWithSetSubclassReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(set):
  pass
obj = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  Object result(&scope, runBuiltin(FUNC(_builtins, _anyset_check), obj));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest, UnderAnysetCheckWithFrozenSetReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newFrozenSet());
  Object result(&scope, runBuiltin(FUNC(_builtins, _anyset_check), obj));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderAnysetCheckWithFrozenSetSubclassReturnsTrue) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(frozenset):
  pass
obj = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  Object result(&scope, runBuiltin(FUNC(_builtins, _anyset_check), obj));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest, UnderBytearrayClearSetsLengthToZero) {
  HandleScope scope(thread_);
  Bytearray array(&scope, runtime_->newBytearray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->bytearrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  ASSERT_FALSE(runBuiltin(FUNC(_builtins, _bytearray_clear), array).isError());
  EXPECT_EQ(array.numItems(), 0);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithEmptyBytearrayReturnsFalse) {
  HandleScope scope(thread_);
  Bytearray array(&scope, runtime_->newBytearray());
  ASSERT_EQ(array.numItems(), 0);
  Object key(&scope, SmallInt::fromWord('a'));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _bytearray_contains), array, key),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithIntBiggerThanCharRaisesValueError) {
  HandleScope scope(thread_);
  Bytearray array(&scope, runtime_->newBytearray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->bytearrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  Object key(&scope, SmallInt::fromWord(256));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _bytearray_contains), array, key),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithByteInBytearrayReturnsTrue) {
  HandleScope scope(thread_);
  Bytearray array(&scope, runtime_->newBytearray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->bytearrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  Object key(&scope, SmallInt::fromWord('2'));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _bytearray_contains), array, key),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithByteNotInBytearrayReturnsFalse) {
  HandleScope scope(thread_);
  Bytearray array(&scope, runtime_->newBytearray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->bytearrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  Object key(&scope, SmallInt::fromWord('x'));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _bytearray_contains), array, key),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest, UnderBytearrayDelitemDeletesItemAtIndex) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int idx(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(
      runBuiltin(FUNC(_builtins, _bytearray_delitem), self, idx).isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "abde"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayDelsliceWithStepEqualsOneAndNoGrowthDeletesSlice) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(
      runBuiltin(FUNC(_builtins, _bytearray_delslice), self, start, stop, step)
          .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "de"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayDelsliceWithStepEqualsTwoAndNoGrowthDeletesSlice) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(
      runBuiltin(FUNC(_builtins, _bytearray_delslice), self, start, stop, step)
          .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "bde"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayJoinWithEmptyIterableReturnsEmptyBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  Object iter(&scope, runtime_->emptyTuple());
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _bytearray_join), self, iter));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, ""));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayJoinWithEmptySeparatorReturnsBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  Object obj1(&scope, runtime_->newBytes(1, 'A'));
  Object obj2(&scope, runtime_->newBytes(2, 'B'));
  Object obj3(&scope, runtime_->newBytes(1, 'A'));
  Tuple iter(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _bytearray_join), self, iter));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "ABBA"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayJoinWithNonEmptyReturnsBytearray) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, ' ');
  List iter(&scope, runtime_->newList());
  Bytes value(&scope, runtime_->newBytes(1, '*'));
  runtime_->listAdd(thread_, iter, value);
  runtime_->listAdd(thread_, iter, value);
  runtime_->listAdd(thread_, iter, value);
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _bytearray_join), self, iter));
  EXPECT_TRUE(isBytearrayEqualsCStr(result, "* * *"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithLargeIntRaisesIndexError) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  Int key(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _bytearray_setitem), self, key, value),
      LayoutId::kIndexError, "cannot fit 'int' into an index-sized integer"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithKeyLargerThanMaxIndexRaisesIndexError) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(self.numItems()));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _bytearray_setitem), self, key, value),
      LayoutId::kIndexError, "index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithNegativeValueRaisesValueError) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(0));
  Int value(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _bytearray_setitem), self, key, value),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetitemWithKeySmallerThanNegativeLengthValueRaisesValueError) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(-self.numItems() - 1));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _bytearray_setitem), self, key, value),
      LayoutId::kIndexError, "index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithValueGreaterThanKMaxByteRaisesValueError) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(0));
  Int value(&scope, SmallInt::fromWord(kMaxByte + 1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _bytearray_setitem), self, key, value),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithNegativeKeyIndexesBackwards) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  Int key(&scope, SmallInt::fromWord(-1));
  Int value(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setitem), self, key, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "ab\001"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithNegativeKeySetsItemAtIndex) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  Int key(&scope, SmallInt::fromWord(1));
  Int value(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setitem), self, key, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "a\001c"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetsliceWithStepEqualsOneAndNoGrowthSetsSlice) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  Bytearray value(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, value, 'A');
  bytearrayAdd(thread_, runtime_, value, 'B');
  bytearrayAdd(thread_, runtime_, value, 'C');
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "ABCde"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetsliceWithStepEqualsTwoAndNoGrowthSetsSlice) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(2));
  Bytearray value(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, value, 'A');
  bytearrayAdd(thread_, runtime_, value, 'B');
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "AbBde"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetsliceWithStepEqualsOneAndSelfAsSourceSetsSliceAndGrows) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(4));
  Int step(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, self)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "abcdee"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetsliceWithStepEqualsOneAndSelfAsSourceStartsAtOneSetsSliceAndGrowsLHS) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(4));
  Int step(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, self)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "aabcdee"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetsliceWithStepEqualsOneAndBytesAsSourceSetsSliceAndGrowsLHS) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(4));
  Int step(&scope, SmallInt::fromWord(1));

  byte src[] = {'o', 'o', 'o', 'o'};
  Bytes value(&scope, runtime_->newBytesWithAll(src));

  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "aooooe"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetsliceWithStepEqualsOneAndMemoryViewOfBytesAsSourceSetsSliceAndGrowsLHS) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(4));
  Int step(&scope, SmallInt::fromWord(1));

  const byte src[] = {'o', 'o', 'o', 'o'};
  MemoryView value(&scope, newMemoryView(src, "b", ReadOnly::ReadWrite));

  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "aooooe"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetsliceWithStepEqualsOneAndMemoryViewOfPointerAsSourceSetsSliceAndGrowsLHS) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(4));
  Int step(&scope, SmallInt::fromWord(1));

  const word length = 4;
  byte memory[] = {'o', 'o', 'o', 'o'};
  Object none(&scope, NoneType::object());
  MemoryView value(
      &scope, runtime_->newMemoryViewFromCPtr(thread_, none, memory, length,
                                              ReadOnly::ReadWrite));

  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "aooooe"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetsliceWithStepEqualsOneAndArrayAsSourceSetsSliceAndGrowsLHS) {
  HandleScope scope(thread_);
  Bytearray self(&scope, runtime_->newBytearray());
  bytearrayAdd(thread_, runtime_, self, 'a');
  bytearrayAdd(thread_, runtime_, self, 'b');
  bytearrayAdd(thread_, runtime_, self, 'c');
  bytearrayAdd(thread_, runtime_, self, 'd');
  bytearrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(1));

  ASSERT_FALSE(runFromCStr(runtime_, R"(
import array
arr = array.array('b', [1, 2, 3, 4, 5])
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "arr"));
  ASSERT_TRUE(value.isArray());

  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _bytearray_setslice), self, start,
                         stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isBytearrayEqualsCStr(self, "a\x01\x02\x03\x04\x05"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytesContainsWithIntBiggerThanCharRaisesValueError) {
  HandleScope scope(thread_);
  const byte contents[] = {'1', '2', '3'};
  Bytes bytes(&scope, runtime_->newBytesWithAll(contents));
  ASSERT_EQ(bytes.length(), 3);
  Object key(&scope, SmallInt::fromWord(256));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(FUNC(_builtins, _bytes_contains), bytes, key),
                    LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesContainsWithByteInBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte contents[] = {'1', '2', '3'};
  Bytes bytes(&scope, runtime_->newBytesWithAll(contents));
  ASSERT_EQ(bytes.length(), 3);
  Object key(&scope, SmallInt::fromWord('2'));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _bytes_contains), bytes, key),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytesContainsWithByteNotInBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte contents[] = {'1', '2', '3'};
  Bytes bytes(&scope, runtime_->newBytesWithAll(contents));
  ASSERT_EQ(bytes.length(), 3);
  Object key(&scope, SmallInt::fromWord('x'));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _bytes_contains), bytes, key),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytesJoinWithEmptyIterableReturnsEmptyBytearray) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime_->newBytes(3, 'a'));
  Object iter(&scope, runtime_->emptyTuple());
  Object result(&scope, runBuiltin(FUNC(_builtins, _bytes_join), self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithEmptySeparatorReturnsBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, Bytes::empty());
  Object obj1(&scope, runtime_->newBytes(1, 'A'));
  Object obj2(&scope, runtime_->newBytes(2, 'B'));
  Object obj3(&scope, runtime_->newBytes(1, 'A'));
  Tuple iter(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  Object result(&scope, runBuiltin(FUNC(_builtins, _bytes_join), self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ABBA"));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithNonEmptyListReturnsBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime_->newBytes(1, ' '));
  List iter(&scope, runtime_->newList());
  Bytes value(&scope, runtime_->newBytes(1, '*'));
  runtime_->listAdd(thread, iter, value);
  runtime_->listAdd(thread, iter, value);
  runtime_->listAdd(thread, iter, value);
  Object result(&scope, runBuiltin(FUNC(_builtins, _bytes_join), self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "* * *"));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithBytesSubclassesReturnsBytes) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes):
  def join(self, iterable):
    # this should not be called - expect bytes.join() instead
    return 0
sep = Foo(b"-")
ac = Foo(b"AC")
dc = Foo(b"DC")
)")
                   .isError());
  HandleScope scope(thread_);
  Object self(&scope, mainModuleAt(runtime_, "sep"));
  Object obj1(&scope, mainModuleAt(runtime_, "ac"));
  Object obj2(&scope, mainModuleAt(runtime_, "dc"));
  Tuple iter(&scope, runtime_->newTupleWith2(obj1, obj2));
  Object result(&scope, runBuiltin(FUNC(_builtins, _bytes_join), self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "AC-DC"));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithMemoryViewReturnsBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime_->newBytes(1, ' '));
  Bytes src1(&scope, newBytesFromCStr(thread_, "hello"));
  MemoryView obj1(&scope,
                  runtime_->newMemoryView(thread_, src1, src1, src1.length(),
                                          ReadOnly::ReadOnly));
  Bytes src2(&scope, newBytesFromCStr(thread_, "world"));
  MemoryView obj2(&scope,
                  runtime_->newMemoryView(thread_, src2, src2, src2.length(),
                                          ReadOnly::ReadOnly));
  Tuple iter(&scope, runtime_->newTupleWith2(obj1, obj2));
  Object result(&scope, runBuiltin(FUNC(_builtins, _bytes_join), self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "hello world"));
}

TEST_F(UnderBuiltinsModuleTest, UnderCodeSetFilenameSetsFilename) {
  HandleScope scope(thread_);
  Code code(&scope, testing::newEmptyCode());
  Str filename(&scope, runtime_->newStrFromCStr("foobar"));
  ASSERT_NE(code.filename(), filename);
  ASSERT_FALSE(runBuiltin(FUNC(_builtins, _code_set_filename), code, filename)
                   .isError());
  EXPECT_EQ(code.filename(), filename);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderDictGetWithNotEnoughArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "_dict_get()"), LayoutId::kTypeError,
      "'_dict_get' takes min 2 positional arguments but 0 given"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderDictGetWithTooManyArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "_dict_get({}, 123, 456, 789)"),
      LayoutId::kTypeError,
      "'_dict_get' takes max 3 positional arguments but 4 given"));
}

TEST_F(UnderBuiltinsModuleTest, UnderDictGetWithUnhashableTypeRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  __hash__ = 2
key = Foo()
)")
                   .isError());
  Dict dict(&scope, runtime_->newDict());
  Object key(&scope, mainModuleAt(runtime_, "key"));
  Object default_obj(&scope, NoneType::object());
  ASSERT_TRUE(
      raised(runBuiltin(FUNC(_builtins, _dict_get), dict, key, default_obj),
             LayoutId::kTypeError));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderDictGetWithIntSubclassHashReturnsDefaultValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class N(int):
  pass
class Foo:
  def __hash__(self):
    return N(12)
key = Foo()
)")
                   .isError());
  Dict dict(&scope, runtime_->newDict());
  Object key(&scope, mainModuleAt(runtime_, "key"));
  Object default_obj(&scope, runtime_->newInt(5));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _dict_get), dict, key, default_obj),
            default_obj);
}

TEST_F(UnderBuiltinsModuleTest, UnderDictGetReturnsDefaultValue) {
  ASSERT_FALSE(
      runFromCStr(runtime_, "res = _dict_get({}, 123, 456)").isError());
  EXPECT_EQ(mainModuleAt(runtime_, "res"), RawSmallInt::fromWord(456));
}

TEST_F(UnderBuiltinsModuleTest, UnderDictGetReturnsNone) {
  ASSERT_FALSE(runFromCStr(runtime_, "result = _dict_get({}, 123)").isError());
  EXPECT_TRUE(mainModuleAt(runtime_, "result").isNoneType());
}

TEST_F(UnderBuiltinsModuleTest, UnderDictGetReturnsValue) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object key(&scope, runtime_->newInt(123));
  word hash = intHash(*key);
  Object value(&scope, runtime_->newInt(456));
  ASSERT_TRUE(dictAtPut(thread_, dict, key, hash, value).isNoneType());
  Object dflt(&scope, runtime_->newInt(789));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _dict_get), dict, key, dflt));
  EXPECT_TRUE(isIntEqualsWord(*result, 456));
}

TEST_F(UnderBuiltinsModuleTest, UnderDictGetWithNonDictRaisesTypeError) {
  HandleScope scope(thread_);
  Object foo(&scope, runtime_->newInt(123));
  Object bar(&scope, runtime_->newInt(456));
  Object baz(&scope, runtime_->newInt(789));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_builtins, _dict_get), foo, bar, baz),
                     LayoutId::kTypeError));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderDictSetitemWithKeyHashReturningNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class E:
  def __hash__(self): return "non int"

d = {}
_dict_setitem(d, E(), 4)
)"),
                            LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST_F(UnderBuiltinsModuleTest, UnderDictSetitemWithExistingKey) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDictWithSize(1));
  Str key(&scope, runtime_->newStrFromCStr("foo"));
  Object val(&scope, runtime_->newInt(0));
  Object val2(&scope, runtime_->newInt(1));
  dictAtPutByStr(thread_, dict, key, val);

  Object result(&scope,
                runBuiltin(FUNC(_builtins, _dict_setitem), dict, key, val2));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dictAtByStr(thread_, dict, key), *val2);
}

TEST_F(UnderBuiltinsModuleTest, UnderDictSetitemWithNonExistentKey) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDictWithSize(1));
  ASSERT_EQ(dict.numItems(), 0);
  Str key(&scope, runtime_->newStrFromCStr("foo"));
  Object val(&scope, runtime_->newInt(0));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _dict_setitem), dict, key, val));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dictAtByStr(thread_, dict, key), *val);
}

TEST_F(UnderBuiltinsModuleTest, UnderDictSetitemWithDictSubclassSetsItem) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class foo(dict):
  pass
d = foo()
)")
                   .isError());
  Dict dict(&scope, mainModuleAt(runtime_, "d"));
  Str key(&scope, runtime_->newStrFromCStr("a"));
  Str value(&scope, runtime_->newStrFromCStr("b"));
  Object result1(&scope,
                 runBuiltin(FUNC(_builtins, _dict_setitem), dict, key, value));
  EXPECT_TRUE(result1.isNoneType());
  Object result2(&scope, dictAtByStr(thread_, dict, key));
  ASSERT_TRUE(result2.isStr());
  EXPECT_EQ(Str::cast(*result2), *value);
}

TEST_F(UnderBuiltinsModuleTest, UnderDivmodReturnsQuotientAndDividend) {
  HandleScope scope(thread_);
  Int number(&scope, SmallInt::fromWord(1234));
  Int divisor(&scope, SmallInt::fromWord(-5));
  Object tuple_obj(&scope,
                   runBuiltin(FUNC(_builtins, _divmod), number, divisor));
  ASSERT_TRUE(tuple_obj.isTuple());
  Tuple tuple(&scope, *tuple_obj);
  ASSERT_EQ(tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), -247));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), -1));
}

TEST_F(UnderBuiltinsModuleTest, UnderFloatDivmodReturnsQuotientAndRemainder) {
  HandleScope scope(thread_);
  Float number(&scope, runtime_->newFloat(3.25));
  Float divisor(&scope, runtime_->newFloat(1.0));
  Tuple result(&scope,
               runBuiltin(FUNC(_builtins, _float_divmod), number, divisor));
  ASSERT_EQ(result.length(), 2);
  Float quotient(&scope, result.at(0));
  Float remainder(&scope, result.at(1));
  EXPECT_EQ(quotient.value(), 3.0);
  EXPECT_EQ(remainder.value(), 0.25);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderFloatDivmodWithZeroDivisorRaisesZeroDivisionError) {
  HandleScope scope(thread_);
  Float number(&scope, runtime_->newFloat(3.25));
  Float divisor(&scope, runtime_->newFloat(0.0));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(FUNC(_builtins, _float_divmod), number, divisor),
                    LayoutId::kZeroDivisionError, "float divmod()"));
}

TEST_F(UnderBuiltinsModuleTest, UnderFloatDivmodWithNanReturnsNan) {
  HandleScope scope(thread_);
  Float number(&scope, runtime_->newFloat(3.25));
  double nan = std::numeric_limits<double>::quiet_NaN();
  ASSERT_TRUE(std::isnan(nan));
  Float divisor(&scope, runtime_->newFloat(nan));
  Tuple result(&scope,
               runBuiltin(FUNC(_builtins, _float_divmod), number, divisor));
  ASSERT_EQ(result.length(), 2);
  Float quotient(&scope, result.at(0));
  Float remainder(&scope, result.at(1));
  EXPECT_TRUE(std::isnan(quotient.value()));
  EXPECT_TRUE(std::isnan(remainder.value()));
}

TEST_F(UnderBuiltinsModuleTest, UnderObjectDunderClassSetterWithTypeSelf) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class M(type):
  pass
class C(metaclass=M):
  pass
)")
                   .isError());
  HandleScope scope(thread_);
  Type c(&scope, mainModuleAt(runtime_, "C"));
  Layout previous_layout(&scope, c.instanceLayout());
  EXPECT_EQ(previous_layout.id(), c.instanceLayoutId());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
C.__class__ = type
C_instance = C()
)")
                   .isError());
  EXPECT_NE(c.instanceLayout(), previous_layout);
  EXPECT_NE(c.instanceLayoutId(), previous_layout.id());
  EXPECT_EQ(Layout::cast(c.instanceLayout()).id(), c.instanceLayoutId());

  EXPECT_EQ(runtime_->typeOf(*c), runtime_->typeAt(LayoutId::kType));
  Object c_inst(&scope, mainModuleAt(runtime_, "C_instance"));
  EXPECT_EQ(runtime_->typeOf(*c_inst), c);
}

// TODO(T64924852): Once module.__setattr__ is fixed, write death test for
// setting __class__ of module.

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectKeysWithUnassignedNumInObjectAttributes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self, p):
    if p:
      self.a = 42
i = C(False)
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));
  Object result(&scope, runBuiltin(FUNC(_builtins, _object_keys), i));
  ASSERT_TRUE(result.isList());
  EXPECT_EQ(List::cast(*result).numItems(), 0);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectKeysWithEmptyDictOverflowReturnsKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
instance = C()
instance.__dict__ = {}
)")
                   .isError());
  Object instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());
  Object result(&scope, runBuiltin(FUNC(_builtins, _object_keys), instance));
  ASSERT_TRUE(result.isList());
  EXPECT_PYLIST_EQ(result, {});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectKeysWithNonEmptyDictOverflowReturnsKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
instance = C()
instance.__dict__ = {"hello": "world", "foo": "bar"}
)")
                   .isError());
  Object instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());
  Object result(&scope, runBuiltin(FUNC(_builtins, _object_keys), instance));
  ASSERT_TRUE(result.isList());
  EXPECT_PYLIST_EQ(result, {"hello", "foo"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectKeysWithInObjectAndDictOverflowReturnsKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def instance():
  pass
instance.hello = "world"
instance.foo = "bar"
)")
                   .isError());
  Object instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout layout(&scope, runtime_->layoutOf(*instance));
  Tuple in_object(&scope, layout.inObjectAttributes());
  ASSERT_GT(in_object.length(), 0);
  ASSERT_TRUE(layout.hasDictOverflow());
  Object result_obj(&scope,
                    runBuiltin(FUNC(_builtins, _object_keys), instance));
  ASSERT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  EXPECT_GT(result.numItems(), 2);
  Str hello(&scope, runtime_->newStrFromCStr("hello"));
  Str foo(&scope, runtime_->newStrFromCStr("foo"));
  EXPECT_TRUE(listContains(result, hello));
  EXPECT_TRUE(listContains(result, foo));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectKeysWithHiddenAttributesDoesNotReturnKey) {
  HandleScope scope(thread_);
  LayoutId layout_id = LayoutId::kUserWarning;
  Object previous_layout(&scope, runtime_->layoutAt(layout_id));
  BuiltinAttribute attrs[] = {
      {ID(__globals__), 0, AttributeFlags::kHidden},
  };
  Type type(&scope, addBuiltinType(thread_, ID(UserWarning), layout_id,
                                   LayoutId::kObject, attrs,
                                   /*size=*/kPointerSize,
                                   /*basetype=*/true));
  Layout layout(&scope, type.instanceLayout());
  runtime_->layoutAtPut(layout_id, *layout);
  Instance instance(&scope, runtime_->newInstance(layout));
  Object result_obj(&scope,
                    runBuiltin(FUNC(_builtins, _object_keys), instance));
  ASSERT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  Str dunder_globals(&scope, runtime_->newStrFromCStr("__globals__"));
  EXPECT_FALSE(listContains(result, dunder_globals));
  EXPECT_EQ(instance.layoutId(), layout.id());
  runtime_->layoutAtPut(layout_id, *previous_layout);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderInstanceDunderDictSetterCoalescesAffectedLayoutsIntoSingleOne) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass

c0 = C()
c0.foo = 4

c1 = C()
c1.bar = 5

c0.__dict__ = {"a": 4}
c1.__dict__ = {"b": 4}
)")
                   .isError());
  Object c0(&scope, mainModuleAt(runtime_, "c0"));
  Object c1(&scope, mainModuleAt(runtime_, "c1"));
  EXPECT_EQ(c0.layoutId(), c1.layoutId());
}

TEST_F(UnderBuiltinsModuleTest, UnderInstanceOverflowDictAllocatesDictionary) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def instance():
  pass
)")
                   .isError());
  Object instance_obj(&scope, mainModuleAt(runtime_, "instance"));
  ASSERT_TRUE(instance_obj.isHeapObject());
  Instance instance(&scope, *instance_obj);
  Layout layout(&scope, runtime_->layoutOf(*instance));
  ASSERT_TRUE(layout.hasDictOverflow());
  word offset = layout.dictOverflowOffset();
  ASSERT_TRUE(instance.instanceVariableAt(offset).isNoneType());

  Object result0(
      &scope, runBuiltin(FUNC(_builtins, _instance_overflow_dict), instance));
  ASSERT_EQ(instance.layoutId(), layout.id());
  Object overflow_dict(&scope, instance.instanceVariableAt(offset));
  EXPECT_TRUE(result0.isDict());
  EXPECT_EQ(result0, overflow_dict);

  Object result1(
      &scope, runBuiltin(FUNC(_builtins, _instance_overflow_dict), instance));
  EXPECT_EQ(result0, result1);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithLittleEndianReturnsSmallInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe};
  Bytes bytes(&scope, runtime_->newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::falseObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type,
                                   bytes, byteorder_big, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xfeca));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithLittleEndianReturnsLargeInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23,
                              0x45, 0x67, 0x89, 0xab, 0xcd};
  Bytes bytes(&scope, runtime_->newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::falseObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Int result(&scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type,
                                bytes, byteorder_big, signed_arg));
  ASSERT_EQ(result.numDigits(), 2);
  EXPECT_EQ(result.digitAt(0), 0x67452301bebafecaU);
  EXPECT_EQ(result.digitAt(1), 0xcdab89U);
}

TEST_F(UnderBuiltinsModuleTest, UnderIntFromBytesWithBigEndianReturnsSmallInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe};
  Bytes bytes(&scope, runtime_->newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::trueObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type,
                                   bytes, byteorder_big, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xcafe));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntFromBytesWithBigEndianReturnsLargeInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23,
                              0x45, 0x67, 0x89, 0xab, 0xcd};
  Bytes bytes(&scope, runtime_->newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::trueObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Int result(&scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type,
                                bytes, byteorder_big, signed_arg));
  ASSERT_EQ(result.numDigits(), 2);
  EXPECT_EQ(result.digitAt(0), 0xbe0123456789abcdU);
  EXPECT_EQ(result.digitAt(1), 0xcafebaU);
}

TEST_F(UnderBuiltinsModuleTest, UnderIntFromBytesWithEmptyBytes) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll(View<byte>(nullptr, 0)));
  Bool bo_big_false(&scope, Bool::falseObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Object result_little(
      &scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type, bytes,
                         bo_big_false, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result_little, 0));

  Bool bo_big_true(&scope, Bool::trueObj());
  Object result_big(
      &scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type, bytes,
                         bo_big_true, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result_big, 0));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithNumberWithDigitHighBitSet) {
  HandleScope scope(thread_);

  // Test special case where a positive number having a high bit set at the end
  // of a "digit" needs an extra digit in the LargeInt representation.
  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytes(kWordSize, 0xff));
  Bool byteorder_big(&scope, Bool::falseObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Int result(&scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type,
                                bytes, byteorder_big, signed_arg));
  const uword expected_digits[] = {kMaxUword, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithNegativeNumberReturnsSmallInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xff};
  Bytes bytes(&scope, runtime_->newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::falseObj());
  Bool signed_arg(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type,
                                   bytes, byteorder_big, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithNegativeNumberReturnsLargeInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_->typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23,
                              0x45, 0x67, 0x89, 0xab, 0xcd};
  Bytes bytes(&scope, runtime_->newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::trueObj());
  Bool signed_arg(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_from_bytes), int_type,
                                   bytes, byteorder_big, signed_arg));
  const uword expected_digits[] = {0xbe0123456789abcd, 0xffffffffffcafeba};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithZeroBaseReturnsCodeLiteral) {
  HandleScope scope(thread_);
  const byte view[] = {'0', 'x', 'b', 'a', '5', 'e'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytearray array(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_new_from_byteslike),
                                   type, array, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xba5e));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithInvalidByteInBytearrayRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'$'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytearray array(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, array, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithInvalidLiteraRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'a'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytearray array(&scope, runtime_->newBytearray());
  runtime_->bytearrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, array, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'a'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithInvalidByteRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'$'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithInvalidLiteralRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'8', '6'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(7));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 7: b'86'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithBytesSubclassReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
foo = Foo(b"42")
)")
                   .isError());
  Object type(&scope, runtime_->typeAt(LayoutId::kInt));
  Object bytes(&scope, mainModuleAt(runtime_, "foo"));
  Object base(&scope, SmallInt::fromWord(21));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(86));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromByteslikeWithZero) {
  HandleScope scope(thread_);
  const byte src[] = {'0'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_new_from_byteslike),
                                   type, bytes, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromByteslikeWithLargeInt) {
  HandleScope scope(thread_);
  const byte src[] = "1844674407370955161500";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_new_from_byteslike),
                                   type, bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0xffffffffffffff9c, 0x63};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromByteslikeWithLargeInt2) {
  HandleScope scope(thread_);
  const byte src[] = "46116860184273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(FUNC(_builtins, _int_new_from_byteslike),
                                   type, bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0x7ffffffffffffff6, 0x2};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithLargeIntWithInvalidDigitRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "461168601$84273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 10: b'461168601$84273879030'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithLeadingPlusReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "+46116860184273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Int result(&scope, runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type,
                                bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0x7ffffffffffffff6, 0x2};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithDoubleLeadingPlusRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "++1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'++1'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithLeadingNegAndSpaceReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "   -46116860184273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Int result(&scope, runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type,
                                bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0x800000000000000a, 0xfffffffffffffffd};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithDoubleLeadingNegRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "--1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'--1'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithHexPrefixAndBaseZeroReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "0x1f";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(31));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithHexPrefixAndBaseSixteenReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "0x1f";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(31));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithHexPrefixAndBaseNineRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "0x1f";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(9));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 9: b'0x1f'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithBaseThreeReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "221";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(3));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(25));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithUnderscoreReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "1_000_000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(1000000));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithLeadingUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "_1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 0: b'_1'"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderIntNewFromByteslikeWithLeadingUnderscoreAndPrefixAndBaseReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "0b_1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(2));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithTrailingUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "1_000_";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 0: b'1_000_'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithDoubleUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "1__000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 0: b'1__000'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithWhitespaceOnLeftReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = " \t\n\t\v-123";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(-123));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithWhitespaceOnRightReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "-234 \t \f \n\t";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(-234));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithWhitespaceOnLeftAndRightReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "  \n\t\r\n  +345 \t  \n\t";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_EQ(
      runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type, bytes, base),
      SmallInt::fromWord(345));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithMemoryviewReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {'-', '1', '2'};
  MemoryView memoryview(&scope, newMemoryView(bytes, "b"));
  MemoryView memoryview_slice(
      &scope, memoryviewGetslice(thread_, memoryview, /*start=*/1, /*stop=*/2,
                                 /*step=*/1));
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type,
                       memoryview_slice, base),
            SmallInt::fromWord(1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteslikeWithMemoryviewSliceReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {'-', '1', '2'};
  Object memoryview(&scope, newMemoryView(bytes, "b"));
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _int_new_from_byteslike), type,
                       memoryview, base),
            SmallInt::fromWord(-12));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromIntWithBoolReturnsSmallInt) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_->typeAt(LayoutId::kInt));
  Object fls(&scope, Bool::falseObj());
  Object tru(&scope, Bool::trueObj());
  Object false_result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_int), type, fls));
  Object true_result(&scope,
                     runBuiltin(FUNC(_builtins, _int_new_from_int), type, tru));
  EXPECT_EQ(false_result, SmallInt::fromWord(0));
  EXPECT_EQ(true_result, SmallInt::fromWord(1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromIntWithSubClassReturnsValueOfSubClass) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubInt(int):
  def __new__(cls, value):
      self = super(SubInt, cls).__new__(cls, value)
      self.name = "subint instance"
      return self

result = SubInt(50)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_FALSE(result.isInt());
  EXPECT_TRUE(isIntEqualsWord(*result, 50));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithZeroBaseReturnsCodeLiteral) {
  HandleScope scope(thread_);
  const char* src = "1985";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 1985));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithInvalidCharRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "$";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: '$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithInvalidLiteralRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "305";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(4));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 4: '305'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithInvalidLiteralRaisesValueErrorBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "g";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: 'g'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeInt) {
  HandleScope scope(thread_);
  const char* src = "1844674407370955161500";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0xffffffffffffff9c, 0x63};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeInt2) {
  HandleScope scope(thread_);
  const char* src = "46116860184273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0x7ffffffffffffff6, 0x2};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLargeIntWithInvalidDigitRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "461168601$84273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 10: '461168601$84273879030'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithOnlySignRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "-";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '-'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLengthOneInfersBase10) {
  HandleScope scope(thread_);
  const char* src = "8";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 8));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLengthOneBase10) {
  HandleScope scope(thread_);
  const char* src = "8";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 8));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseTwo) {
  HandleScope scope(thread_);
  const char* src = "100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(2));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 4));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntInfersBaseTen) {
  HandleScope scope(thread_);
  const char* src = "100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLeadingSpacesRemovesSpaces) {
  HandleScope scope(thread_);
  const char* src = "      100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithOnlySpacesRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "    ";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '    '"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithPlusReturnsPositiveInt) {
  HandleScope scope(thread_);
  const char* src = "+100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithTwoPlusSignsRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "++100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 16: '++100'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseEight) {
  HandleScope scope(thread_);
  const char* src = "0o77712371237123712371237123777";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(8));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  const uword digits[] = {0xa7ca7ca7ca7ca7ff, 0x7fca7c};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntInfersBaseEight) {
  HandleScope scope(thread_);
  const char* src = "0o77712371237123712371237123777";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  const uword digits[] = {0xa7ca7ca7ca7ca7ff, 0x7fca7c};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithOnlyPrefixRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "0x";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '0x'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithMinusAndPrefixRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "-0x";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '-0x'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithPlusAndPrefixRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "+0x";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '+0x'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithJustPrefixAndUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "0x_";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '0x_'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoreIgnoresUnderscore) {
  HandleScope scope(thread_);
  const char* src = "0x_deadbeef";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xdeadbeef));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoresIgnoresUnderscoresBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "0x_d_e_a_d_b_eef";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xdeadbeef));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoresIgnoresUnderscoresBaseTen) {
  HandleScope scope(thread_);
  const char* src = "100_000_000_000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100000000000));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLeadingUnderscoreBaseTenRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "_100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '_100'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithTrailingUnderscoreBaseTenRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "100_";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '100_'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithDoubleUnderscoreBaseTenRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "1__00";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 10: '1__00'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLeadingUnderscoreNoPrefixRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "_abc";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '_abc'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithNegativeZeroReturnsZero) {
  HandleScope scope(thread_);
  const char* src = "-0";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithTwoMinusSignsRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "--100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 16: '--100'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithPositiveZeroReturnsZero) {
  HandleScope scope(thread_);
  const char* src = "+0";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithEmptyStringRaisesValueError) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, Str::empty());
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: ''"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithHexLiteralNoPrefixRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "a";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: 'a'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "0x8000000000000000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  const uword digits[] = {0x8000000000000000, 0x0};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLargeIntInfersBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "0x8000000000000000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  const uword digits[] = {0x8000000000000000, 0x0};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLargeIntBaseSixteenWithLetters) {
  HandleScope scope(thread_);
  const char* src = "0x80000000DEADBEEF";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  const uword digits[] = {0x80000000deadbeef, 0x0};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLargeIntInfersBaseSixteenWithLetters) {
  HandleScope scope(thread_);
  const char* src = "0x80000000DEADBEEF";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  const uword digits[] = {0x80000000deadbeef, 0x0};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseZeroReturnsOne) {
  HandleScope scope(thread_);
  const char* src = "0b1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseTwoReturnsOne) {
  HandleScope scope(thread_);
  const char* src = "0b1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(2));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderIntNewFromStrWithBinaryLiteralBaseSixteenReturnsOneHundredSeventySeven) {
  HandleScope scope(thread_);
  const char* src = "0b1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 177));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseSixteenReturnsEleven) {
  HandleScope scope(thread_);
  const char* src = "0b";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(
      &scope, runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 11));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseEightRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "0b1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(8));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _int_new_from_str), type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 8: '0b1'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderListAppendAppendsObject) {
  HandleScope scope(thread_);
  List list(&scope, runtime_->newList());
  Object value0(&scope, runtime_->newInt(42));
  Object value1(&scope, runtime_->newStrFromCStr("foo"));
  EXPECT_EQ(list.numItems(), 0);
  EXPECT_TRUE(
      runBuiltin(FUNC(_builtins, _list_append), list, value0).isNoneType());
  EXPECT_EQ(list.numItems(), 1);
  EXPECT_TRUE(
      runBuiltin(FUNC(_builtins, _list_append), list, value1).isNoneType());
  ASSERT_EQ(list.numItems(), 2);
  EXPECT_EQ(list.at(0), value0);
  EXPECT_EQ(list.at(1), value1);
}

TEST_F(UnderBuiltinsModuleTest, UnderListAppendWithNonListRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_a_list(&scope, runtime_->newInt(42));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _list_append), not_a_list, not_a_list),
      LayoutId::kTypeError,
      "'<anonymous>' requires a 'list' object but received a 'int'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderListCheckExactWithExactListReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newList());
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _list_check_exact), obj),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListCheckExactWithListSubclassReturnsFalse) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(list):
  pass
obj = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _list_check_exact), obj),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNegativeIndexRemovesRelativeToEnd) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delitem), list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelitemWithLastIndexRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delitem), list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithFirstIndexRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delitem), list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNegativeFirstIndexRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-2));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delitem), list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNegativeLastIndexRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delitem), list, idx).isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNumberGreaterThanSmallIntMaxDoesNotCrash) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int big(&scope, runtime_->newInt(SmallInt::kMaxValue + 100));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_builtins, _list_delitem), list, big),
                     LayoutId::kIndexError));
  EXPECT_PYLIST_EQ(list, {0, 1});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelsliceRemovesItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelsliceRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelsliceRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(2));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelsliceWithStopEqualsLengthRemovesTrailingItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelsliceWithStartEqualsZeroRemovesStartingItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelsliceWithStartEqualsZeroAndStopEqualsLengthRemovesAllItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_EQ(list.numItems(), 0);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelsliceWithStepEqualsTwoDeletesEveryEvenItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(2));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1, 3});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelsliceWithStepEqualsTwoDeletesEveryOddItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(2));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {0, 2, 4});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelsliceWithStepGreaterThanLengthDeletesOneItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(1000));
  ASSERT_TRUE(
      runBuiltin(FUNC(_builtins, _list_delslice), list, start, stop, step)
          .isNoneType());
  EXPECT_PYLIST_EQ(list, {1, 2, 3, 4});
}

TEST_F(UnderBuiltinsModuleTest, UnderListGetitemWithNegativeIndex) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  Object result(&scope, runBuiltin(FUNC(_builtins, _list_getitem), list, idx));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListGetitemWithInvalidNegativeIndexRaisesIndexError) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-4));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(FUNC(_builtins, _list_getitem), list, idx),
                    LayoutId::kIndexError, "list index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListGetitemWithInvalidPositiveIndexRaisesIndexError) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(FUNC(_builtins, _list_getitem), list, idx),
                    LayoutId::kIndexError, "list index out of range"));
}

TEST_F(UnderBuiltinsModuleTest, UnderListSwapSwapsItemsAtIndices) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 4));
  Object i(&scope, SmallInt::fromWord(1));
  Object j(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _list_swap), list, i, j).isNoneType());
  EXPECT_PYLIST_EQ(list, {0, 2, 1, 3});
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatbReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xab, 0xc5};
  Object view(&scope, newMemoryView(bytes, "b"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -59));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatBReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xee, 0xd8};
  Object view(&scope, newMemoryView(bytes, "B"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 216));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatcReturnsBytes) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x03, 0x62};
  Object view(&scope, newMemoryView(bytes, "c"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  const byte expected_bytes[] = {0x62};
  EXPECT_TRUE(isBytesEqualsBytes(result, expected_bytes));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormathReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xcd, 0x2c, 0x5c, 0xfc};
  Object view(&scope, newMemoryView(bytes, "h"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -932));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatHReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xb2, 0x11, 0x94, 0xc0};
  Object view(&scope, newMemoryView(bytes, "H"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 49300));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatiReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x30, 0x8A, 0x43, 0xF2, 0xE1, 0xD6, 0x56, 0xE4};
  Object view(&scope, newMemoryView(bytes, "i"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -464070943));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatAtiReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x30, 0x8A, 0x43, 0xF2, 0xE1, 0xD6, 0x56, 0xE4};
  Object view(&scope, newMemoryView(bytes, "@i"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -464070943));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatIReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x2, 0xBE, 0xA8, 0x3D, 0x74, 0x18, 0xEB, 0x8};
  Object view(&scope, newMemoryView(bytes, "I"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 149624948));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatlReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xD8, 0x76, 0x97, 0xD1, 0x8B, 0xA1, 0xD2, 0x62,
                        0xD9, 0xD2, 0x50, 0x47, 0xC0, 0xA8, 0xB7, 0x81};
  Object view(&scope, newMemoryView(bytes, "l"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -9099618978295131431));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatLReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x24, 0x37, 0x8B, 0x51, 0xCB, 0xB2, 0x16, 0xFB,
                        0xA6, 0xA9, 0x49, 0xB3, 0x59, 0x6A, 0x48, 0x62};
  Object view(&scope, newMemoryView(bytes, "L"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 7082027347532687782));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatqReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x7,  0xE2, 0x42, 0x9E, 0x8F, 0xBF, 0xDB, 0x1B,
                        0x8C, 0x1C, 0x34, 0x40, 0x86, 0x41, 0x2B, 0x23};
  Object view(&scope, newMemoryView(bytes, "q"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 2534191260184616076));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatQReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xD9, 0xC6, 0xD2, 0x40, 0xBD, 0x19, 0xA9, 0xC8,
                        0x8A, 0x1,  0x8B, 0xAF, 0x15, 0x36, 0xC7, 0xBD};
  Object view(&scope, newMemoryView(bytes, "Q"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  const uword expected_digits[] = {0xbdc73615af8b018aul, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatnReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xF2, 0x6F, 0xFA, 0x8B, 0x93, 0xC0, 0xED, 0x9D,
                        0x6D, 0x7C, 0xE3, 0xDC, 0x26, 0xEF, 0xB8, 0xEB};
  Object view(&scope, newMemoryView(bytes, "n"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, -1461155128888034195l));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatNReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x6B, 0x8F, 0x6,  0xA2, 0xE0, 0x13, 0x88, 0x47,
                        0x7E, 0xB6, 0x40, 0x7E, 0x6B, 0x2,  0x9,  0xC0};
  Object view(&scope, newMemoryView(bytes, "N"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  const uword expected_digits[] = {0xc009026b7e40b67eul, 0};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatfReturnsFloat) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x67, 0x32, 0x23, 0x31, 0xB9, 0x70, 0xBC, 0x83};
  Object view(&scope, newMemoryView(bytes, "f"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isFloatEqualsDouble(
      *result, std::strtod("-0x1.78e1720000000p-120", nullptr)));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewGetitemWithFormatdReturnsFloat) {
  HandleScope scope(thread_);
  const byte bytes[] = {0xEA, 0x43, 0xAD, 0x6F, 0x9D, 0x31, 0xE,  0x96,
                        0x28, 0x80, 0x1A, 0xD,  0x87, 0xC,  0xAC, 0x4B};
  Object view(&scope, newMemoryView(bytes, "d"));
  Int index(&scope, runtime_->newInt(1));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isFloatEqualsDouble(
      *result, std::strtod("0x1.c0c870d1a8028p+187", nullptr)));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithFormatQuestionmarkReturnsTrue) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x92, 0xE1, 0x57, 0xEA, 0x81, 0xA8};
  Object view(&scope, newMemoryView(bytes, "?"));
  Int index(&scope, runtime_->newInt(3));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithFormatQuestionmarkReturnsFalse) {
  HandleScope scope(thread_);
  const byte bytes[] = {0x92, 0xE1, 0, 0xEA, 0x81, 0xA8};
  Object view(&scope, newMemoryView(bytes, "?"));
  Int index(&scope, runtime_->newInt(2));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_EQ(result, Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithNegativeIndexReturnsInt) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  Object view(&scope, newMemoryView(bytes, "h"));
  Int index(&scope, runtime_->newInt(-2));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 0x504));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithNonMemoryViewRaisesTypeError) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  Int index(&scope, runtime_->newInt(0));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), none, index));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithTooBigIndexRaisesIndexError) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  Object view(&scope, newMemoryView(bytes, "I"));
  Int index(&scope, runtime_->newInt(2));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kIndexError, "index out of bounds"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithOverflowingIndexRaisesIndexError) {
  HandleScope scope(thread_);
  const byte bytes[] = {0, 1, 2, 3, 4, 5, 6, 7};
  Object view(&scope, newMemoryView(bytes, "I"));
  Int index(&scope, runtime_->newInt(kMaxWord / 2));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kIndexError, "index out of bounds"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithMemoryBufferReadsMemory) {
  HandleScope scope(thread_);
  const word length = 5;
  byte memory[length];
  for (word i = 0; i < length; i++) {
    memory[i] = i;
  }
  Object none(&scope, NoneType::object());
  Object view(&scope, runtime_->newMemoryViewFromCPtr(
                          thread_, none, memory, length, ReadOnly::ReadOnly));
  Int idx(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx), 0));
  idx = Int::cast(SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx), 1));
  idx = Int::cast(SmallInt::fromWord(2));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx), 2));
  idx = Int::cast(SmallInt::fromWord(3));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx), 3));
  idx = Int::cast(SmallInt::fromWord(4));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _memoryview_getitem), view, idx), 4));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewGetitemWithBytearrayReadsFromMutableBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime_->typeAt(LayoutId::kMemoryView));
  Bytearray bytearray(&scope, runtime_->newBytearray());
  const byte byte_array[] = {0xce};
  runtime_->bytearrayExtend(thread, bytearray, byte_array);
  Object result_obj(&scope,
                    runBuiltin(METH(memoryview, __new__), type, bytearray));
  ASSERT_TRUE(result_obj.isMemoryView());
  Object view(&scope, *result_obj);
  Int index(&scope, runtime_->newInt(0));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_getitem), view, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xce));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewItemsizeWithNonMemoryViewRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_memoryview(&scope, runtime_->newInt(12));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _memoryview_itemsize), not_memoryview),
      LayoutId::kTypeError,
      "'<anonymous>' requires a 'memoryview' object but received a 'int'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewItemsizeReturnsSizeOfMemoryItems) {
  HandleScope scope(thread_);
  Bytes bytes(&scope, runtime_->newBytes(5, 'x'));
  MemoryView view(&scope, runtime_->newMemoryView(thread_, bytes, bytes, 5,
                                                  ReadOnly::ReadOnly));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _memoryview_itemsize), view));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewNbytesWithNonMemoryViewRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_memoryview(&scope, runtime_->newInt(12));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _memoryview_nbytes), not_memoryview),
      LayoutId::kTypeError,
      "'<anonymous>' requires a 'memoryview' object but received a 'int'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewNbytesReturnsSizeOfMemoryView) {
  HandleScope scope(thread_);
  Bytes bytes(&scope, runtime_->newBytes(5, 'x'));
  MemoryView view(&scope, runtime_->newMemoryView(thread_, bytes, bytes, 5,
                                                  ReadOnly::ReadOnly));
  Object result(&scope, runBuiltin(FUNC(_builtins, _memoryview_nbytes), view));
  EXPECT_TRUE(isIntEqualsWord(*result, 5));
}

TEST_F(UnderBuiltinsModuleTest, UnderModuleDirListWithFilteredOutPlaceholders) {
  HandleScope scope(thread_);
  Str module_name(&scope, runtime_->newStrFromCStr("module"));
  Module module(&scope, runtime_->newModule(module_name));
  attributeDictInit(thread_, module);

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Str value(&scope, runtime_->newStrFromCStr("value"));

  moduleAtPut(thread_, module, foo, value);
  moduleAtPut(thread_, module, bar, value);
  moduleAtPut(thread_, module, baz, value);

  ValueCell::cast(moduleValueCellAt(thread_, module, bar)).makePlaceholder();

  List keys(&scope, runBuiltin(FUNC(_builtins, _module_dir), module));
  EXPECT_EQ(keys.numItems(), 2);
  EXPECT_EQ(keys.at(0), *foo);
  EXPECT_EQ(keys.at(1), *baz);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithNonexistentAttrReturnsFalse) {
  HandleScope scope(thread_);
  Object obj(&scope, SmallInt::fromWord(0));
  Str name(&scope, runtime_->newStrFromCStr("__foo_bar_baz__"));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _object_type_hasattr), obj, name));
  EXPECT_EQ(result, Bool::falseObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithInstanceAttrReturnsFalse) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foobarbaz = 5
obj = C()
)")
                   .isError());
  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  Str name(&scope, runtime_->newStrFromCStr("foobarbaz"));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _object_type_hasattr), obj, name));
  EXPECT_EQ(result, Bool::falseObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithExistentAttrReturnsTrue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    foobarbaz = 5
obj = C()
)")
                   .isError());
  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  Str name(&scope, runtime_->newStrFromCStr("foobarbaz"));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _object_type_hasattr), obj, name));
  EXPECT_EQ(result, Bool::trueObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithRaisingDescriptorDoesNotRaise) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Desc:
  def __get__(self, obj, type):
    raise UserWarning("foo")
class C:
    foobarbaz = Desc()
obj = C()
)")
                   .isError());
  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  Str name(&scope, runtime_->newStrFromCStr("foobarbaz"));
  Object result(&scope,
                runBuiltin(FUNC(_builtins, _object_type_hasattr), obj, name));
  EXPECT_EQ(result, Bool::trueObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest, UnderOsWriteWithBadFdRaisesOSError) {
  HandleScope scope(thread_);
  Int fd(&scope, SmallInt::fromWord(-1));
  const byte buf[] = {0x1, 0x2};
  Bytes bytes_buf(&scope, runtime_->newBytesWithAll(buf));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_builtins, _os_write), fd, bytes_buf),
                     LayoutId::kOSError));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderOsWriteWithFdNotOpenedForWritingRaisesOSError) {
  HandleScope scope(thread_);
  int fds[2];
  int result = ::pipe(fds);
  ASSERT_EQ(result, 0);
  Int fd(&scope, SmallInt::fromWord(fds[0]));
  const byte buf[] = {0x1, 0x2};
  Bytes bytes_buf(&scope, runtime_->newBytesWithAll(buf));
  EXPECT_TRUE(raised(runBuiltin(FUNC(_builtins, _os_write), fd, bytes_buf),
                     LayoutId::kOSError));
  ::close(fds[0]);
  ::close(fds[1]);
}

TEST_F(UnderBuiltinsModuleTest, UnderOsWriteWritesSizeBytes) {
  HandleScope scope(thread_);
  int fds[2];
  int result = ::pipe(fds);
  ASSERT_EQ(result, 0);
  Int fd(&scope, SmallInt::fromWord(fds[1]));
  byte to_write[] = "hello";
  word count = std::strlen(reinterpret_cast<char*>(to_write));
  Bytes bytes_buf(&scope,
                  runtime_->newBytesWithAll(View<byte>(to_write, count)));
  Object result_obj(&scope,
                    runBuiltin(FUNC(_builtins, _os_write), fd, bytes_buf));
  EXPECT_TRUE(isIntEqualsWord(*result_obj, count));
  ::close(fds[1]);  // Send EOF
  std::unique_ptr<char[]> buf(new char[count + 1]{0});
  result = ::read(fds[0], buf.get(), count);
  EXPECT_EQ(result, count);
  EXPECT_STREQ(buf.get(), reinterpret_cast<char*>(to_write));
  ::close(fds[0]);
}

TEST_F(UnderBuiltinsModuleTest, UnderOsWriteWritesSizeMemoryView) {
  HandleScope scope(thread_);
  int fds[2];
  int result = ::pipe(fds);
  ASSERT_EQ(result, 0);
  Int fd(&scope, SmallInt::fromWord(fds[1]));
  const byte bytes[] = "hello_there";
  MemoryView memoryview(&scope, newMemoryView(bytes, "b"));
  MemoryView memoryview_slice(
      &scope, memoryviewGetslice(thread_, memoryview, /*start=*/0, /*stop=*/5,
                                 /*step=*/1));
  Object result_obj(
      &scope, runBuiltin(FUNC(_builtins, _os_write), fd, memoryview_slice));
  EXPECT_TRUE(isIntEqualsWord(*result_obj, 5));
  ::close(fds[1]);  // Send EOF
  std::unique_ptr<char[]> buf(new char[6]{0});
  result = ::read(fds[0], buf.get(), 5);
  EXPECT_EQ(result, 5);
  EXPECT_STREQ(buf.get(), "hello");
  ::close(fds[0]);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrCompareDigestWithNonASCIIRaisesTypeError) {
  HandleScope scope(thread_);
  Str left(&scope, runtime_->newStrFromCStr("foo\u00E4"));
  Str right(&scope, runtime_->newStrFromCStr("foo\u00E4"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _str_compare_digest), left, right),
      LayoutId::kTypeError,
      "comparing strings with non-ASCII characters is not supported"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrCountWithStartAndEndSearchesWithinBounds) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("ofoodo"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  Object start(&scope, SmallInt::fromWord(2));
  Object end(&scope, SmallInt::fromWord(4));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _str_count), haystack, needle, start, end),
      2));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrCountWithNoneStartStartsFromZero) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("foo"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  Object start(&scope, NoneType::object());
  Object end(&scope, SmallInt::fromWord(haystack.codePointLength()));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _str_count), haystack, needle, start, end),
      2));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrCountWithNoneEndSetsEndToHaystackLength) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("foo"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  Object start(&scope, SmallInt::fromWord(0));
  Object end(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FUNC(_builtins, _str_count), haystack, needle, start, end),
      2));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrFromStrWithStrTypeReturnsValueOfStrType) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = _str_from_str(str, 'value')
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(runtime_->isInstanceOfStr(*result));
  EXPECT_TRUE(result.isStr());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrFromStrWithSubClassTypeReturnsValueOfSubClassType) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Sub(str): pass
result = _str_from_str(Sub, 'value')
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  Object sub(&scope, mainModuleAt(runtime_, "Sub"));
  EXPECT_EQ(runtime_->typeOf(*result), sub);
  EXPECT_TRUE(isStrEqualsCStr(*result, "value"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrCountWithStrNeedleSubClassTypeReturnsSubstr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Sub(str): pass
result = Sub("value u")
count = result.count('u')
)")
                   .isError());
  HandleScope scope(thread_);
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(isIntEqualsWord(*count, 2));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrCountWithSubNeedleSubClassTypeReturnsSubstr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Sub(str): pass
result = Sub("value")
count = result.count(result)
)")
                   .isError());
  HandleScope scope(thread_);
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(isIntEqualsWord(*count, 1));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrGetitemWithSubClassTypeReturnsSubstr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Sub(str): pass
result = Sub("value")
result = result[:2]
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "va"));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrArrayClearSetsNumItemsToZero) {
  HandleScope scope(thread_);
  StrArray self(&scope, runtime_->newStrArray());
  Str other(&scope, runtime_->newStrFromCStr("hello"));
  runtime_->strArrayAddStr(thread_, self, other);
  ASSERT_EQ(self.numItems(), 5);
  EXPECT_TRUE(runBuiltin(FUNC(_builtins, _str_array_clear), self).isNoneType());
  EXPECT_EQ(self.numItems(), 0);

  // Make sure that str does not show up again
  other = runtime_->newStrFromCStr("abcd");
  runtime_->strArrayAddStr(thread_, self, other);
  EXPECT_TRUE(isStrEqualsCStr(runtime_->strFromStrArray(self), "abcd"));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrArrayIaddWithStrReturnsStrArray) {
  HandleScope scope(thread_);
  StrArray self(&scope, runtime_->newStrArray());
  const char* test_str = "hello";
  Str other(&scope, runtime_->newStrFromCStr(test_str));
  StrArray result(&scope,
                  runBuiltin(FUNC(_builtins, _str_array_iadd), self, other));
  EXPECT_TRUE(isStrEqualsCStr(runtime_->strFromStrArray(result), test_str));
  EXPECT_EQ(self, result);
}

TEST_F(UnderBuiltinsModuleTest, UnderStrJoinWithNonStrRaisesTypeError) {
  HandleScope scope(thread_);
  Str sep(&scope, runtime_->newStrFromCStr(","));
  Object obj1(&scope, runtime_->newStrFromCStr("foo"));
  Object obj2(&scope, runtime_->newInt(4));
  Object obj3(&scope, runtime_->newStrFromCStr("bar"));
  Tuple elts(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(_builtins, _str_join), sep, elts), LayoutId::kTypeError,
      "sequence item 1: expected str instance, int found"));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnSingleCharStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("l"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "lo"));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnMultiCharStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("ll"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnExistingSuffix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("lo"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnNonExistentSuffix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("lop"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnExistingPrefix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("he"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "llo"));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnNonExistentPrefix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("hex"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionLargerStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("abcdefghijk"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionEmptyStr) {
  HandleScope scope(thread_);
  Str str(&scope, Str::empty());
  Str sep(&scope, runtime_->newStrFromCStr("a"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_partition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionOnSingleCharStrPartitionsCorrectly) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("l"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionOnMultiCharStrPartitionsCorrectly) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("ll"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionOnSuffixPutsEmptyStrAtEndOfResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("lo"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest,
       RpartitionOnNonExistentSuffixPutsStrAtEndOfResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("lop"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(UnderBuiltinsModuleTest,
       RpartitionOnPrefixPutsEmptyStrAtBeginningOfResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("he"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "llo"));
}

TEST_F(UnderBuiltinsModuleTest,
       RpartitionOnNonExistentPrefixPutsStrAtEndOfResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("hex"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionLargerStrPutsStrAtEndOfResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("foobarbaz"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionEmptyStrReturnsTupleOfEmptyStrings) {
  HandleScope scope(thread_);
  Str str(&scope, Str::empty());
  Str sep(&scope, runtime_->newStrFromCStr("a"));
  Tuple result(&scope, runBuiltin(FUNC(_builtins, _str_rpartition), str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWithStrEqualsSepReturnsTwoEmptyStrings) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("haystack"));
  Str sep(&scope, runtime_->newStrFromCStr("haystack"));
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope,
              runBuiltin(FUNC(_builtins, _str_split), str, sep, maxsplit));
  EXPECT_PYLIST_EQ(result, {"", ""});
}

TEST_F(UnderBuiltinsModuleTest, UnderStrSplitWithSepNotInStrReturnsListOfStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("haystack"));
  Str sep(&scope, runtime_->newStrFromCStr("foobar"));
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope,
              runBuiltin(FUNC(_builtins, _str_split), str, sep, maxsplit));
  ASSERT_EQ(result.numItems(), 1);
  EXPECT_EQ(result.at(0), *str);
}

TEST_F(UnderBuiltinsModuleTest, UnderStrSplitWithSepInUnderStrSplitsOnSep) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello world hello world"));
  Str sep(&scope, runtime_->newStrFromCStr(" w"));
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope,
              runBuiltin(FUNC(_builtins, _str_split), str, sep, maxsplit));
  EXPECT_PYLIST_EQ(result, {"hello", "orld hello", "orld"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWithSepInUnderStrSplitsOnSepMaxsplit) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("a b c d e"));
  Str sep(&scope, runtime_->newStrFromCStr(" "));
  Int maxsplit(&scope, SmallInt::fromWord(2));
  List result(&scope,
              runBuiltin(FUNC(_builtins, _str_split), str, sep, maxsplit));
  EXPECT_PYLIST_EQ(result, {"a", "b", "c d e"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWithSepInUnderStrSplitsOnSepMaxsplitZero) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("a b c d e"));
  Str sep(&scope, runtime_->newStrFromCStr(" "));
  Int maxsplit(&scope, SmallInt::fromWord(0));
  List result(&scope,
              runBuiltin(FUNC(_builtins, _str_split), str, sep, maxsplit));
  EXPECT_PYLIST_EQ(result, {"a b c d e"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWhitespaceSplitsOnUnicodeWhitespace) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(u8"a  \u3000 \t  b\u205fc"));
  Object sep(&scope, NoneType::object());
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope,
              runBuiltin(FUNC(_builtins, _str_split), str, sep, maxsplit));
  EXPECT_PYLIST_EQ(result, {"a", "b", "c"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWhitespaceSplitsOnWhitespaceAtEnd) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("a   \t  b c  "));
  Object sep(&scope, NoneType::object());
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope,
              runBuiltin(FUNC(_builtins, _str_split), str, sep, maxsplit));
  EXPECT_PYLIST_EQ(result, {"a", "b", "c"});
}

TEST_F(UnderBuiltinsModuleTest, UnderTupleCheckExactWithExactTupleReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->emptyTuple());
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _tuple_check_exact), obj),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderTupleCheckExactWithTupleSubclassReturnsFalse) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(tuple):
  pass
obj = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  EXPECT_EQ(runBuiltin(FUNC(_builtins, _tuple_check_exact), obj),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleDeathTest, UnderUnimplementedAbortsProgram) {
  ASSERT_DEATH(static_cast<void>(runFromCStr(runtime_, "_unimplemented()")),
               ".*'_unimplemented' called.");
}

TEST_F(UnderBuiltinsModuleDeathTest, UnderUnimplementedPrintsFunctionName) {
  ASSERT_DEATH(static_cast<void>(runFromCStr(runtime_, R"(
def foobar():
  _unimplemented()
foobar()
)")),
               ".*'_unimplemented' called in function 'foobar'.");
}

}  // namespace testing
}  // namespace py
