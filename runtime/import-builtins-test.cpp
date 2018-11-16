#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(ImportBuiltinsTest, ModuleImporting) {
  Runtime runtime;
  runtime.runFromCStr(R"(
import _imp
  )");
  HandleScope scope;
  Module main(&scope, testing::findModule(&runtime, "__main__"));
  RawObject imp = testing::moduleAt(&runtime, main, "_imp");
  EXPECT_TRUE(imp->isModule());
}

}  // namespace python
