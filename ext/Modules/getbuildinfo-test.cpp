#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using GetBuildInfoExtensionApiTest = ExtensionApi;

TEST_F(GetBuildInfoExtensionApiTest, GetBuildInfoReturnsNonnullString) {
  const char* build_info = Py_GetBuildInfo();
  EXPECT_NE(build_info, nullptr);
}

}  // namespace testing
}  // namespace py
