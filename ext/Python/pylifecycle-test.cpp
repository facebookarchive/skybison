#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"

namespace python {

using PylifecycleExtensionApiTest = ExtensionApi;

TEST_F(PylifecycleExtensionApiTest, FatalErrorPrintsAndAbortsDeathTest) {
  EXPECT_DEATH(Py_FatalError("hello world"), "hello world");
}

}  // namespace python
