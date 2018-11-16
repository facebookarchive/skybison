#include "gtest/gtest.h"

#include "builtins-module.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BuiltinsModuleTest, BuiltinChr) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(chr(65))");
  EXPECT_EQ(result, "A\n");
  ASSERT_DEATH(
      runtime.runFromCString("print(chr(1,2))"),
      "aborting due to pending exception: Unexpected 1 argumment in 'chr'");
  ASSERT_DEATH(
      runtime.runFromCString("print(chr('A'))"),
      "aborting due to pending exception: Unsupported type in builtin 'chr'");
}

TEST(BuiltinsModuleTest, BuiltinInt) { // pystone dependency
  const char* src = R"(
a = int("123")
b = int("-987")
print(a == 123, b == -987, a > b, a, b)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True True True 123 -987\n");
}

TEST(BuiltinsModuleTest, BuiltinIsinstance) {
  Runtime runtime;
  HandleScope scope;

  // Only accepts 2 arguments
  EXPECT_DEATH(
      runtime.runFromCString("print(isinstance(1, 1, 1))"),
      "aborting due to pending exception: isinstance expected 2 arguments");

  // 2nd argument must be a type
  EXPECT_DEATH(
      runtime.runFromCString("print(isinstance(1, 1))"),
      "aborting due to pending exception: isinstance arg 2 must be a type");

  const char* src = R"(
class A: pass
class B(A): pass
class C(A): pass
class D(C, B): pass

def test(a, b):
  print(isinstance(a, b))
)";
  runtime.runFromCString(src);

  // We can move these tests into the python code above once we can
  // call classes.
  Object* object = testing::findModule(&runtime, "__main__");
  ASSERT_TRUE(object->isModule());
  Handle<Module> main(&scope, object);

  // Create an instance of D
  Handle<Object> klass_d(&scope, findInModule(&runtime, main, "D"));
  ASSERT_TRUE(klass_d->isClass());
  Handle<Layout> layout(&scope, Class::cast(*klass_d)->instanceLayout());
  Handle<Object> instance(&scope, runtime.newInstance(layout));

  // Fetch the test function
  object = findInModule(&runtime, main, "test");
  ASSERT_TRUE(object->isFunction());
  Handle<Function> isinstance(&scope, object);

  // isinstance(1, D) should be false
  Handle<ObjectArray> args(&scope, runtime.newObjectArray(2));
  args->atPut(0, SmallInteger::fromWord(100));
  args->atPut(1, *klass_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D, D) should be false
  args->atPut(0, *klass_d);
  args->atPut(1, *klass_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D(), D) should be true
  args->atPut(0, *instance);
  args->atPut(1, *klass_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), C) should be true
  Handle<Object> klass_c(&scope, findInModule(&runtime, main, "C"));
  ASSERT_TRUE(klass_c->isClass());
  args->atPut(1, *klass_c);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), B) should be true
  Handle<Object> klass_b(&scope, findInModule(&runtime, main, "B"));
  ASSERT_TRUE(klass_b->isClass());
  args->atPut(1, *klass_b);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(C(), A) should be true
  Handle<Object> klass_a(&scope, findInModule(&runtime, main, "A"));
  ASSERT_TRUE(klass_a->isClass());
  args->atPut(1, *klass_a);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");
}

TEST(BuiltinsModuleTest, BuiltinLen) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(len([1,2,3]))");
  EXPECT_EQ(result, "3\n");
  ASSERT_DEATH(
      runtime.runFromCString("print(len(1,2))"),
      "aborting due to pending exception: "
      "len\\(\\) takes exactly one argument");
  ASSERT_DEATH(
      runtime.runFromCString("print(len(1))"),
      "aborting due to pending exception: "
      "object has no len()");
}

TEST(ThreadTest, BuiltinLenGetLenFromDict) {
  Runtime runtime;

  runtime.runFromCString(R"(
len0 = len({})
len1 = len({'one': 1})
len5 = len({'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5})
)");

  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> len0(&scope, moduleAt(&runtime, main, "len0"));
  EXPECT_EQ(*len0, SmallInteger::fromWord(0));
  Handle<Object> len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInteger::fromWord(1));
  Handle<Object> len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInteger::fromWord(5));
}

TEST(ThreadTest, BuiltinLenGetLenFromList) {
  Runtime runtime;

  runtime.runFromCString(R"(
len0 = len([])
len1 = len([1])
len5 = len([1,2,3,4,5])
)");

  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> len0(&scope, moduleAt(&runtime, main, "len0"));
  EXPECT_EQ(*len0, SmallInteger::fromWord(0));
  Handle<Object> len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInteger::fromWord(1));
  Handle<Object> len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInteger::fromWord(5));
}

TEST(ThreadTest, BuiltinLenGetLenFromSet) {
  Runtime runtime;

  runtime.runFromCString(R"(
len1 = len({1})
len5 = len({1,2,3,4,5})
)");

  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  // TODO(cshapiro): test the empty set when we have builtins.set defined.
  Handle<Object> len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInteger::fromWord(1));
  Handle<Object> len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInteger::fromWord(5));
}

TEST(BuiltinsModuleTest, BuiltinOrd) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(ord('A'))");
  EXPECT_EQ(result, "65\n");
  ASSERT_DEATH(
      runtime.runFromCString("print(ord(1,2))"),
      "aborting due to pending exception: Unexpected 1 argumment in 'ord'");
  ASSERT_DEATH(
      runtime.runFromCString("print(ord(1))"),
      "aborting due to pending exception: Unsupported type in builtin 'ord'");
}

TEST(BuiltinsModuleTest, BuiltInPrintStdOut) {
  const char* src = R"(
import sys
print("hello", file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello\n");
}

TEST(BuiltinsModuleTest, BuiltInPrintEnd) {
  const char* src = R"(
import sys
print("hi", end='ho')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(BuiltinsModuleTest, BuiltInPrintStdOutEnd) {
  const char* src = R"(
import sys
print("hi", end='ho', file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(BuiltinsModuleTest, BuiltInPrintStdErr) { // pystone dependency
  const char* src = R"(
import sys
print("hi", file=sys.stderr, end='ya')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiya");
}

} // namespace python
