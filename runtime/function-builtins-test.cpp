#include "function-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using FunctionBuiltinsTest = RuntimeFixture;

TEST_F(FunctionBuiltinsTest, ManagedFunctionObjectsExposeDunderCode) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(x):
  return x + 1
code = foo.__code__
)")
                   .isError());
  HandleScope scope(thread_);
  Object code(&scope, mainModuleAt(runtime_, "code"));
  ASSERT_TRUE(code.isCode());
}

TEST_F(FunctionBuiltinsTest,
       ChangingCodeOfFunctionObjectChangesFunctionBehavior) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def foo(x):
  return x + 1
def bar(x):
  return x + 5
foo.__code__ = bar.__code__
a = foo(5)
)"),
                            LayoutId::kAttributeError,
                            "'function.__code__' attribute is read-only"));
}

TEST_F(FunctionBuiltinsTest, DunderGetWithNonFunctionSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object none(&scope, NoneType::object());
  ASSERT_TRUE(runBuiltin(METH(function, __get__), none, none, none).isError());
  Thread* thread = Thread::current();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime_->typeAt(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue().isStr());
}

TEST_F(FunctionBuiltinsTest, DunderGetWithNonNoneInstanceReturnsBoundMethod) {
  HandleScope scope(thread_);
  Object func(&scope, newEmptyFunction());
  Object not_none(&scope, SmallInt::fromWord(1));
  Object not_none_type(&scope, runtime_->typeOf(*not_none));
  Object result(&scope, runBuiltin(METH(function, __get__), func, not_none,
                                   not_none_type));
  ASSERT_TRUE(result.isBoundMethod());
  EXPECT_EQ(BoundMethod::cast(*result).self(), *not_none);
  EXPECT_EQ(BoundMethod::cast(*result).function(), *func);
}

TEST_F(FunctionBuiltinsTest,
       DunderGetWithNoneInstanceAndNoneTypeReturnsBoundMethod) {
  HandleScope scope(thread_);
  Object func(&scope, newEmptyFunction());
  Object none(&scope, NoneType::object());
  Type none_type(&scope, runtime_->typeOf(*none));
  Object result(&scope,
                runBuiltin(METH(function, __get__), func, none, none_type));
  ASSERT_TRUE(result.isBoundMethod());
  EXPECT_EQ(BoundMethod::cast(*result).self(), *none);
  EXPECT_EQ(BoundMethod::cast(*result).function(), *func);
}

TEST_F(FunctionBuiltinsTest, DunderGetWithNoneInstanceReturnsSelf) {
  HandleScope scope(thread_);
  Object func(&scope, newEmptyFunction());
  Object none(&scope, NoneType::object());
  Type some_type(&scope, runtime_->typeOf(*func));
  Object result(&scope,
                runBuiltin(METH(function, __get__), func, none, some_type));
  EXPECT_EQ(result, func);
}

TEST_F(FunctionBuiltinsTest, ReprHandlesNormalFunctions) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(): pass
result = repr(f)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(Str::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function f at 0x"));
}

TEST_F(FunctionBuiltinsTest, ReprHandlesLambda) {
  ASSERT_FALSE(runFromCStr(runtime_, "result = repr(lambda x: x)").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(Str::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function <lambda> at 0x"));
}

TEST_F(FunctionBuiltinsTest, DunderCallCallsFunction) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(a):
  return a
result = f.__call__(3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(FunctionBuiltinsTest, DunderGlobalsIsModuleProxy) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(a):
  return a
result = f.__globals__
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isModuleProxy());
}

}  // namespace testing
}  // namespace py
