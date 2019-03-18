#include "gtest/gtest.h"

#include <sys/stat.h>
#include <sys/types.h>

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ImportlibTest, ImportFrozenModuleDoesNotLoadImportlib) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import operator
)")
                   .isError());
  HandleScope scope;
  Object foo_obj(&scope, moduleAt(&runtime, "__main__", "operator"));
  ASSERT_TRUE(foo_obj.isModule());
  Module foo(&scope, *foo_obj);
  EXPECT_TRUE(isStrEqualsCStr(foo.name(), "operator"));
  EXPECT_TRUE(
      runtime.findModuleById(SymbolId::kUnderFrozenImportlib).isNoneType());
  EXPECT_TRUE(runtime.findModuleById(SymbolId::kUnderFrozenImportlibExternal)
                  .isNoneType());
}

TEST(ImportlibTest, SimpleImport) {
  TemporaryDirectory tempdir;
  writeFile(tempdir.path + "foo.py", "x = 42");
  writeFile(tempdir.path + "bar.py", "x = 67");

  Runtime runtime;
  HandleScope scope;
  List sys_path(&scope, moduleAt(&runtime, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime.newStrFromCStr(tempdir.path.c_str()));
  runtime.listAdd(sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import foo
import bar
)")
                   .isError());
  Object foo_obj(&scope, moduleAt(&runtime, "__main__", "foo"));
  ASSERT_TRUE(foo_obj.isModule());
  Module foo(&scope, *foo_obj);
  EXPECT_TRUE(isStrEqualsCStr(foo.name(), "foo"));

  Object name(&scope, runtime.moduleAtById(foo, SymbolId::kDunderName));
  EXPECT_TRUE(isStrEqualsCStr(*name, "foo"));
  Object package(&scope, runtime.moduleAtById(foo, SymbolId::kDunderPackage));
  EXPECT_TRUE(isStrEqualsCStr(*package, ""));
  Object doc(&scope, runtime.moduleAtById(foo, SymbolId::kDunderDoc));
  EXPECT_TRUE(doc.isNoneType());
  Str str_x(&scope, runtime.newStrFromCStr("x"));
  Object x(&scope, runtime.moduleAt(foo, str_x));
  EXPECT_TRUE(isIntEqualsWord(*x, 42));
}

TEST(ImportlibTest, ImportsEmptyModule) {
  TemporaryDirectory tempdir;
  std::string module_dir = tempdir.path + "somedir";
  ASSERT_EQ(mkdir(module_dir.c_str(), S_IRWXU), 0);

  Runtime runtime;
  HandleScope scope;
  List sys_path(&scope, moduleAt(&runtime, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime.newStrFromCStr(tempdir.path.c_str()));
  runtime.listAdd(sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import somedir
)")
                   .isError());
  Object somedir(&scope, moduleAt(&runtime, "__main__", "somedir"));
  ASSERT_TRUE(somedir.isModule());
}

TEST(ImportlibTest, ImportsModuleWithInitPy) {
  TemporaryDirectory tempdir;
  std::string module_dir = tempdir.path + "bar";
  ASSERT_EQ(mkdir(module_dir.c_str(), S_IRWXU), 0);
  writeFile(module_dir + "/__init__.py", "y = 13");

  Runtime runtime;
  HandleScope scope;
  List sys_path(&scope, moduleAt(&runtime, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime.newStrFromCStr(tempdir.path.c_str()));
  runtime.listAdd(sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import bar
)")
                   .isError());
  Object bar_obj(&scope, moduleAt(&runtime, "__main__", "bar"));
  ASSERT_TRUE(bar_obj.isModule());
  Module bar(&scope, *bar_obj);
  Str str_y(&scope, runtime.newStrFromCStr("y"));
  Object y(&scope, runtime.moduleAt(bar, str_y));
  EXPECT_TRUE(isIntEqualsWord(*y, 13));
}

TEST(ImportlibTest, SubModuleImport) {
  TemporaryDirectory tempdir;
  std::string module_dir = tempdir.path + "baz";
  ASSERT_EQ(mkdir(module_dir.c_str(), S_IRWXU), 0);
  writeFile(module_dir + "/blam.py", "z = 7");

  Runtime runtime;
  HandleScope scope;
  List sys_path(&scope, moduleAt(&runtime, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime.newStrFromCStr(tempdir.path.c_str()));
  runtime.listAdd(sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import baz.blam
)")
                   .isError());
  Object baz_obj(&scope, moduleAt(&runtime, "__main__", "baz"));
  ASSERT_TRUE(baz_obj.isModule());
  Module baz(&scope, *baz_obj);
  Str blam_str(&scope, runtime.newStrFromCStr("blam"));
  Object blam_obj(&scope, runtime.moduleAt(baz, blam_str));
  ASSERT_TRUE(blam_obj.isModule());
  Module blam(&scope, *blam_obj);

  Str str_z(&scope, runtime.newStrFromCStr("z"));
  Object z(&scope, runtime.moduleAt(blam, str_z));
  EXPECT_TRUE(isIntEqualsWord(*z, 7));
}

TEST(ImportlibTest, FromImportsWithRelativeName) {
  TemporaryDirectory tempdir;
  writeFile(tempdir.path + "a.py", "val = 'top val'");
  std::string submodule = tempdir.path + "submodule";
  ASSERT_EQ(mkdir(submodule.c_str(), S_IRWXU), 0);
  writeFile(submodule + "/__init__.py", "from .a import val");
  writeFile(submodule + "/a.py", "val = 'submodule val'");

  Runtime runtime;
  HandleScope scope;
  List sys_path(&scope, moduleAt(&runtime, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime.newStrFromCStr(tempdir.path.c_str()));
  runtime.listAdd(sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import a
import submodule
from submodule.a import val
)")
                   .isError());

  Object top_val(&scope, moduleAt(&runtime, "a", "val"));
  EXPECT_TRUE(isStrEqualsCStr(*top_val, "top val"));
  Object subdir_val(&scope, moduleAt(&runtime, "submodule", "val"));
  EXPECT_TRUE(isStrEqualsCStr(*subdir_val, "submodule val"));
  Object main_val_from_submodule(&scope, moduleAt(&runtime, "__main__", "val"));
  EXPECT_TRUE(isStrEqualsCStr(*main_val_from_submodule, "submodule val"));
}

TEST(ImportlibTest, ImportFindsDefaultModules) {
  Runtime runtime;
  EXPECT_FALSE(runFromCStr(&runtime, "import stat").isError());
}

TEST(ImportlibTest, SysMetaPathIsList) {
  const char* src = R"(
import sys

meta_path = sys.meta_path
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object meta_path(&scope, moduleAt(&runtime, main, "meta_path"));
  ASSERT_TRUE(meta_path.isList());
}

}  // namespace python
