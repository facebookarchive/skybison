#include "gtest/gtest.h"

#include "function-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
namespace testing {

TEST(FunctionBuiltinsTest, NativeFunctionObjectsExposeNoneDunderCode) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
code = len.__code__
)");
  HandleScope scope;
  Object code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code->isNoneType());
}

TEST(FunctionBuiltinsTest, ManagedFunctionObjectsExposeDunderCode) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo(x):
  return x + 1
code = foo.__code__
)");
  HandleScope scope;
  Object code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code->isCode());
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
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a_obj->isSmallInt());
  SmallInt a(&scope, *a_obj);
  ASSERT_EQ(a->value(), 10);
}

TEST(FunctionBuiltinsTest, DunderGetWithNonFunctionSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object none(&scope, NoneType::object());
  ASSERT_TRUE(
      runBuiltin(FunctionBuiltins::dunderGet, none, none, none).isError());
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue()->isStr());
}

TEST(FunctionBuiltinsTest, DunderGetWithNonNoneInstanceReturnsBoundMethod) {
  Runtime runtime;
  HandleScope scope;
  Object func(&scope, runtime.newFunction());
  Object not_none(&scope, SmallInt::fromWord(1));
  Object result(&scope, runBuiltin(FunctionBuiltins::dunderGet, func, not_none,
                                   not_none));
  EXPECT_TRUE(result->isBoundMethod());
}

TEST(FunctionBuiltinsTest,
     DunderGetWithNoneInstanceAndNoneTypeReturnsBoundMethod) {
  Runtime runtime;
  HandleScope scope;
  Object func(&scope, runtime.newFunction());
  Object none(&scope, NoneType::object());
  Type none_type(&scope, runtime.typeOf(none));
  Object result(&scope,
                runBuiltin(FunctionBuiltins::dunderGet, func, none, none_type));
  EXPECT_TRUE(result->isBoundMethod());
}

TEST(FunctionBuiltinsTest, DunderGetWithNoneInstanceReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Object func(&scope, runtime.newFunction());
  Object none(&scope, NoneType::object());
  Type some_type(&scope, runtime.typeOf(func));
  Object result(&scope,
                runBuiltin(FunctionBuiltins::dunderGet, func, none, some_type));
  EXPECT_EQ(result, func);
}

}  // namespace testing
}  // namespace python
