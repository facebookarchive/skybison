#include <stdlib.h>
#include <unistd.h>

#include "gtest/gtest.h"

#include "builtins.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using FaulthandlerModuleDeathTest = RuntimeFixture;
using FaulthandlerModuleTest = RuntimeFixture;

TEST_F(FaulthandlerModuleTest,
       DumpTracebackWithNonIntAllThreadsRaisesTypeError) {
  HandleScope scope(thread_);
  Object fd(&scope, SmallInt::fromWord(2));
  Object all_threads(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FUNC(faulthandler, dump_traceback), fd, all_threads),
      LayoutId::kTypeError,
      "'<anonymous>' requires a 'int' object but received a 'NoneType'"));
}

TEST_F(FaulthandlerModuleTest, DumpTracebackWritesToFileDescriptor) {
  HandleScope scope(thread_);
  int fd;
  std::unique_ptr<char[]> name(OS::temporaryFile("traceback", &fd));
  Object file(&scope, SmallInt::fromWord(fd));
  Object all_threads(&scope, Bool::falseObj());
  Object result(&scope, runBuiltin(FUNC(faulthandler, dump_traceback), file,
                                   all_threads));
  ASSERT_TRUE(result.isNoneType());

  word length;
  FILE* fp = std::fopen(name.get(), "r");
  std::unique_ptr<char[]> actual(OS::readFile(fp, &length));
  char expected[] = R"(Stack (most recent call first):
  File "", line ??? in <anonymous>
)";

  word expected_length = std::strlen(expected);
  ASSERT_EQ(length, expected_length);
  EXPECT_EQ(std::memcmp(actual.get(), expected, expected_length), 0);

  std::fclose(fp);
  unlink(name.get());
}

}  // namespace testing
}  // namespace py
