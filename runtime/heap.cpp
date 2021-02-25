#include "heap.h"

#include <cstring>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace py {

Heap::Heap(word size) { space_ = new Space(size); }

Heap::~Heap() { delete space_; }

NEVER_INLINE bool Heap::allocateRetry(word size, uword* address_out) {
  // Since the allocation failed, invoke the garbage collector and retry.
  collectGarbage();
  return space_->allocate(size, address_out);
}

bool Heap::contains(uword address) { return space_->contains(address); }

void Heap::collectGarbage() { Thread::current()->runtime()->collectGarbage(); }

bool Heap::verify() {
  uword scan = space_->start();
  while (scan < space_->fill()) {
    if (!(*reinterpret_cast<RawObject*>(scan)).isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      RawHeapObject object = HeapObject::fromAddress(scan + RawHeader::kSize);
      // Objects start before the start of the space they are allocated in.
      if (object.baseAddress() < space_->start()) {
        return false;
      }
      // Objects must have their instance data after their header.
      if (object.address() < object.baseAddress()) {
        return false;
      }
      // Objects cannot start after the end of the space they are allocated in.
      if (object.address() > space_->fill()) {
        return false;
      }
      // Objects cannot end after the end of the space they are allocated in.
      uword end = object.baseAddress() + object.size();
      if (end > space_->fill()) {
        return false;
      }
      // Scan pointers that follow the header word, if any.
      if (!object.isRoot()) {
        scan = end;
      } else {
        for (scan += RawHeader::kSize; scan < end; scan += kPointerSize) {
          auto pointer = reinterpret_cast<RawObject*>(scan);
          if ((*pointer).isHeapObject()) {
            if (!space_->isAllocated(HeapObject::cast(*pointer).address())) {
              return false;
            }
          }
        }
      }
    }
  }
  return true;
}

void Heap::visitAllObjects(HeapObjectVisitor* visitor) {
  uword scan = space_->start();
  while (scan < space_->fill()) {
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
