#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(ImpModuleTest, ModuleImporting) {
  Runtime runtime;
  runtime.runFromCStr(R"(
import _imp
  )");
  HandleScope scope;
  RawObject imp = testing::moduleAt(&runtime, "__main__", "_imp");
  EXPECT_TRUE(imp->isModule());
}

TEST(ImportBuiltinsDeathTest, AcquireLock) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.acquire_lock()
  )"),
               "unimplemented: acquire_lock");
}

TEST(ImportBuiltinsDeathTest, CreateBuiltin) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.create_builtin("foo")
  )"),
               "unimplemented: create_builtin");
}

TEST(ImportBuiltinsDeathTest, ExecBuiltin) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.exec_builtin("foo")
  )"),
               "unimplemented: exec_builtin");
}

TEST(ImportBuiltinsDeathTest, ExecDynamic) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.exec_dynamic("foo")
  )"),
               "unimplemented: exec_dynamic");
}

TEST(ImportBuiltinsDeathTest, ExtensionSuffixes) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.extension_suffixes()
  )"),
               "unimplemented: extension_suffixes");
}

TEST(ImportBuiltinsDeathTest, FixCoFilename) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
code = None
source_path = None
_imp._fix_co_filename(code, source_path)
  )"),
               "unimplemented: _fix_co_filename");
}

TEST(ImportBuiltinsDeathTest, GetFrozenObject) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.get_frozen_object("foo")
  )"),
               "unimplemented: get_frozen_object");
}

TEST(ImportBuiltinsDeathTest, IsBuiltin) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.is_builtin("foo")
  )"),
               "unimplemented: is_builtin");
}

TEST(ImportBuiltinsDeathTest, IsFrozen) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.is_frozen("foo")
  )"),
               "unimplemented: is_frozen");
}

TEST(ImportBuiltinsDeathTest, IsFrozenPackage) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.is_frozen_package("foo")
  )"),
               "unimplemented: is_frozen_package");
}

TEST(ImportBuiltinsDeathTest, ReleaseLock) {
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(R"(
import _imp
_imp.release_lock()
  )"),
               "unimplemented: release_lock");
}

}  // namespace python
