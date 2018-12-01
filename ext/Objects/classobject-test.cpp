#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"

namespace python {
using MethodExtensionApiTest = ExtensionApi;

TEST_F(MethodExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyMethod_ClearFreeList(), 0);
}

}  // namespace python
