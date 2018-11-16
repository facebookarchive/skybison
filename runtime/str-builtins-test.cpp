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

TEST(StrBuiltinsTest, StringFormat) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<String> src(&scope, runtime.newStringFromCString("hello %d %g %s"));
  Handle<Object> arg0(&scope, SmallInt::fromWord(123));
  Handle<Object> arg1(&scope, runtime.newFloat(3.14));
  Handle<Object> arg2(&scope, runtime.newStringFromCString("pyros"));
  Handle<ObjectArray> objs(&scope, runtime.newObjectArray(3));
  objs->atPut(0, *arg0);
  objs->atPut(1, *arg1);
  objs->atPut(2, *arg2);
  Object* result = stringFormat(thread, src, objs);
  EXPECT_TRUE(String::cast(result)->equalsCString("hello 123 3.14 pyros"));
}

TEST(StrBuiltinsTest, StringFormat1) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<String> src(&scope, runtime.newStringFromCString("%s"));
  Handle<Object> arg(&scope, runtime.newStringFromCString("pyro"));
  Handle<ObjectArray> objs(&scope, runtime.newObjectArray(1));
  objs->atPut(0, *arg);
  Object* result = stringFormat(thread, src, objs);
  EXPECT_TRUE(String::cast(result)->equalsCString("pyro"));
}

TEST(StrBuiltinsTest, StringFormat2) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<String> src(&scope, runtime.newStringFromCString("%s%s"));
  Handle<Object> arg(&scope, runtime.newStringFromCString("pyro"));
  Handle<ObjectArray> objs(&scope, runtime.newObjectArray(2));
  for (word i = 0; i < objs->length(); i++) {
    objs->atPut(i, *arg);
  }
  Object* result = stringFormat(thread, src, objs);
  EXPECT_TRUE(String::cast(result)->equalsCString("pyropyro"));
}

TEST(StrBuiltinsTest, StringFormatMixed) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<String> src(&scope,
                     runtime.newStringFromCString("1%s,2%s,3%s,4%s,5%s"));
  Handle<Object> arg(&scope, runtime.newStringFromCString("pyro"));
  Handle<ObjectArray> objs(&scope, runtime.newObjectArray(5));
  for (word i = 0; i < objs->length(); i++) {
    objs->atPut(i, *arg);
  }
  Object* result = stringFormat(thread, src, objs);
  Handle<String> str(&scope, result);
  EXPECT_TRUE(str->equalsCString("1pyro,2pyro,3pyro,4pyro,5pyro"));
}

TEST(StrBuiltinsTest, StringFormatMixed2) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<String> src(&scope, runtime.newStringFromCString("%d%s,%d%s,%d%s"));
  Handle<Object> arg(&scope, runtime.newStringFromCString("pyro"));
  Handle<ObjectArray> objs(&scope, runtime.newObjectArray(6));
  for (word i = 0; i < objs->length(); i++) {
    if (i % 2 == 0) {
      objs->atPut(i, SmallInt::fromWord(i / 2 + 1));
    } else {
      objs->atPut(i, *arg);
    }
  }
  Object* result = stringFormat(thread, src, objs);
  Handle<String> str(&scope, result);
  EXPECT_TRUE(str->equalsCString("1pyro,2pyro,3pyro"));
}

TEST(StrBuiltinsDeathTest, StringFormatMalformed) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<String> src(&scope, runtime.newStringFromCString("%"));
  Handle<Object> arg(&scope, runtime.newStringFromCString("pyro"));
  Handle<ObjectArray> objs(&scope, runtime.newObjectArray(1));
  objs->atPut(0, *arg);
  EXPECT_DEATH(stringFormat(thread, src, objs), "");
}

TEST(StrBuiltinsDeathTest, StringFormatMismatch) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Handle<String> src(&scope, runtime.newStringFromCString("%d%s"));
  Handle<Object> arg(&scope, runtime.newStringFromCString("pyro"));
  Handle<ObjectArray> objs(&scope, runtime.newObjectArray(1));
  objs->atPut(0, *arg);
  EXPECT_DEATH(stringFormat(thread, src, objs), "Argument mismatch");
}

}  // namespace python
