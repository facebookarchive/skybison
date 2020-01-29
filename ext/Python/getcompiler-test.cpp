#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using GetCompilerExtensionApiTest = ExtensionApi;

TEST_F(GetCompilerExtensionApiTest, GetCompilerReturnsNonnullString) {
  const char* compiler = Py_GetCompiler();
  EXPECT_NE(compiler, nullptr);
}

}  // namespace testing
}  // namespace py
