#include "gtest/gtest.h"

#include "imp-module.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(ImpModuleTest, ModuleImporting) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _imp
  )");
  RawObject imp = moduleAt(&runtime, "__main__", "_imp");
  EXPECT_TRUE(imp->isModule());
}

TEST(ImportBuiltins, AcquireLockAndReleaseLockWorks) {
  Runtime runtime;
  runBuiltin(UnderImpModule::acquireLock);
  runBuiltin(UnderImpModule::releaseLock);
}

TEST(ImportBuiltinsTest, CreateBuiltinWithoutArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
import _imp
_imp.create_builtin()
)"),
                     LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, CreateBuiltinWithoutSpecNameRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
import _imp
_imp.create_builtin(123)
)"),
                     LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, CreateBuiltinWithNonStrSpecNameRaisesTypeError) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec(5)
_imp.create_builtin(spec)
)"),
                     LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, CreateBuiltinWithNonExistentModuleReturnsNone) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  runFromCStr(&runtime, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("non_existent_module")
result = _imp.create_builtin(spec)
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isNoneType());
}

TEST(ImportBuiltinsTest, CreateBuiltinReturnsModule) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  runFromCStr(&runtime, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("errno")
result = _imp.create_builtin(spec)
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result)->name(), "errno"));
}

TEST(ImportBuiltinsTest, CreateBuiltinWithExArgsReturnsModule) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  runFromCStr(&runtime, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = (DummyModuleSpec("errno"),)
result = _imp.create_builtin(*spec)
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result)->name(), "errno"));
}

TEST(ImportBuiltins, ExecBuiltinWithNonModuleReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Int not_mod(&scope, runtime.newInt(1));
  Object a(&scope, runBuiltin(UnderImpModule::execBuiltin, not_mod));
  EXPECT_TRUE(isIntEqualsWord(*a, 0));
}

TEST(ImportBuiltins, ExecBuiltinWithModuleWithNoDefReturnsZero) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class DummyModuleSpec():
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("errno")
)");
  HandleScope scope;
  Object spec(&scope, moduleAt(&runtime, "__main__", "spec"));
  Object module(&scope, runBuiltin(UnderImpModule::createBuiltin, spec));
  ASSERT_TRUE(module.isModule());

  Object a(&scope, runBuiltin(UnderImpModule::execBuiltin, module));
  EXPECT_TRUE(isIntEqualsWord(*a, 0));
}

TEST(ImportBuiltins, ExecBuiltinWithSingleSlotExecutesCorrectly) {
  using slot_func = int (*)(Module*);
  slot_func mod_exec = [](Module* module) {
    module->setName(
        Thread::currentThread()->runtime()->newStrFromCStr("testing"));
    return 0;
  };

  static PyModuleDef_Slot slots[] = {
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec)},
      {0, nullptr},
  };
  static PyModuleDef def = {
      // Empty header to mimic a PyModuleDef_HEAD_INIT
      {{nullptr, 0, nullptr}, nullptr, 0, nullptr},
      "mymodule",
      nullptr,
      0,
      nullptr,
      slots,
  };

  Runtime runtime;
  HandleScope scope;
  Str name(&scope, runtime.newStrFromCStr("mymodule"));
  Module module(&scope, runtime.newModule(name));
  module.setDef(runtime.newIntFromCPtr(&def));

  Object a(&scope, runBuiltin(UnderImpModule::execBuiltin, module));
  EXPECT_TRUE(isIntEqualsWord(*a, 0));

  Str mod_name(&scope, module.name());
  EXPECT_TRUE(mod_name.equalsCStr("testing"));
}

TEST(ImportBuiltinsDeathTest, ExecDynamic) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
import _imp
_imp.exec_dynamic("foo")
  )"),
               "unimplemented: exec_dynamic");
}

TEST(ImportBuiltinsTest, ExtensionSuffixesReturnsList) {
  Runtime runtime;
  HandleScope scope;
  Object result(&scope, runBuiltin(UnderImpModule::extensionSuffixes));
  ASSERT_TRUE(result.isList());
  EXPECT_PYLIST_EQ(result, {".so"});
}

TEST(ImportBuiltinsDeathTest, FixCoFilename) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
import _imp
code = None
source_path = None
_imp._fix_co_filename(code, source_path)
  )"),
               "unimplemented: _fix_co_filename");
}

TEST(ImportBuiltinsDeathTest, GetFrozenObject) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
import _imp
_imp.get_frozen_object("foo")
  )"),
               "unimplemented: get_frozen_object");
}

TEST(ImportBuiltinsTest, IsBuiltinReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Object result(&scope, runBuiltin(UnderImpModule::isBuiltin, module_name));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST(ImportBuiltinsTest, IsBuiltinReturnsNegativeOne) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("sys"));
  Object result(&scope, runBuiltin(UnderImpModule::isBuiltin, module_name));
  EXPECT_TRUE(isIntEqualsWord(*result, -1));
}

TEST(ImportBuiltinsTest, IsBuiltinReturnsOne) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("errno"));
  Object result(&scope, runBuiltin(UnderImpModule::isBuiltin, module_name));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

TEST(ImportBuiltinsTest, IsFrozenReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Object result(&scope, runBuiltin(UnderImpModule::isFrozen, module_name));
  ASSERT_TRUE(result.isBool());
  EXPECT_FALSE(Bool::cast(*result)->value());
}

TEST(ImportBuiltinsDeathTest, IsFrozenPackage) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
import _imp
_imp.is_frozen_package("foo")
  )"),
               "unimplemented: is_frozen_package");
}

TEST(ImportBuiltinsTest, ReleaseLockWithoutAcquireRaisesRuntimeError) {
  Runtime runtime;
  HandleScope scope;
  Object result(&scope, runBuiltin(UnderImpModule::releaseLock));
  EXPECT_TRUE(raised(*result, LayoutId::kRuntimeError));
}

TEST(ImportBuiltins, AcquireLockCheckRecursiveCallsWorks) {
  Runtime runtime;
  HandleScope scope;
  runBuiltin(UnderImpModule::acquireLock);
  runBuiltin(UnderImpModule::acquireLock);
  runBuiltin(UnderImpModule::releaseLock);
  runBuiltin(UnderImpModule::releaseLock);
  // Make sure that additional releases raise.
  Object result(&scope, runBuiltin(UnderImpModule::releaseLock));
  EXPECT_TRUE(result.isError());
}

TEST(ImportBuiltins, CreateExistingBuiltinDoesNotOverride) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  runFromCStr(&runtime, R"(
import _imp
class DummyModuleSpec:
  def __init__(self, name):
    self.name = name
spec = (DummyModuleSpec("errno"),)
result1 = _imp.create_builtin(*spec)
result2 = _imp.create_builtin(*spec)
)");
  HandleScope scope;
  Object result1(&scope, moduleAt(&runtime, "__main__", "result1"));
  ASSERT_TRUE(result1.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result1)->name(), "errno"));
  Object result2(&scope, moduleAt(&runtime, "__main__", "result2"));
  ASSERT_TRUE(result2.isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result2)->name(), "errno"));
  EXPECT_EQ(*result1, *result2);
}

}  // namespace python
