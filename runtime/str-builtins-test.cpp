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
  EXPECT_EQ(small_str->builtinBase(), LayoutId::kStr);

  Type large_str(&scope, runtime.typeAt(LayoutId::kLargeStr));
  EXPECT_EQ(large_str->builtinBase(), LayoutId::kStr);

  Type str(&scope, runtime.typeAt(LayoutId::kStr));
  EXPECT_EQ(str->builtinBase(), LayoutId::kStr);
}

TEST(StrBuiltinsTest, RichCompareStringEQ) {  // pystone dependency
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

TEST(StrBuiltinsTest, RichCompareStringNE) {  // pystone dependency
  const char* src = R"(
a = "__main__"
if (a != "__main__"):
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "bar\n");
}

TEST(StrBuiltinsTest, RichCompareSingleCharLE) {
  Runtime runtime;

  runtime.runFromCStr(R"(
a_le_b = 'a' <= 'b'
b_le_a = 'a' >= 'b'
a_le_a = 'a' <= 'a'
)");

  HandleScope scope;

  Object a_le_b(&scope, moduleAt(&runtime, "__main__", "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());

  Object b_le_a(&scope, moduleAt(&runtime, "__main__", "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());

  Object a_le_a(&scope, moduleAt(&runtime, "__main__", "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
}

TEST(StrBuiltinsTest, LowerOnASCIILettersReturnsLowerCaseString) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "HELLO".lower()
b = "HeLLo".lower()
c = "hellO".lower()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str b(&scope, moduleAt(&runtime, "__main__", "b"));
  Str c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_PYSTRING_EQ(*a, "hello");
  EXPECT_PYSTRING_EQ(*b, "hello");
  EXPECT_PYSTRING_EQ(*c, "hello");
}

TEST(StrBuiltinsTest, LowerOnLowercaseASCIILettersReturnsSameString) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".lower()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "hello");
}

TEST(StrBuiltinsTest, LowerOnNumbersReturnsSameString) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "foo 123".lower()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "foo 123");
}

TEST(StrBuiltinsTest, DunderNewCallsDunderStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
    def __str__(self):
        return "foo"
a = str.__new__(str, Foo())
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "foo");
}

TEST(StrBuiltinsTest, DunderNewCallsReprIfNoDunderStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
  pass
f = Foo()
a = str.__new__(str, f)
b = repr(f)
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_PYSTRING_EQ(*a, *b);
}

TEST(StrBuiltinsTest, DunderNewWithNoArgsExceptTypeReturnsEmptyString) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = str.__new__(str)
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "");
}

TEST(StrBuiltinsTest, DunderNewWithStrReturnsSameStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = str.__new__(str, "hello")
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "hello");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNoArgsThrows) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("str.__new__()"),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithTooManyArgsThrows) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("str.__new__(str, 1, 2, 3, 4)"),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNonTypeArgThrows) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("str.__new__(1)"),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNonSubtypeArgThrows) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("str.__new__(object)"),
               "aborting due to pending exception");
}

TEST(StrBuiltinsTest, DunderAddWithTwoStringsReturnsConcatenatedString) {
  Runtime runtime;
  HandleScope scope;
  Object str1(&scope, runtime.newStrFromCStr("hello"));
  Object str2(&scope, runtime.newStrFromCStr("world"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(result), "helloworld");
}

TEST(StrBuiltinsTest, DunderAddWithLeftEmptyAndReturnsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str1(&scope, runtime.newStrFromCStr(""));
  Object str2(&scope, runtime.newStrFromCStr("world"));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(result), "world");
}

TEST(StrBuiltinsTest, DunderAddWithRightEmptyAndReturnsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str1(&scope, runtime.newStrFromCStr("hello"));
  Object str2(&scope, runtime.newStrFromCStr(""));
  Object result(&scope, runBuiltin(StrBuiltins::dunderAdd, str1, str2));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(result), "hello");
}

TEST(StrBuiltinsTest, PlusOperatorOnStringsEqualsDunderAdd) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello"
b = "world"
c = a + b
d = a.__add__(b)
)");
  HandleScope scope;
  Str c(&scope, moduleAt(&runtime, "__main__", "c"));
  Str d(&scope, moduleAt(&runtime, "__main__", "d"));

  EXPECT_PYSTRING_EQ(*c, "helloworld");
  EXPECT_PYSTRING_EQ(*d, "helloworld");
}

TEST(StrBuiltinsTest, StringLen) {
  Runtime runtime;
  runtime.runFromCStr(R"(
l1 = len("aloha")
l2 = str.__len__("aloha")
l3 = "aloha".__len__()
)");
  HandleScope scope;
  SmallInt l1(&scope, moduleAt(&runtime, "__main__", "l1"));
  EXPECT_EQ(5, l1->value());
  SmallInt l2(&scope, moduleAt(&runtime, "__main__", "l2"));
  EXPECT_EQ(5, l2->value());
  SmallInt l3(&scope, moduleAt(&runtime, "__main__", "l3"));
  EXPECT_EQ(5, l3->value());
}

TEST(StrBuiltinsTest, StringLenWithEmptyString) {
  Runtime runtime;
  runtime.runFromCStr("l = len('')");
  HandleScope scope;
  SmallInt l(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_EQ(0, l->value());
}

TEST(StrBuiltinsDeathTest, StringLenWithInt) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("l = str.__len__(3)"),
               "descriptor '__len__' requires a 'str' object");
}

TEST(StrBuiltinsDeathTest, StringLenWithExtraArgument) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr("l = 'aloha'.__len__('arg')"),
               "expected 0 arguments");
}

TEST(StrBuiltinsTest, IndexWithSliceWithPositiveInts) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Int one(&scope, RawSmallInt::fromWord(1));
  Int two(&scope, RawSmallInt::fromWord(2));
  Int four(&scope, RawSmallInt::fromWord(4));
  Object none(&scope, NoneType::object());
  Slice slice_a(&scope, runtime.newSlice(one, two, none));
  Object result_a(&scope,
                  runBuiltin(StrBuiltins::dunderGetItem, hello, slice_a));
  ASSERT_TRUE(result_a->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result_a), "e");
  Slice slice_b(&scope, runtime.newSlice(one, four, none));
  Object result_b(&scope,
                  runBuiltin(StrBuiltins::dunderGetItem, hello, slice_b));
  ASSERT_TRUE(result_b->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result_b), "ell");
}

TEST(StrBuiltinsTest, IndexWithSliceWithNegativeInts) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Int negative_two(&scope, RawSmallInt::fromWord(-2));
  Int negative_one(&scope, RawSmallInt::fromWord(-1));
  Int one(&scope, RawSmallInt::fromWord(1));
  Object none(&scope, NoneType::object());
  Slice slice_a(&scope, runtime.newSlice(negative_one, none, none));
  Object result_a(&scope,
                  runBuiltin(StrBuiltins::dunderGetItem, hello, slice_a));
  ASSERT_TRUE(result_a->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result_a), "o");
  Slice slice_b(&scope, runtime.newSlice(one, negative_two, none));
  Object result_b(&scope,
                  runBuiltin(StrBuiltins::dunderGetItem, hello, slice_b));
  ASSERT_TRUE(result_b->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result_b), "el");
}

TEST(StrBuiltinsTest, IndexWithSliceWithStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Int zero(&scope, RawSmallInt::fromWord(0));
  Int one(&scope, RawSmallInt::fromWord(1));
  Int two(&scope, RawSmallInt::fromWord(2));
  Int three(&scope, RawSmallInt::fromWord(3));
  Int five(&scope, RawSmallInt::fromWord(5));
  Slice slice_a(&scope, runtime.newSlice(zero, five, two));
  Object result_a(&scope,
                  runBuiltin(StrBuiltins::dunderGetItem, hello, slice_a));
  ASSERT_TRUE(result_a->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result_a), "hlo");
  Slice slice_b(&scope, runtime.newSlice(one, five, three));
  Object result_b(&scope,
                  runBuiltin(StrBuiltins::dunderGetItem, hello, slice_b));
  ASSERT_TRUE(result_b->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result_b), "eo");
}

TEST(StrBuiltinsTest, EmptyStringIndexWithSliceWithNegativeOneStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr(""));
  Object none(&scope, NoneType::object());
  Int negative_one(&scope, RawSmallInt::fromWord(-1));
  Slice slice(&scope, runtime.newSlice(none, none, negative_one));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "");
}

TEST(StrBuiltinsTest, IndexWithSliceWithNegativeOneStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Object none(&scope, NoneType::object());
  Int negative_one(&scope, RawSmallInt::fromWord(-1));
  Slice slice(&scope, runtime.newSlice(none, none, negative_one));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "olleh");
}

TEST(StrBuiltinsTest, IndexWithSliceWithNegativeTwoStep) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, runtime.newStrFromCStr("hello"));
  Object none(&scope, NoneType::object());
  Int negative_one(&scope, RawSmallInt::fromWord(-2));
  Slice slice(&scope, runtime.newSlice(none, none, negative_one));
  Object result(&scope, runBuiltin(StrBuiltins::dunderGetItem, hello, slice));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "olh");
}

TEST(StrBuiltinsTest, StartsWithEmptyStringReturnsTrue) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith("")
b = "".startswith("")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(StrBuiltinsTest, StartsWithStringReturnsTrue) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith("h")
b = "hello".startswith("he")
c = "hello".startswith("hel")
d = "hello".startswith("hell")
e = "hello".startswith("hello")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  Bool e(&scope, moduleAt(&runtime, "__main__", "e"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
  EXPECT_TRUE(c->value());
  EXPECT_TRUE(d->value());
  EXPECT_TRUE(e->value());
}

TEST(StrBuiltinsTest, StartsWithTooLongPrefixReturnsFalse) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith("hihello")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a->value());
}

TEST(StrBuiltinsTest, StartsWithUnrelatedPrefixReturnsFalse) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith("bob")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a->value());
}

TEST(StrBuiltinsTest, StartsWithStart) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith("e", 1)
b = "hello".startswith("o", 5)
c = "hello".startswith("ell", 1)
d = "hello".startswith("llo", 3)
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a->value());
  EXPECT_FALSE(b->value());
  EXPECT_TRUE(c->value());
  EXPECT_FALSE(d->value());
}

TEST(StrBuiltinsTest, StartsWithStartAndEnd) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith("e", 1, 3)
b = "hello".startswith("el", 1, 4)
c = "hello".startswith("ll", 2, 5)
d = "hello".startswith("ll", 1, 4)
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
  EXPECT_TRUE(c->value());
  EXPECT_FALSE(d->value());
}

TEST(StrBuiltinsTest, StartsWithStartAndEndNegatives) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith("h", 0, -1)
b = "hello".startswith("ll", -3)
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(StrBuiltinsTest, StartsWithTupleOfPrefixes) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".startswith(("h", "lo"))
b = "hello".startswith(("asdf", "foo", "bar"))
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a->value());
  EXPECT_FALSE(b->value());
}

TEST(StrBuiltinsTest, EndsWithEmptyStringReturnsTrue) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith("")
b = "".endswith("")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(StrBuiltinsTest, EndsWithStringReturnsTrue) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith("o")
b = "hello".endswith("lo")
c = "hello".endswith("llo")
d = "hello".endswith("ello")
e = "hello".endswith("hello")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  Bool e(&scope, moduleAt(&runtime, "__main__", "e"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
  EXPECT_TRUE(c->value());
  EXPECT_TRUE(d->value());
  EXPECT_TRUE(e->value());
}

TEST(StrBuiltinsTest, EndsWithTooLongSuffixReturnsFalse) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith("hihello")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a->value());
}

TEST(StrBuiltinsTest, EndsWithUnrelatedSuffixReturnsFalse) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith("bob")
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_FALSE(a->value());
}

TEST(StrBuiltinsTest, EndsWithStart) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith("o", 1)
b = "hello".endswith("o", 5)
c = "hello".endswith("llo", 1)
d = "hello".endswith("llo", 3)
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a->value());
  EXPECT_FALSE(b->value());
  EXPECT_TRUE(c->value());
  EXPECT_FALSE(d->value());
}

TEST(StrBuiltinsTest, EndsWithStartAndEnd) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith("l", 1, 3)
b = "hello".endswith("ll", 1, 4)
c = "hello".endswith("lo", 2, 5)
d = "hello".endswith("llo", 1, 4)
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime, "__main__", "c"));
  Bool d(&scope, moduleAt(&runtime, "__main__", "d"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
  EXPECT_TRUE(c->value());
  EXPECT_FALSE(d->value());
}

TEST(StrBuiltinsTest, EndsWithStartAndEndNegatives) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith("l", 0, -1)
b = "hello".endswith("o", -1)
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(StrBuiltinsTest, EndsWithTupleOfSuffixes) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".endswith(("o", "llo"))
b = "hello".endswith(("asdf", "foo", "bar"))
)");
  HandleScope scope;
  Bool a(&scope, moduleAt(&runtime, "__main__", "a"));
  Bool b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(a->value());
  EXPECT_FALSE(b->value());
}

TEST(StrBuiltinsTest, StringFormat) {
  Runtime runtime;
  runtime.runFromCStr(R"(
n = 123
f = 3.14
s = "pyros"
a = "hello %d %g %s" % (n, f, s)
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "hello 123 3.14 pyros");
}

TEST(StrBuiltinsTest, StringFormatSingleString) {
  Runtime runtime;
  runtime.runFromCStr(R"(
s = "pyro"
a = "%s" % s
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "pyro");
}

TEST(StrBuiltinsTest, StringFormatTwoStrings) {
  Runtime runtime;
  runtime.runFromCStr(R"(
s = "pyro"
a = "%s%s" % (s, s)
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "pyropyro");
}

TEST(StrBuiltinsTest, StringFormatMixed) {
  Runtime runtime;
  runtime.runFromCStr(R"(
s = "pyro"
a = "1%s,2%s,3%s,4%s,5%s" % (s, s, s, s, s)
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "1pyro,2pyro,3pyro,4pyro,5pyro");
}

TEST(StrBuiltinsTest, StringFormatMixed2) {
  Runtime runtime;
  runtime.runFromCStr(R"(
s = "pyro"
a = "%d%s,%d%s,%d%s" % (1, s, 2, s, 3, s)
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "1pyro,2pyro,3pyro");
}

TEST(StrBuiltinsDeathTest, StringFormatMalformed) {
  Runtime runtime;
  const char* src = R"(
a = "%" % ("pyro",)
)";
  EXPECT_DEATH(runtime.runFromCStr(src), "Incomplete format");
}

TEST(StrBuiltinsDeathTest, StringFormatMismatch) {
  Runtime runtime;
  const char* src = R"(
a = "%d%s" % ("pyro",)
)";
  EXPECT_DEATH(runtime.runFromCStr(src), "Argument mismatch");
}

TEST(StrBuiltinsTest, DunderReprOnASCIIStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "'hello'");
}

TEST(StrBuiltinsTest, DunderReprOnASCIINonPrintable) {
  Runtime runtime;
  // 6 is the ACK character.
  runtime.runFromCStr(R"(
a = "\x06".__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "'\\x06'");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithDoubleQuotes) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = 'hello "world"'.__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "'hello \"world\"'");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithSingleQuotes) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello 'world'".__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "\"hello 'world'\"");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithBothQuotes) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello 'world', I am your \"father\"".__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, R"('hello \'world\', I am your "father"')");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithNestedQuotes) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello 'world, \"I am 'your \"father\"'\"'".__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, R"('hello \'world, "I am \'your "father"\'"\'')");
}

TEST(StrBuiltinsTest, DunderReprOnCommonEscapeSequences) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "\n \t \r \\".__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "'\\n \\t \\r \\\\'");
}

TEST(StrBuiltinsTest, DunderStr) {
  const char* src = R"(
result = 'Hello, World!'.__str__()
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "Hello, World!");
}

TEST(StrBuiltinsTest, JoinWithEmptyArray) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = ",".join([])
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "");
}

TEST(StrBuiltinsTest, JoinWithOneElementArray) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = ",".join(["1"])
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "1");
}

TEST(StrBuiltinsTest, JoinWithManyElementArray) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = ",".join(["1", "2", "3"])
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "1,2,3");
}

TEST(StrBuiltinsTest, JoinWithManyElementArrayAndEmptySeparator) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "".join(["1", "2", "3"])
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "123");
}

TEST(StrBuiltinsTest, JoinWithIterable) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = ",".join(("1", "2", "3"))
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_PYSTRING_EQ(*a, "1,2,3");
}

TEST(StrBuiltinsDeathTest, JoinWithNonStringInArrayThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = ",".join(["hello", 1])
)"),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, JoinWithNonStringSeparatorThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
a = str.join(1, ["hello", 1])
)"),
               "aborting due to pending exception");
}

TEST(StrBuiltinsTest, PartitionOnSingleCharStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".partition("l")
)");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "l");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(2)), "lo");
}

TEST(StrBuiltinsTest, PartitionOnMultiCharStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".partition("ll")
)");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "ll");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(2)), "o");
}

TEST(StrBuiltinsTest, PartitionOnSuffix) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".partition("lo")
b = "hello".partition("lop")
)");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));
  Tuple b(&scope, moduleAt(&runtime, "__main__", "b"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "hel");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "lo");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(2)), "");

  ASSERT_EQ(b->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(0)), "hello");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(1)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(2)), "");
}

TEST(StrBuiltinsTest, PartitionOnPrefix) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".partition("he")
b = "hello".partition("hex")
)");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));
  Tuple b(&scope, moduleAt(&runtime, "__main__", "b"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(2)), "llo");

  ASSERT_EQ(b->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(0)), "hello");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(1)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(2)), "");
}

TEST(StrBuiltinsTest, PartitionLargerStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".partition("abcdefghijk")
)");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "hello");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(2)), "");
}

TEST(StrBuiltinsTest, PartitionEmptyStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "".partition("a")
)");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(2)), "");
}

TEST(StrBuiltinsTest, SplitWithOneCharSeparator) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".split("e")
b = "hello".split("l")
)");
  HandleScope scope;

  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a->numItems(), 2);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "h");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "llo");

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b->numItems(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(0)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(1)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(2)), "o");
}

TEST(StrBuiltinsTest, SplitWithEmptySelfReturnsSingleEmptyString) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "".split("a")
)");
  HandleScope scope;
  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a->numItems(), 1);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "");
}

TEST(StrBuiltinsTest, SplitWithMultiCharSeparator) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".split("el")
b = "hello".split("ll")
c = "hello".split("hello")
d = "hellllo".split("ll")
)");
  HandleScope scope;

  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a->numItems(), 2);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "h");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "lo");

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b->numItems(), 2);
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(0)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(1)), "o");

  List c(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_EQ(c->numItems(), 2);
  EXPECT_PYSTRING_EQ(RawStr::cast(c->at(0)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(c->at(1)), "");

  List d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_EQ(d->numItems(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(d->at(0)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(d->at(1)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(d->at(2)), "o");
}

TEST(StrBuiltinsTest, SplitWithMaxSplitBelowPartsStopsEarly) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".split("l", 1)
b = "1,2,3,4".split(",", 2)
)");
  HandleScope scope;

  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a->numItems(), 2);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "lo");

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b->numItems(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(0)), "1");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(1)), "2");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(2)), "3,4");
}

TEST(StrBuiltinsTest, SplitWithMaxSplitGreaterThanNumParts) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = "hello".split("l", 2)
b = "1,2,3,4".split(",", 5)
)");
  HandleScope scope;

  List a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_EQ(a->numItems(), 3);
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(0)), "he");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(1)), "");
  EXPECT_PYSTRING_EQ(RawStr::cast(a->at(2)), "o");

  List b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_EQ(b->numItems(), 4);
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(0)), "1");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(1)), "2");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(2)), "3");
  EXPECT_PYSTRING_EQ(RawStr::cast(b->at(3)), "4");
}

TEST(StrBuiltinsDeathTest, StrStripWithNoArgsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
str.strip()
)"),
               R"(str.strip\(\) needs an argument)");
}

TEST(StrBuiltinsDeathTest, StrLStripWithNoArgsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
str.lstrip()
)"),
               R"(str.lstrip\(\) needs an argument)");
}

TEST(StrBuiltinsDeathTest, StrRStripWithNoArgsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
str.rstrip()
)"),
               R"(str.rstrip\(\) needs an argument)");
}

TEST(StrBuiltinsDeathTest, StrStripTooManyArgsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
"test".strip(None, "test")
)"),
               R"(str.strip\(\) takes at most 1 argument \(2 given\))");
}

TEST(StrBuiltinsDeathTest, StrLStripTooManyArgsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
"test".lstrip(None, "test")
)"),
               R"(str.lstrip\(\) takes at most 1 argument \(2 given\))");
}

TEST(StrBuiltinsDeathTest, StrRStripTooManyArgsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
"test".rstrip(None, "test")
)"),
               R"(str.rstrip\(\) takes at most 1 argument \(2 given\))");
}

TEST(StrBuiltinsDeathTest, StrStripWithNonStrThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
str.strip(None)
)"),
               R"(str.strip\(\) requires a str object)");
}

TEST(StrBuiltinsDeathTest, StrLStripWithNonStrThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
str.lstrip(None)
)"),
               R"(str.lstrip\(\) requires a str object)");
}

TEST(StrBuiltinsDeathTest, StrRStripWithNonStrThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
str.rstrip(None)
)"),
               R"(str.rstrip\(\) requires a str object)");
}

TEST(StrBuiltinsDeathTest, StrStripWithInvalidCharsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
"test".strip(1)
)"),
               R"(str.strip\(\) arg must be None or str)");
}

TEST(StrBuiltinsDeathTest, StrLStripWithInvalidCharsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
"test".lstrip(1)
)"),
               R"(str.lstrip\(\) arg must be None or str)");
}

TEST(StrBuiltinsDeathTest, StrRStripWithInvalidCharsThrowsTypeError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
"test".rstrip(1)
)"),
               R"(str.rstrip\(\) arg must be None or str)");
}

TEST(StrBuiltinsTest, StrStripWithNoneArgStripsBoth) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, none));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "Hello World");
}

TEST(StrBuiltinsTest, StrLStripWithNoneArgStripsLeft) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, none));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "Hello World ");
}

TEST(StrBuiltinsTest, StrRStripWithNoneArgStripsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" Hello World "));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, none));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), " Hello World");
}

TEST(StrBuiltinsTest, StrStripWithoutArgsStripsBoth) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" \n\tHello World\n\t "));
  Object result(&scope, runBuiltin(StrBuiltins::strip, str));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "Hello World");
}

TEST(StrBuiltinsTest, StrLStripWithoutArgsStripsLeft) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" \n\tHello World\n\t "));
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "Hello World\n\t ");
}

TEST(StrBuiltinsTest, StrRStripWithoutArgsStripsRight) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr(" \n\tHello World\n\t "));
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), " \n\tHello World");
}

TEST(StrBuiltinsTest, StrStripWithCharsStripsChars) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::strip, str, chars));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "Hello World");
}

TEST(StrBuiltinsTest, StrLStripWithCharsStripsCharsToLeft) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::lstrip, str, chars));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "Hello Worldcab");
}

TEST(StrBuiltinsTest, StrRStripWithCharsStripsCharsToRight) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("bcaHello Worldcab"));
  Object chars(&scope, runtime.newStrFromCStr("abc"));
  Object result(&scope, runBuiltin(StrBuiltins::rstrip, str, chars));
  ASSERT_TRUE(str->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "bcaHello World");
}

TEST(StrBuiltinsTest, DunderIterReturnsStrIter) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, runtime.newStrFromCStr(""));
  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter->isStrIterator());
}

TEST(StrIteratorBuiltinsTest, CallDunderNextReadsCharactersSequentially) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("ab"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter->isStrIterator());

  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isStr());
  ASSERT_EQ(item1, runtime.newStrFromCStr("a"));

  Object item2(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2->isStr());
  ASSERT_EQ(item2, runtime.newStrFromCStr("b"));
}

TEST(StrIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, runtime.newStrFromCStr(""));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter->isStrIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(StrIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(StrIteratorBuiltinsTest, DunderLengthHintOnEmptyStrIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Str empty_str(&scope, runtime.newStrFromCStr(""));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, empty_str));
  ASSERT_TRUE(iter->isStrIterator());

  Object length_hint(&scope,
                     runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(StrIteratorBuiltinsTest,
     DunderLengthHintOnConsumedStrIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Str str(&scope, runtime.newStrFromCStr("a"));

  Object iter(&scope, runBuiltin(StrBuiltins::dunderIter, str));
  ASSERT_TRUE(iter->isStrIterator());

  Object length_hint1(&scope,
                      runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Object item1(&scope, runBuiltin(StrIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isStr());
  ASSERT_EQ(item1, runtime.newStrFromCStr("a"));

  Object length_hint2(&scope,
                      runBuiltin(StrIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint2->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 0);
}

}  // namespace python
