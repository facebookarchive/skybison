#include "heap.h"

#include "gtest/gtest.h"

#include "globals.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using HeapTest = RuntimeFixture;

TEST(HeapTestNoFixture, AllocateObjects) {
  const int size = OS::kPageSize * 4;
  Heap heap(size);

  // Allocate the first half of the heap.
  uword address1;
  bool result1 = heap.allocate(size / 2, 0, &address1);
  ASSERT_TRUE(result1);
  EXPECT_TRUE(heap.contains(address1));

  // Allocate the second half of the heap.
  uword address2;
  bool result2 = heap.allocate(size / 2, 0, &address2);
  ASSERT_TRUE(result2);
  EXPECT_TRUE(heap.contains(address2));
}

TEST_F(HeapTest, AllocateFails) {
  HandleScope scope(thread_);
  Heap* heap = runtime_->heap();
  word free_space = heap->space()->end() - heap->space()->fill();

  // Allocate the first half of the heap. Use a handle to prevent gc
  word first_half = Utils::roundUp(free_space / 2, kPointerSize * 2);
  Object object1(&scope, heap->createLargeStr(first_half));
  RawObject raw1 = *object1;
  ASSERT_FALSE(raw1.isError());
  EXPECT_TRUE(heap->contains(HeapObject::cast(raw1).address()));

  // Try over allocating.
  uword address2;
  bool result2 = heap->allocate(free_space, 0, &address2);
  ASSERT_FALSE(result2);

  // Allocate the second half of the heap.
  word second_half = heap->space()->end() - heap->space()->fill();
  uword address3;
  bool result3 = heap->allocate(second_half, 0, &address3);
  ASSERT_TRUE(result3);
  EXPECT_TRUE(heap->contains(address3));

  ASSERT_EQ(heap->space()->end(), heap->space()->fill());
}

TEST_F(HeapTest, AllocateBigLargeInt) {
  HandleScope scope(thread_);
  Object result(&scope, runtime_->heap()->createLargeInt(100000));
  ASSERT_TRUE(result.isLargeInt());
  EXPECT_EQ(LargeInt::cast(*result).numDigits(), 100000);
}

TEST_F(HeapTest, AllocateBigInstance) {
  HandleScope scope(thread_);
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  Object result(&scope, runtime_->heap()->createInstance(layout.id(), 100000));
  ASSERT_TRUE(result.isInstance());
  EXPECT_EQ(Instance::cast(*result).headerCountOrOverflow(), 100000);
}

TEST_F(HeapTest, AllocateMutableBytes) {
  HandleScope scope(thread_);
  Object result(&scope, runtime_->heap()->createMutableBytes(15));
  ASSERT_TRUE(result.isMutableBytes());
  EXPECT_EQ(MutableBytes::cast(*result).length(), 15);
}

class DummyVisitor : public HeapObjectVisitor {
 public:
  explicit DummyVisitor() : count_(0) {}

  void visitHeapObject(RawHeapObject obj) {
    visited_.push_back(obj);
    count_++;
  }

  word count() { return count_; }

  bool visited(RawObject obj) {
    for (word i = 0; i < visited_.size(); i++) {
      if (visited_[i] == obj) return true;
    }
    return false;
  }

 private:
  Vector<RawObject> visited_;
  word count_;
};

TEST(HeapTestNoFixture, VisitAllObjectsVisitsAllObjects) {
  Heap heap(OS::kPageSize * 4);
  DummyVisitor visitor;
  EXPECT_EQ(visitor.count(), 0);
  heap.visitAllObjects(&visitor);
  EXPECT_EQ(visitor.count(), 0);
  RawObject obj1 = heap.createLargeStr(10);
  RawObject obj2 = heap.createLargeStr(10);
  RawObject obj3 = heap.createLargeStr(10);
  heap.visitAllObjects(&visitor);
  EXPECT_TRUE(visitor.visited(obj1));
  EXPECT_TRUE(visitor.visited(obj2));
  EXPECT_TRUE(visitor.visited(obj3));
  EXPECT_EQ(visitor.count(), 3);
}

}  // namespace testing
}  // namespace py
