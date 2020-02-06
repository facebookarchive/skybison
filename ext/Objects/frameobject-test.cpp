#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"

extern "C" PyAPI_FUNC(int) PyFrame_ClearFreeList(void);

namespace py {
namespace testing {

using FrameExtensionApiTest = ExtensionApi;

TEST_F(FrameExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyFrame_ClearFreeList(), 0);
}

}  // namespace testing
}  // namespace py
