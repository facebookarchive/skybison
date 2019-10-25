#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using MethodExtensionApiTest = ExtensionApi;

TEST_F(MethodExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyCFunction_ClearFreeList(), 0);
}

}  // namespace py
