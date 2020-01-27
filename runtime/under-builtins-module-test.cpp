#include "under-builtins-module.h"

#include <cmath>

#include "gtest/gtest.h"

#include "builtins-module.h"
#include "bytearray-builtins.h"
#include "dict-builtins.h"
#include "int-builtins.h"
#include "module-builtins.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"

namespace py {

using namespace testing;

using UnderBuiltinsModuleTest = RuntimeFixture;
using UnderBuiltinsModuleDeathTest = RuntimeFixture;

static RawObject createDummyBuiltinFunction(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function::Entry entry = UnderBuiltinsModule::underIntCheck;
  Object name(&scope, runtime->symbols()->UnderIntCheck());
  Tuple parameter_names(&scope, runtime->newTuple(1));
  parameter_names.atPut(0, runtime->symbols()->Self());
  Code code(&scope,
            runtime->newBuiltinCode(/*argcount=*/1, /*posonlyargcount=*/0,
                                    /*kwonlyargcount=*/0,
                                    /*flags=*/0, entry, parameter_names, name));
  Object module(&scope, NoneType::object());
  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, module));
  function.setIntrinsicId(static_cast<word>(SymbolId::kUnderIntCheck));
  return *function;
}

TEST_F(UnderBuiltinsModuleTest, CopyFunctionEntriesCopies) {
  HandleScope scope(thread_);
  Function function(&scope, createDummyBuiltinFunction(thread_));

  ASSERT_FALSE(runFromCStr(runtime_, R"(
def _int_check(self):
  "docstring"
  pass
)")
                   .isError());
  Function python_func(&scope, mainModuleAt(runtime_, "_int_check"));
  copyFunctionEntries(Thread::current(), function, python_func);
  Code base_code(&scope, function.code());
  Code patch_code(&scope, python_func.code());
  EXPECT_EQ(patch_code.code(), base_code.code());
  EXPECT_EQ(python_func.entry(), &builtinTrampoline);
  EXPECT_EQ(python_func.entryKw(), &builtinTrampolineKw);
  EXPECT_EQ(python_func.entryEx(), &builtinTrampolineEx);
  EXPECT_TRUE(isSymbolIdEquals(static_cast<SymbolId>(python_func.intrinsicId()),
                               SymbolId::kUnderIntCheck));
}

TEST_F(UnderBuiltinsModuleDeathTest, CopyFunctionEntriesRedefinitionDies) {
  HandleScope scope(thread_);
  Function function(&scope, createDummyBuiltinFunction(thread_));

  ASSERT_FALSE(runFromCStr(runtime_, R"(
def _int_check(self):
  return True
)")
                   .isError());
  Function python_func(&scope, mainModuleAt(runtime_, "_int_check"));
  ASSERT_DEATH(
      copyFunctionEntries(Thread::current(), function, python_func),
      "Redefinition of native code method '_int_check' in managed code");
}

TEST_F(UnderBuiltinsModuleTest, UnderBytearrayClearSetsLengthToZero) {
  HandleScope scope(thread_);
  ByteArray array(&scope, runtime_->newByteArray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->byteArrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  ASSERT_FALSE(
      runBuiltin(UnderBuiltinsModule::underBytearrayClear, array).isError());
  EXPECT_EQ(array.numItems(), 0);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithEmptyBytearrayReturnsFalse) {
  HandleScope scope(thread_);
  ByteArray array(&scope, runtime_->newByteArray());
  ASSERT_EQ(array.numItems(), 0);
  Object key(&scope, SmallInt::fromWord('a'));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underBytearrayContains, array, key),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithIntBiggerThanCharRaisesValueError) {
  HandleScope scope(thread_);
  ByteArray array(&scope, runtime_->newByteArray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->byteArrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  Object key(&scope, SmallInt::fromWord(256));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underBytearrayContains, array, key),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithByteInBytearrayReturnsTrue) {
  HandleScope scope(thread_);
  ByteArray array(&scope, runtime_->newByteArray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->byteArrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  Object key(&scope, SmallInt::fromWord('2'));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underBytearrayContains, array, key),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayContainsWithByteNotInBytearrayReturnsFalse) {
  HandleScope scope(thread_);
  ByteArray array(&scope, runtime_->newByteArray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_->byteArrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  Object key(&scope, SmallInt::fromWord('x'));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underBytearrayContains, array, key),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest, UnderBytearrayDelitemDeletesItemAtIndex) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  byteArrayAdd(thread_, runtime_, self, 'b');
  byteArrayAdd(thread_, runtime_, self, 'c');
  byteArrayAdd(thread_, runtime_, self, 'd');
  byteArrayAdd(thread_, runtime_, self, 'e');
  Int idx(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(runBuiltin(UnderBuiltinsModule::underBytearrayDelitem, self, idx)
                  .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "abde"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayDelsliceWithStepEqualsOneAndNoGrowthDeletesSlice) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  byteArrayAdd(thread_, runtime_, self, 'b');
  byteArrayAdd(thread_, runtime_, self, 'c');
  byteArrayAdd(thread_, runtime_, self, 'd');
  byteArrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runBuiltin(UnderBuiltinsModule::underBytearrayDelslice, self,
                         start, stop, step)
                  .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "de"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayDelsliceWithStepEqualsTwoAndNoGrowthDeletesSlice) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  byteArrayAdd(thread_, runtime_, self, 'b');
  byteArrayAdd(thread_, runtime_, self, 'c');
  byteArrayAdd(thread_, runtime_, self, 'd');
  byteArrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(runBuiltin(UnderBuiltinsModule::underBytearrayDelslice, self,
                         start, stop, step)
                  .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "bde"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayJoinWithEmptyIterableReturnsEmptyByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  Object iter(&scope, runtime_->emptyTuple());
  Object result(
      &scope, runBuiltin(UnderBuiltinsModule::underBytearrayJoin, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayJoinWithEmptySeparatorReturnsByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  Tuple iter(&scope, runtime_->newTuple(3));
  iter.atPut(0, runtime_->newBytes(1, 'A'));
  iter.atPut(1, runtime_->newBytes(2, 'B'));
  iter.atPut(2, runtime_->newBytes(1, 'A'));
  Object result(
      &scope, runBuiltin(UnderBuiltinsModule::underBytearrayJoin, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ABBA"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearrayJoinWithNonEmptyReturnsByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, ' ');
  List iter(&scope, runtime_->newList());
  Bytes value(&scope, runtime_->newBytes(1, '*'));
  runtime_->listAdd(thread_, iter, value);
  runtime_->listAdd(thread_, iter, value);
  runtime_->listAdd(thread_, iter, value);
  Object result(
      &scope, runBuiltin(UnderBuiltinsModule::underBytearrayJoin, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "* * *"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithLargeIntRaisesIndexError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  Int key(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underBytearraySetitem, self, key, value),
      LayoutId::kIndexError, "cannot fit 'int' into an index-sized integer"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithKeyLargerThanMaxIndexRaisesIndexError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(self.numItems()));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underBytearraySetitem, self, key, value),
      LayoutId::kIndexError, "index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithNegativeValueRaisesValueError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(0));
  Int value(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underBytearraySetitem, self, key, value),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderBytearraySetitemWithKeySmallerThanNegativeLengthValueRaisesValueError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(-self.numItems() - 1));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underBytearraySetitem, self, key, value),
      LayoutId::kIndexError, "index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithValueGreaterThanKMaxByteRaisesValueError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, ' ');
  Int key(&scope, runtime_->newInt(0));
  Int value(&scope, SmallInt::fromWord(kMaxByte + 1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underBytearraySetitem, self, key, value),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithNegativeKeyIndexesBackwards) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  byteArrayAdd(thread_, runtime_, self, 'b');
  byteArrayAdd(thread_, runtime_, self, 'c');
  Int key(&scope, SmallInt::fromWord(-1));
  Int value(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(
      runBuiltin(UnderBuiltinsModule::underBytearraySetitem, self, key, value)
          .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "ab\001"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetitemWithNegativeKeySetsItemAtIndex) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  byteArrayAdd(thread_, runtime_, self, 'b');
  byteArrayAdd(thread_, runtime_, self, 'c');
  Int key(&scope, SmallInt::fromWord(1));
  Int value(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(
      runBuiltin(UnderBuiltinsModule::underBytearraySetitem, self, key, value)
          .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "a\001c"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetsliceWithStepEqualsOneAndNoGrowthSetsSlice) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  byteArrayAdd(thread_, runtime_, self, 'b');
  byteArrayAdd(thread_, runtime_, self, 'c');
  byteArrayAdd(thread_, runtime_, self, 'd');
  byteArrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  ByteArray value(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, value, 'A');
  byteArrayAdd(thread_, runtime_, value, 'B');
  byteArrayAdd(thread_, runtime_, value, 'C');
  EXPECT_TRUE(runBuiltin(UnderBuiltinsModule::underBytearraySetslice, self,
                         start, stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "ABCde"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytearraySetsliceWithStepEqualsTwoAndNoGrowthSetsSlice) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, self, 'a');
  byteArrayAdd(thread_, runtime_, self, 'b');
  byteArrayAdd(thread_, runtime_, self, 'c');
  byteArrayAdd(thread_, runtime_, self, 'd');
  byteArrayAdd(thread_, runtime_, self, 'e');
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(2));
  ByteArray value(&scope, runtime_->newByteArray());
  byteArrayAdd(thread_, runtime_, value, 'A');
  byteArrayAdd(thread_, runtime_, value, 'B');
  EXPECT_TRUE(runBuiltin(UnderBuiltinsModule::underBytearraySetslice, self,
                         start, stop, step, value)
                  .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "AbBde"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytesContainsWithIntBiggerThanCharRaisesValueError) {
  HandleScope scope(thread_);
  const byte contents[] = {'1', '2', '3'};
  Bytes bytes(&scope, runtime_->newBytesWithAll(contents));
  ASSERT_EQ(bytes.length(), 3);
  Object key(&scope, SmallInt::fromWord(256));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underBytesContains, bytes, key),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesContainsWithByteInBytesReturnsTrue) {
  HandleScope scope(thread_);
  const byte contents[] = {'1', '2', '3'};
  Bytes bytes(&scope, runtime_->newBytesWithAll(contents));
  ASSERT_EQ(bytes.length(), 3);
  Object key(&scope, SmallInt::fromWord('2'));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underBytesContains, bytes, key),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytesContainsWithByteNotInBytesReturnsFalse) {
  HandleScope scope(thread_);
  const byte contents[] = {'1', '2', '3'};
  Bytes bytes(&scope, runtime_->newBytesWithAll(contents));
  ASSERT_EQ(bytes.length(), 3);
  Object key(&scope, SmallInt::fromWord('x'));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underBytesContains, bytes, key),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytesJoinWithEmptyIterableReturnsEmptyByteArray) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime_->newBytes(3, 'a'));
  Object iter(&scope, runtime_->emptyTuple());
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithEmptySeparatorReturnsBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, Bytes::empty());
  Tuple iter(&scope, runtime_->newTuple(3));
  iter.atPut(0, runtime_->newBytes(1, 'A'));
  iter.atPut(1, runtime_->newBytes(2, 'B'));
  iter.atPut(2, runtime_->newBytes(1, 'A'));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
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
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
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
  Tuple iter(&scope, runtime_->newTuple(2));
  iter.atPut(0, mainModuleAt(runtime_, "ac"));
  iter.atPut(1, mainModuleAt(runtime_, "dc"));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "AC-DC"));
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
  ASSERT_TRUE(raised(
      runBuiltin(UnderBuiltinsModule::underDictGet, dict, key, default_obj),
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
  EXPECT_EQ(
      runBuiltin(UnderBuiltinsModule::underDictGet, dict, key, default_obj),
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
  dictAtPut(thread_, dict, key, hash, value);
  Object dflt(&scope, runtime_->newInt(789));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underDictGet, dict, key, dflt));
  EXPECT_TRUE(isIntEqualsWord(*result, 456));
}

TEST_F(UnderBuiltinsModuleTest, UnderDictGetWithNonDictRaisesTypeError) {
  HandleScope scope(thread_);
  Object foo(&scope, runtime_->newInt(123));
  Object bar(&scope, runtime_->newInt(456));
  Object baz(&scope, runtime_->newInt(789));
  EXPECT_TRUE(
      raised(runBuiltin(UnderBuiltinsModule::underDictGet, foo, bar, baz),
             LayoutId::kTypeError));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderDictPopitemRemovesAvailableItemAndReturnsTupleOfKeyAndValue) {
  HandleScope scope(thread_);
  // Create {"a": 1, "b": 2}.
  Dict dict(&scope, runtime_->newDict());
  Str a(&scope, runtime_->newStrFromCStr("a"));
  Object a_value(&scope, SmallInt::fromWord(1));
  Str b(&scope, runtime_->newStrFromCStr("b"));
  Object b_value(&scope, SmallInt::fromWord(2));
  dictAtPutByStr(thread_, dict, a, a_value);
  dictAtPutByStr(thread_, dict, b, b_value);

  Tuple result(&scope, runBuiltin(UnderBuiltinsModule::underDictPopitem, dict));
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "a"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
  EXPECT_TRUE(dictAtByStr(thread_, dict, a).isErrorNotFound());
  EXPECT_EQ(dict.numItems(), 1);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderDictPopitemReturnsNoneTypeWhenNoItemIsAvailable) {
  HandleScope scope(thread_);
  // Create {}.
  Dict dict(&scope, runtime_->newDict());
  ASSERT_EQ(dict.numItems(), 0);
  EXPECT_TRUE(
      runBuiltin(UnderBuiltinsModule::underDictPopitem, dict).isNoneType());
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

  Object result(&scope, runBuiltin(UnderBuiltinsModule::underDictSetitem, dict,
                                   key, val2));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dict.numUsableItems(), 5 - 1);
  ASSERT_EQ(dictAtByStr(thread_, dict, key), *val2);
}

TEST_F(UnderBuiltinsModuleTest, UnderDictSetitemWithNonExistentKey) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDictWithSize(1));
  ASSERT_EQ(dict.numItems(), 0);
  ASSERT_EQ(dict.numUsableItems(), 5);
  Str key(&scope, runtime_->newStrFromCStr("foo"));
  Object val(&scope, runtime_->newInt(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underDictSetitem, dict,
                                   key, val));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(dict.numItems(), 1);
  ASSERT_EQ(dict.numUsableItems(), 5 - 1);
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
  Object result1(&scope, runBuiltin(UnderBuiltinsModule::underDictSetitem, dict,
                                    key, value));
  EXPECT_TRUE(result1.isNoneType());
  Object result2(&scope, dictAtByStr(thread_, dict, key));
  ASSERT_TRUE(result2.isStr());
  EXPECT_EQ(Str::cast(*result2), *value);
}

TEST_F(UnderBuiltinsModuleTest, UnderDivmodReturnsQuotientAndDividend) {
  HandleScope scope(thread_);
  Int number(&scope, SmallInt::fromWord(1234));
  Int divisor(&scope, SmallInt::fromWord(-5));
  Object tuple_obj(
      &scope, runBuiltin(UnderBuiltinsModule::underDivmod, number, divisor));
  ASSERT_TRUE(tuple_obj.isTuple());
  Tuple tuple(&scope, *tuple_obj);
  ASSERT_EQ(tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), -247));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), -1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderGetFrameLocalsInModuleScopeReturnsModuleProxy) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = _getframe_locals(0)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isModuleProxy());
  EXPECT_EQ(ModuleProxy::cast(*result).module(), findMainModule(runtime_));
}

TEST_F(UnderBuiltinsModuleTest, UnderFloatDivmodReturnsQuotientAndRemainder) {
  HandleScope scope(thread_);
  Float number(&scope, runtime_->newFloat(3.25));
  Float divisor(&scope, runtime_->newFloat(1.0));
  Tuple result(&scope, runBuiltin(UnderBuiltinsModule::underFloatDivmod, number,
                                  divisor));
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
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underFloatDivmod, number, divisor),
      LayoutId::kZeroDivisionError, "float divmod()"));
}

TEST_F(UnderBuiltinsModuleTest, UnderFloatDivmodWithNanReturnsNan) {
  HandleScope scope(thread_);
  Float number(&scope, runtime_->newFloat(3.25));
  double nan = std::numeric_limits<double>::quiet_NaN();
  ASSERT_TRUE(std::isnan(nan));
  Float divisor(&scope, runtime_->newFloat(nan));
  Tuple result(&scope, runBuiltin(UnderBuiltinsModule::underFloatDivmod, number,
                                  divisor));
  ASSERT_EQ(result.length(), 2);
  Float quotient(&scope, result.at(0));
  Float remainder(&scope, result.at(1));
  EXPECT_TRUE(std::isnan(quotient.value()));
  EXPECT_TRUE(std::isnan(remainder.value()));
}

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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectKeys, i));
  ASSERT_TRUE(result.isList());
  EXPECT_EQ(List::cast(*result).numItems(), 0);
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
      &scope,
      runBuiltin(UnderBuiltinsModule::underInstanceOverflowDict, instance));
  ASSERT_EQ(instance.layoutId(), layout.id());
  Object overflow_dict(&scope, instance.instanceVariableAt(offset));
  EXPECT_TRUE(result0.isDict());
  EXPECT_EQ(result0, overflow_dict);

  Object result1(
      &scope,
      runBuiltin(UnderBuiltinsModule::underInstanceOverflowDict, instance));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
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
  Int result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                int_type, bytes, byteorder_big, signed_arg));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
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
  Int result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                int_type, bytes, byteorder_big, signed_arg));
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
      &scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes, int_type,
                         bytes, bo_big_false, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result_little, 0));

  Bool bo_big_true(&scope, Bool::trueObj());
  Object result_big(&scope,
                    runBuiltin(UnderBuiltinsModule::underIntFromBytes, int_type,
                               bytes, bo_big_true, signed_arg));
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
  Int result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                int_type, bytes, byteorder_big, signed_arg));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
  const uword expected_digits[] = {0xbe0123456789abcd, 0xffffffffffcafeba};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytearrayWithZeroBaseReturnsCodeLiteral) {
  HandleScope scope(thread_);
  const byte view[] = {'0', 'x', 'b', 'a', '5', 'e'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underIntNewFromBytearray, type,
                           array, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xba5e));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytearrayWithInvalidByteRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'$'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytearray, type, array,
                 base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytearrayWithInvalidLiteraRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'a'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime_->newByteArray());
  runtime_->byteArrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytearray, type, array,
                 base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'a'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithInvalidByteRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'$'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithInvalidLiteralRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'8', '6'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(7));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 7: b'86'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithBytesSubclassReturnsSmallInt) {
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      SmallInt::fromWord(86));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromBytesWithZero) {
  HandleScope scope(thread_);
  const byte src[] = {'0'};
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromBytes,
                                   type, bytes, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromBytesWithLargeInt) {
  HandleScope scope(thread_);
  const byte src[] = "1844674407370955161500";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromBytes,
                                   type, bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0xffffffffffffff9c, 0x63};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromBytesWithLargeInt2) {
  HandleScope scope(thread_);
  const byte src[] = "46116860184273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromBytes,
                                   type, bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0x7ffffffffffffff6, 0x2};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithLargeIntWithInvalidDigitRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "461168601$84273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 10: b'461168601$84273879030'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromBytesWithLeadingPlusReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "+46116860184273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Int result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type,
                                bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0x7ffffffffffffff6, 0x2};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithDoubleLeadingPlusRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "++1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'++1'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithLeadingNegAndSpaceReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "   -46116860184273879030";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  Int result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type,
                                bytes, base));
  ASSERT_FALSE(result.isError());
  EXPECT_TRUE(result.isInt());
  const uword digits[] = {0x800000000000000a, 0xfffffffffffffffd};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithDoubleLeadingNegRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "--1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'--1'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithHexPrefixAndBaseZeroReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "0x1f";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_EQ(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      SmallInt::fromWord(31));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithHexPrefixAndBaseSixteenReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "0x1f";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_EQ(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      SmallInt::fromWord(31));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithHexPrefixAndBaseNineRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "0x1f";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(9));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 9: b'0x1f'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromBytesWithBaseThreeReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "221";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(3));
  EXPECT_EQ(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      SmallInt::fromWord(25));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromBytesWithUnderscoreReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "1_000_000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_EQ(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      SmallInt::fromWord(1000000));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithLeadingUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "_1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 0: b'_1'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithLeadingUnderscoreAndPrefixAndBaseReturnsInt) {
  HandleScope scope(thread_);
  const byte src[] = "0b_1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(2));
  EXPECT_EQ(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      SmallInt::fromWord(1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithTrailingUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "1_000_";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 0: b'1_000_'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithDoubleUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const byte src[] = "1__000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_->newBytesWithAll({src, ARRAYSIZE(src) - 1}));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 0: b'1__000'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromIntWithBoolReturnsSmallInt) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_->typeAt(LayoutId::kInt));
  Object fls(&scope, Bool::falseObj());
  Object tru(&scope, Bool::trueObj());
  Object false_result(
      &scope, runBuiltin(UnderBuiltinsModule::underIntNewFromInt, type, fls));
  Object true_result(
      &scope, runBuiltin(UnderBuiltinsModule::underIntNewFromInt, type, tru));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: 'g'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeInt) {
  HandleScope scope(thread_);
  const char* src = "1844674407370955161500";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '-'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLengthOneInfersBase10) {
  HandleScope scope(thread_);
  const char* src = "8";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 8));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLengthOneBase10) {
  HandleScope scope(thread_);
  const char* src = "8";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 8));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseTwo) {
  HandleScope scope(thread_);
  const char* src = "100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 4));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntInfersBaseTen) {
  HandleScope scope(thread_);
  const char* src = "100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLeadingSpacesRemovesSpaces) {
  HandleScope scope(thread_);
  const char* src = "      100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '    '"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithPlusReturnsPositiveInt) {
  HandleScope scope(thread_);
  const char* src = "+100";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 16: '++100'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseEight) {
  HandleScope scope(thread_);
  const char* src = "0o77712371237123712371237123777";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(8));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  const uword digits[] = {0xa7ca7ca7ca7ca7ff, 0x7fca7c};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntInfersBaseEight) {
  HandleScope scope(thread_);
  const char* src = "0o77712371237123712371237123777";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '0x_'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoreIgnoresUnderscore) {
  HandleScope scope(thread_);
  const char* src = "0x_deadbeef";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xdeadbeef));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoresIgnoresUnderscoresBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "0x_d_e_a_d_b_eef";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xdeadbeef));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoresIgnoresUnderscoresBaseTen) {
  HandleScope scope(thread_);
  const char* src = "100_000_000_000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '_abc'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithNegativeZeroReturnsZero) {
  HandleScope scope(thread_);
  const char* src = "-0";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 16: '--100'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithPositiveZeroReturnsZero) {
  HandleScope scope(thread_);
  const char* src = "+0";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithEmptyStringRaisesValueError) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, Str::empty());
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: 'a'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "0x8000000000000000";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseTwoReturnsOne) {
  HandleScope scope(thread_);
  const char* src = "0b1";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 177));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseSixteenReturnsEleven) {
  HandleScope scope(thread_);
  const char* src = "0b";
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Str str(&scope, runtime_->newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
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
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 8: '0b1'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderListCheckExactWithExactListReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newList());
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underListCheckExact, obj),
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
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underListCheckExact, obj),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNegativeIndexRemovesRelativeToEnd) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelitem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelitemWithLastIndexRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelitem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithFirstIndexRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelitem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNegativeFirstIndexRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-2));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelitem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNegativeLastIndexRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelitem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelitemWithNumberGreaterThanSmallIntMaxDoesNotCrash) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int big(&scope, runtime_->newInt(SmallInt::kMaxValue + 100));
  EXPECT_TRUE(
      raised(runBuiltin(UnderBuiltinsModule::underListDelitem, list, big),
             LayoutId::kIndexError));
  EXPECT_PYLIST_EQ(list, {0, 1});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelsliceRemovesItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelsliceRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelsliceRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(2));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
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
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
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
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
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
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
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
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
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
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
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
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelslice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1, 2, 3, 4});
}

TEST_F(UnderBuiltinsModuleTest, UnderListGetitemWithNegativeIndex) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underListGetitem, list, idx));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListGetitemWithInvalidNegativeIndexRaisesIndexError) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-4));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underListGetitem, list, idx),
      LayoutId::kIndexError, "list index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListGetitemWithInvalidPositiveIndexRaisesIndexError) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underListGetitem, list, idx),
      LayoutId::kIndexError, "list index out of range"));
}

TEST_F(UnderBuiltinsModuleTest, UnderListSwapSwapsItemsAtIndices) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 4));
  Object i(&scope, SmallInt::fromWord(1));
  Object j(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(
      runBuiltin(UnderBuiltinsModule::underListSwap, list, i, j).isNoneType());
  EXPECT_PYLIST_EQ(list, {0, 2, 1, 3});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewItemsizeWithNonMemoryViewRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_memoryview(&scope, runtime_->newInt(12));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underMemoryviewItemsize, not_memoryview),
      LayoutId::kTypeError,
      "'<anonymous>' requires a 'memoryview' object but got 'int'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewItemsizeReturnsSizeOfMemoryItems) {
  HandleScope scope(thread_);
  Bytes bytes(&scope, runtime_->newBytes(5, 'x'));
  MemoryView view(
      &scope, runtime_->newMemoryView(thread_, bytes, 5, ReadOnly::ReadOnly));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underMemoryviewItemsize, view));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderMemoryViewNbytesWithNonMemoryViewRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_memoryview(&scope, runtime_->newInt(12));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underMemoryviewNbytes, not_memoryview),
      LayoutId::kTypeError,
      "'<anonymous>' requires a 'memoryview' object but got 'int'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderMemoryViewNbytesReturnsSizeOfMemoryView) {
  HandleScope scope(thread_);
  Bytes bytes(&scope, runtime_->newBytes(5, 'x'));
  MemoryView view(
      &scope, runtime_->newMemoryView(thread_, bytes, 5, ReadOnly::ReadOnly));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underMemoryviewNbytes, view));
  EXPECT_TRUE(isIntEqualsWord(*result, 5));
}

TEST_F(UnderBuiltinsModuleTest, UnderModuleDirListWithFilteredOutPlaceholders) {
  HandleScope scope(thread_);
  Str module_name(&scope, runtime_->newStrFromCStr("module"));
  Module module(&scope, runtime_->newModule(module_name));
  module.setDict(runtime_->newDict());

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Str value(&scope, runtime_->newStrFromCStr("value"));

  moduleAtPut(thread_, module, foo, value);
  moduleAtPut(thread_, module, bar, value);
  moduleAtPut(thread_, module, baz, value);

  ValueCell::cast(moduleValueCellAt(thread_, module, bar)).makePlaceholder();

  List keys(&scope, runBuiltin(UnderBuiltinsModule::underModuleDir, module));
  EXPECT_EQ(keys.numItems(), 2);
  EXPECT_EQ(keys.at(0), *foo);
  EXPECT_EQ(keys.at(1), *baz);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithNonexistentAttrReturnsFalse) {
  HandleScope scope(thread_);
  Object obj(&scope, SmallInt::fromWord(0));
  Str name(&scope, runtime_->newStrFromCStr("__foo_bar_baz__"));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
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
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
  EXPECT_EQ(result, Bool::trueObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest, UnderOsWriteWithBadFdRaisesOSError) {
  HandleScope scope(thread_);
  Int fd(&scope, SmallInt::fromWord(-1));
  const byte buf[] = {0x1, 0x2};
  Bytes bytes_buf(&scope, runtime_->newBytesWithAll(buf));
  EXPECT_TRUE(
      raised(runBuiltin(UnderBuiltinsModule::underOsWrite, fd, bytes_buf),
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
  EXPECT_TRUE(
      raised(runBuiltin(UnderBuiltinsModule::underOsWrite, fd, bytes_buf),
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
  Object result_obj(
      &scope, runBuiltin(UnderBuiltinsModule::underOsWrite, fd, bytes_buf));
  EXPECT_TRUE(isIntEqualsWord(*result_obj, count));
  ::close(fds[1]);  // Send EOF
  std::unique_ptr<char[]> buf(new char[count + 1]{0});
  result = ::read(fds[0], buf.get(), count);
  EXPECT_EQ(result, count);
  EXPECT_STREQ(buf.get(), reinterpret_cast<char*>(to_write));
  ::close(fds[0]);
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithBadPatchFuncRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_func(&scope, runtime_->newInt(12));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(UnderBuiltinsModule::underPatch, not_func),
                    LayoutId::kTypeError, "_patch expects function argument"));
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithMissingFuncRaisesAttributeError) {
  HandleScope scope(thread_);

  Object module_name(&scope, runtime_->newStrFromCStr("foo"));
  Module module(&scope, runtime_->newModule(module_name));
  runtime_->addModule(module);

  Object name(&scope, runtime_->newStrFromCStr("bar"));
  Code code(&scope, newEmptyCode());
  code.setName(*name);
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));

  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underPatch, function),
      LayoutId::kAttributeError, "function bar not found in module foo"));
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithBadBaseFuncRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
not_a_function = 1234

@_patch
def not_a_function():
  pass
)"),
                            LayoutId::kTypeError,
                            "_patch can only patch functions"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrCountWithStartAndEndSearchesWithinBounds) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("ofoodo"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  Object start(&scope, SmallInt::fromWord(2));
  Object end(&scope, SmallInt::fromWord(4));
  EXPECT_TRUE(isIntEqualsWord(runBuiltin(UnderBuiltinsModule::underStrCount,
                                         haystack, needle, start, end),
                              2));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrCountWithNoneStartStartsFromZero) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("foo"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  Object start(&scope, NoneType::object());
  Object end(&scope, SmallInt::fromWord(haystack.codePointLength()));
  EXPECT_TRUE(isIntEqualsWord(runBuiltin(UnderBuiltinsModule::underStrCount,
                                         haystack, needle, start, end),
                              2));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrCountWithNoneEndSetsEndToHaystackLength) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("foo"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  Object start(&scope, SmallInt::fromWord(0));
  Object end(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(runBuiltin(UnderBuiltinsModule::underStrCount,
                                         haystack, needle, start, end),
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

TEST_F(UnderBuiltinsModuleTest, UnderStrarrayClearSetsNumItemsToZero) {
  HandleScope scope(thread_);
  StrArray self(&scope, runtime_->newStrArray());
  Str other(&scope, runtime_->newStrFromCStr("hello"));
  runtime_->strArrayAddStr(thread_, self, other);
  ASSERT_EQ(self.numItems(), 5);
  EXPECT_TRUE(
      runBuiltin(UnderBuiltinsModule::underStrarrayClear, self).isNoneType());
  EXPECT_EQ(self.numItems(), 0);

  // Make sure that str does not show up again
  other = runtime_->newStrFromCStr("abcd");
  runtime_->strArrayAddStr(thread_, self, other);
  EXPECT_TRUE(isStrEqualsCStr(runtime_->strFromStrArray(self), "abcd"));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrarrayIaddWithStrReturnsStrArray) {
  HandleScope scope(thread_);
  StrArray self(&scope, runtime_->newStrArray());
  const char* test_str = "hello";
  Str other(&scope, runtime_->newStrFromCStr(test_str));
  StrArray result(
      &scope, runBuiltin(UnderBuiltinsModule::underStrarrayIadd, self, other));
  EXPECT_TRUE(isStrEqualsCStr(runtime_->strFromStrArray(result), test_str));
  EXPECT_EQ(self, result);
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnSingleCharStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("l"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "lo"));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnMultiCharStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("ll"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnExistingSuffix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("lo"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnNonExistentSuffix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("lop"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnExistingPrefix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("he"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "llo"));
}

TEST_F(UnderBuiltinsModuleTest, PartitionOnNonExistentPrefix) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("hex"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionLargerStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("abcdefghijk"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, PartitionEmptyStr) {
  HandleScope scope(thread_);
  Str str(&scope, Str::empty());
  Str sep(&scope, runtime_->newStrFromCStr("a"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrPartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionOnSingleCharStrPartitionsCorrectly) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("l"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionOnMultiCharStrPartitionsCorrectly) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("ll"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionOnSuffixPutsEmptyStrAtEndOfResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("lo"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
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
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
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
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
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
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionLargerStrPutsStrAtEndOfResult) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Str sep(&scope, runtime_->newStrFromCStr("foobarbaz"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(UnderBuiltinsModuleTest, RpartitionEmptyStrReturnsTupleOfEmptyStrings) {
  HandleScope scope(thread_);
  Str str(&scope, Str::empty());
  Str sep(&scope, runtime_->newStrFromCStr("a"));
  Tuple result(&scope,
               runBuiltin(UnderBuiltinsModule::underStrRpartition, str, sep));
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
  List result(&scope, runBuiltin(UnderBuiltinsModule::underStrSplit, str, sep,
                                 maxsplit));
  EXPECT_PYLIST_EQ(result, {"", ""});
}

TEST_F(UnderBuiltinsModuleTest, UnderStrSplitWithSepNotInStrReturnsListOfStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("haystack"));
  Str sep(&scope, runtime_->newStrFromCStr("foobar"));
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope, runBuiltin(UnderBuiltinsModule::underStrSplit, str, sep,
                                 maxsplit));
  ASSERT_EQ(result.numItems(), 1);
  EXPECT_EQ(result.at(0), *str);
}

TEST_F(UnderBuiltinsModuleTest, UnderStrSplitWithSepInUnderStrSplitsOnSep) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello world hello world"));
  Str sep(&scope, runtime_->newStrFromCStr(" w"));
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope, runBuiltin(UnderBuiltinsModule::underStrSplit, str, sep,
                                 maxsplit));
  EXPECT_PYLIST_EQ(result, {"hello", "orld hello", "orld"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWithSepInUnderStrSplitsOnSepMaxsplit) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("a b c d e"));
  Str sep(&scope, runtime_->newStrFromCStr(" "));
  Int maxsplit(&scope, SmallInt::fromWord(2));
  List result(&scope, runBuiltin(UnderBuiltinsModule::underStrSplit, str, sep,
                                 maxsplit));
  EXPECT_PYLIST_EQ(result, {"a", "b", "c d e"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWithSepInUnderStrSplitsOnSepMaxsplitZero) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("a b c d e"));
  Str sep(&scope, runtime_->newStrFromCStr(" "));
  Int maxsplit(&scope, SmallInt::fromWord(0));
  List result(&scope, runBuiltin(UnderBuiltinsModule::underStrSplit, str, sep,
                                 maxsplit));
  EXPECT_PYLIST_EQ(result, {"a b c d e"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWhitespaceSplitsOnUnicodeWhitespace) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(u8"a  \u3000 \t  b\u205fc"));
  Object sep(&scope, NoneType::object());
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope, runBuiltin(UnderBuiltinsModule::underStrSplit, str, sep,
                                 maxsplit));
  EXPECT_PYLIST_EQ(result, {"a", "b", "c"});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrSplitWhitespaceSplitsOnWhitespaceAtEnd) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("a   \t  b c  "));
  Object sep(&scope, NoneType::object());
  Int maxsplit(&scope, SmallInt::fromWord(100));
  List result(&scope, runBuiltin(UnderBuiltinsModule::underStrSplit, str, sep,
                                 maxsplit));
  EXPECT_PYLIST_EQ(result, {"a", "b", "c"});
}

TEST_F(UnderBuiltinsModuleTest, UnderTupleCheckExactWithExactTupleReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newTuple(0));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underTupleCheckExact, obj),
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
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underTupleCheckExact, obj),
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

}  // namespace py
