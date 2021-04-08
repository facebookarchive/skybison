#include <stdlib.h>

#include <csignal>

#include "gtest/gtest.h"

#include "builtins.h"
#include "file.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using FaulthandlerModuleDeathTest = RuntimeFixture;
using FaulthandlerModuleTest = RuntimeFixture;

TEST_F(FaulthandlerModuleTest, DumpTracebackWritesToFileDescriptor) {
  TemporaryDirectory tempdir;
  std::string temp_file = tempdir.path + "traceback";
  int fd =
      File::open(temp_file.c_str(), File::kCreate | File::kWriteOnly, 0777);
  ASSERT_NE(fd, -1);

  HandleScope scope(thread_);
  Object file(&scope, SmallInt::fromWord(fd));
  Object all_threads(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(FUNC(faulthandler, dump_traceback), file,
                                   all_threads));
  ASSERT_TRUE(result.isNoneType());
  ASSERT_EQ(File::close(fd), 0);

  word length;
  FILE* fp = std::fopen(temp_file.c_str(), "r");
  unique_c_ptr<byte> actual(OS::readFile(fp, &length));
  std::fclose(fp);
  char expected[] = R"(Stack (most recent call first):
  File "", line ??? in <anonymous>
)";

  word expected_length = std::strlen(expected);
  ASSERT_EQ(length, expected_length);
  EXPECT_EQ(std::memcmp(actual.get(), expected, expected_length), 0);
}

TEST_F(FaulthandlerModuleTest, DumpTracebackHandlesPendingSignal) {
  HandleScope scope(thread_);
  Object file(&scope, SmallInt::fromWord(File::kStderr));
  Object all_threads(&scope, Bool::falseObj());

  runtime_->setPendingSignal(thread_, SIGINT);
  EXPECT_TRUE(
      raised(runBuiltin(FUNC(faulthandler, dump_traceback), file, all_threads),
             LayoutId::kKeyboardInterrupt));
}

}  // namespace testing
}  // namespace py
