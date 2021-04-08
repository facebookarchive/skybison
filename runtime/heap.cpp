#include "heap.h"

#include <cstring>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace py {

Heap::Heap(word size) {
  space_ = new Space(size);
  immortal_ = nullptr;
}

Heap::~Heap() {
  delete space_;
  delete immortal_;
}

NEVER_INLINE bool Heap::allocateRetry(word size, uword* address_out) {
  // Since the allocation failed, invoke the garbage collector and retry.
  collectGarbage();
  return space_->allocate(size, address_out);
}

bool Heap::allocateImmortal(word size, uword* address_out) {
  DCHECK(Utils::isAligned(size, kPointerSize), "request %ld not aligned", size);
  if (immortal_ == nullptr) {
    immortal_ = new Space(space_->size());
  }
  if (UNLIKELY(!immortal_->allocate(size, address_out))) {
    return allocateRetry(size, address_out);
  }
  return true;
}

bool Heap::contains(uword address) {
  return space_->contains(address) ||
         (immortal_ && immortal_->contains(address));
}

void Heap::collectGarbage() { Thread::current()->runtime()->collectGarbage(); }

bool Heap::verifySpace(Space* space) {
  uword scan = space->start();
  while (scan < space->fill()) {
    if (!(*reinterpret_cast<RawObject*>(scan)).isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      RawHeapObject object = HeapObject::fromAddress(scan + RawHeader::kSize);
      // Objects start before the start of the space they are allocated in.
      if (object.baseAddress() < space->start()) {
        return false;
      }
      // Objects must have their instance data after their header.
      if (object.address() < object.baseAddress()) {
        return false;
      }
      // Objects cannot start after the end of the space they are allocated in.
      if (object.address() > space->fill()) {
        return false;
      }
      // Objects cannot end after the end of the space they are allocated in.
      uword end = object.baseAddress() + object.size();
      if (end > space->fill()) {
        return false;
      }
      // Scan pointers that follow the header word, if any.
      if (!object.isRoot()) {
        scan = end;
      } else {
        for (scan += RawHeader::kSize; scan < end; scan += kPointerSize) {
          auto pointer = reinterpret_cast<RawObject*>(scan);
          if ((*pointer).isHeapObject() &&
              !inHeap(HeapObject::cast(*pointer).address())) {
            return false;
          }
        }
      }
    }
  }
  return true;
}

void Heap::makeImmortal() {
  if (immortal_ != nullptr) {
    UNIMPLEMENTED("Immortalizing multiple times not supported yet");
  }
  immortal_ = space_;
  space_ = new Space(immortal_->size());
}

void Heap::visitAllObjects(HeapObjectVisitor* visitor) {
  if (immortal_) visitSpace(immortal_, visitor);
  visitSpace(space_, visitor);
}

void Heap::visitSpace(Space* space, HeapObjectVisitor* visitor) {
  uword scan = space->start();
  while (scan < space->fill()) {
    if (!(*reinterpret_cast<RawObject*>(scan)).isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
      continue;
    }
    RawHeapObject object = HeapObject::fromAddress(scan + RawHeader::kSize);
    visitor->visitHeapObject(object);
    scan = object.baseAddress() + object.size();
  }
}

}  // namespace py
