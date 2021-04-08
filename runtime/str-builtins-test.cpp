#include "str-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "set-builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using StrBuiltinsTest = RuntimeFixture;
using StrIterTest = RuntimeFixture;
using StrIteratorBuiltinsTest = RuntimeFixture;
using StringIterTest = RuntimeFixture;

TEST_F(StrBuiltinsTest, BuiltinBase) {
  HandleScope scope(thread_);

  Type small_str(&scope, runtime_->typeAt(LayoutId::kSmallStr));
  EXPECT_EQ(small_str.builtinBase(), LayoutId::kStr);

  Type large_str(&scope, runtime_->typeAt(LayoutId::kLargeStr));
  EXPECT_EQ(large_str.builtinBase(), LayoutId::kStr);

  Type str(&scope, runtime_->typeAt(LayoutId::kStr));
  EXPECT_EQ(str.builtinBase(), LayoutId::kStr);
}

TEST_F(StrBuiltinsTest, RichCompareStringEQ) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "magic string"
if (a == "magic string"):
  result = "foo"
else:
  result = "bar"
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "foo"));
}

TEST_F(StrBuiltinsTest, RichCompareStringEQWithSubClass) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
a = SubStr("magic string")
if (a == "magic string"):
  result = "foo"
else:
  result = "bar"
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "foo"));
}

TEST_F(StrBuiltinsTest, RichCompareStringNE) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "magic string"
result = "bar"
if (a != "magic string"):
  result = "foo"
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "bar"));
}

TEST_F(StrBuiltinsTest, RichCompareStringNEWithSubClass) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
a = SubStr("apple")
result = "bar"
if (a != "apple"):
  result = "foo"
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "bar"));
}

TEST_F(StrBuiltinsTest, RichCompareSingleCharLE) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a_le_b = 'a' <= 'b'
b_le_a = 'a' >= 'b'
a_le_a = 'a' <= 'a'
)")
                   .isError());

  HandleScope scope(thread_);

  Object a_le_b(&scope, mainModuleAt(runtime_, "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());

  Object b_le_a(&scope, mainModuleAt(runtime_, "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());

  Object a_le_a(&scope, mainModuleAt(runtime_, "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
}

TEST_F(StrBuiltinsTest, RichCompareSingleCharLEWithSubClass) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class S(str): pass
a_le_b = S('a') <= S('b')
b_le_a = S('a') >= S('b')
a_le_a = S('a') <= S('a')
)")
                   .isError());

  EXPECT_EQ(mainModuleAt(runtime_, "a_le_b"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(runtime_, "b_le_a"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(runtime_, "a_le_a"), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, DunderNewCallsDunderStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
    def __str__(self):
        return "foo"
a = str.__new__(str, Foo())
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST_F(StrBuiltinsTest, DunderNewCallsReprIfNoDunderStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  pass
f = Foo()
a = str.__new__(str, f)
b = repr(f)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST_F(StrBuiltinsTest, DunderNewWithNoArgsExceptTypeReturnsEmptyString) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = str.__new__(str)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, ""));
}

TEST_F(StrBuiltinsTest, DunderNewWithStrReturnsSameStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = str.__new__(str, "hello")
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
}

TEST_F(StrBuiltinsTest, DunderNewWithTypeCallsTypeDunderStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "a = str.__new__(str, int)").isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "<class 'int'>"));
}

TEST_F(StrBuiltinsTest, DunderNewWithNoArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "str.__new__()"), LayoutId::kTypeError,
      "'str.__new__' takes min 1 positional arguments but 0 given"));
}

TEST_F(StrBuiltinsTest, DunderNewWithTooManyArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "str.__new__(str, 1, 2, 3, 4)"),
      LayoutId::kTypeError,
      "'str.__new__' takes max 4 positional arguments but 5 given"));
}

TEST_F(StrBuiltinsTest, DunderNewWithNonSubtypeArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "str.__new__(object)"),
                            LayoutId::kTypeError,
                            "'__new__': 'object' is not a subclass of 'str'"));
}

TEST_F(StrBuiltinsTest, DunderAddWithTwoStringsReturnsConcatenatedString) {
  HandleScope scope(thread_);
  Object str1(&scope, runtime_->newStrFromCStr("hello"));
  Object str2(&scope, runtime_->newStrFromCStr("world"));
  Object result(&scope, runBuiltin(METH(str, __add__), str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "helloworld"));
}

TEST_F(StrBuiltinsTest,
       DunderAddWithTwoStringsOfSubClassReturnsConcatenatedString) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
str1 = SubStr("hello")
str2 = SubStr("world")
)")
                   .isError());
  Object str1(&scope, mainModuleAt(runtime_, "str1"));
  Object str2(&scope, mainModuleAt(runtime_, "str2"));
  Object result(&scope, runBuiltin(METH(str, __add__), str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "helloworld"));
}

TEST_F(StrBuiltinsTest, DunderAddWithLeftEmptyAndReturnsRight) {
  HandleScope scope(thread_);
  Object str1(&scope, Str::empty());
  Object str2(&scope, runtime_->newStrFromCStr("world"));
  Object result(&scope, runBuiltin(METH(str, __add__), str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "world"));
}

TEST_F(StrBuiltinsTest, DunderAddWithRightEmptyAndReturnsRight) {
  HandleScope scope(thread_);
  Object str1(&scope, runtime_->newStrFromCStr("hello"));
  Object str2(&scope, Str::empty());
  Object result(&scope, runBuiltin(METH(str, __add__), str1, str2));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hello"));
}

TEST_F(StrBuiltinsTest, PlusOperatorOnStringsEqualsDunderAdd) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "hello"
b = "world"
c = a + b
d = a.__add__(b)
)")
                   .isError());
  HandleScope scope(thread_);
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object d(&scope, mainModuleAt(runtime_, "d"));

  EXPECT_TRUE(isStrEqualsCStr(*c, "helloworld"));
  EXPECT_TRUE(isStrEqualsCStr(*d, "helloworld"));
}

TEST_F(StrBuiltinsTest, DunderBoolWithEmptyStringReturnsFalse) {
  HandleScope scope(thread_);
  Str str(&scope, Str::empty());
  EXPECT_EQ(runBuiltin(METH(str, __bool__), str), Bool::falseObj());
}

TEST_F(StrBuiltinsTest, DunderBoolWithNonEmptyStringReturnsTrue) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  EXPECT_EQ(runBuiltin(METH(str, __bool__), str), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, DunderBoolWithNonEmptyStringOfSubClassReturnsTrue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
substr = SubStr("hello")
)")
                   .isError());
  Object substr(&scope, mainModuleAt(runtime_, "substr"));
  EXPECT_EQ(runBuiltin(METH(str, __bool__), substr), Bool::trueObj());
}

TEST_F(StrBuiltinsTest, DunderLenReturnsLength) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l1 = len("aloha")
l2 = str.__len__("aloha")
l3 = "aloha".__len__()
)")
                   .isError());
  HandleScope scope(thread_);
  Object l1(&scope, mainModuleAt(runtime_, "l1"));
  Object l2(&scope, mainModuleAt(runtime_, "l2"));
  Object l3(&scope, mainModuleAt(runtime_, "l3"));
  EXPECT_TRUE(isIntEqualsWord(*l1, 5));
  EXPECT_TRUE(isIntEqualsWord(*l2, 5));
  EXPECT_TRUE(isIntEqualsWord(*l3, 5));
}

TEST_F(StrBuiltinsTest, StringLenWithEmptyString) {
  ASSERT_FALSE(runFromCStr(runtime_, "l = len('')").isError());
  HandleScope scope(thread_);
  Object length(&scope, mainModuleAt(runtime_, "l"));
  EXPECT_TRUE(isIntEqualsWord(*length, 0));
}

TEST_F(StrBuiltinsTest, DunderLenWithNonAsciiReturnsCodePointLength) {
  ASSERT_FALSE(runFromCStr(runtime_, "l = len('\xc3\xa9')").isError());
  HandleScope scope(thread_);
  SmallInt length(&scope, mainModuleAt(runtime_, "l"));
  EXPECT_TRUE(isIntEqualsWord(*length, 1));
}

TEST_F(StrBuiltinsTest, DunderLenWithExtraArgumentRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "l = 'aloha'.__len__('arg')"), LayoutId::kTypeError,
      "'str.__len__' takes max 1 positional arguments but 2 given"));
}

TEST_F(StrBuiltinsTest, DunderMulWithNonIntRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  Object count(&scope, runtime_->newList());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(str, __mul__), self, count), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(StrBuiltinsTest, DunderMulWithDunderIndexReturnsRepeatedStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return 2
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithBadDunderIndexRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    return "foo"
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(str, __mul__), self, count),
                            LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST_F(StrBuiltinsTest, DunderMulPropagatesDunderIndexError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __index__(self):
    raise ArithmeticError("called __index__")
count = C()
)")
                   .isError());
  Object count(&scope, mainModuleAt(runtime_, "count"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(str, __mul__), self, count),
                            LayoutId::kArithmeticError, "called __index__"));
}

TEST_F(StrBuiltinsTest, DunderMulWithLargeIntRaisesOverflowError) {
  HandleScope scope(thread_);
  Object self(&scope, Str::empty());
  const uword digits[] = {1, 1};
  Object count(&scope, runtime_->newIntWithDigits(digits));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(str, __mul__), self, count),
                            LayoutId::kOverflowError,
                            "cannot fit 'int' into an index-sized integer"));
}

TEST_F(StrBuiltinsTest, DunderMulWithOverflowRaisesOverflowError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(SmallInt::kMaxValue / 2));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(str, __mul__), self, count),
                            LayoutId::kOverflowError,
                            "repeated string is too long"));
}

TEST_F(StrBuiltinsTest, DunderMulWithEmptyBytesReturnsEmptyStr) {
  HandleScope scope(thread_);
  Object self(&scope, Str::empty());
  Object count(&scope, runtime_->newInt(10));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(StrBuiltinsTest, DunderMulWithNegativeReturnsEmptyStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(-5));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(StrBuiltinsTest, DunderMulWithZeroReturnsEmptyStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, ""));
}

TEST_F(StrBuiltinsTest, DunderMulWithOneReturnsSamStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithSmallStrReturnsRepeatedSmallStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithSmallStrReturnsRepeatedLargeStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foo"));
  Object count(&scope, SmallInt::fromWord(3));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoofoo"));
}

TEST_F(StrBuiltinsTest, DunderMulWithLargeStrReturnsRepeatedLargeStr) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("foobarbaz"));
  Object count(&scope, SmallInt::fromWord(2));
  Object result(&scope, runBuiltin(METH(str, __mul__), self, count));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobarbazfoobarbaz"));
}

TEST_F(StrBuiltinsTest, DunderRmulCallsDunderMul) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = 3 * 'foo'").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foofoofoo"));
}

TEST_F(StrBuiltinsTest, HasPrefixWithPrefixPastEndOfStrReturnsFalse) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, runtime_->newStrFromCStr("hel"));
  EXPECT_TRUE(strHasPrefix(haystack, needle, 0));
  EXPECT_FALSE(strHasPrefix(haystack, needle, 1));
  EXPECT_FALSE(strHasPrefix(haystack, needle, 3));
}

TEST_F(StrBuiltinsTest, HasPrefixWithNonPrefixPastEndOfStrReturnsFalse) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, runtime_->newStrFromCStr("lop"));
  EXPECT_FALSE(strHasPrefix(haystack, needle, 0));
  EXPECT_FALSE(strHasPrefix(haystack, needle, 1));
  EXPECT_FALSE(strHasPrefix(haystack, needle, 3));
}

TEST_F(StrBuiltinsTest, HasPrefixWithEmptyNeedleReturnsTrue) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str empty(&scope, Str::empty());
  EXPECT_TRUE(strHasPrefix(empty, empty, 0));
  EXPECT_FALSE(strHasPrefix(empty, empty, 1));
  EXPECT_TRUE(strHasPrefix(haystack, empty, 0));
  EXPECT_TRUE(strHasPrefix(haystack, empty, 5));
  EXPECT_FALSE(strHasPrefix(haystack, empty, 6));
}

TEST_F(StrBuiltinsTest, HasSurrogateWithNonSurrogateReturnsFalse) {
  HandleScope scope(thread_);
  Str s1(&scope, SmallStr::fromCStr(""));
  EXPECT_FALSE(strHasSurrogate(s1));

  Str s2(&scope, SmallStr::fromCStr("a"));
  EXPECT_FALSE(strHasSurrogate(s2));

  Str s3(&scope, SmallStr::fromCStr("b10\x04-U."));
  EXPECT_FALSE(strHasSurrogate(s3));

  Str s4(&scope, SmallStr::fromCStr("pav\u00e9"));
  EXPECT_FALSE(strHasSurrogate(s4));

  Str s5(&scope, runtime_->newStrFromCStr("Hello world!"));
  EXPECT_FALSE(strHasSurrogate(s5));

  Str s6(&scope, runtime_->newStrFromCStr("1234567890"));
  EXPECT_FALSE(strHasSurrogate(s6));

  Str s7(&scope, runtime_->newStrFromCStr("\u00c9tudes Op. 10"));
  EXPECT_FALSE(strHasSurrogate(s7));
}

TEST_F(StrBuiltinsTest, HasSurrogateWithSurrogateReturnsTrue) {
  HandleScope scope(thread_);
  Str s1(&scope, SmallStr::fromCodePoint(0xD800));
  EXPECT_TRUE(strHasSurrogate(s1));

  Str s2(&scope, SmallStr::fromCodePoint(0xDFFF));
  EXPECT_TRUE(strHasSurrogate(s2));

  Str s3(&scope, SmallStr::fromCodePoint(0xD1E9));
  EXPECT_TRUE(strHasSurrogate(s3));

  const int32_t view4[] = {'p', 'a', 'd', 'd', 'i', 'n', 'g', 0xD800};
  Str s4(&scope, runtime_->newStrFromUTF32(view4));
  EXPECT_TRUE(strHasSurrogate(s4));

  const int32_t view5[] = {'p', 'a', 'd', 'd', 'i', 'n', 'g', 0xDC81};
  Str s5(&scope, runtime_->newStrFromUTF32(view5));
  EXPECT_TRUE(strHasSurrogate(s5));

  const int32_t view6[] = {'p', 'a', 'd', 0xD800, 0xDFFF};
  Str s6(&scope, runtime_->newStrFromUTF32(view6));
  EXPECT_TRUE(strHasSurrogate(s6));

  const int32_t view7[] = {'p', 'a', 'd', 0xDC00, 0xD910};
  Str s7(&scope, runtime_->newStrFromUTF32(view7));
  EXPECT_TRUE(strHasSurrogate(s7));
}

TEST_F(StrBuiltinsTest, InternStringsInTupleInternsItems) {
  HandleScope scope(thread_);
  Str str0(&scope, runtime_->newStrFromCStr("a"));
  Str str1(&scope, runtime_->newStrFromCStr("hello world"));
  Str str2(&scope, runtime_->newStrFromCStr("hello world foobar"));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str2));

  Tuple tuple(&scope, runtime_->newTupleWith3(str0, str1, str2));
  strInternInTuple(thread_, tuple);
  str0 = tuple.at(0);
  str1 = tuple.at(1);
  str2 = tuple.at(2);
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str0));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str1));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str2));
}

TEST_F(StrBuiltinsTest,
       InternStringConstantsInternsAlphanumericStringsInTuple) {
  HandleScope scope(thread_);
  Str str0(&scope, runtime_->newStrFromCStr("_"));
  Str str1(&scope, runtime_->newStrFromCStr("hello world"));
  Str str2(&scope, runtime_->newStrFromCStr("helloworldfoobar"));
  Str str3(&scope, runtime_->newStrFromCStr("hello_world"));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str2));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str3));
  Tuple tuple(&scope, runtime_->newTupleWith4(str0, str1, str2, str3));
  strInternConstants(thread_, tuple);
  str0 = tuple.at(0);
  str1 = tuple.at(1);
  str2 = tuple.at(2);
  str3 = tuple.at(3);
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str1));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str2));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str3));
}

TEST_F(StrBuiltinsTest, InternStringConstantsInternsStringsInNestedTuples) {
  HandleScope scope(thread_);
  Str str0(&scope, runtime_->newStrFromCStr("_"));
  Str str1(&scope, runtime_->newStrFromCStr("hello world"));
  Str str2(&scope, runtime_->newStrFromCStr("helloworldfoobar"));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str2));

  Int int0(&scope, SmallInt::fromWord(0));
  Int int1(&scope, SmallInt::fromWord(1));
  Tuple inner(&scope, runtime_->newTupleWith3(str0, str1, str2));
  Tuple outer(&scope, runtime_->newTupleWith3(int0, int1, inner));

  strInternConstants(thread_, outer);
  str0 = inner.at(0);
  str1 = inner.at(1);
  str2 = inner.at(2);
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str1));
  EXPECT_TRUE(runtime_->isInternedStr(thread_, str2));
}

TEST_F(StrBuiltinsTest,
       InternStringConstantsInternsStringsInFrozenSetsInTuples) {
  HandleScope scope(thread_);

  Str str0(&scope, runtime_->newStrFromCStr("alpharomeo"));
  Str str1(&scope, runtime_->newStrFromCStr("hello world"));
  Str str2(&scope, runtime_->newStrFromCStr("helloworldfoobar"));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str0));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str1));
  EXPECT_FALSE(runtime_->isInternedStr(thread_, str2));

  Int int0(&scope, SmallInt::fromWord(0));
  Int int1(&scope, SmallInt::fromWord(1));
  FrozenSet inner(&scope, runtime_->newFrozenSet());
  Tuple outer(&scope, runtime_->newTupleWith3(int0, int1, inner));

  setHashAndAdd(thread_, inner, str0);
  setHashAndAdd(thread_, inner, str1);
  setHashAndAdd(thread_, inner, str2);
  strInternConstants(thread_, outer);
  inner = outer.at(2);
  bool all_interned = true;
  bool some_interned = false;
  Object value(&scope, NoneType::object());
  for (word idx = 0; setNextItem(inner, &idx, &value);) {
    Str obj(&scope, *value);
    bool interned = Runtime::isInternedStr(thread_, obj);
    all_interned &= interned;
    some_interned |= interned;
  }
  EXPECT_FALSE(all_interned);
  EXPECT_TRUE(some_interned);
}

TEST_F(StrBuiltinsTest, DunderReprWithPrintableASCIIReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hello"));
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "'hello'"));
}

TEST_F(StrBuiltinsTest, DunderReprWithStrSubclassReturnsStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
substr = SubStr("hello")
)")
                   .isError());
  HandleScope scope(thread_);
  Object substr(&scope, mainModuleAt(runtime_, "substr"));
  Object repr(&scope, runBuiltin(METH(str, __repr__), substr));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "'hello'"));
}

TEST_F(StrBuiltinsTest, DunderReprWithNonPrintableASCIIReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("\x06"));  // ACK character
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"('\x06')"));
}

TEST_F(StrBuiltinsTest, DunderReprWithDoubleQuotesReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hello \"world\""));
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"('hello "world"')"));
}

TEST_F(StrBuiltinsTest, DunderReprWithSingleQuotesReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hello 'world'"));
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"("hello 'world'")"));
}

TEST_F(StrBuiltinsTest, DunderReprWithBothQuotesReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("'hello' \"world\""));
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"('\'hello\' "world"')"));
}

TEST_F(StrBuiltinsTest, DunderReprWithNestedQuotesReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr(
                         R"(hello 'world, "I am 'your "father"'"')"));
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(
      isStrEqualsCStr(*repr, R"('hello \'world, "I am \'your "father"\'"\'')"));
}

TEST_F(StrBuiltinsTest, DunderReprOnCommonEscapeSequences) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("\n \t \r \\"));
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(isStrEqualsCStr(*repr, R"('\n \t \r \\')"));
}

TEST_F(StrBuiltinsTest, DunderReprWithUnicodeReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope,
             runtime_->newStrFromCStr("foo\U0001d4eb\U0001d4ea\U0001d4fb"));
  Object repr(&scope, runBuiltin(METH(str, __repr__), str));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "'foo\U0001d4eb\U0001d4ea\U0001d4fb'"));
}

TEST_F(StrBuiltinsTest, DunderStr) {
  const char* src = R"(
result = 'Hello, World!'.__str__()
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello, World!"));
}

TEST_F(StrBuiltinsTest, SplitWithOneCharSeparator) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "hello".split("e")
b = "hello".split("l")
)")
                   .isError());
  HandleScope scope(thread_);

  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "llo"));

  List b(&scope, mainModuleAt(runtime_, "b"));
  ASSERT_EQ(b.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "o"));
}

TEST_F(StrBuiltinsTest, SplitWithEmptySelfReturnsSingleEmptyString) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "".split("a")
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), ""));
}

TEST_F(StrBuiltinsTest, SplitWithMultiCharSeparator) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "hello".split("el")
b = "hello".split("ll")
c = "hello".split("hello")
d = "hellllo".split("ll")
)")
                   .isError());
  HandleScope scope(thread_);

  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));

  List b(&scope, mainModuleAt(runtime_, "b"));
  ASSERT_EQ(b.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "o"));

  List c(&scope, mainModuleAt(runtime_, "c"));
  ASSERT_EQ(c.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(c.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(c.at(1), ""));

  List d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_EQ(d.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(d.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(d.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(d.at(2), "o"));
}

TEST_F(StrBuiltinsTest, SplitWithMaxSplitZeroReturnsList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "hello".split("x", 0)
b = "hello".split("l", 0)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a_obj(&scope, mainModuleAt(runtime_, "a"));
  Object b_obj(&scope, mainModuleAt(runtime_, "b"));
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "hello".split("l", 1)
b = "1,2,3,4".split(",", 2)
)")
                   .isError());
  HandleScope scope(thread_);

  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), "lo"));

  List b(&scope, mainModuleAt(runtime_, "b"));
  ASSERT_EQ(b.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "3,4"));
}

TEST_F(StrBuiltinsTest, SplitWithMaxSplitGreaterThanNumParts) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "hello".split("l", 2)
b = "1,2,3,4".split(",", 5)
)")
                   .isError());
  HandleScope scope(thread_);
  List a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_EQ(a.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(a.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(a.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(a.at(2), "o"));

  List b(&scope, mainModuleAt(runtime_, "b"));
  ASSERT_EQ(b.numItems(), 4);
  EXPECT_TRUE(isStrEqualsCStr(b.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(2), "3"));
  EXPECT_TRUE(isStrEqualsCStr(b.at(3), "4"));
}

TEST_F(StrBuiltinsTest, SplitEmptyStringWithNoSepReturnsEmptyList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "".split()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(result.numItems(), 0);
}

TEST_F(StrBuiltinsTest, SplitWhitespaceStringWithNoSepReturnsEmptyList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "  \t\n  ".split()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(result.numItems(), 0);
}

TEST_F(StrBuiltinsTest, SplitWhitespaceReturnsComponentParts) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "  \t\n  hello\t\n world".split()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST_F(StrBuiltinsTest,
       SplitWhitespaceWithMaxsplitEqualsNegativeOneReturnsAllResults) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "  \t\n  hello\t\n world".split(maxsplit=-1)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST_F(StrBuiltinsTest,
       SplitWhitespaceWithMaxsplitEqualsZeroReturnsOneElementList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "  \t\n  hello   world   ".split(maxsplit=0)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"hello   world   "});
}

TEST_F(StrBuiltinsTest,
       SplitWhitespaceWithMaxsplitEqualsOneReturnsTwoElementList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "  \t\n  hello world ".split(maxsplit=1)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "world "));
}

TEST_F(StrBuiltinsTest, SplitlinesSplitsOnLineBreaks) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello\nworld\rwhats\r\nup".splitlines()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world", "whats", "up"});
}

TEST_F(StrBuiltinsTest, SplitlinesWithKeependsKeepsLineBreaks) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello\nworld\rwhats\r\nup".splitlines(keepends=True)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"hello\n", "world\r", "whats\r\n", "up"});
}

TEST_F(StrBuiltinsTest, SplitlinesWithNoNewlinesReturnsIdEqualString) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "hello world foo bar"
[result] = s.splitlines()
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "s"), mainModuleAt(runtime_, "result"));
}

TEST_F(StrBuiltinsTest, SplitlinesWithMultiByteNewlineSplitsLine) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello\u2028world".splitlines()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"hello", "world"});
}

TEST_F(StrBuiltinsTest, SplitlinesWithMultiByteNewlineAndKeependsSplitsLine) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello\u2028world".splitlines(keepends=True)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {u8"hello\u2028", "world"});
}

TEST_F(StrBuiltinsTest, RsplitWithOneCharSeparatorSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hello".rsplit("e")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "llo"));
}

TEST_F(StrBuiltinsTest, RsplitWithRepeatedOneCharSeparatorSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hello".rsplit("l")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithEmptySelfReturnsSingleEmptyString) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "".rsplit("a")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 1);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
}

TEST_F(StrBuiltinsTest, RsplitWithMultiCharSeparatorSplitsFromRight) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hello".rsplit("el")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "h"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "lo"));
}

TEST_F(StrBuiltinsTest, RsplitWithRepeatedCharSeparatorSplitsFromRight) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hello".rsplit("ll")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "o"));
}

TEST_F(StrBuiltinsTest,
       RsplitWithSeparatorSameAsInputSplitsIntoEmptyComponents) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hello".rsplit("hello")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
}

TEST_F(StrBuiltinsTest,
       RsplitWithMultiCharSeparatorWithMultipleAppearancesSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hellllo".rsplit("ll")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithMaxSplitZeroReturnsList) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = "hello".rsplit("x", 0)
b = "hello".rsplit("l", 0)
)")
                   .isError());
  HandleScope scope(thread_);
  Object a_obj(&scope, mainModuleAt(runtime_, "a"));
  Object b_obj(&scope, mainModuleAt(runtime_, "b"));
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hello".rsplit("l", 1)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hel"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithMaxSplitBelowNumPartsStopsEarly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "1,2,3,4".rsplit(",", 2)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "1,2"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "3"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "4"));
}

TEST_F(StrBuiltinsTest,
       RsplitWithRepeatedCharAndMaxSplitGreaterThanNumPartsSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "hello".rsplit("l", 2)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 3);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "he"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), ""));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "o"));
}

TEST_F(StrBuiltinsTest, RsplitWithMaxSplitGreaterThanNumPartsSplitsCorrectly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = "1,2,3,4".rsplit(",", 5)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "l"));
  ASSERT_EQ(result.numItems(), 4);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "1"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "2"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "3"));
  EXPECT_TRUE(isStrEqualsCStr(result.at(3), "4"));
}

TEST_F(StrBuiltinsTest, StrStripWithInvalidCharsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
"test".strip(1)
)"),
                            LayoutId::kTypeError,
                            "str.strip() arg must be None or str"));
}

TEST_F(StrBuiltinsTest, StrLStripWithInvalidCharsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
"test".lstrip(1)
)"),
                            LayoutId::kTypeError,
                            "str.lstrip() arg must be None or str"));
}

TEST_F(StrBuiltinsTest, StrRStripWithInvalidCharsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
"test".rstrip(1)
)"),
                            LayoutId::kTypeError,
                            "str.rstrip() arg must be None or str"));
}

TEST_F(StrBuiltinsTest, StripWithNoneArgStripsBoth) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, strip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, LStripWithNoneArgStripsLeft) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, lstrip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World "));
}

TEST_F(StrBuiltinsTest, LStripWithSubClassAndNonArgStripsLeft) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
substr = SubStr(" Hello World ")
)")
                   .isError());
  Object str(&scope, mainModuleAt(runtime_, "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, lstrip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World "));
}

TEST_F(StrBuiltinsTest, RStripWithNoneArgStripsRight) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, rstrip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " Hello World"));
}

TEST_F(StrBuiltinsTest, RStripWithSubClassAndNoneArgStripsRight) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
substr = SubStr(" Hello World ")
)")
                   .isError());
  Object str(&scope, mainModuleAt(runtime_, "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, rstrip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " Hello World"));
}

TEST_F(StrBuiltinsTest, StripWithoutArgsStripsBoth) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, strip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, StripWithSubClassAndWithoutArgsStripsBoth) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
substr = SubStr(" \n\tHello World\n\t ")
)")
                   .isError());
  Object str(&scope, mainModuleAt(runtime_, "substr"));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, strip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, LStripWithoutArgsStripsLeft) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, lstrip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World\n\t "));
}

TEST_F(StrBuiltinsTest, RStripWithoutArgsStripsRight) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr(" \n\tHello World\n\t "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(METH(str, rstrip), str, none));
  EXPECT_TRUE(isStrEqualsCStr(*result, " \n\tHello World"));
}

TEST_F(StrBuiltinsTest, StripWithCharsStripsChars) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime_->newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(METH(str, strip), str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello World"));
}

TEST_F(StrBuiltinsTest, LStripWithCharsStripsCharsToLeft) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime_->newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(METH(str, lstrip), str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "Hello Worldcab"));
}

TEST_F(StrBuiltinsTest, RStripWithCharsStripsCharsToRight) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime_->newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(METH(str, rstrip), str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*result, "bcaHello World"));
}

TEST_F(StrBuiltinsTest, ReplaceWithDefaultCountReplacesAll) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "a1a1a1a".replace("a", "b")
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1b1b1b"));
}

TEST_F(StrBuiltinsTest, ReplaceWithCountReplacesCountedOccurrences) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "a1a1a1a".replace("a", "b", 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1b1a1a"));
}

TEST_F(StrBuiltinsTest, ReplaceWithCountOfIndexTypeReplacesCountedOccurrences) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "a1a1a1a".replace("a", "b", True)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result, "b1a1a1a"));
}

TEST_F(StrBuiltinsTest, ReplaceWithNonMatchingReturnsSameObject) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "a"
result = s is s.replace("z", "b")
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST_F(StrBuiltinsTest, ReplaceWithMissingArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "'aa'.replace('a')"), LayoutId::kTypeError,
      "'str.replace' takes min 3 positional arguments but 2 given"));
}

TEST_F(StrBuiltinsTest, ReplaceWithNonIntCountRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "'aa'.replace('a', 'a', 'a')"),
                    LayoutId::kTypeError,
                    "'str' object cannot be interpreted as an integer"));
}

TEST_F(StrBuiltinsTest, DunderIterReturnsStrIter) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());
  Object iter(&scope, runBuiltin(METH(str, __iter__), empty_str));
  ASSERT_TRUE(iter.isStrIterator());
}

TEST_F(StrBuiltinsTest, DunderIterWithSubClassReturnsStrIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
substr = SubStr("")
)")
                   .isError());
  Object empty_str(&scope, mainModuleAt(runtime_, "substr"));
  Object iter(&scope, runBuiltin(METH(str, __iter__), empty_str));
  EXPECT_TRUE(iter.isStrIterator());
}

TEST_F(StrIteratorBuiltinsTest,
       CallDunderNextReadsAsciiCharactersSequentially) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("ab"));

  Object iter(&scope, runBuiltin(METH(str, __iter__), str));
  ASSERT_TRUE(iter.isStrIterator());

  Object item0(&scope, runBuiltin(METH(str_iterator, __next__), iter));
  EXPECT_TRUE(isStrEqualsCStr(*item0, "a"));

  Object item1(&scope, runBuiltin(METH(str_iterator, __next__), iter));
  EXPECT_TRUE(isStrEqualsCStr(*item1, "b"));
}

TEST_F(StrIteratorBuiltinsTest,
       CallDunderNextReadsUnicodeCharactersSequentially) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(u8"a\u00E4b"));

  Object iter(&scope, runBuiltin(METH(str, __iter__), str));
  ASSERT_TRUE(iter.isStrIterator());

  Object item0(&scope, runBuiltin(METH(str_iterator, __next__), iter));
  EXPECT_TRUE(isStrEqualsCStr(*item0, "a"));

  Object item1(&scope, runBuiltin(METH(str_iterator, __next__), iter));
  EXPECT_EQ(*item1, SmallStr::fromCodePoint(0xe4));

  Object item2(&scope, runBuiltin(METH(str_iterator, __next__), iter));
  EXPECT_TRUE(isStrEqualsCStr(*item2, "b"));
}

TEST_F(StrIteratorBuiltinsTest, DunderIterReturnsSelf) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());

  Object iter(&scope, runBuiltin(METH(str, __iter__), empty_str));
  ASSERT_TRUE(iter.isStrIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(METH(str_iterator, __iter__), iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(StrIteratorBuiltinsTest, DunderLengthHintOnEmptyStrIteratorReturnsZero) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());

  Object iter(&scope, runBuiltin(METH(str, __iter__), empty_str));
  ASSERT_TRUE(iter.isStrIterator());

  Object length_hint(&scope,
                     runBuiltin(METH(str_iterator, __length_hint__), iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(StrIteratorBuiltinsTest,
       DunderLengthHintOnConsumedStrIteratorReturnsZero) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("a"));

  Object iter(&scope, runBuiltin(METH(str, __iter__), str));
  ASSERT_TRUE(iter.isStrIterator());

  Object length_hint1(&scope,
                      runBuiltin(METH(str_iterator, __length_hint__), iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 1));

  // Consume the iterator
  Object item1(&scope, runBuiltin(METH(str_iterator, __next__), iter));
  ASSERT_TRUE(item1.isStr());
  ASSERT_EQ(item1, runtime_->newStrFromCStr("a"));

  Object length_hint2(&scope,
                      runBuiltin(METH(str_iterator, __length_hint__), iter));
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
  Str str(&scope, runtime_->newStrFromCStr("Nothing to strip here"));
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
  Str str(&scope, runtime_->newStrFromCStr("nostrip"));
  ASSERT_TRUE(str.isSmallStr());
  Str lstripped_str(&scope, strStripSpaceLeft(thread_, str));
  EXPECT_EQ(*str, *lstripped_str);

  Str rstripped_str(&scope, strStripSpaceRight(thread_, str));
  EXPECT_EQ(*str, *rstripped_str);

  Str stripped_str(&scope, strStripSpace(thread_, str));
  EXPECT_EQ(*str, *stripped_str);
}

TEST_F(StrBuiltinsTest,
       StripSpaceWithFullyStrippableUnicodeStrReturnsEmptyStr) {
  HandleScope scope(thread_);
  Str str(&scope,
          runtime_->newStrFromCStr(u8"\n\r\t\f \u3000  \u202f \n\t\r\f"));
  Str lstripped_str(&scope, strStripSpaceLeft(thread_, str));
  EXPECT_EQ(lstripped_str.length(), 0);

  Str rstripped_str(&scope, strStripSpaceRight(thread_, str));
  EXPECT_EQ(rstripped_str.length(), 0);

  Str stripped_str(&scope, strStripSpace(thread_, str));
  EXPECT_EQ(stripped_str.length(), 0);
}

TEST_F(StrBuiltinsTest, StripSpaceLeft) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Str lstripped_str(&scope, strStripSpaceLeft(thread_, str));
  ASSERT_TRUE(lstripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str, "strp "));

  Str str1(&scope,
           runtime_->newStrFromCStr("   \n \n\tLot of leading space  "));
  ASSERT_TRUE(str1.isLargeStr());
  Str lstripped_str1(&scope, strStripSpaceLeft(thread_, str1));
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str1, "Lot of leading space  "));

  Str str2(&scope, runtime_->newStrFromCStr(u8"\n\n\n  \u2005    \ntest"));
  ASSERT_TRUE(str2.isLargeStr());
  Str lstripped_str2(&scope, strStripSpaceLeft(thread_, str2));
  ASSERT_TRUE(lstripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str2, "test"));
}

TEST_F(StrBuiltinsTest, StripSpaceRight) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Str rstripped_str(&scope, strStripSpaceRight(thread_, str));
  ASSERT_TRUE(rstripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str, " strp"));

  Str str1(&scope,
           runtime_->newStrFromCStr("  Lot of trailing space\t\n \n    "));
  ASSERT_TRUE(str1.isLargeStr());
  Str rstripped_str1(&scope, strStripSpaceRight(thread_, str1));
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str1, "  Lot of trailing space"));

  Str str2(&scope, runtime_->newStrFromCStr(u8"test\n  \u2004 \n\n"));
  ASSERT_TRUE(str2.isLargeStr());
  Str rstripped_str2(&scope, strStripSpaceRight(thread_, str2));
  ASSERT_TRUE(rstripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str2, "test"));
}

TEST_F(StrBuiltinsTest, StripSpaceBoth) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(" strp "));
  ASSERT_TRUE(str.isSmallStr());
  Str stripped_str(&scope, strStripSpace(thread_, str));
  ASSERT_TRUE(stripped_str.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str, "strp"));

  Str str1(&scope,
           runtime_->newStrFromCStr(
               "\n \n    \n\tLot of leading and trailing space\n \n    "));
  ASSERT_TRUE(str1.isLargeStr());
  Str stripped_str1(&scope, strStripSpace(thread_, str1));
  EXPECT_TRUE(
      isStrEqualsCStr(*stripped_str1, "Lot of leading and trailing space"));

  Str str2(&scope,
           runtime_->newStrFromCStr(u8"\n\u00a0\ttest\t  \u1680    \n\n\n"));
  ASSERT_TRUE(str2.isLargeStr());
  Str stripped_str2(&scope, strStripSpace(thread_, str2));
  ASSERT_TRUE(stripped_str2.isSmallStr());
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str2, "test"));
}

TEST_F(StrBuiltinsTest, StripWithEmptyStrIsIdentity) {
  HandleScope scope(thread_);
  Str empty_str(&scope, Str::empty());
  Str chars(&scope, runtime_->newStrFromCStr("abc"));
  Str lstripped_empty_str(&scope, strStripLeft(thread_, empty_str, chars));
  EXPECT_EQ(*empty_str, *lstripped_empty_str);

  Str rstripped_empty_str(&scope, strStripRight(thread_, empty_str, chars));
  EXPECT_EQ(*empty_str, *rstripped_empty_str);

  Str stripped_empty_str(&scope, strStrip(thread_, empty_str, chars));
  EXPECT_EQ(*empty_str, *stripped_empty_str);
}

TEST_F(StrBuiltinsTest, StripWithFullyStrippableStrReturnsEmptyStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("bbbbaaaaccccdddd"));
  Str chars(&scope, runtime_->newStrFromCStr("abcd"));
  Str lstripped_str(&scope, strStripLeft(thread_, str, chars));
  EXPECT_EQ(lstripped_str.length(), 0);

  Str rstripped_str(&scope, strStripRight(thread_, str, chars));
  EXPECT_EQ(rstripped_str.length(), 0);

  Str stripped_str(&scope, strStrip(thread_, str, chars));
  EXPECT_EQ(stripped_str.length(), 0);
}

TEST_F(StrBuiltinsTest, StripWithEmptyCharsIsIdentity) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(" Just another string "));
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
  Str str(&scope, runtime_->newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime_->newStrFromCStr("abcd"));
  Str stripped_str(&scope, strStrip(thread_, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*stripped_str, "Hello Worl"));
}

TEST_F(StrBuiltinsTest, StripLeft) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime_->newStrFromCStr("abcd"));
  Str lstripped_str(&scope, strStripLeft(thread_, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*lstripped_str, "Hello Worldcab"));
}

TEST_F(StrBuiltinsTest, StripRight) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("bcdHello Worldcab"));
  Str chars(&scope, runtime_->newStrFromCStr("abcd"));
  Str rstripped_str(&scope, strStripRight(thread_, str, chars));
  EXPECT_TRUE(isStrEqualsCStr(*rstripped_str, "bcdHello Worl"));
}

TEST_F(StrBuiltinsTest, CountWithNeedleLargerThanHaystackReturnsZero) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("h"));
  Str needle(&scope, runtime_->newStrFromCStr("hello"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, 0, kMaxWord), 0));
}

TEST_F(StrBuiltinsTest, CountWithSmallNegativeStartIndexesFromEnd) {
  // Index from the end if abs(start) < len(haystack)
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, runtime_->newStrFromCStr("h"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, -1, kMaxWord), 0));
}

TEST_F(StrBuiltinsTest, CountWithLargeNegativeStartIndexesFromStart) {
  // Default to 0 if abs(start) >= len(haystack)
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, runtime_->newStrFromCStr("h"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, -10, kMaxWord), 1));
}

TEST_F(StrBuiltinsTest, CountWithSmallNegativeEndIndexesFromEnd) {
  // Index from the end if abs(end) < len(haystack)
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, 0, -2), 0));
}

TEST_F(StrBuiltinsTest, CountWithLargeNegativeEndIndexesFromStart) {
  // Default to 0 if abs(end) >= len(haystack)
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, 0, -10), 0));
}

TEST_F(StrBuiltinsTest, CountWithSingleCharNeedleFindsNeedle) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("oooo"));
  Str needle(&scope, runtime_->newStrFromCStr("o"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, 0, kMaxWord), 4));
}

TEST_F(StrBuiltinsTest, CountWithMultiCharNeedleFindsNeedle) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("oooo"));
  Str needle(&scope, runtime_->newStrFromCStr("oo"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, 0, kMaxWord), 2));
}

TEST_F(StrBuiltinsTest, CountWithUnicodeNeedleReturnsCount) {
  HandleScope scope(thread_);
  Str haystack(&scope,
               runtime_->newStrFromCStr(
                   u8"\u20ac10 Cr\u00e8me Cr\u00e8me br\u00fbl\u00e9e"));
  Str needle(&scope, runtime_->newStrFromCStr(u8"Cr\u00e8me"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, 0, kMaxWord), 2));
}

TEST_F(StrBuiltinsTest, CountWithNonNormalizedUTF8StringFindsChar) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr(u8"\u0061\u0308\u0304"));
  fprintf(stderr, "'%s'\n", unique_c_ptr<char>(haystack.toCStr()).get());
  Str needle(&scope, runtime_->newStrFromCStr("a"));
  EXPECT_TRUE(isIntEqualsWord(strCount(haystack, needle, 0, kMaxWord), 1));
}

TEST_F(StrBuiltinsTest, FindWithEmptyHaystackAndEmptyNeedleReturnsZero) {
  HandleScope scope(thread_);
  Str haystack(&scope, Str::empty());
  Str needle(&scope, Str::empty());
  EXPECT_EQ(strFind(haystack, needle), 0);
}

TEST_F(StrBuiltinsTest,
       FindWithEmptyHaystackAndNonEmptyNeedleReturnsNegativeOne) {
  HandleScope scope(thread_);
  Str haystack(&scope, Str::empty());
  Str needle(&scope, runtime_->newStrFromCStr("hello"));
  EXPECT_EQ(strFind(haystack, needle), -1);
}

TEST_F(StrBuiltinsTest, FindWithNonEmptyHaystackAndEmptyNeedleReturnsZero) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, Str::empty());
  EXPECT_EQ(strFind(haystack, needle), 0);
}

TEST_F(StrBuiltinsTest, FindWithNonExistentNeedleReturnsNegativeOne) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("hello"));
  Str needle(&scope, runtime_->newStrFromCStr("a"));
  EXPECT_EQ(strFind(haystack, needle), -1);
}

TEST_F(StrBuiltinsTest, FindReturnsIndexOfFirstOccurrence) {
  HandleScope scope(thread_);
  Str haystack(&scope, runtime_->newStrFromCStr("helloworldhelloworld"));
  Str needle(&scope, runtime_->newStrFromCStr("world"));
  EXPECT_EQ(strFind(haystack, needle), 5);
}

TEST_F(StrBuiltinsTest, FindFirstNonWhitespaceWithEmptyStringReturnsZero) {
  HandleScope scope(thread_);
  Str str(&scope, Str::empty());
  EXPECT_EQ(strFindFirstNonWhitespace(str), 0);
}

TEST_F(StrBuiltinsTest, FindFirstNonWhitespaceWithOnlyWhitespaceReturnsLength) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(u8" \u205f "));
  EXPECT_EQ(strFindFirstNonWhitespace(str), str.length());
}

TEST_F(StrBuiltinsTest, FindFirstNonWhitespaceFindsFirstNonWhitespaceChar) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr(u8" \u3000 foo   "));
  EXPECT_EQ(strFindFirstNonWhitespace(str), 5);
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleReturnsZero) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".find("")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 0));
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleReturnsNegativeOne) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".find("", 8)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), -1));
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleAndSliceReturnsStart) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".find("", 3, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 3));
}

TEST_F(StrBuiltinsTest, FindWithEmptyNeedleAndEmptySliceReturnsStart) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".find("", 3, 3)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 3));
}

TEST_F(StrBuiltinsTest, FindWithNegativeStartClipsToZero) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".find("h", -5, 1)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 0));
}

TEST_F(StrBuiltinsTest, FindWithEndPastEndOfStringClipsToLength) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".find("h", 0, 100)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 0));
}

TEST_F(StrBuiltinsTest, FindCallsDunderIndexOnStart) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return 4
result = "bbbbbbbb".find("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(StrBuiltinsTest, FindCallsDunderIndexOnEnd) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return 5
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(StrBuiltinsTest, FindClampsStartReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".find("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), -1));
}

TEST_F(StrBuiltinsTest, FindClampsEndReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(StrBuiltinsTest, FindClampsEndReturningBigNegativeNumber) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return -46116860184273879030
result = "aaaabbbb".find("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), -1));
}

TEST_F(StrBuiltinsTest, FindWithUnicodeReturnsCodePointIndex) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "Cr\u00e8me br\u00fbl\u00e9e"
result = s.find("e")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(StrBuiltinsTest, FindWithStartAfterUnicodeCodePoint) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.find("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 8));
}

TEST_F(StrBuiltinsTest, FindWithDifferentSizeCodePoints) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "Cr\u00e8me \u10348 \u29D98 br\u00fbl\u00e9e"
result = s.find("\u29D98")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 9));
}

TEST_F(StrBuiltinsTest, FindWithOneCharStringFindsChar) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result1 = "hello".find("h")
result2 = "hello".find("e")
result3 = "hello".find("z")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result1"), 0));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result3"), -1));
}

TEST_F(StrBuiltinsTest, FindWithSlicePreservesIndices) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result1 = "hello".find("h", 1)
result2 = "hello".find("e", 1)
result3 = "hello".find("o", 0, 2)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result1"), -1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result3"), -1));
}

TEST_F(StrBuiltinsTest, FindWithMultiCharStringFindsSubstring) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result1 = "hello".find("he")
result2 = "hello".find("el")
result3 = "hello".find("ze")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result1"), 0));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result2"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result3"), -1));
}

TEST_F(StrBuiltinsTest, RfindWithOneCharStringFindsChar) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".rfind("l")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 3));
}

TEST_F(StrBuiltinsTest, RfindCharWithUnicodeReturnsCodePointIndex) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "Cr\u00e8me br\u00fbl\u00e9e"
result = s.rfind("e")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 11));
}

TEST_F(StrBuiltinsTest, RfindCharWithStartAfterUnicodeCodePoint) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.rfind("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 15));
}

TEST_F(StrBuiltinsTest, RfindCharWithDifferentSizeCodePoints) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "Cr\u00e8me \u10348 \u29D98 br\u00fbl\u00e9e\u2070E\u29D98 "
result = s.rfind("\u29D98")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 20));
}

TEST_F(StrBuiltinsTest, RfindWithMultiCharStringFindsSubstring) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "aabbaa".rfind("aa")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(StrBuiltinsTest, RfindCharWithNegativeStartClipsToZero) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".rfind("h", -5, 1)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 0));
}

TEST_F(StrBuiltinsTest, RfindCharWithEndPastEndOfStringClipsToLength) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".rfind("h", 0, 100)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 0));
}

TEST_F(StrBuiltinsTest, RfindWithEndLessThanLengthStartsAtEnd) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "aaaabb".rfind("b", 0, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(StrBuiltinsTest, RfindCallsDunderIndexOnEnd) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return 5
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 4));
}

TEST_F(StrBuiltinsTest, RfindClampsStartReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".rfind("b", C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), -1));
}

TEST_F(StrBuiltinsTest, RfindClampsEndReturningBigNumber) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return 46116860184273879030
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 7));
}

TEST_F(StrBuiltinsTest, RfindClampsEndReturningBigNegativeNumber) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __index__(self):
        return -46116860184273879030
result = "aaaabbbb".rfind("b", 0, C())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), -1));
}

TEST_F(StrBuiltinsTest, RfindWithEmptyHaystackAndNeedleReturnsZero) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "".rfind("")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 0));
}

TEST_F(StrBuiltinsTest, RfindWithEmptyHaystackAndNeedleAndBoundsReturnsZero) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "".rfind("", 0, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 0));
}

TEST_F(StrBuiltinsTest, RfindCharWithEmptyNeedleReturnsLength) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".rfind("")
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

TEST_F(StrBuiltinsTest, RfindCharWithEmptyNeedleReturnsNegativeOne) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".rfind("", 8)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), -1));
}

TEST_F(StrBuiltinsTest, RfindCharWithEmptyNeedleAndSliceReturnsEnd) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".rfind("", 3, 5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

TEST_F(StrBuiltinsTest, RfindWithEmptyNeedleAndEmptySliceReturnsEnd) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "hello".rfind("", 3, 3)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 3));
}

TEST_F(StrBuiltinsTest, IndexWithPresentSubstringReturnsIndex) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
s = "\u20ac10 Cr\u00e8me br\u00fbl\u00e9e"
result = s.index("e", 4)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 8));
}

TEST_F(StrBuiltinsTest, IndexWithMissingSubstringRaisesValueError) {
  EXPECT_TRUE(
      raised(runFromCStr(runtime_, "'h'.index('q')"), LayoutId::kValueError));
}

TEST_F(StrBuiltinsTest, DunderHashReturnsSmallInt) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello world"));
  EXPECT_TRUE(runBuiltin(METH(str, __hash__), str).isSmallInt());
}

TEST_F(StrBuiltinsTest, DunderHashSmallStringReturnsSmallInt) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("h"));
  EXPECT_TRUE(runBuiltin(METH(str, __hash__), str).isSmallInt());
}

TEST_F(StrBuiltinsTest, DunderHashWithEquivalentStringsReturnsSameHash) {
  HandleScope scope(thread_);
  Str str1(&scope, runtime_->newStrFromCStr("hello world foobar"));
  Str str2(&scope, runtime_->newStrFromCStr("hello world foobar"));
  EXPECT_NE(*str1, *str2);
  Object result1(&scope, runBuiltin(METH(str, __hash__), str1));
  Object result2(&scope, runBuiltin(METH(str, __hash__), str2));
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_TRUE(result2.isSmallInt());
  EXPECT_EQ(*result1, *result2);
}

TEST_F(StrBuiltinsTest, DunderHashWithSubclassReturnsSameHash) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(str): pass
i0 = C("abc")
i1 = "abc"
)")
                   .isError());
  Object i0(&scope, mainModuleAt(runtime_, "i0"));
  Object i1(&scope, mainModuleAt(runtime_, "i1"));
  Object result0(&scope, runBuiltin(METH(str, __hash__), i0));
  Object result1(&scope, runBuiltin(METH(str, __hash__), i1));
  EXPECT_TRUE(result0.isSmallInt());
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_EQ(result0, result1);
}

TEST_F(StringIterTest, SimpleIter) {
  HandleScope scope(thread_);

  Str str(&scope, runtime_->newStrFromCStr("test"));
  EXPECT_TRUE(str.equalsCStr("test"));

  StrIterator iter(&scope, runtime_->newStrIterator(str));
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

  Str str(&scope, runtime_->newStrFromCStr("test"));
  EXPECT_TRUE(str.equalsCStr("test"));

  StrIterator iter(&scope, runtime_->newStrIterator(str));
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
  EXPECT_TRUE(raised(runFromCStr(runtime_, "str.__contains__(3, 'foo')"),
                     LayoutId::kTypeError));
}

TEST_F(StrBuiltinsTest, DunderContainsWithNonStrOtherRaisesTypeError) {
  EXPECT_TRUE(raised(runFromCStr(runtime_, "str.__contains__('foo', 3)"),
                     LayoutId::kTypeError));
}

TEST_F(StrBuiltinsTest, DunderContainsWithPresentSubstrReturnsTrue) {
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = str.__contains__('foo', 'f')").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST_F(StrBuiltinsTest, DunderContainsWithNotPresentSubstrReturnsTrue) {
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = str.__contains__('foo', 'q')").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, Bool::falseObj());
}

TEST_F(StrBuiltinsTest, CapitalizeReturnsCapitalizedStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "foo".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "Foo"));
}

TEST_F(StrBuiltinsTest, CapitalizeUpperCaseReturnsUnmodifiedStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "Foo".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "Foo"));
}

TEST_F(StrBuiltinsTest, CapitalizeAllUppercaseReturnsCapitalizedStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "FOO".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "Foo"));
}

TEST_F(StrBuiltinsTest, CapitalizeWithEmptyStrReturnsEmptyStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = "".capitalize()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), ""));
}

TEST_F(StrBuiltinsTest, StrUnderlyingWithStrReturnsSameStr) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("hello"));
  Object underlying(&scope, strUnderlying(*str));
  EXPECT_EQ(*str, *underlying);
}

TEST_F(StrBuiltinsTest, StrUnderlyingWithSubClassReturnsUnderlyingStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class SubStr(str): pass
substr = SubStr("some string")
)")
                   .isError());
  Object substr(&scope, mainModuleAt(runtime_, "substr"));
  ASSERT_FALSE(substr.isStr());
  Object underlying(&scope, strUnderlying(*substr));
  EXPECT_TRUE(isStrEqualsCStr(*underlying, "some string"));
}

}  // namespace testing
}  // namespace py
