#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(ImpModuleTest, ModuleImporting) {
  Runtime runtime;
  runtime.runFromCStr(R"(
import _imp
  )");
  HandleScope scope;
  RawObject imp = testing::moduleAt(&runtime, "__main__", "_imp");
  EXPECT_TRUE(imp->isModule());
}

}  // namespace python
