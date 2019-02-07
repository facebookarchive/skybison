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
  HandleScope scope;
  RawObject imp = moduleAt(&runtime, "__main__", "_imp");
  EXPECT_TRUE(imp->isModule());
}

TEST(ImportBuiltins, AcquireLockAndReleaseLockWorks) {
  Runtime runtime;
  runBuiltin(builtinImpAcquireLock);
  runBuiltin(builtinImpReleaseLock);
}

TEST(ImportBuiltinsTest, CreateBuiltinWithoutArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object result(&scope, runBuiltin(builtinImpCreateBuiltin));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, CreateBuiltinWithoutSpecNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int foo(&scope, RawSmallInt::fromWord(123));
  Object result(&scope, runBuiltin(builtinImpCreateBuiltin, foo));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, CreateBuiltinWithNonStrSpecNameRaisesTypeError) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  runFromCStr(&runtime, R"(
class DummyModuleSpec():
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec(5)
)");
  HandleScope scope;
  Object spec(&scope, moduleAt(&runtime, "__main__", "spec"));
  Object result(&scope, runBuiltin(builtinImpCreateBuiltin, spec));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, CreateBuiltinWithNonExistentModuleReturnsNone) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  runFromCStr(&runtime, R"(
class DummyModuleSpec():
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("non_existent_module")
)");
  HandleScope scope;
  Object spec(&scope, moduleAt(&runtime, "__main__", "spec"));
  Object result(&scope, runBuiltin(builtinImpCreateBuiltin, spec));
  EXPECT_TRUE(result->isNoneType());
}

TEST(ImportBuiltinsTest, CreateBuiltinReturnsModule) {
  Runtime runtime;
  // Mock of importlib._bootstrap.ModuleSpec
  runFromCStr(&runtime, R"(
class DummyModuleSpec():
  def __init__(self, name):
    self.name = name
spec = DummyModuleSpec("errno")
)");
  HandleScope scope;
  Object spec(&scope, moduleAt(&runtime, "__main__", "spec"));
  Object result(&scope, runBuiltin(builtinImpCreateBuiltin, spec));
  ASSERT_TRUE(result->isModule());
  EXPECT_TRUE(isStrEqualsCStr(Module::cast(*result)->name(), "errno"));
}

TEST(ImportBuiltinsDeathTest, ExecBuiltin) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
import _imp
_imp.exec_builtin("foo")
  )"),
               "unimplemented: exec_builtin");
}

TEST(ImportBuiltinsDeathTest, ExecDynamic) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
import _imp
_imp.exec_dynamic("foo")
  )"),
               "unimplemented: exec_dynamic");
}

TEST(ImportBuiltinsTest, ExtensionSuffixesWithoutArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Object result(&scope, runBuiltin(builtinImpExtensionSuffixes, module_name));
  ASSERT_TRUE(result->isError());
  EXPECT_EQ(Thread::currentThread()->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, ExtensionSuffixesReturnsList) {
  Runtime runtime;
  HandleScope scope;
  Object result(&scope, runBuiltin(builtinImpExtensionSuffixes));
  ASSERT_TRUE(result->isList());
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

TEST(ImportBuiltinsTest, IsBuiltinWithoutArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object result(&scope, runBuiltin(builtinImpIsBuiltin));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, IsBuiltinReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Object result(&scope, runBuiltin(builtinImpIsBuiltin, module_name));
  ASSERT_TRUE(result->isInt());
  EXPECT_EQ(Int::cast(*result)->asWord(), 0);
}

TEST(ImportBuiltinsTest, IsBuiltinReturnsNegativeOne) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("sys"));
  Object result(&scope, runBuiltin(builtinImpIsBuiltin, module_name));
  ASSERT_TRUE(result->isInt());
  EXPECT_EQ(Int::cast(*result)->asWord(), -1);
}

TEST(ImportBuiltinsTest, IsBuiltinReturnsOne) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("errno"));
  Object result(&scope, runBuiltin(builtinImpIsBuiltin, module_name));
  ASSERT_TRUE(result->isInt());
  EXPECT_EQ(Int::cast(*result)->asWord(), 1);
}

TEST(ImportBuiltinsTest, IsFrozenWithNoArgsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object result(&scope, runBuiltin(builtinImpIsFrozen));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(ImportBuiltinsTest, IsFrozenReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  Object result(&scope, runBuiltin(builtinImpIsFrozen, module_name));
  ASSERT_TRUE(result->isBool());
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
  Object result(&scope, runBuiltin(builtinImpReleaseLock));
  EXPECT_TRUE(raised(*result, LayoutId::kRuntimeError));
}

TEST(ImportBuiltins, AcquireLockCheckRecursiveCallsWorks) {
  Runtime runtime;
  HandleScope scope;
  runBuiltin(builtinImpAcquireLock);
  runBuiltin(builtinImpAcquireLock);
  runBuiltin(builtinImpReleaseLock);
  runBuiltin(builtinImpReleaseLock);
  // Make sure that additional releases raise.
  Object result(&scope, runBuiltin(builtinImpReleaseLock));
  EXPECT_TRUE(result->isError());
}

}  // namespace python
