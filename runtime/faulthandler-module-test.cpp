#include "faulthandler-module.h"

#include <stdlib.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using FaulthandlerModuleDeathTest = RuntimeFixture;
using FaulthandlerModuleTest = RuntimeFixture;

TEST_F(FaulthandlerModuleDeathTest, UnderSigabrtRaisesSIGABRT) {
  HandleScope scope(thread_);
  Object stderr(&scope, runtime_->sysStderr().value());
  Object false_obj(&scope, Bool::falseObj());
  ASSERT_EQ(runBuiltin(FaulthandlerModule::enable, stderr, false_obj),
            NoneType::object());

  const char* expected = "Fatal Python error: Aborted";
  EXPECT_EXIT(runBuiltin(FaulthandlerModule::underSigabrt),
              ::testing::KilledBySignal(SIGABRT), expected);
}

TEST_F(FaulthandlerModuleTest,
       DumpTracebackWithNonIntAllThreadsRaisesTypeError) {
  HandleScope scope(thread_);
  Object fd(&scope, SmallInt::fromWord(2));
  Object all_threads(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FaulthandlerModule::dumpTraceback, fd, all_threads),
      LayoutId::kTypeError,
      "'<anonymous>' requires a 'int' object but got 'NoneType'"));
}

TEST_F(FaulthandlerModuleTest, DumpTracebackWritesToFileDescriptor) {
  HandleScope scope(thread_);
  int fd;
  std::unique_ptr<char[]> name(OS::temporaryFile("traceback", &fd));
  Object file(&scope, SmallInt::fromWord(fd));
  Object all_threads(&scope, Bool::falseObj());
  Object result(
      &scope, runBuiltin(FaulthandlerModule::dumpTraceback, file, all_threads));
  ASSERT_TRUE(result.isNoneType());

  word length;
  std::unique_ptr<char[]> actual(OS::readFile(name.get(), &length));
  char expected[] = R"(Stack (most recent call first):
  File "", line ??? in <anonymous>
)";

  word expected_length = std::strlen(expected);
  ASSERT_EQ(length, expected_length);
  EXPECT_EQ(std::memcmp(actual.get(), expected, expected_length), 0);

  close(fd);
  unlink(name.get());
}

}  // namespace py
