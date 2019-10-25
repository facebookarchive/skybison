#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using GenExtensionApiTest = ExtensionApi;

TEST_F(GenExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyAsyncGen_ClearFreeLists(), 0);
}

}  // namespace testing
}  // namespace py
