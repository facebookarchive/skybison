#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"
#include "time-module.h"

namespace python {

using namespace testing;

TEST(TimeModuleTest, TimeOpSub) {  // pystone dependency
  const char* src = R"(
import time
starttime = time.time()
for i in range(10):
  pass
nulltime = time.time() - starttime
print(nulltime.__class__ is float)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\n");
}

TEST(TimeModuleTest, TimeOpDiv) {  // pystone dependency
  const char* src = R"(
loops = 50
from time import time
starttime = time()
for i in range(50):
  pass
benchtime = time() - starttime
if benchtime == 0.0:
  loopsPerBenchtime = 0.0
else:
  loopsPerBenchtime = (loops / benchtime)
print(benchtime != 0.0)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\n");
}

TEST(TimeModuleTest, TimeTime) {  // pystone dependency
  const char* src = R"(
import time
t = time.time()
print(t.__class__ is float)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\n");
}

TEST(TimeModuleTest, TimeTimeFromImport) {  // pystone dependency
  const char* src = R"(
from time import time
t = time()
print(t.__class__ is float)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\n");
}

}  // namespace python
