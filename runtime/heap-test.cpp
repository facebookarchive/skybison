#include "gtest/gtest.h"

#include "globals.h"
#include "heap.h"
#include "os.h"
#include "runtime.h"

namespace python {

TEST(HeapTest, AllocateObjects) {
  const int size = OS::kPageSize * 4;
  Heap heap(size);

  // Allocate the first half of the heap.
  Object* raw1 = heap.allocate(size / 2, 0);
  ASSERT_NE(raw1, nullptr);
  EXPECT_TRUE(heap.contains(raw1));

  // Allocate the second half of the heap.
  Object* raw2 = heap.allocate(size / 2, 0);
  ASSERT_NE(raw2, nullptr);
  EXPECT_TRUE(heap.contains(raw2));
}

TEST(HeapTest, AllocateFails) {
  int size = OS::kPageSize * 8;
  Runtime runtime(size);
  Heap* heap = runtime.heap();

  // Allocate the first half of the heap.
  Object* raw1 = heap->allocate(size / 2, 0);
  ASSERT_NE(raw1, Error::object());
  EXPECT_TRUE(heap->contains(raw1));

  // Try over allocating.
  Object* raw2 = heap->allocate(size, 0);
  ASSERT_EQ(raw2, Error::object());

  // Allocate the second half of the heap.
  Object* raw3 = heap->allocate(size / 2, 0);
  ASSERT_NE(raw3, Error::object());
  EXPECT_TRUE(heap->contains(raw3));
}

} // namespace python
