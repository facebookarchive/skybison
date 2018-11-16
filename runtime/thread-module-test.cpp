#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(ThreadModuleTest, ModuleImporting) {
  Runtime runtime;
  runtime.runFromCStr(R"(
import _thread
  )");
  RawObject thread_module = testing::moduleAt(&runtime, "__main__", "_thread");
  EXPECT_TRUE(thread_module->isModule());
}

}  // namespace python
