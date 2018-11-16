#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(CallTest, CallBoundMethod) {
  Runtime runtime;

  const char* src = R"(
def func(self):
  print(self)

def test(callable):
  return callable()
)";
  runtime.runFromCString(src);

  HandleScope scope;
  Handle<Module> module(&scope, findModule(&runtime, "__main__"));
  Handle<Object> function(&scope, findInModule(&runtime, module, "func"));
  ASSERT_TRUE(function->isFunction());

  Handle<Object> self(&scope, SmallInteger::fromWord(1111));
  Handle<BoundMethod> method(&scope, runtime.newBoundMethod(function, self));

  Handle<Object> test(&scope, findInModule(&runtime, module, "test"));
  ASSERT_TRUE(test->isFunction());
  Handle<Function> func(&scope, *test);

  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111\n");
}

TEST(CallTest, CallBoundMethodWithArgs) {
  Runtime runtime;

  const char* src = R"(
def func(self, a, b):
  print(self, a, b)

def test(callable):
  return callable(2222, 3333)
)";
  runtime.runFromCString(src);

  HandleScope scope;
  Handle<Module> module(&scope, findModule(&runtime, "__main__"));
  Handle<Object> function(&scope, findInModule(&runtime, module, "func"));
  ASSERT_TRUE(function->isFunction());

  Handle<Object> self(&scope, SmallInteger::fromWord(1111));
  Handle<BoundMethod> method(&scope, runtime.newBoundMethod(function, self));

  Handle<Object> test(&scope, findInModule(&runtime, module, "test"));
  ASSERT_TRUE(test->isFunction());
  Handle<Function> func(&scope, *test);

  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111 2222 3333\n");
}

TEST(CallTest, CallDefaultArgs) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
def foo(a=1, b=2, c=3):
  print(a, b, c)

print()
foo(33, 22, 11)
foo()
foo(1001)
foo(1001, 1002)
foo(1001, 1002, 1003)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, R"(
33 22 11
1 2 3
1001 2 3
1001 1002 3
1001 1002 1003
)");
}

TEST(CallTest, CallMethodMixPosDefaultArgs) {
  const char* src = R"(
def foo(a, b=2):
  print(a, b)
foo(1)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");
}

TEST(CallTest, CallBoundMethodMixed) {
  const char* src = R"(
class R:
  def __init__(self, a, b=2):
    print(a, b)
a = R(9)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "9 2\n");
}

TEST(CallTest, SingleKW) {
  Runtime runtime;
  const char* src = R"(
def foo(bar):
   print('bar =',bar)
foo(bar=2)
)";
  const char* expected = "bar = 2\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, MixedKW) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, b = 2, c = 3)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, FullKW) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(a = 1, b = 2, c = 3)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KWOutOfOrder1) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(c = 3, a = 1, b = 2)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KWOutOfOrder2) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, c = 3, b = 2)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KeyWordOnly1) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, c = 3);
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KeyWordOnly2) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, b = 2, c = 3);
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KeyWordDefaults) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b = 22, c = 33):
  print(a,b,c)
foo(11, c = 3);
)";
  const char* expected = "11 22 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, VarArgs1) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2,3,4,5,6);
)";
  const char* expected = "1 2 (3, 4, 5, 6)\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, VarArgs2) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2);
)";
  const char* expected = "1 2 ()\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KeyWordOnlyDeath) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 3);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallTest, MissingKeywordDeath) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "got 2 args, expected 3");
}

TEST(CallTest, ArgNameMismatchDeath) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, d = 2, c = 3);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallTest, TooManyKWArgsDeath) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 4, c = 3);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallTest, TooManyArgsDeath) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(1, 2, 3, 4);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallTest, TooFewArgsDeath) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(3, 4);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "got 2 args, expected 3");
}

} // namespace python
