#include "gtest/gtest.h"

#include "runtime.h"
#include "tracked-allocation.h"

namespace python {

TEST(TrackedAllocationTest, CreateTrackedAllocation) {
  Runtime runtime;
  EXPECT_EQ(nullptr, *runtime.trackedAllocations());

  TrackedAllocation::malloc(runtime.trackedAllocations(), 10);
  ASSERT_NE(nullptr, *runtime.trackedAllocations());

  TrackedAllocation* head = *runtime.trackedAllocations();
  EXPECT_EQ(head->next(), head);
  EXPECT_EQ(head->previous(), head);
}

TEST(TrackedAllocationTest, InsertTrackedAllocation) {
  Runtime runtime;
  TrackedAllocation::malloc(runtime.trackedAllocations(), 10);
  ASSERT_NE(nullptr, *runtime.trackedAllocations());
  TrackedAllocation* head = *runtime.trackedAllocations();

  void* mem = TrackedAllocation::malloc(runtime.trackedAllocations(), 15);
  TrackedAllocation* tracked_alloc = static_cast<TrackedAllocation*>(mem) - 1;
  EXPECT_EQ(head->next(), tracked_alloc);
  EXPECT_EQ(head->previous(), tracked_alloc);
  EXPECT_EQ(tracked_alloc->next(), head);
  EXPECT_EQ(tracked_alloc->previous(), head);

  void* mem2 = TrackedAllocation::malloc(runtime.trackedAllocations(), 20);
  TrackedAllocation* tracked_alloc2 = static_cast<TrackedAllocation*>(mem2) - 1;
  EXPECT_EQ(head->next(), tracked_alloc);
  EXPECT_EQ(head->previous(), tracked_alloc2);
  EXPECT_EQ(tracked_alloc->next(), tracked_alloc2);
  EXPECT_EQ(tracked_alloc->previous(), head);
  EXPECT_EQ(tracked_alloc2->next(), head);
  EXPECT_EQ(tracked_alloc2->previous(), tracked_alloc);
}

TEST(TrackedAllocationTest, RemoveTrackedAllocation) {
  Runtime runtime;
  TrackedAllocation::malloc(runtime.trackedAllocations(), 10);
  ASSERT_NE(nullptr, *runtime.trackedAllocations());
  TrackedAllocation* head = *runtime.trackedAllocations();
  void* mem = TrackedAllocation::malloc(runtime.trackedAllocations(), 15);
  TrackedAllocation* tracked_alloc = static_cast<TrackedAllocation*>(mem) - 1;
  void* mem2 = TrackedAllocation::malloc(runtime.trackedAllocations(), 20);
  TrackedAllocation* tracked_alloc2 = static_cast<TrackedAllocation*>(mem2) - 1;

  TrackedAllocation::free(runtime.trackedAllocations(), tracked_alloc);
  EXPECT_EQ(head->next(), tracked_alloc2);
  EXPECT_EQ(head->previous(), tracked_alloc2);
  EXPECT_EQ(tracked_alloc2->next(), head);
  EXPECT_EQ(tracked_alloc2->previous(), head);

  TrackedAllocation::freePtr(runtime.trackedAllocations(), mem2);
  EXPECT_EQ(head->next(), head);
  EXPECT_EQ(head->previous(), head);

  TrackedAllocation::free(runtime.trackedAllocations(), head);
  EXPECT_EQ(nullptr, *runtime.trackedAllocations());
}

} // namespace python
