#pragma once

#include "utils.h"

namespace python {

class TrackedAllocation {
 public:
  static void* malloc(TrackedAllocation** head, word num_bytes);
  static void freePtr(TrackedAllocation** head, void* ptr);
  static void free(TrackedAllocation** head, TrackedAllocation* alloc);

  static void insert(TrackedAllocation** head, TrackedAllocation* alloc);
  static void remove(TrackedAllocation** head, TrackedAllocation* alloc);

  TrackedAllocation* previous() { return previous_; }

  TrackedAllocation* next() { return next_; }

 private:
  TrackedAllocation* previous_;
  TrackedAllocation* next_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(TrackedAllocation);
};

}  // namespace python
