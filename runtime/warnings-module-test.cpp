#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(WarningsModuleTest, ModuleImporting) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _warnings
  )");
  HandleScope scope;
  RawObject warnings = moduleAt(&runtime, "__main__", "_warnings");
  EXPECT_TRUE(warnings->isModule());
}

}  // namespace python
