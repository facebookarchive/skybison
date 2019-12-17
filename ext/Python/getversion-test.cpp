#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using GetVersionExtensionApiTest = ExtensionApi;

TEST_F(GetVersionExtensionApiTest, GetVersionReturnsCString) {
  const char* version = Py_GetVersion();
  ASSERT_NE(version, nullptr);
  PyRun_SimpleString(R"(
import sys
v = sys.version
)");
  PyObjectPtr v(testing::moduleGet("__main__", "v"));
  ASSERT_NE(v, nullptr);
  const char* sys_version = PyUnicode_AsUTF8(v);
  EXPECT_STREQ(version, sys_version);
}

}  // namespace testing
}  // namespace py
