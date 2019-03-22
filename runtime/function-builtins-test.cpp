#include "gtest/gtest.h"

#include "function-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
namespace testing {

TEST(FunctionBuiltinsTest, ManagedFunctionObjectsExposeDunderCode) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(x):
  return x + 1
code = foo.__code__
)");
  HandleScope scope;
  Object code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code.isCode());
}

TEST(FunctionBuiltinsTest,
     ChangingCodeOfFunctionObjectChangesFunctionBehavior) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(x):
  return x + 1
def bar(x):
  return x + 5
foo.__code__ = bar.__code__
a = foo(5)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 10));
}

TEST(FunctionBuiltinsTest, DunderGetWithNonFunctionSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object none(&scope, NoneType::object());
  ASSERT_TRUE(
      runBuiltin(FunctionBuiltins::dunderGet, none, none, none).isError());
  Thread* thread = Thread::current();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue().isStr());
}

TEST(FunctionBuiltinsTest, DunderGetWithNonNoneInstanceReturnsBoundMethod) {
  Runtime runtime;
  HandleScope scope;
  Object func(&scope, runtime.newFunction());
  Object not_none(&scope, SmallInt::fromWord(1));
  Object not_none_type(&scope, runtime.typeOf(*not_none));
  Object result(&scope, runBuiltin(FunctionBuiltins::dunderGet, func, not_none,
                                   not_none_type));
  ASSERT_TRUE(result.isBoundMethod());
  EXPECT_EQ(RawBoundMethod::cast(*result).self(), *not_none);
  EXPECT_EQ(RawBoundMethod::cast(*result).function(), *func);
}

TEST(FunctionBuiltinsTest,
     DunderGetWithNoneInstanceAndNoneTypeReturnsBoundMethod) {
  Runtime runtime;
  HandleScope scope;
  Object func(&scope, runtime.newFunction());
  Object none(&scope, NoneType::object());
  Type none_type(&scope, runtime.typeOf(*none));
  Object result(&scope,
                runBuiltin(FunctionBuiltins::dunderGet, func, none, none_type));
  ASSERT_TRUE(result.isBoundMethod());
  EXPECT_EQ(RawBoundMethod::cast(*result).self(), *none);
  EXPECT_EQ(RawBoundMethod::cast(*result).function(), *func);
}

TEST(FunctionBuiltinsTest, DunderGetWithNoneInstanceReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Object func(&scope, runtime.newFunction());
  Object none(&scope, NoneType::object());
  Type some_type(&scope, runtime.typeOf(*func));
  Object result(&scope,
                runBuiltin(FunctionBuiltins::dunderGet, func, none, some_type));
  EXPECT_EQ(result, func);
}

TEST(FunctionBuiltinsTest, ReprHandlesNormalFunctions) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def f(): pass
result = repr(f)
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(RawStr::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function f at 0x"));
}

TEST(FunctionBuiltinsTest, ReprHandlesLambda) {
  Runtime runtime;
  runFromCStr(&runtime, "result = repr(lambda x: x)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(RawStr::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function <lambda> at 0x"));
}

TEST(FunctionBuiltinsTest, DunderCallCallsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def f(a):
  return a
result = f.__call__(3)
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

}  // namespace testing
}  // namespace python
