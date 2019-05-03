#include "gtest/gtest.h"

#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ModuleBuiltinsTest, DunderName) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys
a = sys.__name__
)")
                   .isError());
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a.equalsCStr("sys"));
}

TEST(ModuleBuiltinsDeathTest, DirectModuleAccessRaisesNameError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "module.__new__()"),
                            LayoutId::kNameError,
                            "name 'module' is not defined"));
}

TEST(ModuleBuiltinsTest, DunderDirReturnsList) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import builtins
list = dir(builtins)
)")
                   .isError());
  Object list(&scope, moduleAt(&runtime, "__main__", "list"));
  Object ord(&scope, runtime.newStrFromCStr("ord"));
  EXPECT_TRUE(listContains(list, ord));
  Object dunder_name(&scope, runtime.newStrFromCStr("__name__"));
  EXPECT_TRUE(listContains(list, dunder_name));
}

TEST(ModuleBuiltinsTest, DunderGetattributeReturnsAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "foo = -6").isError());
  Module module(&scope, runtime.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(ModuleBuiltins::dunderGetattribute, module, name), -6));
}

TEST(ModuleBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "").isError());
  Module module(&scope, runtime.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ModuleBuiltins::dunderGetattribute, module, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST(ModuleBuiltinsTest,
     DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "").isError());
  Module module(&scope, runtime.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ModuleBuiltins::dunderGetattribute, module, name),
      LayoutId::kAttributeError, "module '__main__' has no attribute 'xxx'"));
}

TEST(ModuleBuiltinsDeathTest, DunderNewNotEnoughArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "import sys; type(sys).__new__()"),
      LayoutId::kTypeError,
      "TypeError: 'module.__new__' takes 2 positional arguments but 0 given"));
}

TEST(ModuleBuiltinsDeathTest, DunderNewTooManyArgumentsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "import sys; type(sys).__new__(123, 456, 789)"),
      LayoutId::kTypeError,
      "TypeError: 'module.__new__' takes max 2 positional arguments but 3 "
      "given"));
}

TEST(ModuleBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int nontype(&scope, SmallInt::fromWord(123));
  Str module_name(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, nontype, module_name));
  EXPECT_TRUE(
      raisedWithStr(*module, LayoutId::kTypeError, "not a type object"));
}

TEST(ModuleBuiltinsTest, DunderNewWithNonModuleRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Type nonmodule(&scope, runtime.typeAt(LayoutId::kSmallStr));
  Str module_name(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, nonmodule, module_name));
  EXPECT_TRUE(
      raisedWithStr(*module, LayoutId::kTypeError, "not a subtype of module"));
}

TEST(ModuleBuiltinsTest, DunderNewReturnsModule) {
  Runtime runtime;
  HandleScope scope;
  Type moduletype(&scope, runtime.typeAt(LayoutId::kModule));
  Str module_name(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, moduletype, module_name));
  ASSERT_TRUE(module.isModule());
  Object result_module_name(&scope, RawModule::cast(*module).name());
  EXPECT_TRUE(isStrEquals(result_module_name, module_name));
}

TEST(ModuleBuiltinsTest, DunderNewWithNonStrNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Type moduletype(&scope, runtime.typeAt(LayoutId::kModule));
  Int non_str_name(&scope, SmallInt::fromWord(123));
  Object module(
      &scope, runBuiltin(ModuleBuiltins::dunderNew, moduletype, non_str_name));
  EXPECT_TRUE(raised(*module, LayoutId::kTypeError));
}

TEST(ModuleBuiltinsTest, DunderSetattrSetsAttribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Module module(&scope, runtime.newModule(module_name));
  Object name(&scope, runtime.newStrFromCStr("foobarbaz"));
  Object value(&scope, runtime.newInt(0xf00d));
  EXPECT_TRUE(runBuiltin(ModuleBuiltins::dunderSetattr, module, name, value)
                  .isNoneType());
  EXPECT_TRUE(isIntEqualsWord(runtime.moduleAt(module, name), 0xf00d));
}

TEST(ModuleBuiltinsTest, DunderSetattrWithNonStrNameRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Object module(&scope, runtime.newModule(module_name));
  Object name(&scope, runtime.newFloat(4.4));
  Object value(&scope, runtime.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ModuleBuiltins::dunderSetattr, module, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'float'"));
}

TEST(ModuleBuiltinsTest, DunderDict) {
  Runtime runtime;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys
result = sys.__dict__
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isDict());
}

TEST(ModuleBuiltinsTest, ModuleGetAttributeReturnsInstanceValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "x = 42").isError());
  Module module(&scope, runtime.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(moduleGetAttribute(thread, module, name), 42));
}

TEST(ModuleBuiltinsTest, ModuleGetAttributeWithNonExistentNameReturnsError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_name(&scope, runtime.newStrFromCStr(""));
  Module module(&scope, runtime.newModule(module_name));
  Object name(&scope, runtime.newStrFromCStr("xxx"));
  EXPECT_TRUE(moduleGetAttribute(thread, module, name).isError());
  EXPECT_FALSE(thread->hasPendingException());
}

TEST(ModuleBuiltinsTest, ModuleSetAttrSetsAttribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Module module(&scope, runtime.newModule(module_name));
  Object name(&scope, runtime.internStrFromCStr("bar"));
  Object value(&scope, runtime.newInt(-543));
  EXPECT_TRUE(moduleSetAttr(thread, module, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(runtime.moduleAt(module, name), -543));
}

TEST(ModuleBuiltinsTest, NewModuleDunderReprReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object name(&scope, runtime.newStrFromCStr("hello"));
  Object module(&scope, runtime.newModule(name));
  Object result(
      &scope, Thread::current()->invokeMethod1(module, SymbolId::kDunderRepr));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<module 'hello'>"));
}

TEST(ModuleBuiltinsTest, BuiltinModuleDunderReprReturnsString) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys
result = sys.__repr__()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<module 'sys' (built-in)>"));
}

}  // namespace python
