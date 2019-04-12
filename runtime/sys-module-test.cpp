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

TEST(SysModuleTest,
     ExcInfoWhileExceptionNotBeingHandledReturnsTupleOfThreeNone) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys
result = sys.exc_info()
)")
                   .isError());
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  EXPECT_EQ(result.at(0), NoneType::object());
  EXPECT_EQ(result.at(1), NoneType::object());
  EXPECT_EQ(result.at(2), NoneType::object());
}

TEST(
    SysModuleTest,
    ExcInfoWhileExceptionNotBeingHandledAfterExceptionIsRaisedReturnsTupleOfThreeNone) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys
try:
  raise IndexError(3)
except:
  pass
result = sys.exc_info()
)")
                   .isError());
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  EXPECT_EQ(result.at(0), NoneType::object());
  EXPECT_EQ(result.at(1), NoneType::object());
  EXPECT_EQ(result.at(2), NoneType::object());
}

TEST(SysModuleTest,
     ExcInfoWhileExceptionBeingHandledReturnsTupleOfTypeValueTraceback) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys
try:
  raise IndexError(4)
except:
  result = sys.exc_info()
)")
                   .isError());
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);

  Type expected_type(&scope, runtime.typeAt(LayoutId::kIndexError));
  EXPECT_EQ(result.at(0), expected_type);

  ASSERT_TRUE(result.at(1).isIndexError());
  IndexError actual_value(&scope, result.at(1));
  ASSERT_TRUE(actual_value.args().isTuple());
  Tuple value_args(&scope, actual_value.args());
  ASSERT_EQ(value_args.length(), 1);
  EXPECT_EQ(value_args.at(0), SmallInt::fromWord(4));

  // TODO(T42241510): Traceback is unimplemented yet. Once it's implemented,
  // add a EXPECT_EQ(result.at(2), traceback).
  EXPECT_EQ(result.at(2), NoneType::object());
}

TEST(SysModuleTest, ExcInfoReturnsInfoOfExceptionCurrentlyBeingHandled) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);

  Type expected_type(&scope, runtime.typeAt(LayoutId::kIndexError));
  EXPECT_EQ(result.at(0), expected_type);

  ASSERT_TRUE(result.at(1).isIndexError());
  IndexError actual_value(&scope, result.at(1));
  ASSERT_TRUE(actual_value.args().isTuple());
  Tuple value_args(&scope, actual_value.args());
  ASSERT_EQ(value_args.length(), 1);
  EXPECT_EQ(value_args.at(0), SmallInt::fromWord(5));

  // TODO(T42241510): Traceback is unimplemented yet. Once it's implemented,
  // add a EXPECT_EQ(result.at(2), traceback).
  EXPECT_EQ(result.at(2), NoneType::object());
}

TEST(SysModuleTest, ExecutableIsValid) {
  Runtime runtime;
  HandleScope scope;
  Object executable_obj(&scope, moduleAt(&runtime, "sys", "executable"));
  ASSERT_TRUE(executable_obj.isStr());
  Str executable(&scope, *executable_obj);
  ASSERT_TRUE(executable.length() > 0);
  EXPECT_TRUE(executable.charAt(0) == '/');
  Str test_executable_name(&scope, runtime.newStrFromCStr("python-tests"));
  Object find_result(&scope,
                     strFind(executable, test_executable_name, 0, kMaxWord));
  ASSERT_TRUE(find_result.isInt());
  EXPECT_FALSE(RawInt::cast(*find_result).isNegative());
}

TEST(SysModuleTest,
     GetSizeOfWithObjectWhoseDunderSizeOfReturningNonIntegerRaisesTypeError) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys

class C():
  def __sizeof__(self):  return 'not integer'

exc_raised = False
try:
  sys.getsizeof(C())
except TypeError:
  exc_raised = True
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "exc_raised"), Bool::trueObj());
}

TEST(
    SysModuleTest,
    GetSizeOfWithObjectWhoseDunderSizeOfReturningNonIntegerWithDefaultValueReturnsDefaultValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys

class C():
  def __sizeof__(self):  return 'not integer'

result = sys.getsizeof(C(), 10)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), SmallInt::fromWord(10));
}

TEST(
    SysModuleTest,
    GetSizeOfWithObjectWhoseDunderSizeOfReturningNegativeIntWithDefaultValueRaisesValueError) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys

class C():
  def __sizeof__(self):  return -1

exc_raised = False
try:
  sys.getsizeof(C(), 10)
except ValueError:
  exc_raised = True
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "exc_raised"), Bool::trueObj());
}

TEST(SysModuleTest,
     GetSizeOfWithObjectWhoseDunderSizeReturningPositiveIntReturnsThatInt) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys

class C():
  def __sizeof__(self):  return 100

result = sys.getsizeof(C())
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), SmallInt::fromWord(100));
}

TEST(
    SysModuleTest,
    GetSizeOfWithObjectWhoseDunderSizeReturningPositiveIntWithDefaultValueReturnsThatInt) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys

class C():
  def __sizeof__(self):  return 100

result = sys.getsizeof(C(), 10)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), SmallInt::fromWord(100));
}

TEST(SysModuleTest, StderrWriteWritesToStderrFile) {
  Runtime runtime;
  EXPECT_EQ(compileAndRunToStderrString(&runtime, R"(
import sys
sys.stderr.write("Bonjour")
)"),
            "Bonjour");
}

TEST(SysModuleTest, StdoutWriteWritesToStdoutFile) {
  Runtime runtime;
  EXPECT_EQ(compileAndRunToString(&runtime, R"(
import sys
sys.stdout.write("Hola")
)"),
            "Hola");
}

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

TEST(SysModuleTest, SysExitWithFalseReturnsZero) {
  const char* src = R"(
import sys
sys.exit(False)
)";
  Runtime runtime;
  ASSERT_EXIT(runFromCStr(&runtime, src), ::testing::ExitedWithCode(0), "");
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
  bool is_darwin = !std::strcmp(name.sysname, "Darwin");
  bool is_linux = !std::strcmp(name.sysname, "Linux");
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

TEST(SysModuleTest, MaxsizeIsMaxWord) {
  Runtime runtime;
  HandleScope scope;
  Object maxsize(&scope, moduleAt(&runtime, "sys", "maxsize"));
  EXPECT_TRUE(isIntEqualsWord(*maxsize, kMaxWord));
}

TEST(SysModuleTest, ByteorderIsCorrectString) {
  Runtime runtime;
  HandleScope scope;
  Object byteorder(&scope, moduleAt(&runtime, "sys", "byteorder"));
  EXPECT_TRUE(isStrEqualsCStr(
      *byteorder, endian::native == endian::little ? "little" : "big"));
}

TEST(SysModuleTest, UnderFdFlushFlushesFile) {
  Runtime runtime;
  HandleScope scope;
  char buf[128];
  std::memset(buf, 0, sizeof(buf));
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  char stream_buf[64];
  int res = setvbuf(out, stream_buf, _IOFBF, sizeof(stream_buf));
  ASSERT_EQ(res, 0);
  runtime.setStdoutFile(out);
  Bytes bytes(&scope, runtime.newBytesWithAll(
                          View<byte>(reinterpret_cast<const byte*>("a"), 1)));
  Object under_stdout_fd(&scope, moduleAt(&runtime, "sys", "_stdout_fd"));
  runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes);
  EXPECT_EQ(buf[0], 0);
  Object result(&scope, runBuiltin(SysModule::underFdFlush, under_stdout_fd));
  EXPECT_TRUE(result.isNoneType());
  EXPECT_EQ(buf[0], 'a');
  std::fclose(out);
}

TEST(SysModuleTest, UnderFdFlushWithNonIntFdRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object fd(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdFlush, fd), LayoutId::kTypeError,
      "'<anonymous>' requires a 'int' object but got 'NoneType'"));
}

TEST(SysModuleTest, UnderFdFlushWithInvalidFdRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  Object fd(&scope, runtime.newInt(0xbadf00d));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdFlush, fd), LayoutId::kValueError,
      "'<anonymous>' called with unknown file descriptor"));
}

TEST(SysModuleTest, UnderFdFlushOnFailureRaisesOSError) {
  Runtime runtime;
  HandleScope scope;
  char buf[1] = "";
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  char stream_buf[64];
  int res = setvbuf(out, stream_buf, _IOFBF, sizeof(stream_buf));
  ASSERT_EQ(res, 0);
  runtime.setStdoutFile(out);
  Object under_stdout_fd(&scope, moduleAt(&runtime, "sys", "_stdout_fd"));
  const byte aaa[] = {'a', 'a', 'a'};
  Object bytes(&scope, runtime.newBytesWithAll(aaa));
  runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes);
  // The write may or may not have triggered an error depending on the libc.
  Thread::current()->clearPendingException();
  EXPECT_TRUE(raised(runBuiltin(SysModule::underFdFlush, under_stdout_fd),
                     LayoutId::kOSError));
  std::fclose(out);
}

TEST(SysModuleTest, UnderFdWriteWithStderrFdStrWritesToStderrFile) {
  Runtime runtime;
  HandleScope scope;
  char buf[128];
  std::memset(buf, 0, sizeof(buf));
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  runtime.setStderrFile(out);
  runtime.setStdoutFile(nullptr);
  Object under_stderr_fd(&scope, moduleAt(&runtime, "sys", "_stderr_fd"));
  ASSERT_TRUE(under_stderr_fd.isSmallInt());
  const byte hi[] = {'H', 'i', '!'};
  Object bytes(&scope, runtime.newBytesWithAll(hi));
  Object result(&scope,
                runBuiltin(SysModule::underFdWrite, under_stderr_fd, bytes));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
  std::fclose(out);
  buf[sizeof(buf) - 1] = '\0';
  EXPECT_EQ(std::strcmp(buf, "Hi!"), 0);
}

TEST(SysModuleTest, UnderFdWriteWithStdoutFdStrWritesToStdoutFile) {
  Runtime runtime;
  HandleScope scope;
  char buf[128];
  std::memset(buf, 0, sizeof(buf));
  FILE* out = fmemopen(buf, sizeof(buf), "w");
  ASSERT_NE(out, nullptr);
  runtime.setStderrFile(nullptr);
  runtime.setStdoutFile(out);
  Object under_stdout_fd(&scope, moduleAt(&runtime, "sys", "_stdout_fd"));
  ASSERT_TRUE(under_stdout_fd.isSmallInt());
  const byte yo[] = {'Y', 'o', '!'};
  Object bytes(&scope, runtime.newBytesWithAll(yo));
  Object result(&scope,
                runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
  std::fclose(out);
  buf[sizeof(buf) - 1] = '\0';
  EXPECT_EQ(std::strcmp(buf, "Yo!"), 0);
}

TEST(SysModuleTest, UnderFdWriteWithNonIntFdRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object fd(&scope, NoneType::object());
  Object bytes(&scope, runtime.newBytes(0, 0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdWrite, fd, bytes), LayoutId::kTypeError,
      "'<anonymous>' requires a 'int' object but got 'NoneType'"));
}

TEST(SysModuleTest, UnderFdWriteWithInvalidFdRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  Object fd(&scope, runtime.newInt(0xbadf00d));
  Object bytes(&scope, runtime.newBytes(0, 0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SysModule::underFdWrite, fd, bytes), LayoutId::kValueError,
      "'<anonymous>' called with unknown file descriptor"));
}

TEST(SysModuleTest, UnderFdWriteOnFailureRaisesOSError) {
  Runtime runtime;
  HandleScope scope;
  char buf[1] = "";
  FILE* out = fmemopen(buf, sizeof(buf), "r");
  ASSERT_NE(out, nullptr);
  int res = setvbuf(out, nullptr, _IONBF, 0);
  ASSERT_EQ(res, 0);
  runtime.setStdoutFile(out);
  Object under_stdout_fd(&scope, moduleAt(&runtime, "sys", "_stdout_fd"));
  const byte hi[] = {'H', 'i', '!'};
  Object bytes(&scope, runtime.newBytesWithAll(hi));
  Object result(&scope,
                runBuiltin(SysModule::underFdWrite, under_stdout_fd, bytes));
  EXPECT_TRUE(raised(*result, LayoutId::kOSError));
  std::fclose(out);
}

}  // namespace python
