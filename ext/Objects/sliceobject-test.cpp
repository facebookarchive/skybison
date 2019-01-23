#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using SliceExtensionApiTest = ExtensionApi;

TEST_F(SliceExtensionApiTest, NewReturnsSlice) {
  PyObjectPtr slice(PySlice_New(nullptr, nullptr, nullptr));
  EXPECT_TRUE(PySlice_Check(slice));
}

}  // namespace python
