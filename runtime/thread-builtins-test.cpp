#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(ThreadBuiltinsTest, ModuleImporting) {
  Runtime runtime;
  runtime.runFromCStr(R"(
import _thread
  )");
  HandleScope scope;
  Module main(&scope, testing::findModule(&runtime, "__main__"));
  RawObject threadModule = testing::moduleAt(&runtime, main, "_thread");
  EXPECT_TRUE(threadModule->isModule());
}

}  // namespace python
