#include "gtest/gtest.h"

#include "globals.h"
#include "heap.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(HeapTest, AllocateObjects) {
  const int size = OS::kPageSize * 4;
  Heap heap(size);

  // Allocate the first half of the heap.
  RawObject raw1 = heap.allocate(size / 2, 0);
  ASSERT_FALSE(raw1.isError());
  EXPECT_TRUE(heap.contains(raw1));

  // Allocate the second half of the heap.
  RawObject raw2 = heap.allocate(size / 2, 0);
  ASSERT_FALSE(raw2.isError());
  EXPECT_TRUE(heap.contains(raw2));
}

TEST(HeapTest, AllocateFails) {
  Runtime runtime;
  HandleScope scope;
  Heap* heap = runtime.heap();
  word free_space = heap->space()->end() - heap->space()->fill();

  // Allocate the first half of the heap. Use a handle to prevent gc
  word first_half = Utils::roundUp(free_space / 2, kPointerSize * 2);
  Object object1(&scope, heap->createLargeStr(first_half));
  RawObject raw1 = *object1;
  ASSERT_FALSE(raw1.isError());
  EXPECT_TRUE(heap->contains(raw1));

  // Try over allocating.
  RawObject raw2 = heap->allocate(free_space, 0);
  ASSERT_TRUE(raw2.isError());

  // Allocate the second half of the heap.
  word second_half = heap->space()->end() - heap->space()->fill();
  RawObject raw3 = heap->allocate(second_half, 0);
  ASSERT_FALSE(raw3.isError());
  EXPECT_TRUE(heap->contains(raw3));

  ASSERT_EQ(heap->space()->end(), heap->space()->fill());
}

TEST(HeapTest, AllocateBigLargeInt) {
  Runtime runtime;
  HandleScope scope;
  Object result(&scope, runtime.heap()->createLargeInt(100000));
  ASSERT_TRUE(result.isLargeInt());
  EXPECT_EQ(LargeInt::cast(*result).numDigits(), 100000);
}

TEST(HeapTest, AllocateBigInstance) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutCreateEmpty(Thread::current()));
  Object result(&scope, runtime.heap()->createInstance(layout.id(), 100000));
  ASSERT_TRUE(result.isInstance());
  EXPECT_EQ(Instance::cast(*result).headerCountOrOverflow(), 100000);
}

}  // namespace python
