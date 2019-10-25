#include "gtest/gtest.h"

#include "Python.h"

#include "capi-fixture.h"

namespace py {

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

TEST_F(ObmallocExtensionApiTest, MallocAllocatesMemory) {
  void* ptr = PyObject_Malloc(1);
  EXPECT_NE(ptr, nullptr);
  PyObject_Free(ptr);
}

TEST_F(ObmallocExtensionApiTest, CallocAllocatesMemory) {
  void* ptr = PyObject_Calloc(1, 1);
  EXPECT_NE(ptr, nullptr);
  PyObject_Free(ptr);
}

TEST_F(ObmallocExtensionApiTest, ReallocAllocatesMemory) {
  auto* ptr = reinterpret_cast<char*>(PyObject_Malloc(1));
  ASSERT_NE(ptr, nullptr);
  *ptr = 98;
  ptr = reinterpret_cast<char*>(PyObject_Realloc(ptr, 2));
  ASSERT_NE(ptr, nullptr);
  ptr[1] = 87;

  EXPECT_EQ(*ptr, 98);
  EXPECT_EQ(ptr[1], 87);
  PyObject_Free(ptr);
}

TEST_F(ObmallocExtensionApiTest, ReallocOnlyRetracksPyObjects) {
  auto* ptr = reinterpret_cast<char*>(PyObject_Malloc(1));
  ASSERT_NE(ptr, nullptr);
  *ptr = 98;

  // Trigger a gc
  PyRun_SimpleString(R"(
try:
  import _builtins
  _builtins._gc()
except:
  pass
)");

  ptr = reinterpret_cast<char*>(PyObject_Realloc(ptr, 2));
  ASSERT_NE(ptr, nullptr);
  ptr[1] = 87;

  EXPECT_EQ(*ptr, 98);
  EXPECT_EQ(ptr[1], 87);
  PyObject_Free(ptr);
}

}  // namespace py
