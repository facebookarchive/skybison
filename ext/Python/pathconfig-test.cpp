#include "Python.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using PathconfigExtensionApiTest = ExtensionApi;

// Defaults to `/usr/local` only if python is run from a build dir
// TODO(T67620993): Compare against built-in defaults
// TODO(T67625250): Make sure we test against different values depending on
// whether Pyro is being run from a build directory or not.
TEST_F(PathconfigExtensionApiTest, GetPrefixReturnsUsrLocalPyro) {
  EXPECT_STREQ(Py_GetPrefix(), L"/usr/local");
  EXPECT_STREQ(Py_GetExecPrefix(), L"/usr/local");
}

TEST_F(PathconfigExtensionApiTest, SetPathClearsPrefixAndExecPrefix) {
  Py_SetPath(L"test");
  EXPECT_STREQ(Py_GetPrefix(), L"");
  EXPECT_STREQ(Py_GetExecPrefix(), L"");
  EXPECT_STREQ(Py_GetPath(), L"test");
}

}  // namespace testing
}  // namespace py
