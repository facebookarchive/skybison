#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"

namespace python {
using SetExtensionApiTest = ExtensionApi;

TEST_F(SetExtensionApiTest, ClearFreeListReturnsZero) {
  EXPECT_EQ(PySet_ClearFreeList(), 0);
}

}  // namespace python
