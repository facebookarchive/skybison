#include "gtest/gtest.h"

#include <stdlib.h>
#include <unistd.h>

#include "faulthandler-module.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using FaulthandlerModuleTest = RuntimeFixture;

TEST_F(FaulthandlerModuleTest,
       DumpTracebackWithNonIntAllThreadsRaisesTypeError) {
  HandleScope scope(thread_);
  Object fd(&scope, SmallInt::fromWord(2));
  Object all_threads(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FaulthandlerModule::dumpTraceback, fd, all_threads),
      LayoutId::kTypeError, "an integer is required (got type NoneType)"));
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
  char expected_prefix[] = R"(Traceback (most recent call last):
  File "", in <anonymous>  <native function at 0x)";

  word prefix_length = std::strlen(expected_prefix);
  ASSERT_GT(length, prefix_length);
  EXPECT_EQ(std::memcmp(actual.get(), expected_prefix, prefix_length), 0);

  close(fd);
  unlink(name.get());
}

}  // namespace py
