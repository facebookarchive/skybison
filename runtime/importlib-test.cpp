#include "gtest/gtest.h"

#include <fstream>

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ImportlibTest, SimpleImport) {
  TemporaryDirectory tempdir;
  writeFile(tempdir.path + "foo.py", "x = 42");

  Runtime runtime;
  HandleScope scope;
  List sys_path(&scope, moduleAt(&runtime, "sys", "path"));
  sys_path.setNumItems(0);
  Str temp_dir_str(&scope, runtime.newStrFromCStr(tempdir.path.c_str()));
  runtime.listAdd(sys_path, temp_dir_str);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import foo
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
