#include "gtest/gtest.h"

#include "Python.h"
#include "frameobject.h"  // for PyFrame_ClearFreeList

#include "capi-fixture.h"

namespace py {

using namespace testing;

using FrameExtensionApiTest = ExtensionApi;

TEST_F(FrameExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyFrame_ClearFreeList(), 0);
}

}  // namespace py
