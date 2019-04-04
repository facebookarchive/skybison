#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"

namespace python {

using PylifecycleExtensionApiTest = ExtensionApi;

TEST_F(PylifecycleExtensionApiTest, FatalErrorPrintsAndAbortsDeathTest) {
  EXPECT_DEATH(Py_FatalError("hello world"), "hello world");
}

TEST_F(PylifecycleExtensionApiTest, AtExitRegistersExitFunction) {
  ASSERT_EXIT(PyRun_SimpleString(R"(
def cleanup():
    import sys
    print("foo", file=sys.stderr)

import atexit
atexit.register(cleanup)
raise SystemExit(123)
)"),
              ::testing::ExitedWithCode(123), "foo");
}

}  // namespace python
