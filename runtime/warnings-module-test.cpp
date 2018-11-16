#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(WarningsModuleTest, ModuleImporting) {
  Runtime runtime;
  runtime.runFromCStr(R"(
import _warnings
  )");
  HandleScope scope;
  RawObject warnings = testing::moduleAt(&runtime, "__main__", "_warnings");
  EXPECT_TRUE(warnings->isModule());
}

}  // namespace python
