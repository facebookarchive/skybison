#include "gtest/gtest.h"

#include <cstring>

#include "os.h"

namespace python {

static int count(const byte* array, byte ch, int length) {
  int result = 0;
  for (int i = 0; i < length; i++) {
    if (array[i] == ch) {
      result++;
    }
  }
  return result;
}

TEST(OsTest, allocateUseAndFreeOnePage) {
  // Allocate a page of memory.
  byte* page = OS::allocateMemory(OS::kPageSize);
  ASSERT_NE(page, nullptr);

  // Read from every allocated byte.
  int num_zeroes = count(page, 0, OS::kPageSize);
  // Every byte should have a value of zero.
  ASSERT_EQ(num_zeroes, OS::kPageSize);

  // Write to every allocated byte.
  std::memset(page, 1, OS::kPageSize);
  int num_ones = count(page, 1, OS::kPageSize);
  // Every byte should have a value of one.
  ASSERT_EQ(num_ones, OS::kPageSize);

  // Release the page.
  bool is_free = OS::freeMemory(page, OS::kPageSize);
  EXPECT_TRUE(is_free);
}

TEST(OsTest, allocateUseAndFreeMultiplePages) {
  // Not a multiple of a page.
  const int size = 17 * KiB;

  // Allocate the pages.
  byte* page = OS::allocateMemory(size);
  ASSERT_NE(page, nullptr);

  // Read from every allocated byte.
  int num_zeroes = count(page, 0, size);
  // Every byte should have a value of zero.
  ASSERT_EQ(num_zeroes, size);

  // Write to every allocated byte.
  std::memset(page, 1, size);
  int num_ones = count(page, 1, size);
  // Every byte should have a value of one.
  ASSERT_EQ(num_ones, size);

  // Release the pages.
  bool is_free = OS::freeMemory(page, size);
  EXPECT_TRUE(is_free);
}

} // namespace python
