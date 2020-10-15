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
  bool allocate(word size, word offset, uword* address);
  void collectGarbage();

  bool contains(uword address);
  bool verify();

  Space* space() { return space_; }

  void setSpace(Space* new_space) { space_ = new_space; }

  static int spaceOffset() { return offsetof(Heap, space_); };

  void visitAllObjects(HeapObjectVisitor* visitor);

 private:
  Space* space_;
};

// Left in the header for inling into Runtime::newXXX functions.
inline bool Heap::allocate(word size, word offset, uword* address) {
  DCHECK(space_ != nullptr, "garbage collection is disabled");
  DCHECK(size >= RawHeapObject::kMinimumSize, "allocation %ld too small", size);
  DCHECK(Utils::isAligned(size, kPointerSize), "request %ld not aligned", size);
  // Try allocating.  If the allocation fails, invoke the garbage collector and
  // retry the allocation.
  for (word attempt = 0; attempt < 2 && size < space_->size(); attempt++) {
    uword result = space_->allocate(size);
    if (result != 0) {
      // Allocation succeeded return the address as an object.
      *address = result + offset;
      return true;
    }
    if (attempt == 0) {
      // Allocation failed, garbage collect and retry the allocation.
      collectGarbage();
    }
  }
  return false;
}

}  // namespace py
