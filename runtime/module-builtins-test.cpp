#include "gtest/gtest.h"

#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ModuleBuiltinsTest, DunderName) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
import sys
a = sys.__name__
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a.equalsCStr("sys"));
}

}  // namespace python
