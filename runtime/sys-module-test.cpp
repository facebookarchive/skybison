#include "gtest/gtest.h"

#include <sys/utsname.h>

#include <cstdlib>

#include "runtime.h"
#include "sys-module.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SysModuleTest, SysArgvProgArg) {  // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

for x in sys.argv:
  print(x)
)";
  Runtime runtime;
  const char* argv[2];
  argv[0] = "./python";  // program
  argv[1] = "SysArgv";   // script
  runtime.setArgv(2, argv);
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\nSysArgv\n");
}

TEST(SysModuleTest, SysArgvMultiArgs) {  // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

print(sys.argv[1])

for x in sys.argv:
  print(x)
)";
  Runtime runtime;
  const char* argv[3];
  argv[0] = "./python";  // program
  argv[1] = "SysArgv";   // script
  argv[2] = "200";       // argument
  runtime.setArgv(3, argv);
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "2\n200\nSysArgv\n200\n");
}

TEST(SysModuleTest, SysExit) {
  const char* src = R"(
import sys
sys.exit()
)";
  Runtime runtime;
  ASSERT_EXIT(runFromCStr(&runtime, src), ::testing::ExitedWithCode(0), "");
}

TEST(SysModuleTest, SysExitCode) {  // pystone dependency
  const char* src = R"(
import sys
sys.exit(100)
)";
  Runtime runtime;
  ASSERT_EXIT(runFromCStr(&runtime, src), ::testing::ExitedWithCode(100), "");
}

TEST(SysModuleTest, SysExitWithNonCodeReturnsOne) {  // pystone dependency
  const char* src = R"(
import sys
sys.exit("barf")
)";
  Runtime runtime;
  ASSERT_EXIT(runFromCStr(&runtime, src), ::testing::ExitedWithCode(1), "barf");
}

TEST(SysModuleTest, SysStdOutErr) {  // pystone dependency
  const char* src = R"(
import sys
print(sys.stdout, sys.stderr)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");
}

TEST(SysModuleTest, Platform) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
import sys
sysname = sys.platform
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object sysname(&scope, moduleAt(&runtime, main, "sysname"));
  ASSERT_TRUE(sysname.isStr());
  struct utsname name;
  ASSERT_EQ(uname(&name), 0);
  bool is_darwin = !strcmp(name.sysname, "Darwin");
  bool is_linux = !strcmp(name.sysname, "Linux");
  ASSERT_TRUE(is_darwin || is_linux);
  if (is_darwin) {
    EXPECT_TRUE(RawStr::cast(*sysname).equalsCStr("darwin"));
  }
  if (is_linux) {
    EXPECT_TRUE(RawStr::cast(*sysname).equalsCStr("linux"));
  }
}

TEST(SysModuleTest, PathImporterCache) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
import sys
result = sys.path_importer_cache
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isDict());
}

TEST(SysModuleTest, BuiltinModuleNames) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
import sys
builtin_names = sys.builtin_module_names
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object builtins(&scope, moduleAt(&runtime, main, "builtin_names"));
  ASSERT_TRUE(builtins.isTuple());

  // Test that builtin list is greater than 0
  Tuple builtins_tuple(&scope, *builtins);
  EXPECT_GT(builtins_tuple.length(), 0);

  // Test that sys and _stat are both in the builtin list
  bool builtin_sys = false;
  bool builtin__stat = false;
  for (int i = 0; i < builtins_tuple.length(); i++) {
    builtin_sys |= RawStr::cast(builtins_tuple.at(i)).equalsCStr("sys");
    builtin__stat |= RawStr::cast(builtins_tuple.at(i)).equalsCStr("_stat");
  }
  EXPECT_TRUE(builtin_sys);
  EXPECT_TRUE(builtin__stat);
}

TEST(SysModuleTest, PathWithUnsetPythonPath) {
  unsetenv("PYTHONPATH");
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object sys(&scope, runtime.newStrFromCStr("sys"));
  runtime.importModule(thread, sys);
  Object path_obj(&scope, moduleAt(&runtime, "sys", "path"));
  ASSERT_TRUE(path_obj.isList());
  List path(&scope, *path_obj);
  ASSERT_EQ(path.numItems(), 1);
  ASSERT_TRUE(isStrEqualsCStr(path.at(0), ""));
}

TEST(SysModuleTest, PathWithEmptyPythonPath) {
  setenv("PYTHONPATH", "", 1);
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object sys(&scope, runtime.newStrFromCStr("sys"));
  runtime.importModule(thread, sys);
  Object path_obj(&scope, moduleAt(&runtime, "sys", "path"));
  ASSERT_TRUE(path_obj.isList());
  List path(&scope, *path_obj);
  ASSERT_EQ(path.numItems(), 1);
  ASSERT_TRUE(isStrEqualsCStr(path.at(0), ""));
}

TEST(SysModuleTest, PathWithPythonPath) {
  setenv("PYTHONPATH", "/foo/bar:/baz", 1);
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object sys(&scope, runtime.newStrFromCStr("sys"));
  runtime.importModule(thread, sys);
  Object path_obj(&scope, moduleAt(&runtime, "sys", "path"));
  ASSERT_TRUE(path_obj.isList());
  List path(&scope, *path_obj);
  ASSERT_EQ(path.numItems(), 3);
  ASSERT_TRUE(isStrEqualsCStr(path.at(0), ""));
  ASSERT_TRUE(isStrEqualsCStr(path.at(1), "/foo/bar"));
  ASSERT_TRUE(isStrEqualsCStr(path.at(2), "/baz"));
}

TEST(SysModuleTest, FlagsVerbose) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
import sys
result = sys.flags.verbose
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

}  // namespace python
