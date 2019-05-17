#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(StrBuiltinsTest, BuiltinBase) {
  Runtime runtime;
  HandleScope scope;

  Type small_str(&scope, runtime.typeAt(LayoutId::kSmallStr));
  EXPECT_EQ(small_str.builtinBase(), LayoutId::kStr);

  Type large_str(&scope, runtime.typeAt(LayoutId::kLargeStr));
  EXPECT_EQ(large_str.builtinBase(), LayoutId::kStr);

  Type str(&scope, runtime.typeAt(LayoutId::kStr));
  EXPECT_EQ(str.builtinBase(), LayoutId::kStr);
}

TEST(StrBuiltinsTest, RichCompareStringEQ) {
  const char* src = R"(
a = "__main__"
if (a == "__main__"):
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "foo\n");
}

TEST(StrBuiltinsTest, RichCompareStringEQWithSubClass) {
  const char* src = R"(
class SubStr(str): pass
a = SubStr("__main__")
if (a == "__main__"):
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "foo\n");
}

TEST(StrBuiltinsTest, RichCompareStringNE) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "__main__"
result = "bar"
if (a != "__main__"):
  result = "foo"
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "bar"));
}

TEST(StrBuiltinsTest, RichCompareStringNEWithSubClass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
a = SubStr("apple")
result = "bar"
if (a != "apple"):
  result = "foo"
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "bar"));
}

TEST(StrBuiltinsTest, RichCompareSingleCharLE) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a_le_b = 'a' <= 'b'
b_le_a = 'a' >= 'b'
a_le_a = 'a' <= 'a'
)")
                   .isError());

  HandleScope scope;

  Object a_le_b(&scope, moduleAt(&runtime, "__main__", "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());

  Object b_le_a(&scope, moduleAt(&runtime, "__main__", "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());

  Object a_le_a(&scope, moduleAt(&runtime, "__main__", "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
}

TEST(StrBuiltinsTest, RichCompareSingleCharLEWithSubClass) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class S(str): pass
a_le_b = S('a') <= S('b')
b_le_a = S('a') >= S('b')
a_le_a = S('a') <= S('a')
)")
                   .isError());

  EXPECT_EQ(moduleAt(&runtime, "__main__", "a_le_b"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "b_le_a"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "a_le_a"), Bool::trueObj());
}

TEST(StrBuiltinsTest, LowerOnASCIILettersReturnsLowerCaseString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "HELLO".lower()
b = "HeLLo".lower()
c = "hellO".lower()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "hello"));
  EXPECT_TRUE(isStrEqualsCStr(*c, "hello"));
}

TEST(StrBuiltinsTest, LowerOnASCIILettersWithSubClassReturnsLowerCaseString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
a = SubStr("HELLO").lower()
b = SubStr("HeLLo").lower()
c = SubStr("hellO").lower()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "a"), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "b"), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "c"), "hello"));
}

TEST(StrBuiltinsTest, LowerOnLowercaseASCIILettersReturnsSameString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".lower()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
}

TEST(StrBuiltinsTest, LowerOnNumbersReturnsSameString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "foo 123".lower()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo 123"));
}

TEST(StrBuiltinsTest, DunderNewCallsDunderStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
    def __str__(self):
        return "foo"
a = str.__new__(str, Foo())
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST(StrBuiltinsTest, DunderNewCallsReprIfNoDunderStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass
f = Foo()
a = str.__new__(str, f)
b = repr(f)
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST(StrBuiltinsTest, DunderNewWithNoArgsExceptTypeReturnsEmptyString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = str.__new__(str)
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, ""));
}

TEST(StrBuiltinsTest, DunderNewWithStrReturnsSameStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = str.__new__(str, "hello")
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
}

TEST(StrBuiltinsTest, DunderNewWithTypeCallsTypeDunderStr) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "a = str.__new__(str, int)").isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "<class 'int'>"));
}

TEST(StrBuiltinsTest, DunderNewWithNoArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "str.__new__()"), LayoutId::kTypeError,
      "TypeError: 'str.__new__' takes min 1 positional arguments but 0 given"));
}

TEST(StrBuiltinsTest, DunderNewWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "str.__new__(str, 1, 2, 3, 4)"),
      LayoutId::kTypeError,
      "TypeError: 'str.__new__' takes max 4 positional arguments but 5 given"));
}

TEST(StrBuiltinsTest, DunderNewWithNonTypeArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "str.__new__(1)"),
                            LayoutId::kTypeError, "cls is not a type object"));
}

TEST(StrBuiltinsTest, DunderNewWithNonSubtypeArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "str.__new__(object)"),
                            LayoutId::kTypeError,
                            "cls is not a subtype of str"));
}

TEST(StrBuiltinsTest, DunderAddWithTwoStringsReturnsConcatenatedString) {
  Runtime runtime;
  HandleScope scope;
  Object str1(&scope, runtime.newStrFromCStr("hello"));
  Object str2(&scope, runtime.newStrFromCStr("world"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "helloworld"));
}

TEST(StrBuiltinsTest,
     DunderAddWithTwoStringsOfSubClassReturnsConcatenatedString) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
str1 = SubStr("hello")
str2 = SubStr("world")
)")
                   .isError());
  Object str1(&scope, moduleAt(&runtime, "__main__", "str1"));
  Object str2(&scope, moduleAt(&runtime, "__main__", "str2"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "helloworld"));
}

TEST(StrBuiltinsTest, DunderAddWithLeftEmptyAndReturnsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str1(&scope, Str::empty());
  Object str2(&scope, runtime.newStrFromCStr("world"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "world"));
}

TEST(StrBuiltinsTest, DunderAddWithRightEmptyAndReturnsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str1(&scope, runtime.newStrFromCStr("hello"));
  Object str2(&scope, Str::empty());
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello"));
}

TEST(StrBuiltinsTest, PlusOperatorOnStringsEqualsDunderAdd) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello"
b = "world"
c = a + b
d = a.__add__(b)
)")
                   .isError());
  HandleScope scope;
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));

  EXPECT_TRUE(isStrEqualsCStr(*c, "helloworld"));
  EXPECT_TRUE(isStrEqualsCStr(*d, "helloworld"));
}

TEST(StrBuiltinsTest, DunderBoolWithEmptyStringReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, Str::empty());
  EXPECT_EQ(runBuiltin(StrBuiltins::dunderBool, str), Bool::falseObj());
}

TEST(StrBuiltinsTest, DunderBoolWithNonEmptyStringReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("hello"));
  EXPECT_EQ(runBuiltin(StrBuiltins::dunderBool, str), Bool::trueObj());
}

TEST(StrBuiltinsTest, DunderBoolWithNonEmptyStringOfSubClassReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr("hello")
)")
                   .isError());
  Object substr(&scope, moduleAt(&runtime, "__main__", "substr"));
  EXPECT_EQ(runBuiltin(StrBuiltins::dunderBool, substr), Bool::trueObj());
}

TEST(StrBuiltinsTest, DunderLenReturnsLength) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l1 = len("aloha")
l2 = str.__len__("aloha")
l3 = "aloha".__len__()
)")
                   .isError());
  HandleScope scope;
  Object l1(&scope, moduleAt(&runtime, "__main__", "l1"));
  Object l2(&scope, moduleAt(&runtime, "__main__", "l2"));
  Object l3(&scope, moduleAt(&runtime, "__main__", "l3"));
  EXPECT_TRUE(isIntEqualsWord(*l1, 5));
  EXPECT_TRUE(isIntEqualsWord(*l2, 5));
  EXPECT_TRUE(isIntEqualsWord(*l3, 5));
}

TEST(StrBuiltinsTest, StringLenWithEmptyString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "l = len('')").isError());
  HandleScope scope;
  Object length(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_TRUE(isIntEqualsWord(*length, 0));
}

TEST(StrBuiltinsTest, DunderLenWithNonAsciiReturnsCodePointLength) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "l = len('\xc3\xa9')").isError());
  HandleScope scope;
  SmallInt length(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_TRUE(isIntEqualsWord(*length, 1));
}

TEST(StrBuiltinsTest, DunderLenWithIntRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "l = str.__len__(None)"), LayoutId::kTypeError,
      "'__len__' requires a 'str' object but got 'NoneType'"));
}

TEST(StrBuiltinsTest, DunderLenWithExtraArgumentRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "l = 'aloha'.__len__('arg')"), LayoutId::kTypeError,
      "TypeError: 'str.__len__' takes max 1 positional arguments but 2 given"));
}

TEST(StrBuiltinsTest, DunderMulWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "str.__mul__(None, 1)"), LayoutId::kTypeError,
      "'__mul__' requires a 'str' object but got 'NoneType'"));
}

TEST(StrBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  Object count(&scope, runtime.newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(StrBuiltins::dunderMul, self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST(StrBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoo"));
}

TEST(StrBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST(StrBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, moduleAt(&runtime, "__main__", "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST(StrBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, Str::empty());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime.newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST(StrBuiltinsTest, DunderMulWithOverflowRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raisedWithStr(runBuiltin(StrBuiltins::dunderMul, self, count),
                            LayoutId::kOverflowError,
                            "repeated string is too long"));
}

TEST(StrBuiltinsTest, DunderMulWithEmptyBytesReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, Str::empty());
  Object count(&scope, runtime.newInt(10));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST(StrBuiltinsTest, DunderMulWithNegativeReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST(StrBuiltinsTest, DunderMulWithZeroReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST(StrBuiltinsTest, DunderMulWithOneReturnsSamStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foo"));
}

TEST(StrBuiltinsTest, DunderMulWithSmallStrReturnsRepeatedSmallStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoo"));
}

TEST(StrBuiltinsTest, DunderMulWithSmallStrReturnsRepeatedLargeStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoofoo"));
}

TEST(StrBuiltinsTest, DunderMulWithLargeStrReturnsRepeatedLargeStr) {
  Runtime runtime;
  HandleScope scope;
  Object self(&scope, runtime.newStrFromCStr("foobarbaz"));
  Object count(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(StrBuiltins::dunderMul, self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobarbazfoobarbaz"));
}

TEST(StrBuiltinsTest, DunderRmulCallsDunderMul) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = 3 * 'foo'").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoofoo"));
}

TEST(StrBuiltinsTest, IndexWithLargeIntRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Int index(&scope, runtime.newInt(RawSmallInt::kMaxValue + 1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(raised(*result, LayoutId::kIndexError));
}

TEST(StrBuiltinsTest, IndexWithNegativeIntIndexesFromEnd) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Int index(&scope, RawSmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(isStrEqualsCStr(*result, "h"));
}

TEST(StrBuiltinsTest, IndexWithLessThanNegativeLenRaisesIndexError) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Int index(&scope, RawSmallInt::fromWord(-6));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(raised(*result, LayoutId::kIndexError));
}

TEST(StrBuiltinsTest, IndexWithNonNegativeIntIndexesFromBeginning) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Int index(&scope, RawSmallInt::fromWord(4));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(isStrEqualsCStr(*result, "o"));
}

TEST(StrBuiltinsTest, IndexWithSubClassAndNonNegativeIntIndexesFromBeginning) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr("hello")
)")
                   .isError());
  Object hello(&scope, moduleAt(&runtime, "__main__", "substr"));
  Int index(&scope, RawSmallInt::fromWord(4));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, index));
  EXPECT_TRUE(isStrEqualsCStr(*result, "o"));
}

TEST(StrBuiltinsTest, IndexWithSliceWithPositiveInts) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(1));
  slice.setStop(SmallInt::fromWord(2));
  Object result_a(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_a, "e"));
  slice.setStop(SmallInt::fromWord(4));
  Object result_b(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_b, "ell"));
}

TEST(StrBuiltinsTest, IndexWithSliceWithNegativeInts) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(-1));
  Object result_a(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_a, "o"));
  slice.setStart(SmallInt::fromWord(1));
  slice.setStop(SmallInt::fromWord(-2));
  Object result_b(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result_b, "el"));
}

TEST(StrBuiltinsTest, IndexWithSliceWithStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Slice slice(&scope, runtime.newSlice());
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

TEST(StrBuiltinsTest, EmptyStringIndexWithSliceWithNegativeOneStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, Str::empty());
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(-1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST(StrBuiltinsTest, IndexWithSliceWithNegativeOneStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(-1));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result, "olleh"));
}

TEST(StrBuiltinsTest, IndexWithSliceWithNegativeTwoStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(-2));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  EXPECT_TRUE(isStrEqualsCStr(*result, "olh"));
}

TEST(StrBuiltinsTest, InternStringsInTupleInternsItems) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(3));
  Str str0(&scope, runtime.newStrFromCStr("a"));
  Str str1(&scope, runtime.newStrFromCStr("hello world"));
  Str str2(&scope, runtime.newStrFromCStr("hello world foobar"));
  EXPECT_TRUE(runtime.isInternedStr(thread, str0));
  EXPECT_FALSE(runtime.isInternedStr(thread, str1));
  EXPECT_FALSE(runtime.isInternedStr(thread, str2));
  tuple.atPut(0, *str0);
  tuple.atPut(1, *str1);
  tuple.atPut(2, *str2);
  strInternInTuple(thread, tuple);
  str0 = tuple.at(0);
  str1 = tuple.at(1);
  str2 = tuple.at(2);
  EXPECT_TRUE(runtime.isInternedStr(thread, str0));
  EXPECT_TRUE(runtime.isInternedStr(thread, str1));
  EXPECT_TRUE(runtime.isInternedStr(thread, str2));
}

TEST(StrBuiltinsTest, InternStringConstantsInternsAlphanumericStringsInTuple) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, runtime.newTuple(3));
  Str str0(&scope, runtime.newStrFromCStr("_"));
  Str str1(&scope, runtime.newStrFromCStr("hello world"));
  Str str2(&scope, runtime.newStrFromCStr("helloworldfoobar"));
  EXPECT_TRUE(runtime.isInternedStr(thread, str0));
  EXPECT_FALSE(runtime.isInternedStr(thread, str1));
  EXPECT_FALSE(runtime.isInternedStr(thread, str2));
  tuple.atPut(0, *str0);
  tuple.atPut(1, *str1);
  tuple.atPut(2, *str2);
  strInternConstants(thread, tuple);
  str0 = tuple.at(0);
  str1 = tuple.at(1);
  str2 = tuple.at(2);
  EXPECT_TRUE(runtime.isInternedStr(thread, str0));
  EXPECT_FALSE(runtime.isInternedStr(thread, str1));
  EXPECT_TRUE(runtime.isInternedStr(thread, str2));
}

TEST(StrBuiltinsTest, InternStringConstantsInternsStringsInNestedTuples) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple outer(&scope, runtime.newTuple(3));
  outer.atPut(0, SmallInt::fromWord(0));
  outer.atPut(1, SmallInt::fromWord(1));
  Tuple inner(&scope, runtime.newTuple(3));
  outer.atPut(2, *inner);
  Str str0(&scope, runtime.newStrFromCStr("_"));
  Str str1(&scope, runtime.newStrFromCStr("hello world"));
  Str str2(&scope, runtime.newStrFromCStr("helloworldfoobar"));
  EXPECT_TRUE(runtime.isInternedStr(thread, str0));
  EXPECT_FALSE(runtime.isInternedStr(thread, str1));
  EXPECT_FALSE(runtime.isInternedStr(thread, str2));
  inner.atPut(0, *str0);
  inner.atPut(1, *str1);
  inner.atPut(2, *str2);
  strInternConstants(thread, outer);
  str0 = inner.at(0);
  str1 = inner.at(1);
  str2 = inner.at(2);
  EXPECT_TRUE(runtime.isInternedStr(thread, str0));
  EXPECT_FALSE(runtime.isInternedStr(thread, str1));
  EXPECT_TRUE(runtime.isInternedStr(thread, str2));
}

TEST(StrBuiltinsTest, InternStringConstantsInternsStringsInFrozenSetsInTuples) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple outer(&scope, runtime.newTuple(3));
  outer.atPut(0, SmallInt::fromWord(0));
  outer.atPut(1, SmallInt::fromWord(1));
  FrozenSet inner(&scope, runtime.newFrozenSet());
  outer.atPut(2, *inner);
  Str str0(&scope, runtime.newStrFromCStr("alpharomeo"));
  Str str1(&scope, runtime.newStrFromCStr("hello world"));
  Str str2(&scope, runtime.newStrFromCStr("helloworldfoobar"));
  EXPECT_FALSE(runtime.isInternedStr(thread, str0));
  EXPECT_FALSE(runtime.isInternedStr(thread, str1));
  EXPECT_FALSE(runtime.isInternedStr(thread, str2));
  runtime.setAdd(thread, inner, str0);
  runtime.setAdd(thread, inner, str1);
  runtime.setAdd(thread, inner, str2);
  strInternConstants(thread, outer);
  inner = outer.at(2);
  Tuple data(&scope, inner.data());
  bool all_interned = true;
  bool some_interned = false;
  for (word idx = Set::Bucket::kFirst; Set::Bucket::nextItem(*data, &idx);) {
    Str obj(&scope, Set::Bucket::key(*data, idx));
    bool interned = runtime.isInternedStr(thread, obj);
    all_interned &= interned;
    some_interned |= interned;
  }
  EXPECT_FALSE(all_interned);
  EXPECT_TRUE(some_interned);
}

TEST(StrBuiltinsTest, StartsWithEmptyStringReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith("")
b = "".startswith("")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(StrBuiltinsTest, StartsWithStringReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith("h")
b = "hello".startswith("he")
c = "hello".startswith("hel")
d = "hello".startswith("hell")
e = "hello".startswith("hello")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  Bool e(&scope, moduleAt(&runtime, "__main__", "e"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_TRUE(d.value());
  EXPECT_TRUE(e.value());
}

TEST(StrBuiltinsTest, StartsWithTooLongPrefixReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith("hihello")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST(StrBuiltinsTest, StartsWithUnrelatedPrefixReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith("bob")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST(StrBuiltinsTest, StartsWithStart) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith("e", 1)
b = "hello".startswith("o", 5)
c = "hello".startswith("ell", 1)
d = "hello".startswith("llo", 3)
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST(StrBuiltinsTest, StartsWithStartAndEnd) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith("e", 1, 3)
b = "hello".startswith("el", 1, 4)
c = "hello".startswith("ll", 2, 5)
d = "hello".startswith("ll", 1, 4)
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST(StrBuiltinsTest, StartsWithStartAndEndNegatives) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith("h", 0, -1)
b = "hello".startswith("ll", -3)
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(StrBuiltinsTest, StartsWithTupleOfPrefixes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".startswith(("h", "lo"))
b = "hello".startswith(("asdf", "foo", "bar"))
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
}

TEST(StrBuiltinsTest, EndsWithEmptyStringReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith("")
b = "".endswith("")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(StrBuiltinsTest, EndsWithStringReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith("o")
b = "hello".endswith("lo")
c = "hello".endswith("llo")
d = "hello".endswith("ello")
e = "hello".endswith("hello")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  Bool e(&scope, moduleAt(&runtime, "__main__", "e"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_TRUE(d.value());
  EXPECT_TRUE(e.value());
}

TEST(StrBuiltinsTest, EndsWithTooLongSuffixReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith("hihello")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST(StrBuiltinsTest, EndsWithUnrelatedSuffixReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith("bob")
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a.value());
}

TEST(StrBuiltinsTest, EndsWithStart) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith("o", 1)
b = "hello".endswith("o", 5)
c = "hello".endswith("llo", 1)
d = "hello".endswith("llo", 3)
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST(StrBuiltinsTest, EndsWithStartAndEnd) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith("l", 1, 3)
b = "hello".endswith("ll", 1, 4)
c = "hello".endswith("lo", 2, 5)
d = "hello".endswith("llo", 1, 4)
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
  EXPECT_FALSE(d.value());
}

TEST(StrBuiltinsTest, EndsWithStartAndEndNegatives) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith("l", 0, -1)
b = "hello".endswith("o", -1)
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(StrBuiltinsTest, EndsWithTupleOfSuffixes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".endswith(("o", "llo"))
b = "hello".endswith(("asdf", "foo", "bar"))
)")
                   .isError());
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a.value());
  EXPECT_FALSE(b.value());
}

TEST(StrBuiltinsTest, StringFormat) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
n = 123
f = 3.14
s = "pyros"
a = "hello %d %g %s" % (n, f, s)
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "hello 123 3.14 pyros"));
}

TEST(StrBuiltinsTest, StringFormatSingleString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "pyro"
a = "%s" % s
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "pyro"));
}

TEST(StrBuiltinsTest, StringFormatTwoStrings) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "pyro"
a = "%s%s" % (s, s)
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "pyropyro"));
}

TEST(StrBuiltinsTest, StringFormatMixed) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "pyro"
a = "1%s,2%s,3%s,4%s,5%s" % (s, s, s, s, s)
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "1pyro,2pyro,3pyro,4pyro,5pyro"));
}

TEST(StrBuiltinsTest, StringFormatMixed2) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "pyro"
a = "%d%s,%d%s,%d%s" % (1, s, 2, s, 3, s)
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "1pyro,2pyro,3pyro"));
}

TEST(StrBuiltinsTest, StringFormatMalformed) {
  Runtime runtime;
  const char* src = R"(
a = "%" % ("pyro",)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "Incomplete format"));
}

TEST(StrBuiltinsTest, StringFormatMismatch) {
  Runtime runtime;
  const char* src = R"(
a = "%d%s" % ("pyro",)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "Argument mismatch"));
}

TEST(StrBuiltinsTest, DunderReprOnASCIIStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".__repr__()
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'hello'"));
}

TEST(StrBuiltinsTest, DunderReprOnASCIIStrOfSubClass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr("hello")
a = substr.__repr__()
  )")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "a"), "'hello'"));
}

TEST(StrBuiltinsTest, DunderReprOnASCIINonPrintable) {
  Runtime runtime;
  // 6 is the ACK character.
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "\x06".__repr__()
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'\\x06'"));
}

TEST(StrBuiltinsTest, DunderReprOnStrWithDoubleQuotes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = 'hello "world"'.__repr__()
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'hello \"world\"'"));
}

TEST(StrBuiltinsTest, DunderReprOnStrWithSingleQuotes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello 'world'".__repr__()
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "\"hello 'world'\""));
}

TEST(StrBuiltinsTest, DunderReprOnStrWithBothQuotes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello 'world', I am your \"father\"".__repr__()
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, R"('hello \'world\', I am your "father"')"));
}

TEST(StrBuiltinsTest, DunderReprOnStrWithNestedQuotes) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello 'world, \"I am 'your \"father\"'\"'".__repr__()
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(
      isStrEqualsCStr(*a, R"('hello \'world, "I am \'your "father"\'"\'')"));
}

TEST(StrBuiltinsTest, DunderReprOnCommonEscapeSequences) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "\n \t \r \\".__repr__()
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "'\\n \\t \\r \\\\'"));
}

TEST(StrBuiltinsTest, DunderStr) {
  const char* src = R"(
result = 'Hello, World!'.__str__()
)";
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello, World!"));
}

TEST(StrBuiltinsTest, JoinWithEmptyArray) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = ",".join([])
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, ""));
}

TEST(StrBuiltinsTest, JoinWithOneElementArray) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = ",".join(["1"])
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "1"));
}

TEST(StrBuiltinsTest, JoinWithManyElementArray) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = ",".join(["1", "2", "3"])
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "1,2,3"));
}

TEST(StrBuiltinsTest, JoinWithManyElementArrayAndEmptySeparator) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "".join(["1", "2", "3"])
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "123"));
}

TEST(StrBuiltinsTest, JoinWithIterable) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = ",".join(("1", "2", "3"))
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "1,2,3"));
}

TEST(StrBuiltinsTest, JoinWithNonStringInArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = ",".join(["hello", 1])
)"),
                            LayoutId::kTypeError,
                            "sequence item 1: expected str instance"));
}

TEST(StrBuiltinsTest, JoinWithNonStringSeparatorRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
a = str.join(None, ["hello", 1])
)"),
                    LayoutId::kTypeError,
                    "'join' requires a 'str' object but got 'NoneType'"));
}

TEST(StrBuiltinsTest, PartitionOnSingleCharStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".partition("l")
)")
                   .isError());
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "lo"));
}

TEST(StrBuiltinsTest, PartitionOnMultiCharStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".partition("ll")
)")
                   .isError());
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "o"));
}

TEST(StrBuiltinsTest, PartitionOnSuffix) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".partition("lo")
b = "hello".partition("lop")
)")
                   .isError());
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));
  Tuple b(&scope, moduleAt(&runtime, "__main__", "b"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), ""));

  ASSERT_EQ(b.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), ""));
}

TEST(StrBuiltinsTest, PartitionOnPrefix) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".partition("he")
b = "hello".partition("hex")
)")
                   .isError());
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));
  Tuple b(&scope, moduleAt(&runtime, "__main__", "b"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "llo"));

  ASSERT_EQ(b.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), ""));
}

TEST(StrBuiltinsTest, PartitionLargerStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".partition("abcdefghijk")
)")
                   .isError());
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), ""));
}

TEST(StrBuiltinsTest, PartitionEmptyStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "".partition("a")
)")
                   .isError());
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), ""));
}

TEST(StrBuiltinsTest, SplitWithOneCharSeparator) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".split("e")
b = "hello".split("l")
)")
                   .isError());
  HandleScope scope;

  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "llo"));

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "o"));
}

TEST(StrBuiltinsTest, SplitWithEmptySelfReturnsSingleEmptyString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "".split("a")
)")
                   .isError());
  HandleScope scope;
  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), ""));
}

TEST(StrBuiltinsTest, SplitWithMultiCharSeparator) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".split("el")
b = "hello".split("ll")
c = "hello".split("hello")
d = "hellllo".split("ll")
)")
                   .isError());
  HandleScope scope;

  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "o"));

  List c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_EQ(c.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(c.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(c.at(1), ""));

  List d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_EQ(d.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(d.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(d.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(d.at(2), "o"));
}

TEST(StrBuiltinsTest, SplitWithMaxSplitZeroReturnsList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".split("x", 0)
b = "hello".split("l", 0)
)")
                   .isError());
  HandleScope scope;
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b_obj(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a_obj.isList());
  ASSERT_TRUE(b_obj.isList());
  List a(&scope, *a_obj);
  List b(&scope, *b_obj);
  ASSERT_EQ(a.numItems(), 1);
  ASSERT_EQ(b.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
}

TEST(StrBuiltinsTest, SplitWithMaxSplitBelowNumPartsStopsEarly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".split("l", 1)
b = "1,2,3,4".split(",", 2)
)")
                   .isError());
  HandleScope scope;

  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "3,4"));
}

TEST(StrBuiltinsTest, SplitWithMaxSplitGreaterThanNumParts) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".split("l", 2)
b = "1,2,3,4".split(",", 5)
)")
                   .isError());
  HandleScope scope;
  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "o"));

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b.numItems(), 4);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "3"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(3), "4"));
}

TEST(StrBuiltinsTest, SplitEmptyStringWithNoSepReturnsEmptyList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "".split()
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 0);
}

TEST(StrBuiltinsTest, SplitWhitespaceStringWithNoSepReturnsEmptyList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "  \t\n  ".split()
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 0);
}

TEST(StrBuiltinsTest, SplitWhitespaceReturnsComponentParts) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "  \t\n  hello\t\n world".split()
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST(StrBuiltinsTest,
     SplitWhitespaceWithMaxsplitEqualsNegativeOneReturnsAllResults) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "  \t\n  hello\t\n world".split(maxsplit=-1)
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST(StrBuiltinsTest,
     SplitWhitespaceWithMaxsplitEqualsZeroReturnsOneElementList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "  \t\n  hello   world   ".split(maxsplit=0)
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello   world   "});
}

TEST(StrBuiltinsTest,
     SplitWhitespaceWithMaxsplitEqualsOneReturnsTwoElementList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "  \t\n  hello world ".split(maxsplit=1)
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "world "));
}

TEST(StrBuiltinsTest, SplitlinesSplitsOnLineBreaks) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello\nworld\rwhats\r\nup".splitlines()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world", "whats", "up"});
}

TEST(StrBuiltinsTest, SplitlinesWithKeependsKeepsLineBreaks) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello\nworld\rwhats\r\nup".splitlines(keepends=True)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello\n", "world\r", "whats\r\n", "up"});
}

TEST(StrBuiltinsTest, SplitlinesWithNoNewlinesReturnsIdEqualString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "hello world foo bar"
[result] = s.splitlines()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "s"),
            moduleAt(&runtime, "__main__", "result"));
}

TEST(StrBuiltinsTest, SplitlinesWithMultiByteNewlineSplitsLine) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello\u2028world".splitlines()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST(StrBuiltinsTest, SplitlinesWithMultiByteNewlineAndKeependsSplitsLine) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello\u2028world".splitlines(keepends=True)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {u8"hello\u2028", "world"});
}

TEST(StrBuiltinsTest, RpartitionOnSingleCharStrPartitionsCorrectly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "hello".rpartition("l")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "l"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST(StrBuiltinsTest, RpartitionOnMultiCharStrPartitionsCorrectly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "hello".rpartition("ll")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "ll"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST(StrBuiltinsTest, RpartitionOnSuffixPutsEmptyStrAtEndOfResult) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "hello".rpartition("lo")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST(StrBuiltinsTest, RpartitionOnNonExistentSuffixPutsStrAtEndOfResult) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "hello".rpartition("lop")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST(StrBuiltinsTest, RpartitionOnPrefixPutsEmptyStrAtBeginningOfResult) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "hello".rpartition("he")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "llo"));
}

TEST(StrBuiltinsTest, RpartitionOnNonExistentPrefixPutsStrAtEndOfResult) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "hello".rpartition("hex")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST(StrBuiltinsTest, RpartitionLargerStrPutsStrAtEndOfResult) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "hello".rpartition("foobarbaz")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "hello"));
}

TEST(StrBuiltinsTest, RpartitionEmptyStrReturnsTupleOfEmptyStrings) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t = "".rpartition("a")
)")
                   .isError());
  HandleScope scope;
  Tuple result(&scope, moduleAt(&runtime, "__main__", "t"));
  ASSERT_EQ(result.length(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST(StrBuiltinsTest, RsplitWithOneCharSeparatorSplitsCorrectly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hello".rsplit("e")
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "llo"));
}

TEST(StrBuiltinsTest, RsplitWithRepeatedOneCharSeparatorSplitsCorrectly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hello".rsplit("l")
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST(StrBuiltinsTest, RsplitWithEmptySelfReturnsSingleEmptyString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "".rsplit("a")
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
}

TEST(StrBuiltinsTest, RsplitWithMultiCharSeparatorSplitsFromRight) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hello".rsplit("el")
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
}

TEST(StrBuiltinsTest, RsplitWithRepeatedCharSeparatorSplitsFromRight) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hello".rsplit("ll")
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "o"));
}

TEST(StrBuiltinsTest, RsplitWithSeparatorSameAsInputSplitsIntoEmptyComponents) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hello".rsplit("hello")
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
}

TEST(StrBuiltinsTest,
     RsplitWithMultiCharSeparatorWithMultipleAppearancesSplitsCorrectly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hellllo".rsplit("ll")
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST(StrBuiltinsTest, RsplitWithMaxSplitZeroReturnsList) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".rsplit("x", 0)
b = "hello".rsplit("l", 0)
)")
                   .isError());
  HandleScope scope;
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b_obj(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(a_obj.isList());
  ASSERT_TRUE(b_obj.isList());
  List a(&scope, *a_obj);
  List b(&scope, *b_obj);
  ASSERT_EQ(a.numItems(), 1);
  ASSERT_EQ(b.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "hello"));
}

TEST(StrBuiltinsTest,
     RsplitWithRepeatedCharAndMaxSplitBelowNumPartsStopsEarly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hello".rsplit("l", 1)
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "o"));
}

TEST(StrBuiltinsTest, RsplitWithMaxSplitBelowNumPartsStopsEarly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "1,2,3,4".rsplit(",", 2)
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "1,2"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "3"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "4"));
}

TEST(StrBuiltinsTest,
     RsplitWithRepeatedCharAndMaxSplitGreaterThanNumPartsSplitsCorrectly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "hello".rsplit("l", 2)
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST(StrBuiltinsTest, RsplitWithMaxSplitGreaterThanNumPartsSplitsCorrectly) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = "1,2,3,4".rsplit(",", 5)
)")
                   .isError());
  HandleScope scope;
  List result(&scope, moduleAt(&runtime, "__main__", "l"));
  ASSERT_EQ(result.numItems(), 4);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "3"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(3), "4"));
}

TEST(StrBuiltinsTest, StrStripWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
str.strip(None)
)"),
                    LayoutId::kTypeError,
                    "'strip' requires a 'str' object but got 'NoneType'"));
}

TEST(StrBuiltinsTest, StrLStripWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
str.lstrip(None)
)"),
                    LayoutId::kTypeError,
                    "'lstrip' requires a 'str' object but got 'NoneType'"));
}

TEST(StrBuiltinsTest, StrRStripWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
str.rstrip(None)
)"),
                    LayoutId::kTypeError,
                    "'rstrip' requires a 'str' object but got 'NoneType'"));
}

TEST(StrBuiltinsTest, StrStripWithInvalidCharsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
"test".strip(1)
)"),
                            LayoutId::kTypeError,
                            "str.strip() arg must be None or str"));
}

TEST(StrBuiltinsTest, StrLStripWithInvalidCharsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
"test".lstrip(1)
)"),
                            LayoutId::kTypeError,
                            "str.lstrip() arg must be None or str"));
}

TEST(StrBuiltinsTest, StrRStripWithInvalidCharsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
"test".rstrip(1)
)"),
                            LayoutId::kTypeError,
                            "str.rstrip() arg must be None or str"));
}

TEST(StrBuiltinsTest, StripWithNoneArgStripsBoth) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST(StrBuiltinsTest, LStripWithNoneArgStripsLeft) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World "));
}

TEST(StrBuiltinsTest, LStripWithSubClassAndNonArgStripsLeft) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr(" Hello World ")
)")
                   .isError());
  Object str(&scope, moduleAt(&runtime, "__main__", "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World "));
}

TEST(StrBuiltinsTest, RStripWithNoneArgStripsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " Hello World"));
}

TEST(StrBuiltinsTest, RStripWithSubClassAndNoneArgStripsRight) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr(" Hello World ")
)")
                   .isError());
  Object str(&scope, moduleAt(&runtime, "__main__", "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " Hello World"));
}

TEST(StrBuiltinsTest, StripWithoutArgsStripsBoth) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST(StrBuiltinsTest, StripWithSubClassAndWithoutArgsStripsBoth) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr(" \n\tHello World\n\t ")
)")
                   .isError());
  Object str(&scope, moduleAt(&runtime, "__main__", "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST(StrBuiltinsTest, LStripWithoutArgsStripsLeft) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World\n\t "));
}

TEST(StrBuiltinsTest, RStripWithoutArgsStripsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " \n\tHello World"));
}

TEST(StrBuiltinsTest, StripWithCharsStripsChars) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST(StrBuiltinsTest, LStripWithCharsStripsCharsToLeft) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello Worldcab"));
}

TEST(StrBuiltinsTest, RStripWithCharsStripsCharsToRight) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "bcaHello World"));
}

TEST(StrBuiltinsTest, ReplaceWithDefaultCountReplacesAll) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "a1a1a1a".replace("a", "b")
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1b1b1b"));
}

TEST(StrBuiltinsTest, ReplaceWithCountReplacesCountedOccurrences) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "a1a1a1a".replace("a", "b", 2)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1b1a1a"));
}

TEST(StrBuiltinsTest, ReplaceWithCountOfIndexTypeReplacesCountedOccurrences) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "a1a1a1a".replace("a", "b", True)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1a1a1a"));
}

TEST(StrBuiltinsTest, ReplaceWithNonMatchingReturnsSameObject) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "a"
result = s is s.replace("z", "b")
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST(StrBuiltinsTest, ReplaceWithMissingArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "'aa'.replace('a')"), LayoutId::kTypeError,
      "TypeError: 'str.replace' takes min 3 positional arguments but 2 given"));
}

TEST(StrBuiltinsTest, ReplaceWithNonIntCountRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, "'aa'.replace('a', 'a', 'a')"),
                    LayoutId::kTypeError,
                    "'str' object cannot be interpreted as an integer"));
}

TEST(StrBuiltinsTest, DunderIterReturnsStrIter) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, Str::empty());
  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter.isStrIterator());
}

TEST(StrBuiltinsTest, DunderIterWithSubClassReturnsStrIterator) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr("")
)")
                   .isError());
  Object empty_str(&scope, moduleAt(&runtime, "__main__", "substr"));
  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  EXPECT_TRUE(iter.isStrIterator());
}

TEST(StrIteratorBuiltinsTest, CallDunderNextReadsAsciiCharactersSequentially) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("ab"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter.isStrIterator());

  Object item0(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item0, "a"));

  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item1, "b"));
}

TEST(StrIteratorBuiltinsTest,
     CallDunderNextReadsUnicodeCharactersSequentially) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr(u8"a\u00E4b"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter.isStrIterator());

  Object item0(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item0, "a"));

  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_EQ(*item1, SmallStr::fromCodePoint(0xe4));

  Object item2(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isStrEqualsCStr(*item2, "b"));
}

TEST(StrIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, Str::empty());

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter.isStrIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(StrIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(StrIteratorBuiltinsTest, DunderLengthHintOnEmptyStrIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, Str::empty());

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter.isStrIterator());

  Object length_hint(&scope,
                     runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(StrIteratorBuiltinsTest,
     DunderLengthHintOnConsumedStrIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("a"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter.isStrIterator());

  Object length_hint1(&scope,
                      runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 1));

  // Consume the iterator
  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1.isStr());
  ASSERT_EQ(item1, runtime.newStrFromCStr("a"));

  Object length_hint2(&scope,
                      runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, 0));
}

TEST(StrBuiltinsTest, StripSpaceWithEmptyStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, Str::empty());
  Thread* thread = Thread::current();
  Str lstripped_empty_str(&scope, strStripSpaceLeft(thread, empty_str));
  EXPECT_EQ(*empty_str, *lstripped_empty_str);

  Str rstripped_empty_str(&scope, strStripSpaceRight(thread, empty_str));
  EXPECT_EQ(*empty_str, *rstripped_empty_str);

  Str stripped_empty_str(&scope, strStripSpace(thread, empty_str));
  EXPECT_EQ(*empty_str, *stripped_empty_str);
}

TEST(StrBuiltinsTest, StripSpaceWithUnstrippableStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("Nothing to strip here"));
  ASSERT_TRUE(str.isLargeStr());
  Thread* thread = Thread::current();
  Str lstripped_str(&scope, strStripSpaceLeft(thread, str));
  EXPECT_EQ(*str, *lstripped_str);

  Str rstripped_str(&scope, strStripSpaceRight(thread, str));
  EXPECT_EQ(*str, *rstripped_str);

  Str stripped_str(&scope, strStripSpace(thread, str));
  EXPECT_EQ(*str, *stripped_str);
}

TEST(StrBuiltinsTest, StripSpaceWithUnstrippableSmallStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("nostrip"));
  ASSERT_TRUE(str.isSmallStr());
  Thread* thread = Thread::current();
  Str lstripped_str(&scope, strStripSpaceLeft(thread, str));
  EXPECT_EQ(*str, *lstripped_str);

  Str rstripped_str(&scope, strStripSpaceRight(thread, str));
  EXPECT_EQ(*str, *rstripped_str);

  Str stripped_str(&scope, strStripSpace(thread, str));
  EXPECT_EQ(*str, *stripped_str);
}

TEST(StrBuiltinsTest, StripSpaceWithFullyStrippableStrReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("\n\r\t\f         \n\t\r\f"));
  Thread* thread = Thread::current();
  Str lstripped_str(&scope, strStripSpaceLeft(thread, str));
  EXPECT_EQ(lstripped_str.length(), 0);

  Str rstripped_str(&scope, strStripSpaceRight(thread, str));
  EXPECT_EQ(rstripped_str.length(), 0);

  Str stripped_str(&scope, strStripSpace(thread, str));
  EXPECT_EQ(stripped_str.length(), 0);
}

TEST(StrBuiltinsTest, StripSpaceLeft) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Thread* thread = Thread::current();
  Str lstripped_str(&scope, strStripSpaceLeft(thread, str));
  ASSERT_TRUE(lstripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str, "strp "));

  Str str1(&scope, runtime.newStrFromCStr("   \n \n\tLot of leading space  "));
  ASSERT_TRUE(str1.isLargeStr());
  Str lstripped_str1(&scope, strStripSpaceLeft(thread, str1));
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str1, "Lot of leading space  "));

  Str str2(&scope, runtime.newStrFromCStr("\n\n\n              \ntest"));
  ASSERT_TRUE(str2.isLargeStr());
  Str lstripped_str2(&scope, strStripSpaceLeft(thread, str2));
  ASSERT_TRUE(lstripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str2, "test"));
}

TEST(StrBuiltinsTest, StripSpaceRight) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Thread* thread = Thread::current();
  Str rstripped_str(&scope, strStripSpaceRight(thread, str));
  ASSERT_TRUE(rstripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str, " strp"));

  Str str1(&scope,
           runtime.newStrFromCStr("  Lot of trailing space\t\n \n    "));
  ASSERT_TRUE(str1.isLargeStr());
  Str rstripped_str1(&scope, strStripSpaceRight(thread, str1));
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str1, "  Lot of trailing space"));

  Str str2(&scope, runtime.newStrFromCStr("test\n      \n\n\n"));
  ASSERT_TRUE(str2.isLargeStr());
  Str rstripped_str2(&scope, strStripSpaceRight(thread, str2));
  ASSERT_TRUE(rstripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str2, "test"));
}

TEST(StrBuiltinsTest, StripSpaceBoth) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Thread* thread = Thread::current();
  Str stripped_str(&scope, strStripSpace(thread, str));
  ASSERT_TRUE(stripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str, "strp"));

  Str str1(&scope,
           runtime.newStrFromCStr(
               "\n \n    \n\tLot of leading and trailing space\n \n    "));
  ASSERT_TRUE(str1.isLargeStr());
  Str stripped_str1(&scope, strStripSpace(thread, str1));
  EXPECT_TRUE(
      isStrEqualsCStr(*stripped_str1, "Lot of leading and trailing space"));

  Str str2(&scope, runtime.newStrFromCStr("\n\ttest\t      \n\n\n"));
  ASSERT_TRUE(str2.isLargeStr());
  Str stripped_str2(&scope, strStripSpace(thread, str2));
  ASSERT_TRUE(stripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str2, "test"));
}

TEST(StrBuiltinsTest, StripWithEmptyStrIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, Str::empty());
  Str chars(&scope, runtime.newStrFromCStr("abc"));
  Thread* thread = Thread::current();
  Str lstripped_empty_str(&scope, strStripLeft(thread, empty_str, chars));
  EXPECT_EQ(*empty_str, *lstripped_empty_str);

  Str rstripped_empty_str(&scope, strStripRight(thread, empty_str, chars));
  EXPECT_EQ(*empty_str, *rstripped_empty_str);

  Str stripped_empty_str(&scope, strStrip(thread, empty_str, chars));
  EXPECT_EQ(*empty_str, *stripped_empty_str);
}

TEST(StrBuiltinsTest, StripWithFullyStrippableStrReturnsEmptyStr) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("bbbbaaaaccccdddd"));
  Str chars(&scope, runtime.newStrFromCStr("abcd"));
  Thread* thread = Thread::current();
  Str lstripped_str(&scope, strStripLeft(thread, str, chars));
  EXPECT_EQ(lstripped_str.length(), 0);

  Str rstripped_str(&scope, strStripRight(thread, str, chars));
  EXPECT_EQ(rstripped_str.length(), 0);

  Str stripped_str(&scope, strStrip(thread, str, chars));
  EXPECT_EQ(stripped_str.length(), 0);
}

TEST(StrBuiltinsTest, StripWithEmptyCharsIsIdentity) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr(" Just another string "));
  Str chars(&scope, Str::empty());
  Thread* thread = Thread::current();
  Str lstripped_str(&scope, strStripLeft(thread, str, chars));
  EXPECT_EQ(*str, *lstripped_str);

  Str rstripped_str(&scope, strStripRight(thread, str, chars));
  EXPECT_EQ(*str, *rstripped_str);

  Str stripped_str(&scope, strStrip(thread, str, chars));
  EXPECT_EQ(*str, *stripped_str);
}

TEST(StrBuiltinsTest, StripBoth) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime.newStrFromCStr("abcd"));
  Thread* thread = Thread::current();
  Str stripped_str(&scope, strStrip(thread, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str, "Hello Worl"));
}

TEST(StrBuiltinsTest, StripLeft) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime.newStrFromCStr("abcd"));
  Thread* thread = Thread::current();
  Str lstripped_str(&scope, strStripLeft(thread, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str, "Hello Worldcab"));
}

TEST(StrBuiltinsTest, StripRight) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime.newStrFromCStr("abcd"));
  Thread* thread = Thread::current();
  Str rstripped_str(&scope, strStripRight(thread, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str, "bcdHello Worl"));
}

TEST(StrBuiltinsTest, FindWithEmptyNeedleReturnsZero) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".find("")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 0));
}

TEST(StrBuiltinsTest, FindWithEmptyNeedleReturnsNegativeOne) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".find("", 8)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), -1));
}

TEST(StrBuiltinsTest, FindWithEmptyNeedleAndSliceReturnsStart) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".find("", 3, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 3));
}

TEST(StrBuiltinsTest, FindWithEmptyNeedleAndEmptySliceReturnsStart) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".find("", 3, 3)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 3));
}

TEST(StrBuiltinsTest, FindWithNegativeStartClipsToZero) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".find("h", -5, 1)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 0));
}

TEST(StrBuiltinsTest, FindWithEndPastEndOfStringClipsToLength) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".find("h", 0, 100)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 0));
}

TEST(StrBuiltinsTest, FindCallsDunderIndexOnStart) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return 4
result = "bbbbbbbb".find("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 4));
}

TEST(StrBuiltinsTest, FindCallsDunderIndexOnEnd) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return 5
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 4));
}

TEST(StrBuiltinsTest, FindClampsStartReturningBigNumber) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".find("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), -1));
}

TEST(StrBuiltinsTest, FindClampsEndReturningBigNumber) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 4));
}

TEST(StrBuiltinsTest, FindClampsEndReturningBigNegativeNumber) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return -46116860184273879030
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), -1));
}

TEST(StrBuiltinsTest, FindWithUnicodeReturnsCodePointIndex) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "Cr\u00e8me br\u00fbl\u00e9e"
result = s.find("e")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 4));
}

TEST(StrBuiltinsTest, FindWithStartAfterUnicodeCodePoint) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.find("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 8));
}

TEST(StrBuiltinsTest, FindWithDifferentSizeCodePoints) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "Cr\u00e8me \u10348 \u29D98 br\u00fbl\u00e9e"
result = s.find("\u29D98")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 9));
}

TEST(StrBuiltinsTest, FindWithOneCharStringFindsChar) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result1 = "hello".find("h")
result2 = "hello".find("e")
result3 = "hello".find("z")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result1"), 0));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result3"), -1));
}

TEST(StrBuiltinsTest, FindWithSlicePreservesIndices) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result1 = "hello".find("h", 1)
result2 = "hello".find("e", 1)
result3 = "hello".find("o", 0, 2)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result1"), -1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result3"), -1));
}

TEST(StrBuiltinsTest, FindWithMultiCharStringFindsSubstring) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result1 = "hello".find("he")
result2 = "hello".find("el")
result3 = "hello".find("ze")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result1"), 0));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result3"), -1));
}

TEST(StrBuiltinsTest, RfindWithOneCharStringFindsChar) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".rfind("l")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 3));
}

TEST(StrBuiltinsTest, RfindCharWithUnicodeReturnsCodePointIndex) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "Cr\u00e8me br\u00fbl\u00e9e"
result = s.rfind("e")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 11));
}

TEST(StrBuiltinsTest, RfindCharWithStartAfterUnicodeCodePoint) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.rfind("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 15));
}

TEST(StrBuiltinsTest, RfindCharWithDifferentSizeCodePoints) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "Cr\u00e8me \u10348 \u29D98 br\u00fbl\u00e9e\u2070E\u29D98 "
result = s.rfind("\u29D98")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 20));
}

TEST(StrBuiltinsTest, RfindWithMultiCharStringFindsSubstring) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "aabbaa".rfind("aa")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 4));
}

TEST(StrBuiltinsTest, RfindCharWithNegativeStartClipsToZero) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".rfind("h", -5, 1)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 0));
}

TEST(StrBuiltinsTest, RfindCharWithEndPastEndOfStringClipsToLength) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".rfind("h", 0, 100)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 0));
}

TEST(StrBuiltinsTest, RfindCallsDunderIndexOnEnd) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return 5
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 4));
}

TEST(StrBuiltinsTest, RfindClampsStartReturningBigNumber) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".rfind("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), -1));
}

TEST(StrBuiltinsTest, RfindClampsEndReturningBigNumber) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 7));
}

TEST(StrBuiltinsTest, RfindClampsEndReturningBigNegativeNumber) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __index__(self):
        return -46116860184273879030
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), -1));
}

TEST(StrBuiltinsTest, RfindCharWithEmptyNeedleReturnsLength) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".rfind("")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 5));
}

TEST(StrBuiltinsTest, RfindCharWithEmptyNeedleReturnsNegativeOne) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".rfind("", 8)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), -1));
}

TEST(StrBuiltinsTest, RfindCharWithEmptyNeedleAndSliceReturnsEnd) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".rfind("", 3, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 5));
}

TEST(StrBuiltinsTest, RfindWithEmptyNeedleAndEmptySliceReturnsEnd) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "hello".rfind("", 3, 3)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 3));
}

TEST(StrBuiltinsTest, IndexWithPresentSubstringReturnsIndex) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.index("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "result"), 8));
}

TEST(StrBuiltinsTest, IndexWithMissingSubstringRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "'h'.index('q')"), LayoutId::kValueError));
}

TEST(StrBuiltinsTest, DunderHashReturnsSmallInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("hello world"));
  EXPECT_TRUE(runBuiltin(StrBuiltins::dunderHash, str).isSmallInt());
}

TEST(StrBuiltinsTest, DunderHashSmallStringReturnsSmallInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("h"));
  EXPECT_TRUE(runBuiltin(StrBuiltins::dunderHash, str).isSmallInt());
}

TEST(StrBuiltinsTest, DunderHashWithEquivalentStringsReturnsSameHash) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str1(&scope, runtime.newStrFromCStr("hello world foobar"));
  Str str2(&scope, runtime.newStrFromCStr("hello world foobar"));
  EXPECT_NE(*str1, *str2);
  Object result1(&scope, runBuiltin(StrBuiltins::dunderHash, str1));
  Object result2(&scope, runBuiltin(StrBuiltins::dunderHash, str2));
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_TRUE(result2.isSmallInt());
  EXPECT_EQ(*result1, *result2);
}

TEST(StringIterTest, SimpleIter) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::current();

  Str str(&scope, runtime.newStrFromCStr("test"));
  EXPECT_TRUE(str.equalsCStr("test"));

  StrIterator iter(&scope, runtime.newStrIterator(str));
  Object ch(&scope, strIteratorNext(thread, iter));
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("t"));

  ch = strIteratorNext(thread, iter);
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("e"));

  ch = strIteratorNext(thread, iter);
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("s"));

  ch = strIteratorNext(thread, iter);
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("t"));

  ch = strIteratorNext(thread, iter);
  ASSERT_TRUE(ch.isError());
}

TEST(StringIterTest, SetIndex) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::current();

  Str str(&scope, runtime.newStrFromCStr("test"));
  EXPECT_TRUE(str.equalsCStr("test"));

  StrIterator iter(&scope, runtime.newStrIterator(str));
  iter.setIndex(1);
  Object ch(&scope, strIteratorNext(thread, iter));
  ASSERT_TRUE(ch.isStr());
  EXPECT_TRUE(Str::cast(*ch).equalsCStr("e"));

  iter.setIndex(5);
  ch = strIteratorNext(thread, iter);
  // Index should not have advanced.
  ASSERT_EQ(iter.index(), 5);
  ASSERT_TRUE(ch.isError());
}

TEST(StrBuiltinsTest, DunderContainsWithNonStrSelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, "str.__contains__(3, 'foo')"),
                     LayoutId::kTypeError));
}

TEST(StrBuiltinsTest, DunderContainsWithNonStrOtherRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, "str.__contains__('foo', 3)"),
                     LayoutId::kTypeError));
}

TEST(StrBuiltinsTest, DunderContainsWithPresentSubstrReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = str.__contains__('foo', 'f')").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST(StrBuiltinsTest, DunderContainsWithNotPresentSubstrReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = str.__contains__('foo', 'q')").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "str.isalnum(None)"),
                            LayoutId::kTypeError,
                            "isalnum expected 'str' but got NoneType"));
}

TEST(StrBuiltinsTest, IsalnumWithEmptyStringReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isalnum('')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithCharacterBelowZeroReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isalnum('/')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithCharacterAboveNineReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isalnum(':')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithNumbersReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime,
                  "result = all([str.isalnum(x) for x in '0123456789'])")
          .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsalnumWithCharacterBelowLowerAReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isalnum('`')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithCharacterAboveLowerZReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isalnum('{')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithLowercaseLettersReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime,
                           "result = all([str.isalnum(x) for x in "
                           "'abcdefghijklmnopqrstuvwxyz'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsalnumWithCharacterBelowUpperAReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isalnum('@')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithCharacterAboveUpperZReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isalnum('[')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsalnumWithUppercaseLettersReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime,
                           "result = all([str.isalnum(x) for x in "
                           "'ABCDEFGHIJKLMNOPQRSTUVWXYZ'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsspaceWithEmptyStringReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = ''.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsspaceWithNonSpaceReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = ' a '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsspaceWithNewlineReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = ' \n '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsspaceWithTabReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = ' \t '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsspaceWithCarriageReturnReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = ' \r '.isspace()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsupperWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "str.isupper(None)"),
                            LayoutId::kTypeError,
                            "isupper expected 'str' but got NoneType"));
}

TEST(StrBuiltinsTest, IsupperWithEmptyStringReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isupper('')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsupperWithCharacterBelowUpperAReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isupper('@')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsupperWithCharacterAboveUpperZReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.isupper('[')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsupperWithUppercaseLettersReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime,
                           "result = all([str.isupper(x) for x in "
                           "'ABCDEFGHIJKLMNOPQRSTUVWXYZ'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IslowerWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "str.islower(None)"),
                            LayoutId::kTypeError,
                            "islower expected 'str' but got NoneType"));
}

TEST(StrBuiltinsTest, IslowerWithEmptyStringReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.islower('')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IslowerWithCharacterBelowLowerAReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.islower('`')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IslowerWithCharacterAboveLowerZReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = str.islower('{')").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IslowerWithLowercaseLettersReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime,
                           "result = all([str.islower(x) for x in "
                           "'abcdefghijklmnopqrstuvwxyz'])")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, UpperOnASCIILettersReturnsUpperCaseString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "hello".upper()
b = "HeLLo".upper()
c = "hellO".upper()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(*c, "HELLO"));
}

TEST(StrBuiltinsTest, UpperOnASCIILettersOfSubClassReturnsUpperCaseString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
a = SubStr("hello").upper()
b = SubStr("HeLLo").upper()
c = SubStr("hellO").upper()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "a"), "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "b"), "HELLO"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "c"), "HELLO"));
}

TEST(StrBuiltinsTest, UpperOnUppercaseASCIILettersReturnsSameString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "HELLO".upper()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "HELLO"));
}

TEST(StrBuiltinsTest, UpperOnNumbersReturnsSameString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = "foo 123".upper()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "FOO 123"));
}

TEST(StrBuiltinsTest, CapitalizeWithNonStrRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "str.capitalize(1)"), LayoutId::kTypeError,
      "'capitalize' requires a 'str' instance but got 'int'"));
}

TEST(StrBuiltinsTest, CapitalizeReturnsCapitalizedStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "foo".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "Foo"));
}

TEST(StrBuiltinsTest, CapitalizeUpperCaseReturnsUnmodifiedStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "Foo".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "Foo"));
}

TEST(StrBuiltinsTest, CapitalizeAllUppercaseReturnsCapitalizedStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "FOO".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "Foo"));
}

TEST(StrBuiltinsTest, CapitalizeWithEmptyStrReturnsEmptyStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), ""));
}

TEST(StrBuiltinsTest, IsidentifierWithEmptyStringReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsidentifierWithNumberReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "9".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsidentifierWithPeriodReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = ".".isidentifier()
print(result)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(StrBuiltinsTest, IsidentifierWithLowercaseLetterReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "a".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsidentifierWithUppercaseLetterReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "A".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsidentifierWithUnderscoreReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "_".isidentifier()
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsidentifierWithOnlyLettersReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "abc".isidentifier()
print(result)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, IsidentifierWithLettersAndNumbersReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = "abc213".isidentifier()
print(result)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(StrBuiltinsTest, StrUnderlyingWithStrReturnsSameStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str str(&scope, runtime.newStrFromCStr("hello"));
  Object underlying(&scope, strUnderlying(thread, str));
  EXPECT_EQ(*str, *underlying);
}

TEST(StrBuiltinsTest, StrUnderlyingWithSubClassReturnsUnderlyingStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class SubStr(str): pass
substr = SubStr("some string")
)")
                   .isError());
  Object substr(&scope, moduleAt(&runtime, "__main__", "substr"));
  ASSERT_FALSE(substr.isStr());
  Object underlying(&scope, strUnderlying(thread, substr));
  EXPECT_TRUE(isStrEqualsCStr(*underlying, "some string"));
}

}  // namespace python
