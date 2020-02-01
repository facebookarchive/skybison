#include "under-imp-module.h"

#include "gtest/gtest.h"

#include "capi-handles.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
using namespace testing;

using UnderImpModuleTest = RuntimeFixture;
using ImportBuiltinsDeathTest = RuntimeFixture;
using ImportBuiltinsTest = RuntimeFixture;

TEST_F(UnderImpModuleTest, ModuleImporting) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _imp
  )")
                   .isError());
  RawObject imp = mainModuleAt(runtime_, "_imp");
  EXPECT_TRUE(imp.isModule());
}

TEST_F(ImportBuiltinsTest, AcquireLockAndReleaseLockWorks) {
  runBuiltin(FUNC(_imp, acquire_lock));
  runBuiltin(FUNC(_imp, release_lock));
}

TEST_F(ImportBuiltinsTest, CreateBuiltinWithoutArgsRaisesTypeError) {
  EXPECT_TRUE(raised(runFromCStr(runtime_, R"(
import _imp
_imp.create_builtin()
)"),
                     LayoutId::kTypeError));
}

TEST_F(ImportBuiltinsTest, CreateBuiltinWithoutSpecNameRaisesTypeError) {
  EXPECT_TRUE(raised(runFromCStr(runtime_, R"(
import _imp
_imp.create_builtin(123)
)"),
                     LayoutId::kTypeError));
}

TEST_F(ImportBuiltinsTest, CreateBuiltinWithNonStrSpecNameRaisesTypeError) {
  // Mock of importlib._bootstrap.ModuleSpec
  EXPECT_TRUE(raised(runFromCStr(runtime_, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec(5)
_imp.create_builtin(spec)
)"),
                     LayoutId::kTypeError));
}

TEST_F(ImportBuiltinsTest, CreateBuiltinWithNonExistentModuleReturnsNone) {
  // Mock of importlib._bootstrap.ModuleSpec
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("non_existent_module")
result = _imp.create_builtin(spec)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isNoneType());
}

TEST_F(ImportBuiltinsTest, CreateBuiltinReturnsModule) {
  // Mock of importlib._bootstrap.ModuleSpec
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("errno")
result = _imp.create_builtin(spec)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result).name(), "errno"));
}

TEST_F(ImportBuiltinsTest, CreateBuiltinWithExArgsReturnsModule) {
  // Mock of importlib._bootstrap.ModuleSpec
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = (DummyModuleSpec("errno"),)
result = _imp.create_builtin(*spec)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result).name(), "errno"));
}

TEST_F(ImportBuiltinsTest, ExecBuiltinWithNonModuleReturnsZero) {
  HandleScope scope(thread_);
  Int not_mod(&scope, runtime_->newInt(1));
  Object a(&scope, runBuiltin(FUNC(_imp, exec_builtin), not_mod));
  EXPECT_TRUE(isIntEqualsWord(*a, 0));
}

TEST_F(ImportBuiltinsTest, ExecBuiltinWithModuleWithNoDefReturnsZero) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class DummyModuleSpec():
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("errno")
)")
                   .isError());
  HandleScope scope(thread_);
  Object spec(&scope, mainModuleAt(runtime_, "spec"));
  Object module(&scope, runBuiltin(FUNC(_imp, create_builtin), spec));
  ASSERT_TRUE(module.isModule());

  Object a(&scope, runBuiltin(FUNC(_imp, exec_builtin), module));
  EXPECT_TRUE(isIntEqualsWord(*a, 0));
}

TEST_F(ImportBuiltinsTest, ExecBuiltinWithSingleSlotExecutesCorrectly) {
  using slot_func = int (*)(Module*);
  slot_func mod_exec = [](Module* module) {
    module->setName(Thread::current()->runtime()->newStrFromCStr("testing"));
    return 0;
  };

  static PyModuleDef_Slot slots[] = {
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec)},
      {0, nullptr},
  };
  static PyModuleDef def = {
      // Empty header to mimic a PyModuleDef_HEAD_INIT
      {{0, 0}, nullptr, 0, nullptr}, "mymodule", nullptr, 0, nullptr, slots,
  };

  HandleScope scope(thread_);
  Str name(&scope, runtime_->newStrFromCStr("mymodule"));
  Module module(&scope, runtime_->newModule(name));
  module.setDef(runtime_->newIntFromCPtr(&def));

  Object a(&scope, runBuiltin(FUNC(_imp, exec_builtin), module));
  EXPECT_TRUE(isIntEqualsWord(*a, 0));

  Str mod_name(&scope, module.name());
  EXPECT_TRUE(mod_name.equalsCStr("testing"));
}

TEST_F(ImportBuiltinsTest, ExecDynamic) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _imp
mod = _imp.exec_dynamic("foo")
  )")
                   .isError());
  HandleScope scope(thread_);
  Object mod(&scope, mainModuleAt(runtime_, "mod"));
  ASSERT_TRUE(mod.isNoneType());
}

TEST_F(ImportBuiltinsTest, ExtensionSuffixesReturnsList) {
  HandleScope scope(thread_);
  Object result(&scope, runBuiltin(FUNC(_imp, extension_suffixes)));
  ASSERT_TRUE(result.isList());
  EXPECT_PYLIST_EQ(result, {".so"});
}

TEST_F(ImportBuiltinsTest, IsBuiltinReturnsZero) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr("foo"));
  Object result(&scope, runBuiltin(FUNC(_imp, is_builtin), module_name));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(ImportBuiltinsTest, IsBuiltinReturnsOne) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr("errno"));
  Object result(&scope, runBuiltin(FUNC(_imp, is_builtin), module_name));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST_F(ImportBuiltinsTest, IsFrozenReturnsFalse) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr("foo"));
  Object result(&scope, runBuiltin(FUNC(_imp, is_frozen), module_name));
  ASSERT_TRUE(result.isBool());
  EXPECT_FALSE(Bool::cast(*result).value());
}

TEST_F(ImportBuiltinsTest, ReleaseLockWithoutAcquireRaisesRuntimeError) {
  HandleScope scope(thread_);
  Object result(&scope, runBuiltin(FUNC(_imp, release_lock)));
  EXPECT_TRUE(raised(*result, LayoutId::kRuntimeError));
}

TEST_F(ImportBuiltinsTest, AcquireLockCheckRecursiveCallsWorks) {
  HandleScope scope(thread_);
  runBuiltin(FUNC(_imp, acquire_lock));
  runBuiltin(FUNC(_imp, acquire_lock));
  runBuiltin(FUNC(_imp, release_lock));
  runBuiltin(FUNC(_imp, release_lock));
  // Make sure that additional releases raise.
  Object result(&scope, runBuiltin(FUNC(_imp, release_lock)));
  EXPECT_TRUE(result.isError());
}

TEST_F(ImportBuiltinsTest, CreateExistingBuiltinDoesNotOverride) {
  // Mock of importlib._bootstrap.ModuleSpec
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = (DummyModuleSpec("errno"),)
result1 = _imp.create_builtin(*spec)
result2 = _imp.create_builtin(*spec)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result1(&scope, mainModuleAt(runtime_, "result1"));
  ASSERT_TRUE(result1.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result1).name(), "errno"));
  Object result2(&scope, mainModuleAt(runtime_, "result2"));
  ASSERT_TRUE(result2.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result2).name(), "errno"));
  EXPECT_EQ(*result1, *result2);
}

}  // namespace py
