#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(ThreadModuleTest, ModuleImporting) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _thread
  )");
  RawObject thread_module = moduleAt(&runtime, "__main__", "_thread");
  EXPECT_TRUE(thread_module->isModule());
}

}  // namespace python
