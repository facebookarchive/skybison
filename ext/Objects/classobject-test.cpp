#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"

namespace python {
using ClassExtensionApiTest = ExtensionApi;

TEST_F(ClassExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyMethod_ClearFreeList(), 0);
}

}  // namespace python
