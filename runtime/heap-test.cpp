#include "gtest/gtest.h"

#include "globals.h"
#include "heap.h"
#include "os.h"

namespace python {

TEST(HeapTest, AllocateObjects) {
  const int size = Os::kPageSize * 4;
  Heap heap(size);

  // Allocate the first half of the heap.
  Object* raw1 = heap.allocate(size / 2);
  ASSERT_NE(raw1, nullptr);
  EXPECT_TRUE(heap.contains(raw1));

  // Allocate the second half of the heap.
  Object* raw2 = heap.allocate(size / 2);
  ASSERT_NE(raw2, nullptr);
  EXPECT_TRUE(heap.contains(raw2));
}

TEST(HeapTest, AllocateFails) {
  const int size = Os::kPageSize * 4;
  Heap heap(size);

  // Allocate the first half of the heap.
  Object* raw1 = heap.allocate(size / 2);
  ASSERT_NE(raw1, nullptr);
  EXPECT_TRUE(heap.contains(raw1));

  // Try over allocating.
  Object* raw2 = heap.allocate(size);
  ASSERT_EQ(raw2, nullptr);

  // Allocate the second half of the heap.
  Object* raw3 = heap.allocate(size / 2);
  ASSERT_NE(raw3, nullptr);
  EXPECT_TRUE(heap.contains(raw3));
}

} // namespace python
