#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using GetPlatformExtensionApiTest = ExtensionApi;

TEST_F(GetPlatformExtensionApiTest, GetPlatformReturnsNonnullString) {
  const char* platform = Py_GetPlatform();
  EXPECT_NE(platform, nullptr);
}

}  // namespace testing
}  // namespace py
