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

TEST_F(ObmallocExtensionApiTest, MemResizeAssignsToPointer) {
  void* ptr = nullptr;
  PyMem_Resize(ptr, int, 128);
  EXPECT_NE(ptr, nullptr);
  PyMem_Free(ptr);
}

TEST_F(ObmallocExtensionApiTest, MemResizeMovesContents) {
  char* ptr = PyMem_New(char, 1);
  ASSERT_NE(ptr, nullptr);
  *ptr = 98;

  // Allocate the next word and resize to a much larger memory
  void* intervening_allocation = PyMem_New(char, 1);
  PyMem_Resize(ptr, char, 65536);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 98);
  ptr[65535] = 87;
  PyMem_FREE(intervening_allocation);

  PyMem_RESIZE(ptr, char, 1048576);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 98);
  EXPECT_EQ(ptr[65535], 87);
  PyMem_FREE(ptr);
}

}  // namespace python
