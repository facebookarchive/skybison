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
  Str a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(a.equalsCStr("sys"));
}

TEST_F(ModuleBuiltinsTest, DunderNameOverwritesDoesNotAffectModuleNameField) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
sys.__name__ = "ysy"
)")
                   .isError());
  HandleScope scope(thread_);
  Module sys_module(&scope, mainModuleAt(&runtime_, "sys"));
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

TEST_F(ModuleBuiltinsTest, DunderSetattrSetsAttribute) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_.newStrFromCStr("foo"));
  Module module(&scope, runtime_.newModule(module_name));
  Object name(&scope, runtime_.newStrFromCStr("foobarbaz"));
  Object value(&scope, runtime_.newInt(0xf00d));
  EXPECT_TRUE(runBuiltin(ModuleBuiltins::dunderSetattr, module, name, value)
                  .isNoneType());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(thread_, module, name), 0xf00d));
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result.isModuleProxy());
}

static RawModule createTestingModule(Thread* thread,
                                     Object* builtins_dict_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  // Create a builtins module.
  Str builtins_name(&scope, runtime->symbols()->Builtins());
  Module builtins_module(&scope, runtime->newModule(builtins_name));
  Dict builtins_dict(&scope, runtime->newDict());
  builtins_module.setDict(*builtins_dict);

  // Create a module dict with builtins in it.
  Dict module_dict(&scope, runtime->newDict());
  Object dunder_builtins_name(&scope, runtime->symbols()->DunderBuiltins());
  runtime->dictAtPutInValueCell(thread, module_dict, dunder_builtins_name,
                                builtins_module);

  if (builtins_dict_out != nullptr) {
    *builtins_dict_out = *builtins_dict;
  }
  Str module_name(&scope, runtime->newStrFromCStr("__main__"));
  Module module(&scope, runtime->newModule(module_name));
  module.setDict(*module_dict);
  return *module;
}

TEST_F(ModuleBuiltinsTest, ModuleAtIgnoresBuiltinsEntry) {
  HandleScope scope(thread_);
  Object builtins_dict_obj(&scope, NoneType::object());
  Module module(&scope, createTestingModule(thread_, &builtins_dict_obj));
  Dict builtins_dict(&scope, *builtins_dict_obj);

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str foo_in_builtins(&scope, runtime_.newStrFromCStr("foo_in_builtins"));
  moduleDictAtPut(thread_, builtins_dict, foo, foo_in_builtins);
  EXPECT_TRUE(moduleAt(thread_, module, foo).isErrorNotFound());
}

TEST_F(ModuleBuiltinsTest, ModuleAtReturnsValuePutByModuleAtPut) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_, nullptr));
  Object key(&scope, runtime_.newStrFromCStr("a"));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  moduleAtPut(thread_, module, key, value);
  EXPECT_EQ(moduleAt(thread_, module, key), *value);
}

TEST_F(ModuleBuiltinsTest, ModuleAtReturnsErrorNotFoundForPlaceholder) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_, nullptr));
  Object key(&scope, runtime_.newStrFromCStr("a"));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  moduleAtPut(thread_, module, key, value);

  Dict module_dict(&scope, module.dict());
  ValueCell value_cell(&scope, runtime_.dictAt(thread_, module_dict, key));
  value_cell.makePlaceholder();
  EXPECT_TRUE(moduleAt(thread_, module, key).isErrorNotFound());
}

TEST_F(ModuleBuiltinsTest, ModuleAtByIdReturnsValuePutByModuleAtPutById) {
  HandleScope scope(thread_);
  Module module(&scope, createTestingModule(thread_, nullptr));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  moduleAtPutById(thread_, module, SymbolId::kNotImplemented, value);
  EXPECT_EQ(moduleAtById(thread_, module, SymbolId::kNotImplemented), *value);
}

TEST_F(ModuleBuiltinsTest,
       ModuleAtPutDoesNotInvalidateCachedModuleDictValueCell) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  EXPECT_FALSE(runFromCStr(&runtime, R"(
a = 4

def foo():
  return a

foo()
)")
                   .isError());
  Module module(&scope, findMainModule(&runtime));
  Dict module_dict(&scope, module.dict());
  Str a(&scope, runtime.newStrFromCStr("a"));
  ValueCell value_cell_a(&scope, moduleDictValueCellAt(thread, module_dict, a));

  // The looked up module entry got cached in function foo().
  Function function_foo(&scope, mainModuleAt(&runtime, "foo"));
  Tuple caches(&scope, function_foo.caches());
  ASSERT_EQ(icLookupGlobalVar(*caches, 0), *value_cell_a);

  // Updating global variable a does not invalidate the cache.
  Str new_value(&scope, runtime_.newStrFromCStr("value"));
  moduleAtPut(thread_, module, a, new_value);
  EXPECT_EQ(icLookupGlobalVar(*caches, 0), *value_cell_a);
  EXPECT_EQ(value_cell_a.value(), *new_value);
}

TEST_F(ModuleBuiltinsTest, ModuleAtPutInvalidatesCachedBuiltinsValueCell) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  EXPECT_FALSE(runFromCStr(&runtime, R"(
__builtins__.a = 4

def foo():
  return a

foo()
)")
                   .isError());
  Module module(&scope, findMainModule(&runtime));
  Dict module_dict(&scope, module.dict());
  Dict builtins_dict(&scope, moduleDictBuiltins(thread, module_dict));
  Str a(&scope, runtime.newStrFromCStr("a"));
  Str new_value(&scope, runtime_.newStrFromCStr("value"));
  ValueCell value_cell_a(&scope,
                         moduleDictValueCellAt(thread, builtins_dict, a));

  // The looked up module entry got cached in function foo().
  Function function_foo(&scope, mainModuleAt(&runtime, "foo"));
  Tuple caches(&scope, function_foo.caches());
  ASSERT_EQ(icLookupGlobalVar(*caches, 0), *value_cell_a);

  ASSERT_FALSE(mainModuleAt(&runtime, "__builtins__").isErrorNotFound());

  // Updating global variable a does invalidate the cache since it shadows
  // __builtins__.a.
  moduleAtPut(thread, module, a, new_value);
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(ModuleBuiltinsTest, ModuleDictAtReturnsValueCellValue) {
  HandleScope scope(thread_);
  Dict globals(&scope, runtime_.newDict());
  Object key(&scope, runtime_.newStrFromCStr("a"));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  moduleDictAtPut(thread_, globals, key, value);
  Object result(&scope, moduleDictAt(thread_, globals, key));
  EXPECT_EQ(result, value);
}

TEST_F(ModuleBuiltinsTest, ModuleDictAtPutReturnsStoredValue) {
  HandleScope scope(thread_);
  Dict globals(&scope, runtime_.newDict());
  Object key(&scope, runtime_.newStrFromCStr("a"));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  Object result(&scope, moduleDictAtPut(thread_, globals, key, value));
  EXPECT_EQ(*result, value);
}

TEST_F(ModuleBuiltinsTest, ModuleDictAtReturnsErrorNotFoundForPlaceholder) {
  HandleScope scope(thread_);
  Dict globals(&scope, runtime_.newDict());
  Object key(&scope, runtime_.newStrFromCStr("a"));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  moduleDictAtPut(thread_, globals, key, value);
  Object value_cell_obj(&scope, runtime_.dictAt(thread_, globals, key));
  ASSERT_TRUE(value_cell_obj.isValueCell());
  ValueCell value_cell(&scope, *value_cell_obj);
  value_cell.makePlaceholder();
  Object result(&scope, moduleDictAt(thread_, globals, key));
  EXPECT_TRUE(result.isErrorNotFound());
}

TEST_F(ModuleBuiltinsTest, ModuleDictValueCellAtReturnsValueCell) {
  HandleScope scope(thread_);
  Dict globals(&scope, runtime_.newDict());
  Object key(&scope, runtime_.newStrFromCStr("a"));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  moduleDictAtPut(thread_, globals, key, value);
  Object result(&scope, moduleDictValueCellAt(thread_, globals, key));
  ASSERT_TRUE(result.isValueCell());
  EXPECT_EQ(ValueCell::cast(*result).value(), value);
}

TEST_F(ModuleBuiltinsTest,
       ModuleDictValueCellAtReturnsErrorNotFoundForPlaceholder) {
  HandleScope scope(thread_);
  Dict globals(&scope, runtime_.newDict());
  Object key(&scope, runtime_.newStrFromCStr("a"));
  Object value(&scope, runtime_.newStrFromCStr("a's value"));
  moduleDictAtPut(thread_, globals, key, value);
  Object value_cell_obj(&scope, runtime_.dictAt(thread_, globals, key));
  ASSERT_TRUE(value_cell_obj.isValueCell());
  ValueCell value_cell(&scope, *value_cell_obj);
  value_cell.makePlaceholder();
  Object result(&scope, moduleDictValueCellAt(thread_, globals, key));
  EXPECT_TRUE(result.isErrorNotFound());
}

TEST_F(ModuleBuiltinsTest, ModuleDictBuiltinsReturnsDunderBuiltins) {
  HandleScope scope(thread_);
  Dict globals(&scope, runtime_.newDict());
  Object name(&scope, runtime_.newStrFromCStr("mybuiltins"));
  Module module(&scope, runtime_.newModule(name));
  Object dunder_builtins_name(&scope, runtime_.newStrFromCStr("__builtins__"));
  moduleDictAtPut(thread_, globals, dunder_builtins_name, module);

  Dict result(&scope, moduleDictBuiltins(thread_, globals));
  EXPECT_EQ(result, module.dict());
}

TEST_F(ModuleBuiltinsTest, ModuleDictBuiltinsReturnsMinimalDict) {
  HandleScope scope(thread_);
  Dict globals(&scope, runtime_.newDict());
  Dict result(&scope, moduleDictBuiltins(thread_, globals));
  EXPECT_EQ(result.numItems(), 1);
  Object none_name(&scope, runtime_.newStrFromCStr("None"));
  EXPECT_EQ(moduleDictAt(thread_, result, none_name), NoneType::object());
}

TEST_F(ModuleBuiltinsTest,
       ModuleDictRemoveInvalidateCachedModuleDictValueCell) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  EXPECT_FALSE(runFromCStr(&runtime, R"(
a = 4

def foo():
  return a

foo()
)")
                   .isError());
  Module module(&scope, findMainModule(&runtime));
  Dict module_dict(&scope, module.dict());
  Str a(&scope, runtime.newStrFromCStr("a"));
  ValueCell value_cell_a(&scope, moduleDictValueCellAt(thread, module_dict, a));

  // The looked up module entry got cached in function foo().
  Function function_foo(&scope, mainModuleAt(&runtime, "foo"));
  Tuple caches(&scope, function_foo.caches());
  ASSERT_EQ(icLookupGlobalVar(*caches, 0), *value_cell_a);

  // Updating global variable a does not invalidate the cache.
  moduleDictRemove(thread_, module_dict, a);
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(ModuleBuiltinsTest, ModuleDictKeysFiltersOutPlaceholders) {
  HandleScope scope(thread_);
  Dict module_dict(&scope, runtime_.newDict());

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str bar(&scope, runtime_.newStrFromCStr("bar"));
  Str baz(&scope, runtime_.newStrFromCStr("baz"));
  Str value(&scope, runtime_.newStrFromCStr("value"));

  moduleDictAtPut(thread_, module_dict, foo, value);
  moduleDictAtPut(thread_, module_dict, bar, value);
  moduleDictAtPut(thread_, module_dict, baz, value);

  ValueCell::cast(runtime_.dictAt(thread_, module_dict, bar)).makePlaceholder();

  List keys(&scope, moduleDictKeys(thread_, module_dict));
  EXPECT_EQ(keys.numItems(), 2);
  EXPECT_EQ(keys.at(0), *foo);
  EXPECT_EQ(keys.at(1), *baz);
}

TEST_F(ModuleBuiltinsTest, ModuleLenReturnsItemCountExcludingPlaceholders) {
  HandleScope scope(thread_);
  Str module_name(&scope, runtime_.newStrFromCStr("mymodule"));
  Module module(&scope, runtime_.newModule(module_name));

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str bar(&scope, runtime_.newStrFromCStr("bar"));
  Str baz(&scope, runtime_.newStrFromCStr("baz"));
  Str value(&scope, runtime_.newStrFromCStr("value"));

  moduleAtPut(thread_, module, foo, value);
  moduleAtPut(thread_, module, bar, value);
  moduleAtPut(thread_, module, baz, value);

  SmallInt previous_len(&scope, moduleLen(thread_, module));

  Dict module_dict(&scope, module.dict());
  ValueCell::cast(runtime_.dictAt(thread_, module_dict, bar)).makePlaceholder();

  SmallInt after_len(&scope, moduleLen(thread_, module));
  EXPECT_EQ(previous_len.value(), after_len.value() + 1);
}

TEST_F(ModuleBuiltinsTest, ModuleValuesFiltersOutPlaceholders) {
  HandleScope scope(thread_);
  Str module_name(&scope, runtime_.newStrFromCStr("mymodule"));
  Module module(&scope, runtime_.newModule(module_name));

  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  Str foo_value(&scope, runtime_.newStrFromCStr("foo_value"));
  Str bar(&scope, runtime_.newStrFromCStr("bar"));
  Str bar_value(&scope, runtime_.newStrFromCStr("bar_value"));
  Str baz(&scope, runtime_.newStrFromCStr("baz"));
  Str baz_value(&scope, runtime_.newStrFromCStr("baz_value"));

  moduleAtPut(thread_, module, foo, foo_value);
  moduleAtPut(thread_, module, bar, bar_value);
  moduleAtPut(thread_, module, baz, baz_value);

  Dict module_dict(&scope, module.dict());
  ValueCell::cast(runtime_.dictAt(thread_, module_dict, bar)).makePlaceholder();

  List values(&scope, moduleValues(thread_, module));
  EXPECT_TRUE(listContains(values, foo_value));
  EXPECT_FALSE(listContains(values, bar_value));
  EXPECT_TRUE(listContains(values, baz_value));
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
  EXPECT_TRUE(isIntEqualsWord(moduleAt(thread_, module, name), -543));
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

  moduleDictAtPut(thread_, module_dict, foo, value);
  moduleDictAtPut(thread_, module_dict, bar, value);
  moduleDictAtPut(thread_, module_dict, baz, value);
  moduleDictAtPut(thread_, module_dict, qux, value);

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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<module 'sys' (built-in)>"));
}

}  // namespace python
