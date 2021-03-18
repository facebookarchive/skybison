#pragma once

#include "globals.h"
#include "objects.h"
#include "space.h"
#include "visitor.h"

namespace py {

class Heap {
 public:
  explicit Heap(word size);
  ~Heap();

  // Returns true if allocation succeeded and writes output address + offset to
  // *address. Returns false otherwise.
  bool allocate(word size, uword* address_out);
  void collectGarbage();

  bool contains(uword address);
  bool verify();

  Space* space() { return space_; }

  void setSpace(Space* new_space) { space_ = new_space; }

  static int spaceOffset() { return offsetof(Heap, space_); };

  void visitAllObjects(HeapObjectVisitor* visitor);

 private:
  bool allocateRetry(word size, uword* address_out);

  Space* space_;
};

inline bool Heap::allocate(word size, uword* address_out) {
  DCHECK(space_ != nullptr, "garbage collection is disabled");
  DCHECK(Utils::isAligned(size, kPointerSize), "request %ld not aligned", size);

  if (UNLIKELY(!space_->allocate(size, address_out))) {
    return allocateRetry(size, address_out);
  }
  return true;
}

}  // namespace py
