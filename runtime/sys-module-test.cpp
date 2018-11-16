#include "gtest/gtest.h"

#include "runtime.h"
#include "sys-module.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SysModuleTest, SysArgvProgArg) { // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

for x in sys.argv:
  print(x)
)";
  Runtime runtime;
  const char* argv[2];
  argv[0] = "./python"; // program
  argv[1] = "SysArgv"; // script
  runtime.setArgv(2, argv);
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\nSysArgv\n");
}

TEST(SysModuleTest, SysArgvMultiArgs) { // pystone dependency
  const char* src = R"(
import sys
print(len(sys.argv))

print(sys.argv[1])

for x in sys.argv:
  print(x)
)";
  Runtime runtime;
  const char* argv[3];
  argv[0] = "./python"; // program
  argv[1] = "SysArgv"; // script
  argv[2] = "200"; // argument
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
  ASSERT_EXIT(runtime.runFromCString(src), ::testing::ExitedWithCode(0), "");
}

TEST(SysModuleTest, SysExitCode) { // pystone dependency
  const char* src = R"(
import sys
sys.exit(100)
)";
  Runtime runtime;
  ASSERT_EXIT(runtime.runFromCString(src), ::testing::ExitedWithCode(100), "");
}

TEST(SysModuleTest, SysStdOutErr) { // pystone dependency
  const char* src = R"(
import sys
print(sys.stdout, sys.stderr)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");
}

} // namespace python
