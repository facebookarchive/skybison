#include "gtest/gtest.h"

#include "runtime.h"
#include "tracked-allocation.h"

namespace python {

TEST(TrackedAllocationTest, CreateTrackedAllocation) {
  TrackedAllocation* tracked_allocations = nullptr;

  TrackedAllocation::malloc(&tracked_allocations, 10);
  ASSERT_NE(nullptr, tracked_allocations);

  TrackedAllocation* head = tracked_allocations;
  EXPECT_EQ(head->next(), head);
  EXPECT_EQ(head->previous(), head);
}

TEST(TrackedAllocationTest, InsertTrackedAllocation) {
  TrackedAllocation* tracked_allocations = nullptr;

  TrackedAllocation::malloc(&tracked_allocations, 10);
  ASSERT_NE(nullptr, tracked_allocations);
  TrackedAllocation* head = tracked_allocations;

  void* mem = TrackedAllocation::malloc(&tracked_allocations, 15);
  TrackedAllocation* tracked_alloc = static_cast<TrackedAllocation*>(mem) - 1;
  EXPECT_EQ(head->next(), tracked_alloc);
  EXPECT_EQ(head->previous(), tracked_alloc);
  EXPECT_EQ(tracked_alloc->next(), head);
  EXPECT_EQ(tracked_alloc->previous(), head);

  void* mem2 = TrackedAllocation::calloc(&tracked_allocations, 1, 20);
  TrackedAllocation* tracked_alloc2 = static_cast<TrackedAllocation*>(mem2) - 1;
  EXPECT_EQ(head->next(), tracked_alloc);
  EXPECT_EQ(head->previous(), tracked_alloc2);
  EXPECT_EQ(tracked_alloc->next(), tracked_alloc2);
  EXPECT_EQ(tracked_alloc->previous(), head);
  EXPECT_EQ(tracked_alloc2->next(), head);
  EXPECT_EQ(tracked_alloc2->previous(), tracked_alloc);
}

TEST(TrackedAllocationTest, RemoveTrackedAllocation) {
  TrackedAllocation* tracked_allocations = nullptr;

  TrackedAllocation::malloc(&tracked_allocations, 10);
  ASSERT_NE(nullptr, tracked_allocations);
  TrackedAllocation* head = tracked_allocations;
  void* mem = TrackedAllocation::malloc(&tracked_allocations, 15);
  TrackedAllocation* tracked_alloc = static_cast<TrackedAllocation*>(mem) - 1;
  void* mem2 = TrackedAllocation::calloc(&tracked_allocations, 3, 20);
  TrackedAllocation* tracked_alloc2 = static_cast<TrackedAllocation*>(mem2) - 1;

  TrackedAllocation::free(&tracked_allocations, tracked_alloc);
  EXPECT_EQ(head->next(), tracked_alloc2);
  EXPECT_EQ(head->previous(), tracked_alloc2);
  EXPECT_EQ(tracked_alloc2->next(), head);
  EXPECT_EQ(tracked_alloc2->previous(), head);

  TrackedAllocation::freePtr(&tracked_allocations, mem2);
  EXPECT_EQ(head->next(), head);
  EXPECT_EQ(head->previous(), head);

  TrackedAllocation::free(&tracked_allocations, head);
  EXPECT_EQ(nullptr, tracked_allocations);
}

}  // namespace python
