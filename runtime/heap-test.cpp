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
  Runtime runtime;
  HandleScope scope;
  Heap* heap = runtime.heap();
  word free_space = heap->space()->end() - heap->space()->fill();

  // Allocate the first half of the heap. Use a handle to prevent gc
  word first_half = Utils::roundUp(free_space / 2, kPointerSize * 2);
  Handle<Object> object1(&scope, heap->createLargeStr(first_half));
  Object* raw1 = *object1;
  ASSERT_NE(raw1, Error::object());
  EXPECT_TRUE(heap->contains(raw1));

  // Try over allocating.
  Object* raw2 = heap->allocate(free_space, 0);
  ASSERT_EQ(raw2, Error::object());

  // Allocate the second half of the heap.
  word second_half = heap->space()->end() - heap->space()->fill();
  Object* raw3 = heap->allocate(second_half, 0);
  ASSERT_NE(raw3, Error::object());
  EXPECT_TRUE(heap->contains(raw3));

  ASSERT_EQ(heap->space()->end(), heap->space()->fill());
}

}  // namespace python
