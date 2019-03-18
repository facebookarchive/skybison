#include "gtest/gtest.h"

#include "Python.h"

#include "capi-fixture.h"

namespace python {

using namespace testing;

using ObmallocExtensionApiTest = ExtensionApi;

TEST_F(ObmallocExtensionApiTest, RawStrdupDuplicatesStr) {
  const char* str = "hello, world";
  char* dup = _PyMem_RawStrdup(str);
  EXPECT_NE(dup, str);
  EXPECT_STREQ(dup, str);
  PyMem_RawFree(dup);
}

TEST_F(ObmallocExtensionApiTest, StrdupDuplicatesStr) {
  const char* str = "hello, world";
  char* dup = _PyMem_Strdup(str);
  EXPECT_NE(dup, str);
  EXPECT_STREQ(dup, str);
  PyMem_Free(dup);
}

}  // namespace python
