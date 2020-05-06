#include "module-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "ic.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using ModuleBuiltinsDeathTest = RuntimeFixture;
using ModuleBuiltinsTest = RuntimeFixture;

TEST_F(ModuleBuiltinsTest, DunderName) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import sys
a = sys.__name__
)")
                   .isError());
  HandleScope scope(thread_);
  Str a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(a.equalsCStr("sys"));
}

TEST_F(ModuleBuiltinsTest, DunderNameOverwritesDoesNotAffectModuleNameField) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import sys
sys.__name__ = "ysy"
)")
                   .isError());
  HandleScope scope(thread_);
  Module sys_module(&scope, mainModuleAt(runtime_, "sys"));
  EXPECT_TRUE(isStrEqualsCStr(sys_module.name(), "sys"));
}

// TODO(T39575976): Add tests verifying internal names's hiding.
TEST_F(ModuleBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "foo = -6").isError());
  Module module(&scope, runtime_->findModuleById(ID(__main__)));
  Object name(&scope, runtime_->newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(METH(module, __getattribute__), module, name), -6));
}

TEST_F(ModuleBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "").isError());
  Module module(&scope, runtime_->findModuleById(ID(__main__)));
  Object name(&scope, runtime_->newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(module, __getattribute__), module, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(ModuleBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "").isError());
  Module module(&scope, runtime_->findModuleById(ID(__main__)));
  Object name(&scope, runtime_->newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(module, __getattribute__), module, name),
      LayoutId::kAttributeError, "module '__main__' has no attribute 'xxx'"));
}

TEST_F(ModuleBuiltinsTest, DunderSetattrSetsAttribute) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr("foo"));
  Module module(&scope, runtime_->newModule(module_name));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foobarbaz"));
  Object value(&scope, runtime_->newInt(0xf00d));
  EXPECT_TRUE(
      runBuiltin(METH(module, __setattr__), module, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(thread_, module, name), 0xf00d));
}

TEST_F(ModuleBuiltinsTest, DunderSetattrWithNonStrNameRaisesTypeError) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr("foo"));
  Object module(&scope, runtime_->newModule(module_name));
  Object name(&scope, runtime_->newFloat(4.4));
  Object value(&scope, runtime_->newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(METH(module, __setattr__), module, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'float'"));
}

TEST_F(ModuleBuiltinsTest, DunderDict) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import sys
result = sys.__dict__
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isModuleProxy());
}

static RawModule createTestingModule(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  // Create a builtins module.
  Module builtins_module(&scope, runtime->createModule(thread, ID(builtins)));
  Dict builtins_dict(&scope, runtime->newDict());
  builtins_module.setDict(*builtins_dict);

  // Create a module dict with builtins in it.
  Dict module_dict(&scope, runtime->newDict());
  Object dunder_builtins_name(&scope, runtime->symbols()->at(ID(__builtins__)));
  dictAtPutInValueCellByStr(thread, module_dict, dunder_builtins_name,
                            builtins_module);

  Module module(&scope, runtime->findOrCreateMainModule());
  module.setDict(*module_dict);
  return *module;
}

TEST_F(ModuleBuiltinsTest, ModuleAtIgnoresBuiltinsEntry) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_));
  Module builtins(&scope, moduleAtById(thread_, module, ID(__builtins__)));

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Str foo_in_builtins(&scope, runtime_->newStrFromCStr("foo_in_builtins"));
  moduleAtPut(thread_, builtins, foo, foo_in_builtins);
  EXPECT_TRUE(moduleAt(thread_, module, foo).isErrorNotFound());
}

TEST_F(ModuleBuiltinsTest, ModuleAtReturnsValuePutByModuleAtPut) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "a"));
  Object value(&scope, runtime_->newStrFromCStr("a's value"));
  moduleAtPut(thread_, module, name, value);
  EXPECT_EQ(moduleAt(thread_, module, name), *value);
}

TEST_F(ModuleBuiltinsTest, ModuleAtReturnsErrorNotFoundForPlaceholder) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "a"));
  Object value(&scope, runtime_->newStrFromCStr("a's value"));
  moduleAtPut(thread_, module, name, value);

  Dict module_dict(&scope, module.dict());
  ValueCell value_cell(&scope, dictAtByStr(thread_, module_dict, name));
  value_cell.makePlaceholder();
  EXPECT_TRUE(moduleAt(thread_, module, name).isErrorNotFound());
}

TEST_F(ModuleBuiltinsTest, ModuleAtByIdReturnsValuePutByModuleAtPutById) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_));
  Object value(&scope, runtime_->newStrFromCStr("a's value"));
  moduleAtPutById(thread_, module, ID(NotImplemented), value);
  EXPECT_EQ(moduleAtById(thread_, module, ID(NotImplemented)), *value);
}

TEST_F(ModuleBuiltinsTest, ModuleAtReturnsValuePutByModuleAtPutByCStr) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_));
  Object value(&scope, runtime_->newStrFromCStr("a's value"));
  const char* name_cstr = "a";
  moduleAtPutByCStr(thread_, module, name_cstr, value);
  Object name(&scope, Runtime::internStrFromCStr(thread_, name_cstr));
  EXPECT_EQ(moduleAt(thread_, module, name), *value);
}

TEST_F(ModuleBuiltinsTest,
       ModuleAtPutDoesNotInvalidateCachedModuleDictValueCell) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
a = 4

def foo():
  return a

foo()
)")
                   .isError());
  Module module(&scope, findMainModule(runtime_));
  Object a(&scope, Runtime::internStrFromCStr(thread_, "a"));
  ValueCell value_cell_a(&scope, moduleValueCellAt(thread_, module, a));

  // The looked up module entry got cached in function foo().
  Function function_foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableTuple caches(&scope, function_foo.caches());
  ASSERT_EQ(icLookupGlobalVar(*caches, 0), *value_cell_a);

  // Updating global variable a does not invalidate the cache.
  Str new_value(&scope, runtime_->newStrFromCStr("value"));
  moduleAtPut(thread_, module, a, new_value);
  EXPECT_EQ(icLookupGlobalVar(*caches, 0), *value_cell_a);
  EXPECT_EQ(value_cell_a.value(), *new_value);
}

TEST_F(ModuleBuiltinsTest, ModuleAtPutInvalidatesCachedBuiltinsValueCell) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
__builtins__.a = 4

def foo():
  return a

foo()
)")
                   .isError());
  Module module(&scope, findMainModule(runtime_));
  Module builtins(&scope, moduleAtById(thread_, module, ID(__builtins__)));
  Object a(&scope, Runtime::internStrFromCStr(thread_, "a"));
  Str new_value(&scope, runtime_->newStrFromCStr("value"));
  ValueCell value_cell_a(&scope, moduleValueCellAt(thread_, builtins, a));

  // The looked up module entry got cached in function foo().
  Function function_foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableTuple caches(&scope, function_foo.caches());
  ASSERT_EQ(icLookupGlobalVar(*caches, 0), *value_cell_a);

  ASSERT_FALSE(mainModuleAt(runtime_, "__builtins__").isErrorNotFound());

  // Updating global variable a does invalidate the cache since it shadows
  // __builtins__.a.
  moduleAtPut(thread_, module, a, new_value);
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(ModuleBuiltinsTest, ModuleKeysFiltersOutPlaceholders) {
  HandleScope scope(thread_);
  Str name(&scope, Str::empty());
  Module module(&scope, runtime_->newModule(name));
  Dict module_dict(&scope, runtime_->newDict());
  module.setDict(*module_dict);

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Str value(&scope, runtime_->newStrFromCStr("value"));

  moduleAtPut(thread_, module, foo, value);
  moduleAtPut(thread_, module, bar, value);
  moduleAtPut(thread_, module, baz, value);

  ValueCell::cast(moduleValueCellAt(thread_, module, bar)).makePlaceholder();

  List keys(&scope, moduleKeys(thread_, module));
  EXPECT_EQ(keys.numItems(), 2);
  EXPECT_EQ(keys.at(0), *foo);
  EXPECT_EQ(keys.at(1), *baz);
}

TEST_F(ModuleBuiltinsTest, ModuleLenReturnsItemCountExcludingPlaceholders) {
  HandleScope scope(thread_);
  Object module_name(&scope, Runtime::internStrFromCStr(thread_, "mymodule"));
  Module module(&scope, runtime_->newModule(module_name));

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Str value(&scope, runtime_->newStrFromCStr("value"));

  moduleAtPut(thread_, module, foo, value);
  moduleAtPut(thread_, module, bar, value);
  moduleAtPut(thread_, module, baz, value);

  SmallInt previous_len(&scope, moduleLen(thread_, module));

  Dict module_dict(&scope, module.dict());
  word hash = strHash(thread_, *bar);
  ValueCell::cast(dictAt(thread_, module_dict, bar, hash)).makePlaceholder();

  SmallInt after_len(&scope, moduleLen(thread_, module));
  EXPECT_EQ(previous_len.value(), after_len.value() + 1);
}

TEST_F(ModuleBuiltinsTest, ModuleRemoveInvalidatesCachedModuleDictValueCell) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
a = 4

def foo():
  return a

foo()
)")
                   .isError());

  // The looked up module entry got cached in function foo().
  Function function_foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableTuple caches(&scope, function_foo.caches());

  Module module(&scope, findMainModule(runtime_));
  Str a(&scope, runtime_->newStrFromCStr("a"));
  ASSERT_EQ(icLookupGlobalVar(*caches, 0),
            moduleValueCellAt(thread_, module, a));

  word hash = strHash(thread_, *a);
  EXPECT_FALSE(moduleRemove(thread_, module, a, hash).isError());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(ModuleBuiltinsTest, ModuleValuesFiltersOutPlaceholders) {
  HandleScope scope(thread_);
  Str module_name(&scope, runtime_->newStrFromCStr("mymodule"));
  Module module(&scope, runtime_->newModule(module_name));

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Str foo_value(&scope, runtime_->newStrFromCStr("foo_value"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Str bar_value(&scope, runtime_->newStrFromCStr("bar_value"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Str baz_value(&scope, runtime_->newStrFromCStr("baz_value"));

  moduleAtPut(thread_, module, foo, foo_value);
  moduleAtPut(thread_, module, bar, bar_value);
  moduleAtPut(thread_, module, baz, baz_value);

  Dict module_dict(&scope, module.dict());
  ValueCell::cast(dictAtByStr(thread_, module_dict, bar)).makePlaceholder();

  List values(&scope, moduleValues(thread_, module));
  EXPECT_TRUE(listContains(values, foo_value));
  EXPECT_FALSE(listContains(values, bar_value));
  EXPECT_TRUE(listContains(values, baz_value));
}

TEST_F(ModuleBuiltinsTest, ModuleGetAttributeReturnsInstanceValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "x = 42").isError());
  Module module(&scope, runtime_->findModuleById(ID(__main__)));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "x"));
  EXPECT_TRUE(isIntEqualsWord(moduleGetAttribute(thread_, module, name), 42));
}

TEST_F(ModuleBuiltinsTest, ModuleGetAttributeWithNonExistentNameReturnsError) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr(""));
  Module module(&scope, runtime_->newModule(module_name));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "xxx"));
  EXPECT_TRUE(moduleGetAttribute(thread_, module, name).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(ModuleBuiltinsTest, ModuleSetAttrSetsAttribute) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr("foo"));
  Module module(&scope, runtime_->newModule(module_name));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object value(&scope, runtime_->newInt(-543));
  EXPECT_TRUE(moduleSetAttr(thread_, module, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(thread_, module, name), -543));
}

TEST_F(ModuleBuiltinsTest, NewModuleDunderReprReturnsString) {
  HandleScope scope(thread_);
  Object name(&scope, runtime_->newStrFromCStr("hello"));
  Object module(&scope, runtime_->newModule(name));
  Object result(&scope, Thread::current()->invokeMethod1(module, ID(__repr__)));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<module 'hello'>"));
}

TEST_F(ModuleBuiltinsTest, BuiltinModuleDunderReprReturnsString) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import sys
result = sys.__repr__()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<module 'sys' (built-in)>"));
}

TEST_F(ModuleBuiltinsTest, ModuleIsSealed) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kModule));
  EXPECT_TRUE(type.hasFlag(Type::Flag::kSealSubtypeLayouts));
  EXPECT_TRUE(type.isSealed());
}

TEST_F(ModuleBuiltinsTest, ModuleSubclassIsSealed) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C(module): pass
)")
                   .isError());
  Type type(&scope, mainModuleAt(runtime_, "C"));
  EXPECT_TRUE(type.hasFlag(Type::Flag::kSealSubtypeLayouts));
  EXPECT_TRUE(type.isSealed());
}

}  // namespace testing
}  // namespace py
