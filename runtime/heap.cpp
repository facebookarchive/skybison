#include "heap.h"

#include <cstring>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace py {

Heap::Heap(word size) { space_ = new Space(size); }

Heap::~Heap() { delete space_; }

template <typename T>
static word allocationSize() {
  return T::kSize + RawHeader::kSize;
}

bool Heap::allocate(word size, word offset, uword* address) {
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
      Thread::current()->runtime()->collectGarbage();
    }
  }
  return false;
}

bool Heap::contains(uword address) { return space_->contains(address); }

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

RawObject Heap::createType(LayoutId metaclass_id) {
  uword address;
  CHECK(allocate(allocationSize<RawType>(), RawHeader::kSize, &address),
        "out of memory");
  word num_attributes = RawType::kSize / kPointerSize;
  RawObject result = Instance::initialize(address, num_attributes, metaclass_id,
                                          NoneType::object());
  DCHECK(Thread::current()->runtime()->isInstanceOfType(result),
         "Invalid Type");
  return result;
}

RawObject Heap::createComplex(double real, double imag) {
  uword address;
  CHECK(allocate(allocationSize<RawComplex>(), RawHeader::kSize, &address),
        "out of memory");
  return Complex::cast(Complex::initialize(address, real, imag));
}

RawObject Heap::createFloat(double value) {
  uword address;
  CHECK(allocate(allocationSize<RawFloat>(), RawHeader::kSize, &address),
        "out of memory");
  return Float::cast(Float::initialize(address, value));
}

RawObject Heap::createEllipsis() {
  uword address;
  CHECK(allocate(allocationSize<RawEllipsis>(), RawHeader::kSize, &address),
        "out of memory");
  return Ellipsis::cast(Ellipsis::initialize(address));
}

RawObject Heap::createInstance(LayoutId layout_id, word num_attributes) {
  word size = Instance::allocationSize(num_attributes);
  uword address;
  CHECK(allocate(size, HeapObject::headerSize(num_attributes), &address),
        "out of memory");
  return Instance::cast(Instance::initialize(address, num_attributes, layout_id,
                                             NoneType::object()));
}

RawObject Heap::createLargeBytes(word length) {
  DCHECK(length > SmallBytes::kMaxLength, "fits into a SmallBytes");
  word size = LargeBytes::allocationSize(length);
  uword address;
  CHECK(allocate(size, LargeBytes::headerSize(length), &address),
        "out of memory");
  return LargeBytes::cast(
      DataArray::initialize(address, length, LayoutId::kLargeBytes));
}

RawObject Heap::createLargeInt(word num_digits) {
  DCHECK(num_digits > 0, "num_digits must be positive");
  word size = LargeInt::allocationSize(num_digits);
  uword address;
  CHECK(allocate(size, LargeInt::headerSize(num_digits), &address),
        "out of memory");
  return LargeInt::cast(LargeInt::initialize(address, num_digits));
}

RawObject Heap::createLargeStr(word length) {
  DCHECK(length > RawSmallStr::kMaxLength,
         "string len %ld is too small to be a large string", length);
  word size = LargeStr::allocationSize(length);
  uword address;
  CHECK(allocate(size, LargeStr::headerSize(length), &address),
        "out of memory");
  return LargeStr::cast(
      DataArray::initialize(address, length, LayoutId::kLargeStr));
}

RawObject Heap::createLayout(LayoutId layout_id) {
  uword address;
  CHECK(allocate(allocationSize<RawLayout>(), RawHeader::kSize, &address),
        "out of memory");
  return Layout::cast(
      Layout::initialize(address, layout_id, RawNoneType::object()));
}

RawObject Heap::createMutableBytes(word length) {
  DCHECK(length >= 0, "cannot allocate negative size");
  word size = MutableBytes::allocationSize(length);
  uword address;
  CHECK(allocate(size, MutableBytes::headerSize(length), &address),
        "out of memory");
  return MutableBytes::cast(
      DataArray::initialize(address, length, LayoutId::kMutableBytes));
}

RawObject Heap::createMutableTuple(word length) {
  word size = MutableTuple::allocationSize(length);
  uword address;
  CHECK(allocate(size, HeapObject::headerSize(length), &address),
        "out of memory");
  return MutableTuple::cast(MutableTuple::initialize(address, length));
}

RawObject Heap::createPointer(void* c_ptr, word length) {
  uword address;
  CHECK(allocate(allocationSize<RawPointer>(), RawHeader::kSize, &address),
        "out of memory");
  return Pointer::cast(Pointer::initialize(address, c_ptr, length));
}

RawObject Heap::createTuple(word length) {
  word size = Tuple::allocationSize(length);
  uword address;
  CHECK(allocate(size, HeapObject::headerSize(length), &address),
        "out of memory");
  return Tuple::cast(Tuple::initialize(address, length));
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
