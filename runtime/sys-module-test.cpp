#include "gtest/gtest.h"

#include <sys/utsname.h>

#include <cstdio>
#include <cstdlib>

#include "runtime.h"
#include "str-builtins.h"
#include "sys-module.h"
#include "test-utils.h"

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
  Object result_obj(&scope, moduleAt(&runtime_, "__main__", "result"));
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
  Object result_obj(&scope, moduleAt(&runtime_, "__main__", "result"));
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
  Object result_obj(&scope, moduleAt(&runtime_, "__main__", "result"));
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
  Object result_obj(&scope, moduleAt(&runtime_, "__main__", "result"));
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
  Object executable_obj(&scope, moduleAt(&runtime_, "sys", "executable"));
  ASSERT_TRUE(executable_obj.isStr());
  Str executable(&scope, *executable_obj);
  ASSERT_TRUE(executable.length() > 0);
  EXPECT_TRUE(executable.charAt(0) == '/');
  Str test_executable_name(&scope, runtime_.newStrFromCStr("python-tests"));
  Int find_result(&scope,
                  strFind(executable, test_executable_name, 0, kMaxWord));
  EXPECT_FALSE(find_result.isNegative());
}

TEST_F(SysModuleTest, StderrWriteWritesToStderrFile) {
  EXPECT_EQ(compileAndRunToStderrString(&runtime_, R"(
import sys
sys.stderr.write("Bonjour")
)"),
            "Bonjour");
}

TEST_F(SysModuleTest, StdoutWriteWritesToStdoutFile) {
  EXPECT_EQ(compileAndRunToString(&runtime_, R"(
import sys
sys.stdout.write("Hola")
)"),
            "Hola");
}

TEST_F(SysModuleTest, SysArgvProgArg) {  // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

for x in sys.argv:
  print(x)
)";
  const char* argv[2];
  argv[0] = "./python";  // program
  argv[1] = "SysArgv";   // script
  runtime_.setArgv(thread_, 2, argv);
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "1\nSysArgv\n");
}

TEST_F(SysModuleTest, SysArgvMultiArgs) {  // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

print(sys.argv[1])

for x in sys.argv:
  print(x)
)";
  const char* argv[3];
  argv[0] = "./python";  // program
  argv[1] = "SysArgv";   // script
  argv[2] = "200";       // argument
  runtime_.setArgv(thread_, 3, argv);
  std::string output = compileAndRunToString(&runtime_, src);
  EXPECT_EQ(output, "2\n200\nSysArgv\n200\n");
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
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object sysname(&scope, moduleAt(&runtime_, main, "sysname"));
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
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(result.isDict());
}

TEST_F(SysModuleTest, BuiltinModuleNames) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys
builtin_names = sys.builtin_module_names
)")
                   .isError());
  Module main(&scope, findModule(&runtime_, "__main__"));
  Object builtins(&scope, moduleAt(&runtime_, main, "builtin_names"));
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
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 0));
}

TEST_F(SysModuleTest, MaxsizeIsMaxWord) {
  HandleScope scope(thread_);
  Object maxsize(&scope, moduleAt(&runtime_, "sys", "maxsize"));
  EXPECT_TRUE(isIntEqualsWord(*maxsize, kMaxWord));
}

TEST_F(SysModuleTest, ByteorderIsCorrectString) {
  HandleScope scope(thread_);
  Object byteorder(&scope, moduleAt(&runtime_, "sys", "byteorder"));
  EXPECT_TRUE(isStrEqualsCStr(
      *byteorder, endian::native == endian::little ? "little" : "big"));
}

TEST_F(SysModuleTest, UnderFdFlushFlushesFile) {
  HandleScope scope(thread_);
  char buf[128];
  std::memset(buf, 0, sizeof(buf));
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  char stream_buf[64];
  int res = setvbuf(out, stream_buf, _IOFBF, sizeof(stream_buf));
  ASSERT_EQ(res, 0);
  runtime_.setStdoutFile(out);
  Bytes bytes(&scope, runtime_.newBytesWithAll(
                          View<byte>(reinterpret_cast<const byte*>("a"), 1)));
  Object under_stdout_fd(&scope, moduleAt(&runtime_, "sys", "_stdout_fd"));
  runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes);
  EXPECT_EQ(buf[0], 0);
  Object result(&scope, runBuiltin(SysModule::underFdFlush, under_stdout_fd));
  EXPECT_TRUE(result.isNoneType());
  EXPECT_EQ(buf[0], 'a');
  std::fclose(out);
}

TEST_F(SysModuleTest, UnderFdFlushWithNonIntFdRaisesTypeError) {
  HandleScope scope(thread_);
  Object fd(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdFlush, fd), LayoutId::kTypeError,
      "'<anonymous>' requires a 'int' object but got 'NoneType'"));
}

TEST_F(SysModuleTest, UnderFdFlushWithInvalidFdRaisesValueError) {
  HandleScope scope(thread_);
  Object fd(&scope, runtime_.newInt(0xbadf00d));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdFlush, fd), LayoutId::kValueError,
      "'<anonymous>' called with unknown file descriptor"));
}

TEST_F(SysModuleTest, UnderFdFlushOnFailureRaisesOSError) {
  HandleScope scope(thread_);
  char buf[1] = "";
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  char stream_buf[64];
  int res = setvbuf(out, stream_buf, _IOFBF, sizeof(stream_buf));
  ASSERT_EQ(res, 0);
  runtime_.setStdoutFile(out);
  Object under_stdout_fd(&scope, moduleAt(&runtime_, "sys", "_stdout_fd"));
  const byte aaa[] = {'a', 'a', 'a'};
  Object bytes(&scope, runtime_.newBytesWithAll(aaa));
  runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes);
  // The write may or may not have triggered an error depending on the libc.
  thread_->clearPendingException();
  EXPECT_TRUE(raised(runBuiltin(SysModule::underFdFlush, under_stdout_fd),
                     LayoutId::kOSError));
  std::fclose(out);
}

TEST_F(SysModuleTest, UnderFdWriteWithStderrFdStrWritesToStderrFile) {
  HandleScope scope(thread_);
  char buf[128];
  std::memset(buf, 0, sizeof(buf));
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  runtime_.setStderrFile(out);
  runtime_.setStdoutFile(nullptr);
  Object under_stderr_fd(&scope, moduleAt(&runtime_, "sys", "_stderr_fd"));
  ASSERT_TRUE(under_stderr_fd.isSmallInt());
  const byte hi[] = {'H', 'i', '!'};
  Object bytes(&scope, runtime_.newBytesWithAll(hi));
  Object result(&scope,
                runBuiltin(SysModule::underFdWrite, under_stderr_fd, bytes));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
  std::fclose(out);
  buf[sizeof(buf) - 1] = '\0';
  EXPECT_EQ(std::strcmp(buf, "Hi!"), 0);
}

TEST_F(SysModuleTest, UnderFdWriteWithStdoutFdStrWritesToStdoutFile) {
  HandleScope scope(thread_);
  char buf[128];
  std::memset(buf, 0, sizeof(buf));
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  runtime_.setStderrFile(nullptr);
  runtime_.setStdoutFile(out);
  Object under_stdout_fd(&scope, moduleAt(&runtime_, "sys", "_stdout_fd"));
  ASSERT_TRUE(under_stdout_fd.isSmallInt());
  const byte yo[] = {'Y', 'o', '!'};
  Object bytes(&scope, runtime_.newBytesWithAll(yo));
  Object result(&scope,
                runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
  std::fclose(out);
  buf[sizeof(buf) - 1] = '\0';
  EXPECT_EQ(std::strcmp(buf, "Yo!"), 0);
}

TEST_F(SysModuleTest, UnderFdWriteWithNonIntFdRaisesTypeError) {
  HandleScope scope(thread_);
  Object fd(&scope, NoneType::object());
  Object bytes(&scope, runtime_.newBytes(0, 0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdWrite, fd, bytes), LayoutId::kTypeError,
      "'<anonymous>' requires a 'int' object but got 'NoneType'"));
}

TEST_F(SysModuleTest, UnderFdWriteWithInvalidFdRaisesValueError) {
  HandleScope scope(thread_);
  Object fd(&scope, runtime_.newInt(0xbadf00d));
  Object bytes(&scope, runtime_.newBytes(0, 0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdWrite, fd, bytes), LayoutId::kValueError,
      "'<anonymous>' called with unknown file descriptor"));
}

TEST_F(SysModuleTest, UnderFdWriteOnFailureRaisesOSError) {
  HandleScope scope(thread_);
  char buf[1] = "";
  FILE* out = fmemopen(buf, sizeof(buf), "r");
  ASSERT_NE(out, nullptr);
  int res = setvbuf(out, nullptr, _IONBF, 0);
  ASSERT_EQ(res, 0);
  runtime_.setStdoutFile(out);
  Object under_stdout_fd(&scope, moduleAt(&runtime_, "sys", "_stdout_fd"));
  const byte hi[] = {'H', 'i', '!'};
  Object bytes(&scope, runtime_.newBytesWithAll(hi));
  Object result(&scope,
                runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes));
  EXPECT_TRUE(raised(*result, LayoutId::kOSError));
  std::fclose(out);
}

}  // namespace python
