#include "gtest/gtest.h"

#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using ModuleBuiltinsDeathTest = RuntimeFixture;
using ModuleBuiltinsTest = RuntimeFixture;

TEST_F(ModuleBuiltinsTest, DunderName) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
a = sys.__name__
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(a.equalsCStr("sys"));
}

TEST_F(ModuleBuiltinsTest, DunderNameOverwritesDoesNotAffectModuleNameField) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
sys.__name__ = "ysy"
)")
                   .isError());
  HandleScope scope(thread_);
  Module sys_module(&scope, moduleAt(&runtime_, "__main__", "sys"));
  EXPECT_TRUE(isStrEqualsCStr(sys_module.name(), "sys"));
}

// TODO(T39575976): Add tests verifying internal names's hiding.
TEST_F(ModuleBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "foo = -6").isError());
  Module module(&scope, runtime_.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(ModuleBuiltins::dunderGetattribute, module, name), -6));
}

TEST_F(ModuleBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "").isError());
  Module module(&scope, runtime_.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime_.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ModuleBuiltins::dunderGetattribute, module, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(ModuleBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "").isError());
  Module module(&scope, runtime_.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ModuleBuiltins::dunderGetattribute, module, name),
      LayoutId::kAttributeError, "module '__main__' has no attribute 'xxx'"));
}

TEST_F(ModuleBuiltinsDeathTest, DunderNewNotEnoughArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "import sys; type(sys).__new__()"),
      LayoutId::kTypeError,
      "TypeError: 'module.__new__' takes 2 positional arguments but 0 given"));
}

TEST_F(ModuleBuiltinsDeathTest, DunderNewTooManyArgumentsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "import sys; type(sys).__new__(123, 456, 789)"),
      LayoutId::kTypeError,
      "TypeError: 'module.__new__' takes max 2 positional arguments but 3 "
      "given"));
}

TEST_F(ModuleBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Int nontype(&scope, SmallInt::fromWord(123));
  Str module_name(&scope, runtime_.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, nontype, module_name));
  EXPECT_TRUE(
      raisedWithStr(*module, LayoutId::kTypeError, "not a type object"));
}

TEST_F(ModuleBuiltinsTest, DunderNewWithNonModuleRaisesTypeError) {
  HandleScope scope(thread_);
  Type nonmodule(&scope, runtime_.typeAt(LayoutId::kSmallStr));
  Str module_name(&scope, runtime_.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, nonmodule, module_name));
  EXPECT_TRUE(
      raisedWithStr(*module, LayoutId::kTypeError, "not a subtype of module"));
}

TEST_F(ModuleBuiltinsTest, DunderNewReturnsModule) {
  HandleScope scope(thread_);
  Type moduletype(&scope, runtime_.typeAt(LayoutId::kModule));
  Str module_name(&scope, runtime_.newStrFromCStr("foo"));
  Object module(&scope,
                runBuiltin(ModuleBuiltins::dunderNew, moduletype, module_name));
  ASSERT_TRUE(module.isModule());
  Object result_module_name(&scope, Module::cast(*module).name());
  EXPECT_TRUE(isStrEquals(result_module_name, module_name));
}

TEST_F(ModuleBuiltinsTest, DunderNewWithNonStrNameRaisesTypeError) {
  HandleScope scope(thread_);
  Type moduletype(&scope, runtime_.typeAt(LayoutId::kModule));
  Int non_str_name(&scope, SmallInt::fromWord(123));
  Object module(
      &scope, runBuiltin(ModuleBuiltins::dunderNew, moduletype, non_str_name));
  EXPECT_TRUE(raised(*module, LayoutId::kTypeError));
}

TEST_F(ModuleBuiltinsTest, DunderSetattrSetsAttribute) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_.newStrFromCStr("foo"));
  Module module(&scope, runtime_.newModule(module_name));
  Object name(&scope, runtime_.newStrFromCStr("foobarbaz"));
  Object value(&scope, runtime_.newInt(0xf00d));
  EXPECT_TRUE(runBuiltin(ModuleBuiltins::dunderSetattr, module, name, value)
                  .isNoneType());
  EXPECT_TRUE(isIntEqualsWord(runtime_.moduleAt(module, name), 0xf00d));
}

TEST_F(ModuleBuiltinsTest, DunderSetattrWithNonStrNameRaisesTypeError) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_.newStrFromCStr("foo"));
  Object module(&scope, runtime_.newModule(module_name));
  Object name(&scope, runtime_.newFloat(4.4));
  Object value(&scope, runtime_.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(ModuleBuiltins::dunderSetattr, module, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'float'"));
}

TEST_F(ModuleBuiltinsTest, DunderDict) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
result = sys.__dict__
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(result.isDict());
}

TEST_F(ModuleBuiltinsTest, ModuleDictKeysFiltersOutPlaceholders) {
  HandleScope scope(thread_);
  Dict module_dict(&scope, runtime_.newDict());

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str bar(&scope, runtime_.newStrFromCStr("bar"));
  Str baz(&scope, runtime_.newStrFromCStr("baz"));
  Str value(&scope, runtime_.newStrFromCStr("value"));

  runtime_.moduleDictAtPut(thread_, module_dict, foo, value);
  runtime_.moduleDictAtPut(thread_, module_dict, bar, value);
  runtime_.moduleDictAtPut(thread_, module_dict, baz, value);

  ValueCell::cast(runtime_.dictAt(thread_, module_dict, bar)).makePlaceholder();

  List keys(&scope, moduleDictKeys(thread_, module_dict));
  EXPECT_EQ(keys.numItems(), 2);
  EXPECT_EQ(keys.at(0), *foo);
  EXPECT_EQ(keys.at(1), *baz);
}

TEST_F(ModuleBuiltinsTest, ModuleGetAttributeReturnsInstanceValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "x = 42").isError());
  Module module(&scope, runtime_.findModuleById(SymbolId::kDunderMain));
  Object name(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(moduleGetAttribute(thread_, module, name), 42));
}

TEST_F(ModuleBuiltinsTest, ModuleGetAttributeWithNonExistentNameReturnsError) {
  Thread* thread = Thread::current();
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_.newStrFromCStr(""));
  Module module(&scope, runtime_.newModule(module_name));
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(moduleGetAttribute(thread_, module, name).isError());
  EXPECT_FALSE(thread->hasPendingException());
}

TEST_F(ModuleBuiltinsTest, ModuleSetAttrSetsAttribute) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_.newStrFromCStr("foo"));
  Module module(&scope, runtime_.newModule(module_name));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  Object value(&scope, runtime_.newInt(-543));
  EXPECT_TRUE(moduleSetAttr(thread_, module, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(runtime_.moduleAt(module, name), -543));
}

TEST_F(ModuleBuiltinsTest, NewModuleDunderReprReturnsString) {
  HandleScope scope(thread_);
  Object name(&scope, runtime_.newStrFromCStr("hello"));
  Object module(&scope, runtime_.newModule(name));
  Object result(
      &scope, Thread::current()->invokeMethod1(module, SymbolId::kDunderRepr));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<module 'hello'>"));
}

TEST_F(ModuleBuiltinsTest, NextModuleDictItemReturnsNextNonPlaceholder) {
  HandleScope scope(thread_);
  Dict module_dict(&scope, runtime_.newDict());

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str bar(&scope, runtime_.newStrFromCStr("bar"));
  Str baz(&scope, runtime_.newStrFromCStr("baz"));
  Str qux(&scope, runtime_.newStrFromCStr("qux"));
  Str value(&scope, runtime_.newStrFromCStr("value"));

  runtime_.moduleDictAtPut(thread_, module_dict, foo, value);
  runtime_.moduleDictAtPut(thread_, module_dict, bar, value);
  runtime_.moduleDictAtPut(thread_, module_dict, baz, value);
  runtime_.moduleDictAtPut(thread_, module_dict, qux, value);

  // Only baz is not Placeholder.
  ValueCell::cast(runtime_.dictAt(thread_, module_dict, foo)).makePlaceholder();
  ValueCell::cast(runtime_.dictAt(thread_, module_dict, bar)).makePlaceholder();
  ValueCell::cast(runtime_.dictAt(thread_, module_dict, qux)).makePlaceholder();

  Tuple buckets(&scope, module_dict.data());
  word i = Dict::Bucket::kFirst;
  EXPECT_TRUE(nextModuleDictItem(*buckets, &i));
  EXPECT_TRUE(isStrEqualsCStr(Dict::Bucket::key(*buckets, i), "baz"));
  EXPECT_FALSE(nextModuleDictItem(*buckets, &i));
}

TEST_F(ModuleBuiltinsTest, BuiltinModuleDunderReprReturnsString) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
result = sys.__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<module 'sys' (built-in)>"));
}

}  // namespace python
