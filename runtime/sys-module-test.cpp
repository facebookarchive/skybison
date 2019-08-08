#include "gtest/gtest.h"

#include <sys/utsname.h>

#include <cstdio>
#include <cstdlib>

#include "runtime.h"
#include "str-builtins.h"
#include "sys-module.h"
#include "test-utils.h"
#include "under-builtins-module.h"

namespace python {

using namespace testing;

using SysModuleTest = RuntimeFixture;

TEST_F(SysModuleTest,
       ExcInfoWhileExceptionNotBeingHandledReturnsTupleOfThreeNone) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
result = sys.exc_info()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  EXPECT_EQ(result.at(0), NoneType::object());
  EXPECT_EQ(result.at(1), NoneType::object());
  EXPECT_EQ(result.at(2), NoneType::object());
}

TEST_F(
    SysModuleTest,
    ExcInfoWhileExceptionNotBeingHandledAfterExceptionIsRaisedReturnsTupleOfThreeNone) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
try:
  raise IndexError(3)
except:
  pass
result = sys.exc_info()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  EXPECT_EQ(result.at(0), NoneType::object());
  EXPECT_EQ(result.at(1), NoneType::object());
  EXPECT_EQ(result.at(2), NoneType::object());
}

TEST_F(SysModuleTest,
       ExcInfoWhileExceptionBeingHandledReturnsTupleOfTypeValueTraceback) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
try:
  raise IndexError(4)
except:
  result = sys.exc_info()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);

  Type expected_type(&scope, runtime_.typeAt(LayoutId::kIndexError));
  EXPECT_EQ(result.at(0), expected_type);

  ASSERT_TRUE(result.at(1).isIndexError());
  IndexError actual_value(&scope, result.at(1));
  ASSERT_TRUE(actual_value.args().isTuple());
  Tuple value_args(&scope, actual_value.args());
  ASSERT_EQ(value_args.length(), 1);
  EXPECT_EQ(value_args.at(0), SmallInt::fromWord(4));

  // TODO(T42241510): Traceback support isn't implemented yet. Once it's ready,
  // inspect result.at(2) here.
}

TEST_F(SysModuleTest, ExcInfoReturnsInfoOfExceptionCurrentlyBeingHandled) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
try:
  raise IndexError(4)
except:
  try:
    raise IndexError(5)
  except:
    result = sys.exc_info()
)")
                   .isError());
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);

  Type expected_type(&scope, runtime_.typeAt(LayoutId::kIndexError));
  EXPECT_EQ(result.at(0), expected_type);

  ASSERT_TRUE(result.at(1).isIndexError());
  IndexError actual_value(&scope, result.at(1));
  ASSERT_TRUE(actual_value.args().isTuple());
  Tuple value_args(&scope, actual_value.args());
  ASSERT_EQ(value_args.length(), 1);
  EXPECT_EQ(value_args.at(0), SmallInt::fromWord(5));

  // TODO(T42241510): Traceback support isn't implemented yet. Once it's ready,
  // inspect result.at(2) here.
}

TEST_F(SysModuleTest, ExecutableIsValid) {
  HandleScope scope(thread_);
  Object executable_obj(&scope, moduleAtByCStr(&runtime_, "sys", "executable"));
  ASSERT_TRUE(executable_obj.isStr());
  Str executable(&scope, *executable_obj);
  ASSERT_TRUE(executable.charLength() > 0);
  EXPECT_TRUE(executable.charAt(0) == '/');
  Str test_executable_name(&scope, runtime_.newStrFromCStr("python-tests"));
  Int find_result(&scope,
                  strFind(executable, test_executable_name, 0, kMaxWord));
  EXPECT_FALSE(find_result.isNegative());
}

TEST_F(SysModuleTest, SysExit) {
  const char* src = R"(
import sys
sys.exit()
)";
  ASSERT_EXIT(static_cast<void>(runFromCStr(&runtime_, src)),
              ::testing::ExitedWithCode(0), "");
}

TEST_F(SysModuleTest, SysExitCode) {  // pystone dependency
  const char* src = R"(
import sys
sys.exit(100)
)";
  ASSERT_EXIT(static_cast<void>(runFromCStr(&runtime_, src)),
              ::testing::ExitedWithCode(100), "");
}

TEST_F(SysModuleTest, SysExitWithNonCodeReturnsOne) {  // pystone dependency
  const char* src = R"(
import sys
sys.exit("barf")
)";
  ASSERT_EXIT(static_cast<void>(runFromCStr(&runtime_, src)),
              ::testing::ExitedWithCode(1), "barf");
}

TEST_F(SysModuleTest, SysExitWithFalseReturnsZero) {
  const char* src = R"(
import sys
sys.exit(False)
)";
  ASSERT_EXIT(static_cast<void>(runFromCStr(&runtime_, src)),
              ::testing::ExitedWithCode(0), "");
}

TEST_F(SysModuleTest, Platform) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
sysname = sys.platform
)")
                   .isError());
  Object sysname(&scope, mainModuleAt(&runtime_, "sysname"));
  ASSERT_TRUE(sysname.isStr());
  struct utsname name;
  ASSERT_EQ(uname(&name), 0);
  bool is_darwin = !std::strcmp(name.sysname, "Darwin");
  bool is_linux = !std::strcmp(name.sysname, "Linux");
  ASSERT_TRUE(is_darwin || is_linux);
  if (is_darwin) {
    EXPECT_TRUE(Str::cast(*sysname).equalsCStr("darwin"));
  }
  if (is_linux) {
    EXPECT_TRUE(Str::cast(*sysname).equalsCStr("linux"));
  }
}

TEST_F(SysModuleTest, PathImporterCache) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
result = sys.path_importer_cache
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result.isDict());
}

TEST_F(SysModuleTest, BuiltinModuleNames) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
builtin_names = sys.builtin_module_names
)")
                   .isError());
  Object builtins(&scope, mainModuleAt(&runtime_, "builtin_names"));
  ASSERT_TRUE(builtins.isTuple());

  // Test that builtin list is greater than 0
  Tuple builtins_tuple(&scope, *builtins);
  EXPECT_GT(builtins_tuple.length(), 0);

  // Test that sys and _stat are both in the builtin list
  bool builtin_sys = false;
  bool builtin__stat = false;
  for (int i = 0; i < builtins_tuple.length(); i++) {
    builtin_sys |= Str::cast(builtins_tuple.at(i)).equalsCStr("sys");
    builtin__stat |= Str::cast(builtins_tuple.at(i)).equalsCStr("_stat");
  }
  EXPECT_TRUE(builtin_sys);
  EXPECT_TRUE(builtin__stat);
}

TEST_F(SysModuleTest, FlagsVerbose) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
result = sys.flags.verbose
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(SysModuleTest, MaxsizeIsMaxWord) {
  HandleScope scope(thread_);
  Object maxsize(&scope, moduleAtByCStr(&runtime_, "sys", "maxsize"));
  EXPECT_TRUE(isIntEqualsWord(*maxsize, kMaxWord));
}

TEST_F(SysModuleTest, ByteorderIsCorrectString) {
  HandleScope scope(thread_);
  Object byteorder(&scope, moduleAtByCStr(&runtime_, "sys", "byteorder"));
  EXPECT_TRUE(isStrEqualsCStr(
      *byteorder, endian::native == endian::little ? "little" : "big"));
}

}  // namespace python
