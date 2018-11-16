#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

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

  runtime.runFromCString(R"(
a_le_b = 'a' <= 'b'
b_le_a = 'a' >= 'b'
a_le_a = 'a' <= 'a'
)");

  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Bool::trueObj());

  Handle<Object> b_le_a(&scope, moduleAt(&runtime, main, "b_le_a"));
  EXPECT_EQ(*b_le_a, Bool::falseObj());

  Handle<Object> a_le_a(&scope, moduleAt(&runtime, main, "a_le_a"));
  EXPECT_EQ(*a_le_a, Bool::trueObj());
}

TEST(StrBuiltinsTest, LowerOnASCIILettersReturnsLowerCaseString) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "HELLO".lower()
b = "HeLLo".lower()
c = "hellO".lower()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));
  Handle<String> c(&scope, moduleAt(&runtime, main, "c"));
  EXPECT_PYSTRING_EQ(*a, "hello");
  EXPECT_PYSTRING_EQ(*b, "hello");
  EXPECT_PYSTRING_EQ(*c, "hello");
}

TEST(StrBuiltinsTest, LowerOnLowercaseASCIILettersReturnsSameString) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello".lower()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "hello");
}

TEST(StrBuiltinsTest, LowerOnNumbersReturnsSameString) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "foo 123".lower()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "foo 123");
}

TEST(StrBuiltinsTest, DunderNewCallsDunderStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
class Foo:
    def __str__(self):
        return "foo"
a = str.__new__(str, Foo())
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "foo");
}

TEST(StrBuiltinsTest, DunderNewCallsReprIfNoDunderStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
class Foo:
  pass
f = Foo()
a = str.__new__(str, f)
b = repr(f)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_PYSTRING_EQ(*a, *b);
}

TEST(StrBuiltinsTest, DunderNewWithNoArgsExceptTypeReturnsEmptyString) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = str.__new__(str)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "");
}

TEST(StrBuiltinsTest, DunderNewWithStrReturnsSameStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = str.__new__(str, "hello")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "hello");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNoArgsThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__()
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithTooManyArgsThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__(str, 1, 2, 3, 4)
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNonTypeArgThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__(1)
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNonSubtypeArgThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__(object)
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

TEST(StrBuiltinsTest, DunderAddWithTwoStringsReturnsConcatenatedString) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.newStringFromCString("hello"));
  frame->setLocal(1, runtime.newStringFromCString("world"));
  Object* str = builtinStringAdd(thread, frame, 2);
  ASSERT_TRUE(str->isString());
  EXPECT_PYSTRING_EQ(String::cast(str), "helloworld");
}

TEST(StrBuiltinsTest, DunderAddWithLeftEmptyAndReturnsRight) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.newStringFromCString(""));
  frame->setLocal(1, runtime.newStringFromCString("world"));
  Object* str = builtinStringAdd(thread, frame, 2);
  ASSERT_TRUE(str->isString());
  EXPECT_PYSTRING_EQ(String::cast(str), "world");
}

TEST(StrBuiltinsTest, DunderAddWithRightEmptyAndReturnsRight) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.newStringFromCString("hello"));
  frame->setLocal(1, runtime.newStringFromCString(""));
  Object* str = builtinStringAdd(thread, frame, 2);
  ASSERT_TRUE(str->isString());
  EXPECT_PYSTRING_EQ(String::cast(str), "hello");
}

TEST(StrBuiltinsTest, PlusOperatorOnStringsEqualsDunderAdd) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello"
b = "world"
c = a + b
d = a.__add__(b)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> c(&scope, moduleAt(&runtime, main, "c"));
  Handle<String> d(&scope, moduleAt(&runtime, main, "d"));

  EXPECT_PYSTRING_EQ(*c, "helloworld");
  EXPECT_PYSTRING_EQ(*d, "helloworld");
}

TEST(StrBuiltinsTest, StringLen) {
  Runtime runtime;
  runtime.runFromCString(R"(
l1 = len("aloha")
l2 = str.__len__("aloha")
l3 = "aloha".__len__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<SmallInt> l1(&scope, moduleAt(&runtime, main, "l1"));
  EXPECT_EQ(5, l1->value());
  Handle<SmallInt> l2(&scope, moduleAt(&runtime, main, "l2"));
  EXPECT_EQ(5, l2->value());
  Handle<SmallInt> l3(&scope, moduleAt(&runtime, main, "l3"));
  EXPECT_EQ(5, l3->value());
}

TEST(StrBuiltinsTest, StringLenWithEmptyString) {
  Runtime runtime;
  runtime.runFromCString(R"(
l = len("")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<SmallInt> l(&scope, moduleAt(&runtime, main, "l"));
  EXPECT_EQ(0, l->value());
}

TEST(StrBuiltinsDeathTest, StringLenWithInt) {
  Runtime runtime;
  const char* code = R"(
l = str.__len__(3)
)";
  EXPECT_DEATH(runtime.runFromCString(code),
               "descriptor '__len__' requires a 'str' object");
}

TEST(StrBuiltinsDeathTest, StringLenWithExtraArgument) {
  Runtime runtime;
  const char* code = R"(
l = "aloha".__len__("arg")
)";
  EXPECT_DEATH(runtime.runFromCString(code), "expected 0 arguments");
}

TEST(StrBuiltinsTest, IndexWithSliceWithPositiveInts) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello"
b = a[1:2]
c = a[1:4]
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));
  Handle<String> c(&scope, moduleAt(&runtime, main, "c"));

  EXPECT_PYSTRING_EQ(*a, "hello");
  EXPECT_PYSTRING_EQ(*b, "e");
  EXPECT_PYSTRING_EQ(*c, "ell");
}

TEST(StrBuiltinsTest, IndexWithSliceWithNegativeInts) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello"
b = a[-1:]
c = a[1:-2]
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));
  Handle<String> c(&scope, moduleAt(&runtime, main, "c"));

  EXPECT_PYSTRING_EQ(*a, "hello");
  EXPECT_PYSTRING_EQ(*b, "o");
  EXPECT_PYSTRING_EQ(*c, "el");
}

TEST(StrBuiltinsTest, IndexWithSliceWithStep) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello"
b = a[0:5:2]
c = a[1:5:3]
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));
  Handle<String> c(&scope, moduleAt(&runtime, main, "c"));

  EXPECT_PYSTRING_EQ(*a, "hello");
  EXPECT_PYSTRING_EQ(*b, "hlo");
  EXPECT_PYSTRING_EQ(*c, "eo");
}

TEST(StrBuiltinsTest, StringFormat) {
  Runtime runtime;
  runtime.runFromCString(R"(
n = 123
f = 3.14
s = "pyros"
a = "hello %d %g %s" % (n, f, s)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "hello 123 3.14 pyros");
}

TEST(StrBuiltinsTest, StringFormatSingleString) {
  Runtime runtime;
  runtime.runFromCString(R"(
s = "pyro"
a = "%s" % s
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "pyro");
}

TEST(StrBuiltinsTest, StringFormatTwoStrings) {
  Runtime runtime;
  runtime.runFromCString(R"(
s = "pyro"
a = "%s%s" % (s, s)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "pyropyro");
}

TEST(StrBuiltinsTest, StringFormatMixed) {
  Runtime runtime;
  runtime.runFromCString(R"(
s = "pyro"
a = "1%s,2%s,3%s,4%s,5%s" % (s, s, s, s, s)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "1pyro,2pyro,3pyro,4pyro,5pyro");
}

TEST(StrBuiltinsTest, StringFormatMixed2) {
  Runtime runtime;
  runtime.runFromCString(R"(
s = "pyro"
a = "%d%s,%d%s,%d%s" % (1, s, 2, s, 3, s)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "1pyro,2pyro,3pyro");
}

TEST(StrBuiltinsDeathTest, StringFormatMalformed) {
  Runtime runtime;
  const char* src = R"(
a = "%" % ("pyro",)
)";
  EXPECT_DEATH(runtime.runFromCString(src), "Incomplete format");
}

TEST(StrBuiltinsDeathTest, StringFormatMismatch) {
  Runtime runtime;
  const char* src = R"(
a = "%d%s" % ("pyro",)
)";
  EXPECT_DEATH(runtime.runFromCString(src), "Argument mismatch");
}

TEST(StrBuiltinsTest, DunderReprOnASCIIStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello".__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "'hello'");
}

TEST(StrBuiltinsTest, DunderReprOnASCIINonPrintable) {
  Runtime runtime;
  // 6 is the ACK character.
  runtime.runFromCString(R"(
a = "\x06".__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "'\\x06'");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithDoubleQuotes) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = 'hello "world"'.__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "'hello \"world\"'");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithSingleQuotes) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello 'world'".__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "\"hello 'world'\"");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithBothQuotes) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello 'world', I am your \"father\"".__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, R"('hello \'world\', I am your "father"')");
}

TEST(StrBuiltinsTest, DunderReprOnStrWithNestedQuotes) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello 'world, \"I am 'your \"father\"'\"'".__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, R"('hello \'world, "I am \'your "father"\'"\'')");
}

TEST(StrBuiltinsTest, DunderReprOnCommonEscapeSequences) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "\n \t \r \\".__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "'\\n \\t \\r \\\\'");
}

TEST(StrBuiltinsTest, PartitionOnSingleCharStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello".partition("l")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(a->at(0)), "he");
  EXPECT_PYSTRING_EQ(String::cast(a->at(1)), "l");
  EXPECT_PYSTRING_EQ(String::cast(a->at(2)), "lo");
}

TEST(StrBuiltinsTest, PartitionOnMultiCharStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello".partition("ll")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(a->at(0)), "he");
  EXPECT_PYSTRING_EQ(String::cast(a->at(1)), "ll");
  EXPECT_PYSTRING_EQ(String::cast(a->at(2)), "o");
}

TEST(StrBuiltinsTest, PartitionOnSuffix) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello".partition("lo")
b = "hello".partition("lop")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<ObjectArray> b(&scope, moduleAt(&runtime, main, "b"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(a->at(0)), "hel");
  EXPECT_PYSTRING_EQ(String::cast(a->at(1)), "lo");
  EXPECT_PYSTRING_EQ(String::cast(a->at(2)), "");

  ASSERT_EQ(b->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(b->at(0)), "hello");
  EXPECT_PYSTRING_EQ(String::cast(b->at(1)), "");
  EXPECT_PYSTRING_EQ(String::cast(b->at(2)), "");
}

TEST(StrBuiltinsTest, PartitionOnPrefix) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello".partition("he")
b = "hello".partition("hex")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<ObjectArray> b(&scope, moduleAt(&runtime, main, "b"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(a->at(0)), "");
  EXPECT_PYSTRING_EQ(String::cast(a->at(1)), "he");
  EXPECT_PYSTRING_EQ(String::cast(a->at(2)), "llo");

  ASSERT_EQ(b->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(b->at(0)), "hello");
  EXPECT_PYSTRING_EQ(String::cast(b->at(1)), "");
  EXPECT_PYSTRING_EQ(String::cast(b->at(2)), "");
}

TEST(StrBuiltinsTest, PartitionLargerStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "hello".partition("abcdefghijk")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(a->at(0)), "hello");
  EXPECT_PYSTRING_EQ(String::cast(a->at(1)), "");
  EXPECT_PYSTRING_EQ(String::cast(a->at(2)), "");
}

TEST(StrBuiltinsTest, PartitionEmptyStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = "".partition("a")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_PYSTRING_EQ(String::cast(a->at(0)), "");
  EXPECT_PYSTRING_EQ(String::cast(a->at(1)), "");
  EXPECT_PYSTRING_EQ(String::cast(a->at(2)), "");
}

}  // namespace python
