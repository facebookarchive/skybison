#include "gtest/gtest.h"

#include "bytearray-builtins.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"
#include "under-builtins-module.h"

namespace python {

using namespace testing;

using UnderBuiltinsModuleTest = RuntimeFixture;
using UnderBuiltinsModuleDeathTest = RuntimeFixture;

TEST_F(UnderBuiltinsModuleTest, CopyFunctionEntriesCopies) {
  HandleScope scope(thread_);
  Function::Entry entry = UnderBuiltinsModule::underIntCheck;
  Str qualname(&scope, runtime_.symbols()->UnderIntCheck());
  Function func(&scope, runtime_.newBuiltinFunction(SymbolId::kUnderIntCheck,
                                                    qualname, entry));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def _int_check(self):
  "docstring"
  pass
)")
                   .isError());
  Function python_func(&scope, moduleAt(&runtime_, "__main__", "_int_check"));
  copyFunctionEntries(Thread::current(), func, python_func);
  Code base_code(&scope, func.code());
  Code patch_code(&scope, python_func.code());
  EXPECT_EQ(patch_code.code(), base_code.code());
  EXPECT_EQ(python_func.entry(), &builtinTrampoline);
  EXPECT_EQ(python_func.entryKw(), &builtinTrampolineKw);
  EXPECT_EQ(python_func.entryEx(), &builtinTrampolineEx);
}

TEST_F(UnderBuiltinsModuleDeathTest, CopyFunctionEntriesRedefinitionDies) {
  HandleScope scope(thread_);
  Function::Entry entry = UnderBuiltinsModule::underIntCheck;
  Str qualname(&scope, runtime_.symbols()->UnderIntCheck());
  Function func(&scope, runtime_.newBuiltinFunction(SymbolId::kUnderIntCheck,
                                                    qualname, entry));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def _int_check(self):
  return True
)")
                   .isError());
  Function python_func(&scope, moduleAt(&runtime_, "__main__", "_int_check"));
  ASSERT_DEATH(
      copyFunctionEntries(Thread::current(), func, python_func),
      "Redefinition of native code method '_int_check' in managed code");
}

TEST_F(UnderBuiltinsModuleTest, UnderByteArrayClearSetsLengthToZero) {
  HandleScope scope(thread_);
  ByteArray array(&scope, runtime_.newByteArray());
  const byte byte_array[] = {'1', '2', '3'};
  runtime_.byteArrayExtend(thread_, array, byte_array);
  ASSERT_EQ(array.numItems(), 3);
  ASSERT_FALSE(
      runBuiltin(UnderBuiltinsModule::underByteArrayClear, array).isError());
  EXPECT_EQ(array.numItems(), 0);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArrayJoinWithEmptyIterableReturnsEmptyByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, 'a');
  Object iter(&scope, runtime_.emptyTuple());
  Object result(
      &scope, runBuiltin(UnderBuiltinsModule::underByteArrayJoin, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, ""));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArrayJoinWithEmptySeparatorReturnsByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  Tuple iter(&scope, runtime_.newTuple(3));
  iter.atPut(0, runtime_.newBytes(1, 'A'));
  iter.atPut(1, runtime_.newBytes(2, 'B'));
  iter.atPut(2, runtime_.newBytes(1, 'A'));
  Object result(
      &scope, runBuiltin(UnderBuiltinsModule::underByteArrayJoin, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "ABBA"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArrayJoinWithNonEmptyReturnsByteArray) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, ' ');
  List iter(&scope, runtime_.newList());
  Bytes value(&scope, runtime_.newBytes(1, '*'));
  runtime_.listAdd(thread_, iter, value);
  runtime_.listAdd(thread_, iter, value);
  runtime_.listAdd(thread_, iter, value);
  Object result(
      &scope, runBuiltin(UnderBuiltinsModule::underByteArrayJoin, self, iter));
  EXPECT_TRUE(isByteArrayEqualsCStr(result, "* * *"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArraySetitemWithLargeIntRaisesIndexError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  Int key(&scope, runtime_.newInt(SmallInt::kMaxValue + 1));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underByteArraySetItem, self, key, value),
      LayoutId::kIndexError, "cannot fit 'int' into an index-sized integer"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArraySetitemWithKeyLargerThanMaxIndexRaisesIndexError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, ' ');
  Int key(&scope, runtime_.newInt(self.numItems()));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underByteArraySetItem, self, key, value),
      LayoutId::kIndexError, "index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArraySetitemWithNegativeValueRaisesValueError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, ' ');
  Int key(&scope, runtime_.newInt(0));
  Int value(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underByteArraySetItem, self, key, value),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(
    UnderBuiltinsModuleTest,
    UnderByteArraySetitemWithKeySmallerThanNegativeLengthValueRaisesValueError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, ' ');
  Int key(&scope, runtime_.newInt(-self.numItems() - 1));
  Int value(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underByteArraySetItem, self, key, value),
      LayoutId::kIndexError, "index out of range"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArraySetitemWithValueGreaterThanKMaxByteRaisesValueError) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, ' ');
  Int key(&scope, runtime_.newInt(0));
  Int value(&scope, SmallInt::fromWord(kMaxByte + 1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underByteArraySetItem, self, key, value),
      LayoutId::kValueError, "byte must be in range(0, 256)"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArraySetitemWithNegativeKeyIndexesBackwards) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, 'a');
  byteArrayAdd(thread_, &runtime_, self, 'b');
  byteArrayAdd(thread_, &runtime_, self, 'c');
  Int key(&scope, SmallInt::fromWord(-1));
  Int value(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(
      runBuiltin(UnderBuiltinsModule::underByteArraySetItem, self, key, value)
          .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "ab\001"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderByteArraySetitemWithNegativeKeySetsItemAtIndex) {
  HandleScope scope(thread_);
  ByteArray self(&scope, runtime_.newByteArray());
  byteArrayAdd(thread_, &runtime_, self, 'a');
  byteArrayAdd(thread_, &runtime_, self, 'b');
  byteArrayAdd(thread_, &runtime_, self, 'c');
  Int key(&scope, SmallInt::fromWord(1));
  Int value(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(
      runBuiltin(UnderBuiltinsModule::underByteArraySetItem, self, key, value)
          .isNoneType());
  EXPECT_TRUE(isByteArrayEqualsCStr(self, "a\001c"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderBytesJoinWithEmptyIterableReturnsEmptyByteArray) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime_.newBytes(3, 'a'));
  Object iter(&scope, runtime_.emptyTuple());
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, ""));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithEmptySeparatorReturnsBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, Bytes::empty());
  Tuple iter(&scope, runtime_.newTuple(3));
  iter.atPut(0, runtime_.newBytes(1, 'A'));
  iter.atPut(1, runtime_.newBytes(2, 'B'));
  iter.atPut(2, runtime_.newBytes(1, 'A'));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "ABBA"));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithNonEmptyListReturnsBytes) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Bytes self(&scope, runtime_.newBytes(1, ' '));
  List iter(&scope, runtime_.newList());
  Bytes value(&scope, runtime_.newBytes(1, '*'));
  runtime_.listAdd(thread, iter, value);
  runtime_.listAdd(thread, iter, value);
  runtime_.listAdd(thread, iter, value);
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "* * *"));
}

TEST_F(UnderBuiltinsModuleTest, UnderBytesJoinWithBytesSubclassesReturnsBytes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Bytes self(&scope, moduleAt(&runtime_, "__main__", "sep"));
  Tuple iter(&scope, runtime_.newTuple(2));
  iter.atPut(0, moduleAt(&runtime_, "__main__", "ac"));
  iter.atPut(1, moduleAt(&runtime_, "__main__", "dc"));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underBytesJoin, self, iter));
  EXPECT_TRUE(isBytesEqualsCStr(result, "AC-DC"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithLittleEndianReturnsSmallInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe};
  Bytes bytes(&scope, runtime_.newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::falseObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xfeca));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithLittleEndianReturnsLargeInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23,
                              0x45, 0x67, 0x89, 0xab, 0xcd};
  Bytes bytes(&scope, runtime_.newBytesWithAll(bytes_array));
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

  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe};
  Bytes bytes(&scope, runtime_.newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::trueObj());
  Bool signed_arg(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xcafe));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntFromBytesWithBigEndianReturnsLargeInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23,
                              0x45, 0x67, 0x89, 0xab, 0xcd};
  Bytes bytes(&scope, runtime_.newBytesWithAll(bytes_array));
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

  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_.newBytesWithAll(View<byte>(nullptr, 0)));
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
  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_.newBytes(kWordSize, 0xff));
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

  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xff};
  Bytes bytes(&scope, runtime_.newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::falseObj());
  Bool signed_arg(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntFromBytesWithNegativeNumberReturnsLargeInt) {
  HandleScope scope(thread_);

  Type int_type(&scope, runtime_.typeAt(LayoutId::kInt));
  const byte bytes_array[] = {0xca, 0xfe, 0xba, 0xbe, 0x01, 0x23,
                              0x45, 0x67, 0x89, 0xab, 0xcd};
  Bytes bytes(&scope, runtime_.newBytesWithAll(bytes_array));
  Bool byteorder_big(&scope, Bool::trueObj());
  Bool signed_arg(&scope, Bool::trueObj());
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntFromBytes,
                                   int_type, bytes, byteorder_big, signed_arg));
  const uword expected_digits[] = {0xbe0123456789abcd, 0xffffffffffcafeba};
  EXPECT_TRUE(isIntEqualsDigits(*result, expected_digits));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteArrayWithZeroBaseReturnsCodeLiteral) {
  HandleScope scope(thread_);
  const byte view[] = {'0', 'x', 'b', 'a', '5', 'e'};
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime_.newByteArray());
  runtime_.byteArrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope,
                runBuiltin(UnderBuiltinsModule::underIntNewFromByteArray, type,
                           array, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xba5e));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteArrayWithInvalidByteRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'$'};
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime_.newByteArray());
  runtime_.byteArrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromByteArray, type, array,
                 base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromByteArrayWithInvalidLiteraRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'a'};
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  ByteArray array(&scope, runtime_.newByteArray());
  runtime_.byteArrayExtend(thread_, array, view);
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromByteArray, type, array,
                 base),
      LayoutId::kValueError, "invalid literal for int() with base 10: b'a'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithZeroBaseReturnsCodeLiteral) {
  HandleScope scope(thread_);
  const byte view[] = {'0', '4', '3'};
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_.newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromBytes,
                                   type, bytes, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 043));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithInvalidByteRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'$'};
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_.newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: b'$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithInvalidLiteralRaisesValueError) {
  HandleScope scope(thread_);
  const byte view[] = {'8', '6'};
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, runtime_.newBytesWithAll(view));
  Int base(&scope, SmallInt::fromWord(7));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      LayoutId::kValueError, "invalid literal for int() with base 7: b'86'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromBytesWithBytesSubclassReturnsSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
foo = Foo(b"42")
)")
                   .isError());
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Bytes bytes(&scope, moduleAt(&runtime_, "__main__", "foo"));
  Int base(&scope, SmallInt::fromWord(21));
  EXPECT_EQ(
      runBuiltin(UnderBuiltinsModule::underIntNewFromBytes, type, bytes, base),
      SmallInt::fromWord(86));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromIntWithBoolReturnsSmallInt) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_.typeAt(LayoutId::kInt));
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
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubInt(int):
  def __new__(cls, value):
      self = super(SubInt, cls).__new__(cls, value)
      self.name = "subint instance"
      return self

result = SubInt(50)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_FALSE(result.isInt());
  EXPECT_TRUE(isIntEqualsWord(*result, 50));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithZeroBaseReturnsCodeLiteral) {
  HandleScope scope(thread_);
  const char* src = "1985";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 1985));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithInvalidCharRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "$";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(36));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 36: '$'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithInvalidLiteralRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "305";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(4));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 4: '305'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeInt) {
  HandleScope scope(thread_);
  const char* src = "1844674407370955161500";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '-'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLengthOneInfersBase10) {
  HandleScope scope(thread_);
  const char* src = "8";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 8));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLengthOneBase10) {
  HandleScope scope(thread_);
  const char* src = "8";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 8));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseTwo) {
  HandleScope scope(thread_);
  const char* src = "100";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 4));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntInfersBaseTen) {
  HandleScope scope(thread_);
  const char* src = "100";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLeadingSpacesRemovesSpaces) {
  HandleScope scope(thread_);
  const char* src = "      100";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithOnlySpacesRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "    ";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '    '"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithPlusReturnsPositiveInt) {
  HandleScope scope(thread_);
  const char* src = "+100";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithTwoPlusSignsRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "++100";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 16: '++100'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseEight) {
  HandleScope scope(thread_);
  const char* src = "0o77712371237123712371237123777";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(8));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  const uword digits[] = {0xa7ca7ca7ca7ca7ff, 0x7fca7c};
  EXPECT_TRUE(isIntEqualsDigits(*result, digits));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntInfersBaseEight) {
  HandleScope scope(thread_);
  const char* src = "0o77712371237123712371237123777";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '0x'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithMinusAndPrefixRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "-0x";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '-0x'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithPlusAndPrefixRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "+0x";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '+0x'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithJustPrefixAndUnderscoreRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "0x_";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '0x_'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoreIgnoresUnderscore) {
  HandleScope scope(thread_);
  const char* src = "0x_deadbeef";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xdeadbeef));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoresIgnoresUnderscoresBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "0x_d_e_a_d_b_eef";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0xdeadbeef));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithUnderscoresIgnoresUnderscoresBaseTen) {
  HandleScope scope(thread_);
  const char* src = "100_000_000_000";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 100000000000));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithLeadingUnderscoreBaseTenRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "_100";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '_100'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithTrailingUnderscoreBaseTenRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "100_";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(10));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: '100_'"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithDoubleUnderscoreBaseTenRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "1__00";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 16: '_abc'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithNegativeZeroReturnsZero) {
  HandleScope scope(thread_);
  const char* src = "-0";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithTwoMinusSignsRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "--100";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError,
      "invalid literal for int() with base 16: '--100'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithPositiveZeroReturnsZero) {
  HandleScope scope(thread_);
  const char* src = "+0";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithEmptyStringRaisesValueError) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 10: 'a'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderIntNewFromStrWithLargeIntBaseSixteen) {
  HandleScope scope(thread_);
  const char* src = "0x8000000000000000";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseTwoReturnsOne) {
  HandleScope scope(thread_);
  const char* src = "0b1";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
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
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 177));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseSixteenReturnsEleven) {
  HandleScope scope(thread_);
  const char* src = "0b";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(16));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underIntNewFromStr,
                                   type, str, base));
  EXPECT_TRUE(isIntEqualsWord(*result, 11));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderIntNewFromStrWithBinaryLiteralBaseEightRaisesValueError) {
  HandleScope scope(thread_);
  const char* src = "0b1";
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Str str(&scope, runtime_.newStrFromCStr(src));
  Int base(&scope, SmallInt::fromWord(8));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(UnderBuiltinsModule::underIntNewFromStr, type, str, base),
      LayoutId::kValueError, "invalid literal for int() with base 8: '0b1'"));
}

TEST_F(UnderBuiltinsModuleTest, UnderListCheckExactWithExactListReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newList());
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underListCheckExact, obj),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListCheckExactWithListSubclassReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(list):
  pass
obj = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, moduleAt(&runtime_, "__main__", "obj"));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underListCheckExact, obj),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelItemWithNegativeIndexRemovesRelativeToEnd) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Object idx(&scope, SmallInt::fromWord(-3));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelItem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelItemWithLastIndexRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelItem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelItemWithFirstIndexRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(0));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelItem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelItemWithNegativeFirstIndexRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-2));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelItem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelItemWithNegativeLastIndexRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Object idx(&scope, SmallInt::fromWord(-1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelItem, list, idx)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelItemWithNumberGreaterThanSmallIntMaxDoesNotCrash) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int big(&scope, runtime_.newInt(SmallInt::kMaxValue + 100));
  EXPECT_TRUE(
      raised(runBuiltin(UnderBuiltinsModule::underListDelItem, list, big),
             LayoutId::kIndexError));
  EXPECT_PYLIST_EQ(list, {0, 1});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelSliceRemovesItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelSliceRemovesFirstItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest, UnderListDelSliceRemovesLastItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 2));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(2));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {0});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelSliceWithStopEqualsLengthRemovesTrailingItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelSliceWithStartEqualsZeroRemovesStartingItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(1));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {2, 3});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelSliceWithStartEqualsZeroAndStopEqualsLengthRemovesAllItems) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(1, 4));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(3));
  Int step(&scope, SmallInt::fromWord(1));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_EQ(list.numItems(), 0);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelSliceWithStepEqualsTwoDeletesEveryEvenItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(2));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1, 3});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelSliceWithStepEqualsTwoDeletesEveryOddItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(1));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(2));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {0, 2, 4});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderListDelSliceWithStepGreaterThanLengthDeletesOneItem) {
  HandleScope scope(thread_);
  List list(&scope, listFromRange(0, 5));
  Int start(&scope, SmallInt::fromWord(0));
  Int stop(&scope, SmallInt::fromWord(5));
  Int step(&scope, SmallInt::fromWord(1000));
  ASSERT_TRUE(runBuiltin(UnderBuiltinsModule::underListDelSlice, list, start,
                         stop, step)
                  .isNoneType());
  EXPECT_PYLIST_EQ(list, {1, 2, 3, 4});
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithNonexistentAttrReturnsFalse) {
  HandleScope scope(thread_);
  Object obj(&scope, SmallInt::fromWord(0));
  Str name(&scope, runtime_.newStrFromCStr("__foo_bar_baz__"));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
  EXPECT_EQ(result, Bool::falseObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithInstanceAttrReturnsFalse) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foobarbaz = 5
obj = C()
)")
                   .isError());
  Object obj(&scope, moduleAt(&runtime_, "__main__", "obj"));
  Str name(&scope, runtime_.newStrFromCStr("foobarbaz"));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
  EXPECT_EQ(result, Bool::falseObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithExistentAttrReturnsTrue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    foobarbaz = 5
obj = C()
)")
                   .isError());
  Object obj(&scope, moduleAt(&runtime_, "__main__", "obj"));
  Str name(&scope, runtime_.newStrFromCStr("foobarbaz"));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
  EXPECT_EQ(result, Bool::trueObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderObjectTypeHasattrWithRaisingDescriptorDoesNotRaise) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Desc:
  def __get__(self, obj, type):
    raise UserWarning("foo")
class C:
    foobarbaz = Desc()
obj = C()
)")
                   .isError());
  Object obj(&scope, moduleAt(&runtime_, "__main__", "obj"));
  Str name(&scope, runtime_.newStrFromCStr("foobarbaz"));
  Object result(&scope, runBuiltin(UnderBuiltinsModule::underObjectTypeHasattr,
                                   obj, name));
  EXPECT_EQ(result, Bool::trueObj());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithBadPatchFuncRaisesTypeError) {
  HandleScope scope(thread_);
  Object not_func(&scope, runtime_.newInt(12));
  EXPECT_TRUE(
      raisedWithStr(runBuiltin(UnderBuiltinsModule::underPatch, not_func),
                    LayoutId::kTypeError, "_patch expects function argument"));
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithMissingFuncRaisesAttributeError) {
  HandleScope scope(thread_);
  SymbolId func_name = SymbolId::kHex;
  Str qualname(&scope, runtime_.symbols()->at(func_name));
  Function func(
      &scope, runtime_.newBuiltinFunction(func_name, qualname,
                                          UnderBuiltinsModule::underIntCheck));
  Str module_name(&scope, runtime_.newStrFromCStr("foo"));
  Module module(&scope, runtime_.newModule(module_name));
  runtime_.addModule(module);
  func.setModule(*module_name);
  EXPECT_TRUE(raisedWithStr(runBuiltin(UnderBuiltinsModule::underPatch, func),
                            LayoutId::kAttributeError,
                            "function hex not found in module foo"));
}

TEST_F(UnderBuiltinsModuleTest, UnderPatchWithBadBaseFuncRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
not_a_function = 1234

@_patch
def not_a_function():
  pass
)"),
                            LayoutId::kTypeError,
                            "_patch can only patch functions"));
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrFromStrWithStrTypeReturnsValueOfStrType) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = _str_from_str(str, 'value')
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(runtime_.isInstanceOfStr(*result));
  EXPECT_TRUE(result.isStr());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderStrFromStrWithSubClassTypeReturnsValueOfSubClassType) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Sub(str): pass
result = _str_from_str(Sub, 'value')
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  Object sub(&scope, moduleAt(&runtime_, "__main__", "Sub"));
  EXPECT_EQ(runtime_.typeOf(*result), sub);
  EXPECT_TRUE(isStrEqualsCStr(*result, "value"));
}

TEST_F(UnderBuiltinsModuleTest, UnderStrArrayIaddWithStrReturnsStrArray) {
  HandleScope scope(thread_);
  StrArray self(&scope, runtime_.newStrArray());
  const char* test_str = "hello";
  Str other(&scope, runtime_.newStrFromCStr(test_str));
  StrArray result(
      &scope, runBuiltin(UnderBuiltinsModule::underStrArrayIadd, self, other));
  EXPECT_TRUE(isStrEqualsCStr(runtime_.strFromStrArray(result), test_str));
  EXPECT_EQ(self, result);
}

TEST_F(UnderBuiltinsModuleTest,
       UnderTypeDictKeysWithPlaceholdersSkipsPlaceholders) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  // We expect to get this key back
  Str key(&scope, runtime_.newStrFromCStr("foo"));
  Object value(&scope, NoneType::object());
  runtime_.typeDictAtPut(thread_, dict, key, value);

  // But not this one
  Str ignore_key(&scope, runtime_.newStrFromCStr("bar"));
  Object ignore_value(&scope, NoneType::object());
  runtime_.typeDictAtPut(thread_, dict, ignore_key, ignore_value);
  ValueCell cell(&scope, runtime_.dictAt(thread_, dict, ignore_key));
  cell.makePlaceholder();

  List result(&scope, runBuiltin(UnderBuiltinsModule::underTypeDictKeys, dict));
  EXPECT_PYLIST_EQ(result, {"foo"});
}

TEST_F(UnderBuiltinsModuleTest, UnderTupleCheckExactWithExactTupleReturnsTrue) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newTuple(0));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underTupleCheckExact, obj),
            Bool::trueObj());
}

TEST_F(UnderBuiltinsModuleTest,
       UnderTupleCheckExactWithTupleSubclassReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(tuple):
  pass
obj = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object obj(&scope, moduleAt(&runtime_, "__main__", "obj"));
  EXPECT_EQ(runBuiltin(UnderBuiltinsModule::underTupleCheckExact, obj),
            Bool::falseObj());
}

TEST_F(UnderBuiltinsModuleDeathTest, UnderUnimplementedAbortsProgram) {
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime_, "_unimplemented()")),
               ".*'_unimplemented' called.");
}

TEST_F(UnderBuiltinsModuleDeathTest, UnderUnimplementedPrintsFunctionName) {
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime_, R"(
def foobar():
  _unimplemented()
foobar()
)")),
               ".*'_unimplemented' called in function 'foobar'.");
}

}  // namespace python
