#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(ThreadModuleTest, ModuleImporting) {
  Runtime runtime;
  runtime.runFromCStr(R"(
import _thread
  )");
  HandleScope scope;
  RawObject threadModule = testing::moduleAt(&runtime, "__main__", "_thread");
  EXPECT_TRUE(threadModule->isModule());
}

}  // namespace python
