#include "space.h"

#include "gtest/gtest.h"

namespace py {

TEST(SpaceTest, Allocate) {
  Space space(64 * kKiB);
  EXPECT_EQ(space.start(), space.fill());
  EXPECT_LT(space.start(), space.end());
  EXPECT_TRUE(space.contains(space.start()));
  EXPECT_FALSE(space.isAllocated(space.fill()));
  EXPECT_FALSE(space.contains(space.end()));

  uword address;
  EXPECT_TRUE(space.allocate(10 * kPointerSize, &address));
  EXPECT_TRUE(space.isAllocated(address));
  EXPECT_FALSE(space.isAllocated(space.fill()));

  EXPECT_EQ(space.start(), address);
  EXPECT_LT(space.start(), space.fill());
  EXPECT_LT(space.fill(), space.end());
  EXPECT_TRUE(space.contains(address));
  EXPECT_TRUE(space.contains(space.fill()));
  EXPECT_FALSE(space.isAllocated(space.fill()));

  space.reset();
  EXPECT_FALSE(space.isAllocated(address));
  EXPECT_EQ(space.start(), space.fill());
}

}  // namespace py
