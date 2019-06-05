#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using StrBuiltinsTest = RuntimeFixture;
using StrIterTest = RuntimeFixture;
using StrIteratorBuiltinsTest = RuntimeFixture;
using StringIterTest = RuntimeFixture;

TEST_F(StrBuiltinsTest, BuiltinBase) {
  HandleScope scope(thread_);

  Type small_str(&scope, runtime_.typeAt(LayoutId::kSmallStr));
  EXPECT_EQ(small_str.builtinBase(), LayoutId::kStr);

  Type large_str(&scope, runtime_.typeAt(LayoutId::kLargeStr));
  EXPECT_EQ(large_str.builtinBase(), LayoutId::kStr);

  Type str(&scope, runtime_.typeAt(LayoutId::kStr));
  EXPECT_EQ(str.builtinBase(), LayoutId::kStr);
}

TEST_F(StrBuiltinsTest, RichCompareStringEQ) {
  const char* src = R"(
a = "__main__"
if (a == "__main__"):
  print("foo")
else:
  print("bar")
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "foo\n");
}

TEST_F(StrBuiltinsTest, RichCompareStringEQWithSubClass) {
  const char* src = R"(
class SubStr(str): pass
a = SubStr("__main__")
if (a == "__main__"):
  print("foo")
else:
  print("bar")
)";
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "foo\n");
}

TEST_F(StrBuiltinsTest, RichCompareStringNE) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "__main__"
result = "bar"
if (a != "__main__"):
  result = "foo"
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"), "bar"));
}

TEST_F(StrBuiltinsTest, RichCompareStringNEWithSubClass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
a = SubStr("apple")
result = "bar"
if (a != "apple"):
  result = "foo"
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"), "bar"));
}

TEST_F(StrBuiltinsTest, RichCompareSingleCharLE) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a_le_b = 'a' <= 'b'
b_le_a = 'a' >= 'b'
a_le_a = 'a' <= 'a'
)")
                   .isError());

  HandleScope scope(thread_);

  Object a_le_b(&scope, moduleAt(&runtime_, "__main__", "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());

  Object b_le_a(&scope, moduleAt(&runtime_, "__main__", "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());

  Object a_le_a(&scope, moduleAt(&runtime_, "__main__", "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
}

TEST_F(StrBuiltinsTest, RichCompareSingleCharLEWithSubClass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class S(str): pass
a_le_b = S('a') <= S('b')
b_le_a = S('a') >= S('b')
a_le_a = S('a') <= S('a')
)")
                   .isError());

  EXPECT_EQ(moduleAt(&runtime_, "__main__", "a_le_b"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "b_le_a"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "a_le_a"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, LowerOnASCIILettersReturnsLowerCaseString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "HELLO".lower()
b = "HeLLo".lower()
c = "hellO".lower()
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Object c(&scope, moduleAt(&runtime_, "__main__", "c"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "hello"));
  EXPECT_TRUE(isStrEqualsCStr(*c, "hello"));
}

TEST_F(StrBuiltinsTest, LowerOnASCIILettersWithSubClassReturnsLowerCaseString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
a = SubStr("HELLO").lower()
b = SubStr("HeLLo").lower()
c = SubStr("hellO").lower()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "a"), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "b"), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "c"), "hello"));
}

TEST_F(StrBuiltinsTest, LowerOnLowercaseASCIILettersReturnsSameString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".lower()
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
}

TEST_F(StrBuiltinsTest, LowerOnNumbersReturnsSameString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "foo 123".lower()
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo 123"));
}

TEST_F(StrBuiltinsTest, DunderNewCallsDunderStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
    def __str__(self):
        return "foo"
a = str.__new__(str, Foo())
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST_F(StrBuiltinsTest, DunderNewCallsReprIfNoDunderStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass
f = Foo()
a = str.__new__(str, f)
b = repr(f)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST_F(StrBuiltinsTest, DunderNewWithNoArgsExceptTypeReturnsEmptyString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = str.__new__(str)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, ""));
}

TEST_F(StrBuiltinsTest, DunderNewWithStrReturnsSameStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = str.__new__(str, "hello")
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
}

TEST_F(StrBuiltinsTest, DunderNewWithTypeCallsTypeDunderStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "a = str.__new__(str, int)").isError());
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "<class 'int'>"));
}

TEST_F(StrBuiltinsTest, DunderNewWithNoArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "str.__new__()"), LayoutId::kTypeError,
      "TypeError: 'str.__new__' takes min 1 positional arguments but 0 given"));
}

TEST_F(StrBuiltinsTest, DunderNewWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "str.__new__(str, 1, 2, 3, 4)"),
      LayoutId::kTypeError,
      "TypeError: 'str.__new__' takes max 4 positional arguments but 5 given"));
}

TEST_F(StrBuiltinsTest, DunderNewWithNonTypeArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "str.__new__(1)"),
                            LayoutId::kTypeError, "cls is not a type object"));
}

TEST_F(StrBuiltinsTest, DunderNewWithNonSubtypeArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "str.__new__(object)"),
                            LayoutId::kTypeError,
                            "cls is not a subtype of str"));
}

TEST_F(StrBuiltinsTest, DunderAddWithTwoStringsReturnsConcatenatedString) {
  HandleScope scope(thread_);
  Object str1(&scope, runtime_.newStrFromCStr("hello"));
  Object str2(&scope, runtime_.newStrFromCStr("world"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "helloworld"));
}

TEST_F(StrBuiltinsTest,
       DunderAddWithTwoStringsOfSubClassReturnsConcatenatedString) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
str1 = SubStr("hello")
str2 = SubStr("world")
)")
                   .isError());
  Object str1(&scope, moduleAt(&runtime_, "__main__", "str1"));
  Object str2(&scope, moduleAt(&runtime_, "__main__", "str2"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "helloworld"));
}

TEST_F(StrBuiltinsTest, DunderAddWithLeftEmptyAndReturnsRight) {
  HandleScope scope(thread_);
  Object str1(&scope, Str::empty());
  Object str2(&scope, runtime_.newStrFromCStr("world"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "world"));
}

TEST_F(StrBuiltinsTest, DunderAddWithRightEmptyAndReturnsRight) {
  HandleScope scope(thread_);
  Object str1(&scope, runtime_.newStrFromCStr("hello"));
  Object str2(&scope, Str::empty());
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello"));
}

TEST_F(StrBuiltinsTest, PlusOperatorOnStringsEqualsDunderAdd) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello"
b = "world"
c = a + b
d = a.__add__(b)
)")
                   .isError());
  HandleScope scope(thread_);
  Object c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Object d(&scope, moduleAt(&runtime_, "__main__", "d"));

  EXPECT_TRUE(isStrEqualsCStr(*c, "helloworld"));
  EXPECT_TRUE(isStrEqualsCStr(*d, "helloworld"));
}

TEST_F(StrBuiltinsTest, DunderBoolWithEmptyStringReturnsFalse) {
  HandleScope scope(thread_);
  Str str(&scope, Str::empty());
  EXPECT_EQ(runBuiltin(StrBuiltins::dunderBool, str), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, DunderBoolWithNonEmptyStringReturnsTrue) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("hello"));
  EXPECT_EQ(runBuiltin(StrBuiltins::dunderBool, str), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, DunderBoolWithNonEmptyStringOfSubClassReturnsTrue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr("hello")
)")
                   .isError());
  Object substr(&scope, moduleAt(&runtime_, "__main__", "substr"));
  EXPECT_EQ(runBuiltin(StrBuiltins::dunderBool, substr), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, DunderLenReturnsLength) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l1 = len("aloha")
l2 = str.__len__("aloha")
l3 = "aloha".__len__()
)")
                   .isError());
  HandleScope scope(thread_);
  Object l1(&scope, moduleAt(&runtime_, "__main__", "l1"));
  Object l2(&scope, moduleAt(&runtime_, "__main__", "l2"));
  Object l3(&scope, moduleAt(&runtime_, "__main__", "l3"));
  EXPECT_TRUE(isIntEqualsWord(*l1, 5));
  EXPECT_TRUE(isIntEqualsWord(*l2, 5));
  EXPECT_TRUE(isIntEqualsWord(*l3, 5));
}

TEST_F(StrBuiltinsTest, StringLenWithEmptyString) {
  ASSERT_FALSE(runFromCStr(&runtime_, "l = len('')").isError());
  HandleScope scope(thread_);
  Object length(&scope, moduleAt(&runtime_, "__main__", "l"));
  EXPECT_TRUE(isIntEqualsWord(*length, 0));
}

TEST_F(StrBuiltinsTest, DunderLenWithNonAsciiReturnsCodePointLength) {
  ASSERT_FALSE(runFromCStr(&runtime_, "l = len('\xc3\xa9')").isError());
  HandleScope scope(thread_);
  SmallInt length(&scope, moduleAt(&runtime_, "__main__", "l"));
  EXPECT_TRUE(isIntEqualsWord(*length, 1));
}

TEST_F(StrBuiltinsTest, DunderLenWithIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "l = str.__len__(None)"), LayoutId::kTypeError,
      "'__len__' requires a 'str' object but got 'NoneType'"));
}

TEST_F(StrBuiltinsTest, DunderLenWithExtraArgumentRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "l = 'aloha'.__len__('arg')"),
      LayoutId::kTypeError,
      "TypeError: 'str.__len__' takes max 1 positional arguments but 2 given"));
}

TEST_F(StrBuiltinsTest, DunderMulWithNonStrRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "str.__mul__(None, 1)"), LayoutId::kTypeError,
      "'__mul__' requires a 'str' object but got 'NoneType'"));
}

TEST_F(StrBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  Object count(&scope, runtime_.newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(StrBuiltins::dunderMul, self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(StrBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime_, "__main__", "count"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime_, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(StrBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime_, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST_F(StrBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  HandleScope scope(thread_);
  Object self(&scope, Str::empty());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime_.newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(StrBuiltinsTest, DunderMulWithOverflowRaisesOverflowError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "repeated string is too long"));
}

TEST_F(StrBuiltinsTest, DunderMulWithEmptyBytesReturnsEmptyStr) {
  HandleScope scope(thread_);
  Object self(&scope, Str::empty());
  Object count(&scope, runtime_.newInt(10));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(StrBuiltinsTest, DunderMulWithNegativeReturnsEmptyStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(StrBuiltinsTest, DunderMulWithZeroReturnsEmptyStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(StrBuiltinsTest, DunderMulWithOneReturnsSamStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithSmallStrReturnsRepeatedSmallStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithSmallStrReturnsRepeatedLargeStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoofoo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithLargeStrReturnsRepeatedLargeStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_.newStrFromCStr("foobarbaz"));
  Object count(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobarbazfoobarbaz"));
}

TEST_F(StrBuiltinsTest, DunderRmulCallsDunderMul) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = 3 * 'foo'").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoofoo"));
}

TEST_F(StrBuiltinsTest, IndexWithLargeIntRaisesIndexError) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Int index(&scope, runtime_.newInt(RawSmallInt::kMaxValue + 1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(raised(*result, LayoutId::kIndexError));
}

TEST_F(StrBuiltinsTest, IndexWithNegativeIntIndexesFromEnd) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Int index(&scope, RawSmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(isStrEqualsCStr(*result, "h"));
}

TEST_F(StrBuiltinsTest, IndexWithLessThanNegativeLenRaisesIndexError) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Int index(&scope, RawSmallInt::fromWord(-6));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(raised(*result, LayoutId::kIndexError));
}

TEST_F(StrBuiltinsTest, IndexWithNonNegativeIntIndexesFromBeginning) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Int index(&scope, RawSmallInt::fromWord(4));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(isStrEqualsCStr(*result, "o"));
}

TEST_F(StrBuiltinsTest,
       IndexWithSubClassAndNonNegativeIntIndexesFromBeginning) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr("hello")
)")
                   .isError());
  Object hello(&scope, moduleAt(&runtime_, "__main__", "substr"));
  Int index(&scope, RawSmallInt::fromWord(4));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(isStrEqualsCStr(*result, "o"));
}

TEST_F(StrBuiltinsTest, IndexWithSliceWithPositiveInts) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Slice slice(&scope, runtime_.newSlice());
  slice.setStart(SmallInt::fromWord(1));
  slice.setStop(SmallInt::fromWord(2));
  Object result_a(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_a, "e"));
  slice.setStop(SmallInt::fromWord(4));
  Object result_b(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_b, "ell"));
}

TEST_F(StrBuiltinsTest, IndexWithSliceWithNegativeInts) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Slice slice(&scope, runtime_.newSlice());
  slice.setStart(SmallInt::fromWord(-1));
  Object result_a(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_a, "o"));
  slice.setStart(SmallInt::fromWord(1));
  slice.setStop(SmallInt::fromWord(-2));
  Object result_b(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_b, "el"));
}

TEST_F(StrBuiltinsTest, IndexWithSliceWithStep) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Slice slice(&scope, runtime_.newSlice());
  slice.setStart(SmallInt::fromWord(0));
  slice.setStop(SmallInt::fromWord(5));
  slice.setStep(SmallInt::fromWord(2));
  Object result_a(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_a, "hlo"));
  slice.setStart(SmallInt::fromWord(1));
  slice.setStep(SmallInt::fromWord(3));
  Object result_b(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_b, "eo"));
}

TEST_F(StrBuiltinsTest, EmptyStringIndexWithSliceWithNegativeOneStep) {
  HandleScope scope(thread_);
  Str hello(&scope, Str::empty());
  Slice slice(&scope, runtime_.newSlice());
  slice.setStep(SmallInt::fromWord(-1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(StrBuiltinsTest, IndexWithSliceWithNegativeOneStep) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Slice slice(&scope, runtime_.newSlice());
  slice.setStep(SmallInt::fromWord(-1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result, "olleh"));
}

TEST_F(StrBuiltinsTest, IndexWithSliceWithNegativeTwoStep) {
  HandleScope scope(thread_);
  Str hello(&scope, runtime_.newStrFromCStr("hello"));
  Slice slice(&scope, runtime_.newSlice());
  slice.setStep(SmallInt::fromWord(-2));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result, "olh"));
}

TEST_F(StrBuiltinsTest, InternStringsInTupleInternsItems) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(3));
  Str str0(&scope, runtime_.newStrFromCStr("a"));
  Str str1(&scope, runtime_.newStrFromCStr("hello world"));
  Str str2(&scope, runtime_.newStrFromCStr("hello world foobar"));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str2));
  tuple.atPut(0, *str0);
  tuple.atPut(1, *str1);
  tuple.atPut(2, *str2);
  strInternInTuple(thread_, tuple);
  str0 = tuple.at(0);
  str1 = tuple.at(1);
  str2 = tuple.at(2);
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str0));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str1));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str2));
}

TEST_F(StrBuiltinsTest,
       InternStringConstantsInternsAlphanumericStringsInTuple) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(3));
  Str str0(&scope, runtime_.newStrFromCStr("_"));
  Str str1(&scope, runtime_.newStrFromCStr("hello world"));
  Str str2(&scope, runtime_.newStrFromCStr("helloworldfoobar"));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str2));
  tuple.atPut(0, *str0);
  tuple.atPut(1, *str1);
  tuple.atPut(2, *str2);
  strInternConstants(thread_, tuple);
  str0 = tuple.at(0);
  str1 = tuple.at(1);
  str2 = tuple.at(2);
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str1));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str2));
}

TEST_F(StrBuiltinsTest, InternStringConstantsInternsStringsInNestedTuples) {
  HandleScope scope(thread_);
  Tuple outer(&scope, runtime_.newTuple(3));
  outer.atPut(0, SmallInt::fromWord(0));
  outer.atPut(1, SmallInt::fromWord(1));
  Tuple inner(&scope, runtime_.newTuple(3));
  outer.atPut(2, *inner);
  Str str0(&scope, runtime_.newStrFromCStr("_"));
  Str str1(&scope, runtime_.newStrFromCStr("hello world"));
  Str str2(&scope, runtime_.newStrFromCStr("helloworldfoobar"));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str2));
  inner.atPut(0, *str0);
  inner.atPut(1, *str1);
  inner.atPut(2, *str2);
  strInternConstants(thread_, outer);
  str0 = inner.at(0);
  str1 = inner.at(1);
  str2 = inner.at(2);
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str1));
  EXPECT_TRUE(runtime_.isInternedStr(thread_, str2));
}

TEST_F(StrBuiltinsTest,
       InternStringConstantsInternsStringsInFrozenSetsInTuples) {
  HandleScope scope(thread_);
  Tuple outer(&scope, runtime_.newTuple(3));
  outer.atPut(0, SmallInt::fromWord(0));
  outer.atPut(1, SmallInt::fromWord(1));
  FrozenSet inner(&scope, runtime_.newFrozenSet());
  outer.atPut(2, *inner);
  Str str0(&scope, runtime_.newStrFromCStr("alpharomeo"));
  Str str1(&scope, runtime_.newStrFromCStr("hello world"));
  Str str2(&scope, runtime_.newStrFromCStr("helloworldfoobar"));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_.isInternedStr(thread_, str2));
  runtime_.setAdd(thread_, inner, str0);
  runtime_.setAdd(thread_, inner, str1);
  runtime_.setAdd(thread_, inner, str2);
  strInternConstants(thread_, outer);
  inner = outer.at(2);
  Tuple data(&scope, inner.data());
  bool all_interned = true;
  bool some_interned = false;
  for (word idx = Set::Bucket::kFirst; Set::Bucket::nextItem(*data, &idx);) {
    Str obj(&scope, Set::Bucket::key(*data, idx));
    bool interned = runtime_.isInternedStr(thread_, obj);
    all_interned &= interned;
    some_interned |= interned;
  }
  EXPECT_FALSE(all_interned);
  EXPECT_TRUE(some_interned);
}

TEST_F(StrBuiltinsTest, StartsWithEmptyStringReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith("")
b = "".startswith("")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST_F(StrBuiltinsTest, StartsWithStringReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith("h")
b = "hello".startswith("he")
c = "hello".startswith("hel")
d = "hello".startswith("hell")
e = "hello".startswith("hello")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime_, "__main__", "d"));
  Bool e(&scope, moduleAt(&runtime_, "__main__", "e"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_TRUE(d.value());
  EXPECT_TRUE(e.value());
}

TEST_F(StrBuiltinsTest, StartsWithTooLongPrefixReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith("hihello")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST_F(StrBuiltinsTest, StartsWithUnrelatedPrefixReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith("bob")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST_F(StrBuiltinsTest, StartsWithStart) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith("e", 1)
b = "hello".startswith("o", 5)
c = "hello".startswith("ell", 1)
d = "hello".startswith("llo", 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime_, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST_F(StrBuiltinsTest, StartsWithStartAndEnd) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith("e", 1, 3)
b = "hello".startswith("el", 1, 4)
c = "hello".startswith("ll", 2, 5)
d = "hello".startswith("ll", 1, 4)
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime_, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST_F(StrBuiltinsTest, StartsWithStartAndEndNegatives) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith("h", 0, -1)
b = "hello".startswith("ll", -3)
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST_F(StrBuiltinsTest, StartsWithTupleOfPrefixes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".startswith(("h", "lo"))
b = "hello".startswith(("asdf", "foo", "bar"))
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
}

TEST_F(StrBuiltinsTest, EndsWithEmptyStringReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith("")
b = "".endswith("")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST_F(StrBuiltinsTest, EndsWithStringReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith("o")
b = "hello".endswith("lo")
c = "hello".endswith("llo")
d = "hello".endswith("ello")
e = "hello".endswith("hello")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime_, "__main__", "d"));
  Bool e(&scope, moduleAt(&runtime_, "__main__", "e"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_TRUE(d.value());
  EXPECT_TRUE(e.value());
}

TEST_F(StrBuiltinsTest, EndsWithTooLongSuffixReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith("hihello")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST_F(StrBuiltinsTest, EndsWithUnrelatedSuffixReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith("bob")
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST_F(StrBuiltinsTest, EndsWithStart) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith("o", 1)
b = "hello".endswith("o", 5)
c = "hello".endswith("llo", 1)
d = "hello".endswith("llo", 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime_, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST_F(StrBuiltinsTest, EndsWithStartAndEnd) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith("l", 1, 3)
b = "hello".endswith("ll", 1, 4)
c = "hello".endswith("lo", 2, 5)
d = "hello".endswith("llo", 1, 4)
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime_, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST_F(StrBuiltinsTest, EndsWithStartAndEndNegatives) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith("l", 0, -1)
b = "hello".endswith("o", -1)
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST_F(StrBuiltinsTest, EndsWithTupleOfSuffixes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".endswith(("o", "llo"))
b = "hello".endswith(("asdf", "foo", "bar"))
)")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
}

TEST_F(StrBuiltinsTest, StringFormat) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
n = 123
f = 3.14
s = "pyros"
a = "hello %d %g %s" % (n, f, s)
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "hello 123 3.14 pyros"));
}

TEST_F(StrBuiltinsTest, StringFormatSingleString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "pyro"
a = "%s" % s
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "pyro"));
}

TEST_F(StrBuiltinsTest, StringFormatTwoStrings) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "pyro"
a = "%s%s" % (s, s)
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "pyropyro"));
}

TEST_F(StrBuiltinsTest, StringFormatMixed) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "pyro"
a = "1%s,2%s,3%s,4%s,5%s" % (s, s, s, s, s)
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "1pyro,2pyro,3pyro,4pyro,5pyro"));
}

TEST_F(StrBuiltinsTest, StringFormatMixed2) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "pyro"
a = "%d%s,%d%s,%d%s" % (1, s, 2, s, 3, s)
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "1pyro,2pyro,3pyro"));
}

TEST_F(StrBuiltinsTest, StringFormatMalformed) {
  const char* src = R"(
a = "%" % ("pyro",)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "Incomplete format"));
}

TEST_F(StrBuiltinsTest, StringFormatMismatch) {
  const char* src = R"(
a = "%d%s" % ("pyro",)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "Argument mismatch"));
}

TEST_F(StrBuiltinsTest, DunderReprOnASCIIStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'hello'"));
}

TEST_F(StrBuiltinsTest, DunderReprOnASCIIStrOfSubClass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr("hello")
a = substr.__repr__()
  )")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "a"), "'hello'"));
}

TEST_F(StrBuiltinsTest, DunderReprOnASCIINonPrintable) {
  // 6 is the ACK character.
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "\x06".__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'\\x06'"));
}

TEST_F(StrBuiltinsTest, DunderReprOnStrWithDoubleQuotes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 'hello "world"'.__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'hello \"world\"'"));
}

TEST_F(StrBuiltinsTest, DunderReprOnStrWithSingleQuotes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello 'world'".__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "\"hello 'world'\""));
}

TEST_F(StrBuiltinsTest, DunderReprOnStrWithBothQuotes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello 'world', I am your \"father\"".__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, R"('hello \'world\', I am your "father"')"));
}

TEST_F(StrBuiltinsTest, DunderReprOnStrWithNestedQuotes) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello 'world, \"I am 'your \"father\"'\"'".__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(
      isStrEqualsCStr(*a, R"('hello \'world, "I am \'your "father"\'"\'')"));
}

TEST_F(StrBuiltinsTest, DunderReprOnCommonEscapeSequences) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "\n \t \r \\".__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'\\n \\t \\r \\\\'"));
}

TEST_F(StrBuiltinsTest, DunderStr) {
  const char* src = R"(
result = 'Hello, World!'.__str__()
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello, World!"));
}

TEST_F(StrBuiltinsTest, JoinWithEmptyArray) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = ",".join([])
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, ""));
}

TEST_F(StrBuiltinsTest, JoinWithOneElementArray) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = ",".join(["1"])
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "1"));
}

TEST_F(StrBuiltinsTest, JoinWithManyElementArray) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = ",".join(["1", "2", "3"])
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "1,2,3"));
}

TEST_F(StrBuiltinsTest, JoinWithManyElementArrayAndEmptySeparator) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "".join(["1", "2", "3"])
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "123"));
}

TEST_F(StrBuiltinsTest, JoinWithIterable) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = ",".join(("1", "2", "3"))
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "1,2,3"));
}

TEST_F(StrBuiltinsTest, JoinWithNonStringInArrayRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
a = ",".join(["hello", 1])
)"),
                            LayoutId::kTypeError,
                            "sequence item 1: expected str instance"));
}

TEST_F(StrBuiltinsTest, JoinWithNonStringSeparatorRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
a = str.join(None, ["hello", 1])
)"),
                    LayoutId::kTypeError,
                    "'join' requires a 'str' object but got 'NoneType'"));
}

TEST_F(StrBuiltinsTest, PartitionOnSingleCharStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".partition("l")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple a(&scope, moduleAt(&runtime_, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "lo"));
}

TEST_F(StrBuiltinsTest, PartitionOnMultiCharStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".partition("ll")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple a(&scope, moduleAt(&runtime_, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "o"));
}

TEST_F(StrBuiltinsTest, PartitionOnSuffix) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".partition("lo")
b = "hello".partition("lop")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Tuple b(&scope, moduleAt(&runtime_, "__main__", "b"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), ""));

  ASSERT_EQ(b.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), ""));
}

TEST_F(StrBuiltinsTest, PartitionOnPrefix) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".partition("he")
b = "hello".partition("hex")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Tuple b(&scope, moduleAt(&runtime_, "__main__", "b"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "llo"));

  ASSERT_EQ(b.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), ""));
}

TEST_F(StrBuiltinsTest, PartitionLargerStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".partition("abcdefghijk")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple a(&scope, moduleAt(&runtime_, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), ""));
}

TEST_F(StrBuiltinsTest, PartitionEmptyStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "".partition("a")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple a(&scope, moduleAt(&runtime_, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), ""));
}

TEST_F(StrBuiltinsTest, SplitWithOneCharSeparator) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".split("e")
b = "hello".split("l")
)")
                   .isError());
  HandleScope scope(thread_);

  List a(&scope, moduleAt(&runtime_, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "llo"));

  List b(&scope, moduleAt(&runtime_, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "o"));
}

TEST_F(StrBuiltinsTest, SplitWithEmptySelfReturnsSingleEmptyString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "".split("a")
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, moduleAt(&runtime_, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), ""));
}

TEST_F(StrBuiltinsTest, SplitWithMultiCharSeparator) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".split("el")
b = "hello".split("ll")
c = "hello".split("hello")
d = "hellllo".split("ll")
)")
                   .isError());
  HandleScope scope(thread_);

  List a(&scope, moduleAt(&runtime_, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));

  List b(&scope, moduleAt(&runtime_, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "o"));

  List c(&scope, moduleAt(&runtime_, "__main__", "c"));
  ASSERT_EQ(c.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(c.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(c.at(1), ""));

  List d(&scope, moduleAt(&runtime_, "__main__", "d"));
  ASSERT_EQ(d.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(d.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(d.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(d.at(2), "o"));
}

TEST_F(StrBuiltinsTest, SplitWithMaxSplitZeroReturnsList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".split("x", 0)
b = "hello".split("l", 0)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b_obj(&scope, moduleAt(&runtime_, "__main__", "b"));
  ASSERT_TRUE(a_obj.isList());
  ASSERT_TRUE(b_obj.isList());
  List a(&scope, *a_obj);
  List b(&scope, *b_obj);
  ASSERT_EQ(a.numItems(), 1);
  ASSERT_EQ(b.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
}

TEST_F(StrBuiltinsTest, SplitWithMaxSplitBelowNumPartsStopsEarly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".split("l", 1)
b = "1,2,3,4".split(",", 2)
)")
                   .isError());
  HandleScope scope(thread_);

  List a(&scope, moduleAt(&runtime_, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));

  List b(&scope, moduleAt(&runtime_, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "3,4"));
}

TEST_F(StrBuiltinsTest, SplitWithMaxSplitGreaterThanNumParts) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".split("l", 2)
b = "1,2,3,4".split(",", 5)
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, moduleAt(&runtime_, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "o"));

  List b(&scope, moduleAt(&runtime_, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 4);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "3"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(3), "4"));
}

TEST_F(StrBuiltinsTest, SplitEmptyStringWithNoSepReturnsEmptyList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "".split()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 0);
}

TEST_F(StrBuiltinsTest, SplitWhitespaceStringWithNoSepReturnsEmptyList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "  \t\n  ".split()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 0);
}

TEST_F(StrBuiltinsTest, SplitWhitespaceReturnsComponentParts) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "  \t\n  hello\t\n world".split()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST_F(StrBuiltinsTest,
       SplitWhitespaceWithMaxsplitEqualsNegativeOneReturnsAllResults) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "  \t\n  hello\t\n world".split(maxsplit=-1)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST_F(StrBuiltinsTest,
       SplitWhitespaceWithMaxsplitEqualsZeroReturnsOneElementList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "  \t\n  hello   world   ".split(maxsplit=0)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello   world   "});
}

TEST_F(StrBuiltinsTest,
       SplitWhitespaceWithMaxsplitEqualsOneReturnsTwoElementList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "  \t\n  hello world ".split(maxsplit=1)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "world "));
}

TEST_F(StrBuiltinsTest, SplitlinesSplitsOnLineBreaks) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello\nworld\rwhats\r\nup".splitlines()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world", "whats", "up"});
}

TEST_F(StrBuiltinsTest, SplitlinesWithKeependsKeepsLineBreaks) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello\nworld\rwhats\r\nup".splitlines(keepends=True)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello\n", "world\r", "whats\r\n", "up"});
}

TEST_F(StrBuiltinsTest, SplitlinesWithNoNewlinesReturnsIdEqualString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "hello world foo bar"
[result] = s.splitlines()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "s"),
            moduleAt(&runtime_, "__main__", "result"));
}

TEST_F(StrBuiltinsTest, SplitlinesWithMultiByteNewlineSplitsLine) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello\u2028world".splitlines()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST_F(StrBuiltinsTest, SplitlinesWithMultiByteNewlineAndKeependsSplitsLine) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello\u2028world".splitlines(keepends=True)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {u8"hello\u2028", "world"});
}

TEST_F(StrBuiltinsTest, RpartitionOnSingleCharStrPartitionsCorrectly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "hello".rpartition("l")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RpartitionOnMultiCharStrPartitionsCorrectly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "hello".rpartition("ll")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RpartitionOnSuffixPutsEmptyStrAtEndOfResult) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "hello".rpartition("lo")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(StrBuiltinsTest, RpartitionOnNonExistentSuffixPutsStrAtEndOfResult) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "hello".rpartition("lop")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(StrBuiltinsTest, RpartitionOnPrefixPutsEmptyStrAtBeginningOfResult) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "hello".rpartition("he")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "llo"));
}

TEST_F(StrBuiltinsTest, RpartitionOnNonExistentPrefixPutsStrAtEndOfResult) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "hello".rpartition("hex")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(StrBuiltinsTest, RpartitionLargerStrPutsStrAtEndOfResult) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "hello".rpartition("foobarbaz")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST_F(StrBuiltinsTest, RpartitionEmptyStrReturnsTupleOfEmptyStrings) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
t = "".rpartition("a")
)")
                   .isError());
  HandleScope scope(thread_);
  Tuple result(&scope, moduleAt(&runtime_, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(StrBuiltinsTest, RsplitWithOneCharSeparatorSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hello".rsplit("e")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "llo"));
}

TEST_F(StrBuiltinsTest, RsplitWithRepeatedOneCharSeparatorSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hello".rsplit("l")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithEmptySelfReturnsSingleEmptyString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "".rsplit("a")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
}

TEST_F(StrBuiltinsTest, RsplitWithMultiCharSeparatorSplitsFromRight) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hello".rsplit("el")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
}

TEST_F(StrBuiltinsTest, RsplitWithRepeatedCharSeparatorSplitsFromRight) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hello".rsplit("ll")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "o"));
}

TEST_F(StrBuiltinsTest,
       RsplitWithSeparatorSameAsInputSplitsIntoEmptyComponents) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hello".rsplit("hello")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
}

TEST_F(StrBuiltinsTest,
       RsplitWithMultiCharSeparatorWithMultipleAppearancesSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hellllo".rsplit("ll")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithMaxSplitZeroReturnsList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".rsplit("x", 0)
b = "hello".rsplit("l", 0)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b_obj(&scope, moduleAt(&runtime_, "__main__", "b"));
  ASSERT_TRUE(a_obj.isList());
  ASSERT_TRUE(b_obj.isList());
  List a(&scope, *a_obj);
  List b(&scope, *b_obj);
  ASSERT_EQ(a.numItems(), 1);
  ASSERT_EQ(b.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
}

TEST_F(StrBuiltinsTest,
       RsplitWithRepeatedCharAndMaxSplitBelowNumPartsStopsEarly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hello".rsplit("l", 1)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithMaxSplitBelowNumPartsStopsEarly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "1,2,3,4".rsplit(",", 2)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "1,2"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "3"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "4"));
}

TEST_F(StrBuiltinsTest,
       RsplitWithRepeatedCharAndMaxSplitGreaterThanNumPartsSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "hello".rsplit("l", 2)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithMaxSplitGreaterThanNumPartsSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = "1,2,3,4".rsplit(",", 5)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, moduleAt(&runtime_, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 4);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "3"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(3), "4"));
}

TEST_F(StrBuiltinsTest, StrStripWithNonStrRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
str.strip(None)
)"),
                    LayoutId::kTypeError,
                    "'strip' requires a 'str' object but got 'NoneType'"));
}

TEST_F(StrBuiltinsTest, StrLStripWithNonStrRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
str.lstrip(None)
)"),
                    LayoutId::kTypeError,
                    "'lstrip' requires a 'str' object but got 'NoneType'"));
}

TEST_F(StrBuiltinsTest, StrRStripWithNonStrRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
str.rstrip(None)
)"),
                    LayoutId::kTypeError,
                    "'rstrip' requires a 'str' object but got 'NoneType'"));
}

TEST_F(StrBuiltinsTest, StrStripWithInvalidCharsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
"test".strip(1)
)"),
                            LayoutId::kTypeError,
                            "str.strip() arg must be None or str"));
}

TEST_F(StrBuiltinsTest, StrLStripWithInvalidCharsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
"test".lstrip(1)
)"),
                            LayoutId::kTypeError,
                            "str.lstrip() arg must be None or str"));
}

TEST_F(StrBuiltinsTest, StrRStripWithInvalidCharsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
"test".rstrip(1)
)"),
                            LayoutId::kTypeError,
                            "str.rstrip() arg must be None or str"));
}

TEST_F(StrBuiltinsTest, StripWithNoneArgStripsBoth) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, LStripWithNoneArgStripsLeft) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World "));
}

TEST_F(StrBuiltinsTest, LStripWithSubClassAndNonArgStripsLeft) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr(" Hello World ")
)")
                   .isError());
  Object str(&scope, moduleAt(&runtime_, "__main__", "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World "));
}

TEST_F(StrBuiltinsTest, RStripWithNoneArgStripsRight) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " Hello World"));
}

TEST_F(StrBuiltinsTest, RStripWithSubClassAndNoneArgStripsRight) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr(" Hello World ")
)")
                   .isError());
  Object str(&scope, moduleAt(&runtime_, "__main__", "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " Hello World"));
}

TEST_F(StrBuiltinsTest, StripWithoutArgsStripsBoth) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, StripWithSubClassAndWithoutArgsStripsBoth) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr(" \n\tHello World\n\t ")
)")
                   .isError());
  Object str(&scope, moduleAt(&runtime_, "__main__", "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, LStripWithoutArgsStripsLeft) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World\n\t "));
}

TEST_F(StrBuiltinsTest, RStripWithoutArgsStripsRight) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " \n\tHello World"));
}

TEST_F(StrBuiltinsTest, StripWithCharsStripsChars) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime_.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, LStripWithCharsStripsCharsToLeft) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime_.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello Worldcab"));
}

TEST_F(StrBuiltinsTest, RStripWithCharsStripsCharsToRight) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime_.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "bcaHello World"));
}

TEST_F(StrBuiltinsTest, ReplaceWithDefaultCountReplacesAll) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "a1a1a1a".replace("a", "b")
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1b1b1b"));
}

TEST_F(StrBuiltinsTest, ReplaceWithCountReplacesCountedOccurrences) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "a1a1a1a".replace("a", "b", 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1b1a1a"));
}

TEST_F(StrBuiltinsTest, ReplaceWithCountOfIndexTypeReplacesCountedOccurrences) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "a1a1a1a".replace("a", "b", True)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1a1a1a"));
}

TEST_F(StrBuiltinsTest, ReplaceWithNonMatchingReturnsSameObject) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "a"
result = s is s.replace("z", "b")
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST_F(StrBuiltinsTest, ReplaceWithMissingArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "'aa'.replace('a')"), LayoutId::kTypeError,
      "TypeError: 'str.replace' takes min 3 positional arguments but 2 given"));
}

TEST_F(StrBuiltinsTest, ReplaceWithNonIntCountRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, "'aa'.replace('a', 'a', 'a')"),
                    LayoutId::kTypeError,
                    "'str' object cannot be interpreted as an integer"));
}

TEST_F(StrBuiltinsTest, DunderIterReturnsStrIter) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());
  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter.isStrIterator());
}

TEST_F(StrBuiltinsTest, DunderIterWithSubClassReturnsStrIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr("")
)")
                   .isError());
  Object empty_str(&scope, moduleAt(&runtime_, "__main__", "substr"));
  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  EXPECT_TRUE(iter.isStrIterator());
}

TEST_F(StrIteratorBuiltinsTest,
       CallDunderNextReadsAsciiCharactersSequentially) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("ab"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter.isStrIterator());

  Object item0(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item0, "a"));

  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item1, "b"));
}

TEST_F(StrIteratorBuiltinsTest,
       CallDunderNextReadsUnicodeCharactersSequentially) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr(u8"a\u00E4b"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter.isStrIterator());

  Object item0(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item0, "a"));

  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_EQ(*item1, SmallStr::fromCodePoint(0xe4));

  Object item2(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item2, "b"));
}

TEST_F(StrIteratorBuiltinsTest, DunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter.isStrIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(StrIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(StrIteratorBuiltinsTest, DunderLengthHintOnEmptyStrIteratorReturnsZero) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter.isStrIterator());

  Object length_hint(&scope,
                     runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(StrIteratorBuiltinsTest,
       DunderLengthHintOnConsumedStrIteratorReturnsZero) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("a"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter.isStrIterator());

  Object length_hint1(&scope,
                      runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 1));

  // Consume the iterator
  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1.isStr());
  ASSERT_EQ(item1, runtime_.newStrFromCStr("a"));

  Object length_hint2(&scope,
                      runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, 0));
}

TEST_F(StrBuiltinsTest, StripSpaceWithEmptyStrIsIdentity) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());
  Str lstripped_empty_str(&scope, strStripSpaceLeft(thread_, empty_str));
  EXPECT_EQ(*empty_str, *lstripped_empty_str);

  Str rstripped_empty_str(&scope, strStripSpaceRight(thread_, empty_str));
  EXPECT_EQ(*empty_str, *rstripped_empty_str);

  Str stripped_empty_str(&scope, strStripSpace(thread_, empty_str));
  EXPECT_EQ(*empty_str, *stripped_empty_str);
}

TEST_F(StrBuiltinsTest, StripSpaceWithUnstrippableStrIsIdentity) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("Nothing to strip here"));
  ASSERT_TRUE(str.isLargeStr());
  Str lstripped_str(&scope, strStripSpaceLeft(thread_, str));
  EXPECT_EQ(*str, *lstripped_str);

  Str rstripped_str(&scope, strStripSpaceRight(thread_, str));
  EXPECT_EQ(*str, *rstripped_str);

  Str stripped_str(&scope, strStripSpace(thread_, str));
  EXPECT_EQ(*str, *stripped_str);
}

TEST_F(StrBuiltinsTest, StripSpaceWithUnstrippableSmallStrIsIdentity) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("nostrip"));
  ASSERT_TRUE(str.isSmallStr());
  Str lstripped_str(&scope, strStripSpaceLeft(thread_, str));
  EXPECT_EQ(*str, *lstripped_str);

  Str rstripped_str(&scope, strStripSpaceRight(thread_, str));
  EXPECT_EQ(*str, *rstripped_str);

  Str stripped_str(&scope, strStripSpace(thread_, str));
  EXPECT_EQ(*str, *stripped_str);
}

TEST_F(StrBuiltinsTest, StripSpaceWithFullyStrippableStrReturnsEmptyStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("\n\r\t\f         \n\t\r\f"));
  Str lstripped_str(&scope, strStripSpaceLeft(thread_, str));
  EXPECT_EQ(lstripped_str.length(), 0);

  Str rstripped_str(&scope, strStripSpaceRight(thread_, str));
  EXPECT_EQ(rstripped_str.length(), 0);

  Str stripped_str(&scope, strStripSpace(thread_, str));
  EXPECT_EQ(stripped_str.length(), 0);
}

TEST_F(StrBuiltinsTest, StripSpaceLeft) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Str lstripped_str(&scope, strStripSpaceLeft(thread_, str));
  ASSERT_TRUE(lstripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str, "strp "));

  Str str1(&scope, runtime_.newStrFromCStr("   \n \n\tLot of leading space  "));
  ASSERT_TRUE(str1.isLargeStr());
  Str lstripped_str1(&scope, strStripSpaceLeft(thread_, str1));
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str1, "Lot of leading space  "));

  Str str2(&scope, runtime_.newStrFromCStr("\n\n\n              \ntest"));
  ASSERT_TRUE(str2.isLargeStr());
  Str lstripped_str2(&scope, strStripSpaceLeft(thread_, str2));
  ASSERT_TRUE(lstripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str2, "test"));
}

TEST_F(StrBuiltinsTest, StripSpaceRight) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Str rstripped_str(&scope, strStripSpaceRight(thread_, str));
  ASSERT_TRUE(rstripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str, " strp"));

  Str str1(&scope,
           runtime_.newStrFromCStr("  Lot of trailing space\t\n \n    "));
  ASSERT_TRUE(str1.isLargeStr());
  Str rstripped_str1(&scope, strStripSpaceRight(thread_, str1));
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str1, "  Lot of trailing space"));

  Str str2(&scope, runtime_.newStrFromCStr("test\n      \n\n\n"));
  ASSERT_TRUE(str2.isLargeStr());
  Str rstripped_str2(&scope, strStripSpaceRight(thread_, str2));
  ASSERT_TRUE(rstripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str2, "test"));
}

TEST_F(StrBuiltinsTest, StripSpaceBoth) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Str stripped_str(&scope, strStripSpace(thread_, str));
  ASSERT_TRUE(stripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str, "strp"));

  Str str1(&scope,
           runtime_.newStrFromCStr(
               "\n \n    \n\tLot of leading and trailing space\n \n    "));
  ASSERT_TRUE(str1.isLargeStr());
  Str stripped_str1(&scope, strStripSpace(thread_, str1));
  EXPECT_TRUE(
      isStrEqualsCStr(*stripped_str1, "Lot of leading and trailing space"));

  Str str2(&scope, runtime_.newStrFromCStr("\n\ttest\t      \n\n\n"));
  ASSERT_TRUE(str2.isLargeStr());
  Str stripped_str2(&scope, strStripSpace(thread_, str2));
  ASSERT_TRUE(stripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str2, "test"));
}

TEST_F(StrBuiltinsTest, StripWithEmptyStrIsIdentity) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());
  Str chars(&scope, runtime_.newStrFromCStr("abc"));
  Str lstripped_empty_str(&scope, strStripLeft(thread_, empty_str, chars));
  EXPECT_EQ(*empty_str, *lstripped_empty_str);

  Str rstripped_empty_str(&scope, strStripRight(thread_, empty_str, chars));
  EXPECT_EQ(*empty_str, *rstripped_empty_str);

  Str stripped_empty_str(&scope, strStrip(thread_, empty_str, chars));
  EXPECT_EQ(*empty_str, *stripped_empty_str);
}

TEST_F(StrBuiltinsTest, StripWithFullyStrippableStrReturnsEmptyStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("bbbbaaaaccccdddd"));
  Str chars(&scope, runtime_.newStrFromCStr("abcd"));
  Str lstripped_str(&scope, strStripLeft(thread_, str, chars));
  EXPECT_EQ(lstripped_str.length(), 0);

  Str rstripped_str(&scope, strStripRight(thread_, str, chars));
  EXPECT_EQ(rstripped_str.length(), 0);

  Str stripped_str(&scope, strStrip(thread_, str, chars));
  EXPECT_EQ(stripped_str.length(), 0);
}

TEST_F(StrBuiltinsTest, StripWithEmptyCharsIsIdentity) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr(" Just another string "));
  Str chars(&scope, Str::empty());
  Str lstripped_str(&scope, strStripLeft(thread_, str, chars));
  EXPECT_EQ(*str, *lstripped_str);

  Str rstripped_str(&scope, strStripRight(thread_, str, chars));
  EXPECT_EQ(*str, *rstripped_str);

  Str stripped_str(&scope, strStrip(thread_, str, chars));
  EXPECT_EQ(*str, *stripped_str);
}

TEST_F(StrBuiltinsTest, StripBoth) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime_.newStrFromCStr("abcd"));
  Str stripped_str(&scope, strStrip(thread_, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str, "Hello Worl"));
}

TEST_F(StrBuiltinsTest, StripLeft) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime_.newStrFromCStr("abcd"));
  Str lstripped_str(&scope, strStripLeft(thread_, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str, "Hello Worldcab"));
}

TEST_F(StrBuiltinsTest, StripRight) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime_.newStrFromCStr("abcd"));
  Str rstripped_str(&scope, strStripRight(thread_, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str, "bcdHello Worl"));
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleReturnsZero) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".find("")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 0));
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleReturnsNegativeOne) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".find("", 8)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), -1));
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleAndSliceReturnsStart) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".find("", 3, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 3));
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleAndEmptySliceReturnsStart) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".find("", 3, 3)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 3));
}

TEST_F(StrBuiltinsTest, FindWithNegativeStartClipsToZero) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".find("h", -5, 1)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 0));
}

TEST_F(StrBuiltinsTest, FindWithEndPastEndOfStringClipsToLength) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".find("h", 0, 100)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 0));
}

TEST_F(StrBuiltinsTest, FindCallsDunderIndexOnStart) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return 4
result = "bbbbbbbb".find("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 4));
}

TEST_F(StrBuiltinsTest, FindCallsDunderIndexOnEnd) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return 5
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 4));
}

TEST_F(StrBuiltinsTest, FindClampsStartReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".find("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), -1));
}

TEST_F(StrBuiltinsTest, FindClampsEndReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 4));
}

TEST_F(StrBuiltinsTest, FindClampsEndReturningBigNegativeNumber) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return -46116860184273879030
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), -1));
}

TEST_F(StrBuiltinsTest, FindWithUnicodeReturnsCodePointIndex) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "Cr\u00e8me br\u00fbl\u00e9e"
result = s.find("e")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 4));
}

TEST_F(StrBuiltinsTest, FindWithStartAfterUnicodeCodePoint) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.find("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 8));
}

TEST_F(StrBuiltinsTest, FindWithDifferentSizeCodePoints) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "Cr\u00e8me \u10348 \u29D98 br\u00fbl\u00e9e"
result = s.find("\u29D98")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 9));
}

TEST_F(StrBuiltinsTest, FindWithOneCharStringFindsChar) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result1 = "hello".find("h")
result2 = "hello".find("e")
result3 = "hello".find("z")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result1"), 0));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result3"), -1));
}

TEST_F(StrBuiltinsTest, FindWithSlicePreservesIndices) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result1 = "hello".find("h", 1)
result2 = "hello".find("e", 1)
result3 = "hello".find("o", 0, 2)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result1"), -1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result3"), -1));
}

TEST_F(StrBuiltinsTest, FindWithMultiCharStringFindsSubstring) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result1 = "hello".find("he")
result2 = "hello".find("el")
result3 = "hello".find("ze")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result1"), 0));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result3"), -1));
}

TEST_F(StrBuiltinsTest, RfindWithOneCharStringFindsChar) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".rfind("l")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 3));
}

TEST_F(StrBuiltinsTest, RfindCharWithUnicodeReturnsCodePointIndex) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "Cr\u00e8me br\u00fbl\u00e9e"
result = s.rfind("e")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 11));
}

TEST_F(StrBuiltinsTest, RfindCharWithStartAfterUnicodeCodePoint) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.rfind("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 15));
}

TEST_F(StrBuiltinsTest, RfindCharWithDifferentSizeCodePoints) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "Cr\u00e8me \u10348 \u29D98 br\u00fbl\u00e9e\u2070E\u29D98 "
result = s.rfind("\u29D98")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 20));
}

TEST_F(StrBuiltinsTest, RfindWithMultiCharStringFindsSubstring) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "aabbaa".rfind("aa")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 4));
}

TEST_F(StrBuiltinsTest, RfindCharWithNegativeStartClipsToZero) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".rfind("h", -5, 1)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 0));
}

TEST_F(StrBuiltinsTest, RfindCharWithEndPastEndOfStringClipsToLength) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".rfind("h", 0, 100)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 0));
}

TEST_F(StrBuiltinsTest, RfindCallsDunderIndexOnEnd) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return 5
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 4));
}

TEST_F(StrBuiltinsTest, RfindClampsStartReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".rfind("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), -1));
}

TEST_F(StrBuiltinsTest, RfindClampsEndReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 7));
}

TEST_F(StrBuiltinsTest, RfindClampsEndReturningBigNegativeNumber) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __index__(self):
        return -46116860184273879030
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), -1));
}

TEST_F(StrBuiltinsTest, RfindCharWithEmptyNeedleReturnsLength) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".rfind("")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 5));
}

TEST_F(StrBuiltinsTest, RfindCharWithEmptyNeedleReturnsNegativeOne) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".rfind("", 8)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), -1));
}

TEST_F(StrBuiltinsTest, RfindCharWithEmptyNeedleAndSliceReturnsEnd) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".rfind("", 3, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 5));
}

TEST_F(StrBuiltinsTest, RfindWithEmptyNeedleAndEmptySliceReturnsEnd) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "hello".rfind("", 3, 3)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 3));
}

TEST_F(StrBuiltinsTest, IndexWithPresentSubstringReturnsIndex) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.index("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result"), 8));
}

TEST_F(StrBuiltinsTest, IndexWithMissingSubstringRaisesValueError) {
  EXPECT_TRUE(
      raised(runFromCStr(&runtime_, "'h'.index('q')"), LayoutId::kValueError));
}

TEST_F(StrBuiltinsTest, DunderHashReturnsSmallInt) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("hello world"));
  EXPECT_TRUE(runBuiltin(StrBuiltins::dunderHash, str).isSmallInt());
}

TEST_F(StrBuiltinsTest, DunderHashSmallStringReturnsSmallInt) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("h"));
  EXPECT_TRUE(runBuiltin(StrBuiltins::dunderHash, str).isSmallInt());
}

TEST_F(StrBuiltinsTest, DunderHashWithEquivalentStringsReturnsSameHash) {
  HandleScope scope(thread_);
  Str str1(&scope, runtime_.newStrFromCStr("hello world foobar"));
  Str str2(&scope, runtime_.newStrFromCStr("hello world foobar"));
  EXPECT_NE(*str1, *str2);
  Object result1(&scope, runBuiltin(StrBuiltins::dunderHash, str1));
  Object result2(&scope, runBuiltin(StrBuiltins::dunderHash, str2));
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_TRUE(result2.isSmallInt());
  EXPECT_EQ(*result1, *result2);
}

TEST_F(StringIterTest, SimpleIter) {
  HandleScope scope(thread_);

  Str str(&scope, runtime_.newStrFromCStr("test"));
  EXPECT_TRUE(str.equalsCStr("test"));

  StrIterator iter(&scope, runtime_.newStrIterator(str));
  Object ch(&scope, strIteratorNext(thread_, iter));
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("t"));

  ch = strIteratorNext(thread_, iter);
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("e"));

  ch = strIteratorNext(thread_, iter);
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("s"));

  ch = strIteratorNext(thread_, iter);
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("t"));

  ch = strIteratorNext(thread_, iter);
  ASSERT_TRUE(ch.isError());
}

TEST_F(StringIterTest, SetIndex) {
  HandleScope scope(thread_);

  Str str(&scope, runtime_.newStrFromCStr("test"));
  EXPECT_TRUE(str.equalsCStr("test"));

  StrIterator iter(&scope, runtime_.newStrIterator(str));
  iter.setIndex(1);
  Object ch(&scope, strIteratorNext(thread_, iter));
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("e"));

  iter.setIndex(5);
  ch = strIteratorNext(thread_, iter);
  // Index should not have advanced.
  ASSERT_EQ(iter.index(), 5);
  ASSERT_TRUE(ch.isError());
}

TEST_F(StrBuiltinsTest, DunderContainsWithNonStrSelfRaisesTypeError) {
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "str.__contains__(3, 'foo')"),
                     LayoutId::kTypeError));
}

TEST_F(StrBuiltinsTest, DunderContainsWithNonStrOtherRaisesTypeError) {
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "str.__contains__('foo', 3)"),
                     LayoutId::kTypeError));
}

TEST_F(StrBuiltinsTest, DunderContainsWithPresentSubstrReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.__contains__('foo', 'f')")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST_F(StrBuiltinsTest, DunderContainsWithNotPresentSubstrReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.__contains__('foo', 'q')")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(*result, Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithNonStrRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "str.isalnum(None)"),
                            LayoutId::kTypeError,
                            "isalnum expected 'str' but got NoneType"));
}

TEST_F(StrBuiltinsTest, IsalnumWithEmptyStringReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isalnum('')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithCharacterBelowZeroReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isalnum('/')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithCharacterAboveNineReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isalnum(':')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithNumbersReturnsTrue) {
  ASSERT_FALSE(
      runFromCStr(&runtime_,
                  "result = all([str.isalnum(x) for x in '0123456789'])")
          .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithCharacterBelowLowerAReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isalnum('`')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithCharacterAboveLowerZReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isalnum('{')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithLowercaseLettersReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_,
                           "result = all([str.isalnum(x) for x in "
                           "'abcdefghijklmnopqrstuvwxyz'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithCharacterBelowUpperAReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isalnum('@')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithCharacterAboveUpperZReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isalnum('[')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsalnumWithUppercaseLettersReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_,
                           "result = all([str.isalnum(x) for x in "
                           "'ABCDEFGHIJKLMNOPQRSTUVWXYZ'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsspaceWithEmptyStringReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = ''.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsspaceWithNonSpaceReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = ' a '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsspaceWithNewlineReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = ' \n '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsspaceWithTabReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = ' \t '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsspaceWithCarriageReturnReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = ' \r '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsupperWithNonStrRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "str.isupper(None)"),
                            LayoutId::kTypeError,
                            "isupper expected 'str' but got NoneType"));
}

TEST_F(StrBuiltinsTest, IsupperWithEmptyStringReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isupper('')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsupperWithCharacterBelowUpperAReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isupper('@')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsupperWithCharacterAboveUpperZReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.isupper('[')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsupperWithUppercaseLettersReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_,
                           "result = all([str.isupper(x) for x in "
                           "'ABCDEFGHIJKLMNOPQRSTUVWXYZ'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IslowerWithNonStrRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "str.islower(None)"),
                            LayoutId::kTypeError,
                            "islower expected 'str' but got NoneType"));
}

TEST_F(StrBuiltinsTest, IslowerWithEmptyStringReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.islower('')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IslowerWithCharacterBelowLowerAReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.islower('`')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IslowerWithCharacterAboveLowerZReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = str.islower('{')").isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IslowerWithLowercaseLettersReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_,
                           "result = all([str.islower(x) for x in "
                           "'abcdefghijklmnopqrstuvwxyz'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, UpperOnASCIILettersReturnsUpperCaseString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "hello".upper()
b = "HeLLo".upper()
c = "hellO".upper()
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Object c(&scope, moduleAt(&runtime_, "__main__", "c"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(*c, "HELLO"));
}

TEST_F(StrBuiltinsTest, UpperOnASCIILettersOfSubClassReturnsUpperCaseString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
a = SubStr("hello").upper()
b = SubStr("HeLLo").upper()
c = SubStr("hellO").upper()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "a"), "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "b"), "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "c"), "HELLO"));
}

TEST_F(StrBuiltinsTest, UpperOnUppercaseASCIILettersReturnsSameString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "HELLO".upper()
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "HELLO"));
}

TEST_F(StrBuiltinsTest, UpperOnNumbersReturnsSameString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = "foo 123".upper()
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "FOO 123"));
}

TEST_F(StrBuiltinsTest, CapitalizeWithNonStrRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "str.capitalize(1)"), LayoutId::kTypeError,
      "'capitalize' requires a 'str' instance but got 'int'"));
}

TEST_F(StrBuiltinsTest, CapitalizeReturnsCapitalizedStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "foo".capitalize()
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"), "Foo"));
}

TEST_F(StrBuiltinsTest, CapitalizeUpperCaseReturnsUnmodifiedStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "Foo".capitalize()
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"), "Foo"));
}

TEST_F(StrBuiltinsTest, CapitalizeAllUppercaseReturnsCapitalizedStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "FOO".capitalize()
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"), "Foo"));
}

TEST_F(StrBuiltinsTest, CapitalizeWithEmptyStrReturnsEmptyStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"), ""));
}

TEST_F(StrBuiltinsTest, IsidentifierWithEmptyStringReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsidentifierWithNumberReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "9".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsidentifierWithPeriodReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = ".".isidentifier()
print(result)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, IsidentifierWithLowercaseLetterReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "a".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsidentifierWithUppercaseLetterReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "A".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsidentifierWithUnderscoreReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "_".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsidentifierWithOnlyLettersReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "abc".isidentifier()
print(result)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, IsidentifierWithLettersAndNumbersReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = "abc213".isidentifier()
print(result)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, StrUnderlyingWithStrReturnsSameStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("hello"));
  Object underlying(&scope, strUnderlying(thread_, str));
  EXPECT_EQ(*str, *underlying);
}

TEST_F(StrBuiltinsTest, StrUnderlyingWithSubClassReturnsUnderlyingStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class SubStr(str): pass
substr = SubStr("some string")
)")
                   .isError());
  Object substr(&scope, moduleAt(&runtime_, "__main__", "substr"));
  ASSERT_FALSE(substr.isStr());
  Object underlying(&scope, strUnderlying(thread_, substr));
  EXPECT_TRUE(isStrEqualsCStr(*underlying, "some string"));
}

}  // namespace python
