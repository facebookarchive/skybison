#include "gtest/gtest.h"

#include <sys/stat.h>
#include <sys/types.h>

#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using ImportlibTest = RuntimeFixture;

TEST_F(ImportlibTest, SimpleImport) {
  TemporaryDirectory tempdir;
  writeFile(tempdir.path + "foo.py", "x = 42");
  writeFile(tempdir.path + "bar.py", "x = 67");

  HandleScope scope(thread_);
  List sys_path(&scope, moduleAtByCStr(&runtime_, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime_.newStrFromCStr(tempdir.path.c_str()));
  runtime_.listAdd(thread_, sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import foo
import bar
)")
                   .isError());
  Object foo_obj(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(foo_obj.isModule());
  Module foo(&scope, *foo_obj);
  EXPECT_TRUE(isStrEqualsCStr(foo.name(), "foo"));

  Object name(&scope, moduleAtById(thread_, foo, SymbolId::kDunderName));
  EXPECT_TRUE(isStrEqualsCStr(*name, "foo"));
  Object package(&scope, moduleAtById(thread_, foo, SymbolId::kDunderPackage));
  EXPECT_TRUE(isStrEqualsCStr(*package, ""));
  Object doc(&scope, moduleAtById(thread_, foo, SymbolId::kDunderDoc));
  EXPECT_TRUE(doc.isNoneType());
  Str str_x(&scope, runtime_.newStrFromCStr("x"));
  Object x(&scope, moduleAtByStr(thread_, foo, str_x));
  EXPECT_TRUE(isIntEqualsWord(*x, 42));
}

TEST_F(ImportlibTest, ImportsEmptyModule) {
  TemporaryDirectory tempdir;
  std::string module_dir = tempdir.path + "somedir";
  ASSERT_EQ(mkdir(module_dir.c_str(), S_IRWXU), 0);

  HandleScope scope(thread_);
  List sys_path(&scope, moduleAtByCStr(&runtime_, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime_.newStrFromCStr(tempdir.path.c_str()));
  runtime_.listAdd(thread_, sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import somedir
)")
                   .isError());
  Object somedir(&scope, mainModuleAt(&runtime_, "somedir"));
  ASSERT_TRUE(somedir.isModule());
}

TEST_F(ImportlibTest, ImportsModuleWithInitPy) {
  TemporaryDirectory tempdir;
  std::string module_dir = tempdir.path + "bar";
  ASSERT_EQ(mkdir(module_dir.c_str(), S_IRWXU), 0);
  writeFile(module_dir + "/__init__.py", "y = 13");

  HandleScope scope(thread_);
  List sys_path(&scope, moduleAtByCStr(&runtime_, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime_.newStrFromCStr(tempdir.path.c_str()));
  runtime_.listAdd(thread_, sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import bar
)")
                   .isError());
  Object bar_obj(&scope, mainModuleAt(&runtime_, "bar"));
  ASSERT_TRUE(bar_obj.isModule());
  Module bar(&scope, *bar_obj);
  Str str_y(&scope, runtime_.newStrFromCStr("y"));
  Object y(&scope, moduleAtByStr(thread_, bar, str_y));
  EXPECT_TRUE(isIntEqualsWord(*y, 13));
}

TEST_F(ImportlibTest, SubModuleImport) {
  TemporaryDirectory tempdir;
  std::string module_dir = tempdir.path + "baz";
  ASSERT_EQ(mkdir(module_dir.c_str(), S_IRWXU), 0);
  writeFile(module_dir + "/blam.py", "z = 7");

  HandleScope scope(thread_);
  List sys_path(&scope, moduleAtByCStr(&runtime_, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime_.newStrFromCStr(tempdir.path.c_str()));
  runtime_.listAdd(thread_, sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import baz.blam
)")
                   .isError());
  Object baz_obj(&scope, mainModuleAt(&runtime_, "baz"));
  ASSERT_TRUE(baz_obj.isModule());
  Module baz(&scope, *baz_obj);
  Str blam_str(&scope, runtime_.newStrFromCStr("blam"));
  Object blam_obj(&scope, moduleAtByStr(thread_, baz, blam_str));
  ASSERT_TRUE(blam_obj.isModule());
  Module blam(&scope, *blam_obj);

  Str str_z(&scope, runtime_.newStrFromCStr("z"));
  Object z(&scope, moduleAtByStr(thread_, blam, str_z));
  EXPECT_TRUE(isIntEqualsWord(*z, 7));
}

TEST_F(ImportlibTest, FromImportsWithRelativeName) {
  TemporaryDirectory tempdir;
  writeFile(tempdir.path + "a.py", "val = 'top val'");
  std::string submodule = tempdir.path + "submodule";
  ASSERT_EQ(mkdir(submodule.c_str(), S_IRWXU), 0);
  writeFile(submodule + "/__init__.py", "from .a import val");
  writeFile(submodule + "/a.py", "val = 'submodule val'");

  HandleScope scope(thread_);
  List sys_path(&scope, moduleAtByCStr(&runtime_, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime_.newStrFromCStr(tempdir.path.c_str()));
  runtime_.listAdd(thread_, sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import a
import submodule
from submodule.a import val
)")
                   .isError());

  Object top_val(&scope, moduleAtByCStr(&runtime_, "a", "val"));
  EXPECT_TRUE(isStrEqualsCStr(*top_val, "top val"));
  Object subdir_val(&scope, moduleAtByCStr(&runtime_, "submodule", "val"));
  EXPECT_TRUE(isStrEqualsCStr(*subdir_val, "submodule val"));
  Object main_val_from_submodule(&scope, mainModuleAt(&runtime_, "val"));
  EXPECT_TRUE(isStrEqualsCStr(*main_val_from_submodule, "submodule val"));
}

TEST_F(ImportlibTest, ImportFindsDefaultModules) {
  EXPECT_FALSE(runFromCStr(&runtime_, "import stat").isError());
}

TEST_F(ImportlibTest, SysMetaPathIsList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys

meta_path = sys.meta_path
)")
                   .isError());
  Object meta_path(&scope, mainModuleAt(&runtime_, "meta_path"));
  ASSERT_TRUE(meta_path.isList());
}

}  // namespace python
