#include "tracked-allocation.h"

namespace python {

void* TrackedAllocation::malloc(TrackedAllocation** head, word num_bytes) {
  word size = sizeof(TrackedAllocation) + num_bytes;
  TrackedAllocation* alloc = static_cast<TrackedAllocation*>(std::malloc(size));
  TrackedAllocation::insert(head, alloc);
  return static_cast<void*>(alloc + 1);
}

void* TrackedAllocation::calloc(TrackedAllocation** head, word num_elems,
                                word num_bytes) {
  word size = sizeof(TrackedAllocation) + num_elems * num_bytes;
  TrackedAllocation* alloc =
      static_cast<TrackedAllocation*>(std::calloc(1, size));
  TrackedAllocation::insert(head, alloc);
  return static_cast<void*>(alloc + 1);
}

void TrackedAllocation::freePtr(TrackedAllocation** head, void* ptr) {
  TrackedAllocation::free(head, static_cast<TrackedAllocation*>(ptr) - 1);
}

void TrackedAllocation::free(TrackedAllocation** head,
                             TrackedAllocation* alloc) {
  TrackedAllocation::remove(head, alloc);
  std::free(static_cast<void*>(alloc));
}

void TrackedAllocation::insert(TrackedAllocation** head,
                               TrackedAllocation* alloc) {
  if (*head == nullptr) {
    alloc->previous_ = alloc;
    alloc->next_ = alloc;
    *head = alloc;
  } else {
    TrackedAllocation* previous = (*head)->previous_;
    previous->next_ = alloc;
    (*head)->previous_ = alloc;
    alloc->previous_ = previous;
    alloc->next_ = *head;
  }
}

void TrackedAllocation::remove(TrackedAllocation** head,
                               TrackedAllocation* alloc) {
  if ((*head)->next_ == *head) {
    *head = nullptr;
  } else {
    alloc->previous_->next_ = alloc->next_;
    alloc->next_->previous_ = alloc->previous_;
    if (*head == alloc) {
      *head = alloc->next_;
    }
  }
}

}  // namespace python
