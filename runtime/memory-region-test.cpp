#include "gtest/gtest.h"

#include "memory-region.h"

namespace py {
namespace testing {

TEST(MemoryRegionTest, CopyFrom) {
  char from[] = {'X'};
  char to[3];
  MemoryRegion to_region(to, sizeof(to));
  MemoryRegion from_region(from, sizeof(from));

  std::memset(to, 0, sizeof(to));
  to_region.copyFrom(0, from_region);
  EXPECT_EQ(to[0], 'X');
  EXPECT_EQ(to[1], 0);
  EXPECT_EQ(to[2], 0);

  std::memset(to, 0, sizeof(to));
  to_region.copyFrom(1, from_region);
  EXPECT_EQ(to[0], 0);
  EXPECT_EQ(to[1], 'X');
  EXPECT_EQ(to[2], 0);

  std::memset(to, 0, sizeof(to));
  to_region.copyFrom(2, from_region);
  EXPECT_EQ(to[0], 0);
  EXPECT_EQ(to[1], 0);
  EXPECT_EQ(to[2], 'X');
}

}  // namespace testing
}  // namespace py
