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
  bool allocateImmortal(word size, uword* address_out);
  void collectGarbage();

  bool contains(uword address);
  bool verify() {
    if (!verifySpace(space_)) return false;
    if (immortal_ && !verifySpace(immortal_)) return false;
    return true;
  }

  Space* space() { return space_; }
  Space* immortal() { return immortal_; }

  void setSpace(Space* new_space) { space_ = new_space; }
  void makeImmortal();

  bool isImmortal(uword address) const {
    return immortal_ && immortal_->isAllocated(address);
  }
  bool isMortal(uword address) const {
    return space_ && space_->isAllocated(address);
  }
  bool inHeap(uword address) const {
    return isMortal(address) || isImmortal(address);
  }

  static int spaceOffset() { return offsetof(Heap, space_); };

  void visitAllObjects(HeapObjectVisitor* visitor);

 private:
  bool allocateRetry(word size, uword* address_out);
  bool verifySpace(Space*);
  void visitSpace(Space* space, HeapObjectVisitor* visitor);

  Space* space_;
  Space* immortal_;
};

inline bool Heap::allocate(word size, uword* address_out) {
  DCHECK(Utils::isAligned(size, kPointerSize), "request %ld not aligned", size);
  if (UNLIKELY(!space_->allocate(size, address_out))) {
    return allocateRetry(size, address_out);
  }
  return true;
}

}  // namespace py
