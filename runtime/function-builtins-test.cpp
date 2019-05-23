#include "gtest/gtest.h"

#include "function-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
namespace testing {

TEST(FunctionBuiltinsTest, ManagedFunctionObjectsExposeDunderCode) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(x):
  return x + 1
code = foo.__code__
)")
                   .isError());
  HandleScope scope;
  Object code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code.isCode());
}

TEST(FunctionBuiltinsTest,
     ChangingCodeOfFunctionObjectChangesFunctionBehavior) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
def foo(x):
  return x + 1
def bar(x):
  return x + 5
foo.__code__ = bar.__code__
a = foo(5)
)"),
                            LayoutId::kAttributeError,
                            "'__code__' attribute is read-only"));
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
  EXPECT_EQ(BoundMethod::cast(*result).self(), *not_none);
  EXPECT_EQ(BoundMethod::cast(*result).function(), *func);
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
  EXPECT_EQ(BoundMethod::cast(*result).self(), *none);
  EXPECT_EQ(BoundMethod::cast(*result).function(), *func);
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

TEST(FunctionBuiltinsTest, DunderGetattributeReturnsAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(): pass
foo.bar = -4
)")
                   .isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object name(&scope, runtime.newStrFromCStr("bar"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FunctionBuiltins::dunderGetattribute, foo, name), -4));
}

TEST(FunctionBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "def foo(): pass").isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object name(&scope, runtime.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FunctionBuiltins::dunderGetattribute, foo, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST(FunctionBuiltinsTest,
     DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "def foo(): pass").isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object name(&scope, runtime.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FunctionBuiltins::dunderGetattribute, foo, name),
      LayoutId::kAttributeError, "function 'foo' has no attribute 'xxx'"));
}

TEST(FunctionBuiltinsTest, DunderSetattrSetsAttribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "def foo(): pass").isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object name(&scope, runtime.newStrFromCStr("foobarbaz"));
  Object value(&scope, runtime.newInt(1337));
  EXPECT_TRUE(runBuiltin(FunctionBuiltins::dunderSetattr, foo, name, value)
                  .isNoneType());
  ASSERT_TRUE(foo.isFunction());
  ASSERT_TRUE(Function::cast(*foo).dict().isDict());
  Dict function_dict(&scope, Function::cast(*foo).dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime.dictAt(thread, function_dict, name), 1337));
}

TEST(FunctionBuiltinsTest, DunderSetattrWithNonStringNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "def foo(): pass").isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object name(&scope, runtime.newInt(0));
  Object value(&scope, runtime.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FunctionBuiltins::dunderSetattr, foo, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST(FunctionBuiltinsTest, ReprHandlesNormalFunctions) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(): pass
result = repr(f)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(Str::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function f at 0x"));
}

TEST(FunctionBuiltinsTest, ReprHandlesLambda) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "result = repr(lambda x: x)").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(Str::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function <lambda> at 0x"));
}

TEST(FunctionBuiltinsTest, DunderCallCallsFunction) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(a):
  return a
result = f.__call__(3)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST(FunctionBuiltinsTest, DunderGlobalsIsDict) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(a):
  return a
result = f.__globals__
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isDict());
}

TEST(FunctionBuiltinsTest, FunctionGlobalsIsEqualToModuleDict) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys
function_globals = sys.exit.__globals__
module_dict = sys.__dict__
)")
                   .isError());
  HandleScope scope;
  Object function_globals(&scope,
                          moduleAt(&runtime, "__main__", "function_globals"));
  Object module_dict(&scope, moduleAt(&runtime, "__main__", "module_dict"));
  EXPECT_EQ(*function_globals, *module_dict);
}

TEST(FunctionBuiltinsTest, FunctionSetAttrSetsAttribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "def foo(): pass").isError());
  Object foo_obj(&scope, moduleAt(&runtime, "__main__", "foo"));
  ASSERT_TRUE(foo_obj.isFunction());
  Function foo(&scope, *foo_obj);
  Object name(&scope, runtime.internStrFromCStr(thread, "bar"));
  Object value(&scope, runtime.newInt(6789));
  EXPECT_TRUE(functionSetAttr(thread, foo, name, value).isNoneType());
  ASSERT_TRUE(foo.dict().isDict());
  Dict function_dict(&scope, foo.dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime.dictAt(thread, function_dict, name), 6789));
}

}  // namespace testing
}  // namespace python
