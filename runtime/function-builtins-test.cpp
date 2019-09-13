#include "gtest/gtest.h"

#include "function-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {
namespace testing {

using FunctionBuiltinsTest = RuntimeFixture;

TEST_F(FunctionBuiltinsTest, ManagedFunctionObjectsExposeDunderCode) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(x):
  return x + 1
code = foo.__code__
)")
                   .isError());
  HandleScope scope(thread_);
  Object code(&scope, mainModuleAt(&runtime_, "code"));
  ASSERT_TRUE(code.isCode());
}

TEST_F(FunctionBuiltinsTest,
       ChangingCodeOfFunctionObjectChangesFunctionBehavior) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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
  ASSERT_TRUE(
      runBuiltin(FunctionBuiltins::dunderGet, none, none, none).isError());
  Thread* thread = Thread::current();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime_.typeAt(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue().isStr());
}

TEST_F(FunctionBuiltinsTest, DunderGetWithNonNoneInstanceReturnsBoundMethod) {
  HandleScope scope(thread_);
  Object func(&scope, newEmptyFunction());
  Object not_none(&scope, SmallInt::fromWord(1));
  Object not_none_type(&scope, runtime_.typeOf(*not_none));
  Object result(&scope, runBuiltin(FunctionBuiltins::dunderGet, func, not_none,
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
  Type none_type(&scope, runtime_.typeOf(*none));
  Object result(&scope,
                runBuiltin(FunctionBuiltins::dunderGet, func, none, none_type));
  ASSERT_TRUE(result.isBoundMethod());
  EXPECT_EQ(BoundMethod::cast(*result).self(), *none);
  EXPECT_EQ(BoundMethod::cast(*result).function(), *func);
}

TEST_F(FunctionBuiltinsTest, DunderGetWithNoneInstanceReturnsSelf) {
  HandleScope scope(thread_);
  Object func(&scope, newEmptyFunction());
  Object none(&scope, NoneType::object());
  Type some_type(&scope, runtime_.typeOf(*func));
  Object result(&scope,
                runBuiltin(FunctionBuiltins::dunderGet, func, none, some_type));
  EXPECT_EQ(result, func);
}

TEST_F(FunctionBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(): pass
foo.bar = -4
)")
                   .isError());
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  Object name(&scope, runtime_.newStrFromCStr("bar"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(FunctionBuiltins::dunderGetattribute, foo, name), -4));
}

TEST_F(FunctionBuiltinsTest,
       DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "def foo(): pass").isError());
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  Object name(&scope, runtime_.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FunctionBuiltins::dunderGetattribute, foo, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(FunctionBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "def foo(): pass").isError());
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FunctionBuiltins::dunderGetattribute, foo, name),
      LayoutId::kAttributeError, "function 'foo' has no attribute 'xxx'"));
}

TEST_F(FunctionBuiltinsTest, DunderSetattrSetsAttribute) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime_, "def foo(): pass").isError());
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  Str name(&scope, runtime_.newStrFromCStr("foobarbaz"));
  Object value(&scope, runtime_.newInt(1337));
  EXPECT_TRUE(runBuiltin(FunctionBuiltins::dunderSetattr, foo, name, value)
                  .isNoneType());
  ASSERT_TRUE(foo.isFunction());
  ASSERT_TRUE(Function::cast(*foo).dict().isDict());
  Dict function_dict(&scope, Function::cast(*foo).dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime_.dictAtByStr(thread, function_dict, name), 1337));
}

TEST_F(FunctionBuiltinsTest, DunderSetattrWithNonStringNameRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "def foo(): pass").isError());
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  Object name(&scope, runtime_.newInt(0));
  Object value(&scope, runtime_.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FunctionBuiltins::dunderSetattr, foo, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(FunctionBuiltinsTest, ReprHandlesNormalFunctions) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def f(): pass
result = repr(f)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(Str::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function f at 0x"));
}

TEST_F(FunctionBuiltinsTest, ReprHandlesLambda) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = repr(lambda x: x)").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(Str::cast(*result).toCStr());
  EXPECT_TRUE(std::strstr(result_str.get(), "<function <lambda> at 0x"));
}

TEST_F(FunctionBuiltinsTest, DunderCallCallsFunction) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def f(a):
  return a
result = f.__call__(3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(FunctionBuiltinsTest, DunderGlobalsIsDict) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def f(a):
  return a
result = f.__globals__
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result.isDict());
}

// TODO(T48823386): Update this test once we pass ModuleProxy in
// function.__globals__.
TEST_F(FunctionBuiltinsTest, FunctionGlobalsIsEqualToModuleProxyContainedDict) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
function_globals = sys.exit.__globals__
module_proxy = sys.__dict__
)")
                   .isError());
  HandleScope scope(thread_);
  Object function_globals(&scope, mainModuleAt(&runtime_, "function_globals"));
  ModuleProxy module_proxy(&scope, mainModuleAt(&runtime_, "module_proxy"));
  Object module_dict(&scope, Module::cast(module_proxy.module()).dict());
  EXPECT_EQ(*function_globals, *module_dict);
}

TEST_F(FunctionBuiltinsTest, FunctionSetAttrSetsAttribute) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime_, "def foo(): pass").isError());
  Object foo_obj(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(foo_obj.isFunction());
  Function foo(&scope, *foo_obj);
  Str name(&scope, runtime_.internStrFromCStr(thread, "bar"));
  Object name_hash(&scope, strHash(thread_, *name));
  Object value(&scope, runtime_.newInt(6789));
  EXPECT_TRUE(
      functionSetAttr(thread, foo, name, name_hash, value).isNoneType());
  ASSERT_TRUE(foo.dict().isDict());
  Dict function_dict(&scope, foo.dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime_.dictAtByStr(thread, function_dict, name), 6789));
}

}  // namespace testing
}  // namespace python
