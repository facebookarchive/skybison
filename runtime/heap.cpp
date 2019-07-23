#include "heap.h"

#include <cstring>

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace python {

Heap::Heap(word size) { space_ = new Space(size); }

Heap::~Heap() { delete space_; }

template <typename T>
static word allocationSize() {
  return T::kSize + RawHeader::kSize;
}

RawObject Heap::allocate(word size, word offset) {
  DCHECK(size >= RawHeapObject::kMinimumSize, "allocation %ld too small", size);
  DCHECK(Utils::isAligned(size, kPointerSize), "request %ld not aligned", size);
  // Try allocating.  If the allocation fails, invoke the garbage collector and
  // retry the allocation.
  for (word attempt = 0; attempt < 2 && size < space_->size(); attempt++) {
    uword address = space_->allocate(size);
    if (address != 0) {
      // Allocation succeeded return the address as an object.
      return HeapObject::fromAddress(address + offset);
    }
    if (attempt == 0) {
      // Allocation failed, garbage collect and retry the allocation.
      Thread::current()->runtime()->collectGarbage();
    }
  }
  return Error::outOfMemory();
}

bool Heap::contains(RawObject address) {
  return space_->contains(address.raw());
}

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
  RawObject raw = allocate(allocationSize<RawType>(), RawHeader::kSize);
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawType>();
  result.setHeader(Header::from(RawType::kSize / kPointerSize, 0, metaclass_id,
                                ObjectFormat::kObjects));
  result.initialize(RawType::kSize, NoneType::object());
  DCHECK(Thread::current()->runtime()->isInstanceOfType(result),
         "Invalid Type");
  return result;
}

RawObject Heap::createComplex(double real, double imag) {
  RawObject raw = allocate(allocationSize<RawComplex>(), RawHeader::kSize);
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawComplex>();
  result.setHeader(Header::from(RawComplex::kSize, 0, LayoutId::kComplex,
                                ObjectFormat::kData));
  result.initialize(real, imag);
  return Complex::cast(result);
}

RawObject Heap::createFloat(double value) {
  RawObject raw = allocate(allocationSize<RawFloat>(), RawHeader::kSize);
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawFloat>();
  result.setHeader(
      Header::from(RawFloat::kSize, 0, LayoutId::kFloat, ObjectFormat::kData));
  result.initialize(value);
  return Float::cast(result);
}

RawObject Heap::createEllipsis() {
  RawObject raw = allocate(allocationSize<RawEllipsis>(), RawHeader::kSize);
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawEllipsis>();
  result.setHeader(Header::from(RawEllipsis::kSize, 0, LayoutId::kEllipsis,
                                ObjectFormat::kData));
  return Ellipsis::cast(result);
}

RawObject Heap::createInstance(LayoutId layout_id, word num_attributes) {
  word size = Instance::allocationSize(num_attributes);
  RawObject raw = allocate(size, HeapObject::headerSize(num_attributes));
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawInstance>();
  result.setHeaderAndOverflow(num_attributes, 0, layout_id,
                              ObjectFormat::kObjects);
  result.initialize(num_attributes * kPointerSize, NoneType::object());
  return result;
}

RawObject Heap::createLargeBytes(word length) {
  DCHECK(length > SmallBytes::kMaxLength, "fits into a SmallBytes");
  word size = LargeBytes::allocationSize(length);
  RawObject raw = allocate(size, LargeBytes::headerSize(length));
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawLargeBytes>();
  result.setHeaderAndOverflow(length, 0, LayoutId::kLargeBytes,
                              ObjectFormat::kData);
  return LargeBytes::cast(result);
}

RawObject Heap::createLargeInt(word num_digits) {
  DCHECK(num_digits > 0, "num_digits must be positive");
  word size = LargeInt::allocationSize(num_digits);
  RawObject raw = allocate(size, LargeInt::headerSize(num_digits));
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawLargeInt>();
  result.setHeaderAndOverflow(num_digits * kWordSize, 0, LayoutId::kLargeInt,
                              ObjectFormat::kData);
  return LargeInt::cast(result);
}

RawObject Heap::createLargeStr(word length) {
  DCHECK(length > RawSmallStr::kMaxLength,
         "string len %ld is too small to be a large string", length);
  word size = LargeStr::allocationSize(length);
  RawObject raw = allocate(size, LargeStr::headerSize(length));
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawLargeStr>();
  result.setHeaderAndOverflow(length, 0, LayoutId::kLargeStr,
                              ObjectFormat::kData);
  return LargeStr::cast(result);
}

RawObject Heap::createLayout(LayoutId layout_id) {
  RawObject raw = allocate(allocationSize<RawLayout>(), RawHeader::kSize);
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawLayout>();
  result.setHeader(Header::from(RawLayout::kSize / kPointerSize,
                                static_cast<word>(layout_id), LayoutId::kLayout,
                                ObjectFormat::kObjects));
  result.initialize(RawLayout::kSize, NoneType::object());
  return Layout::cast(result);
}

RawObject Heap::createMutableBytes(word length) {
  DCHECK(length >= 0, "cannot allocate negative size");
  word size = MutableBytes::allocationSize(length);
  RawObject raw = allocate(size, MutableBytes::headerSize(length));
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawMutableBytes>();
  result.setHeaderAndOverflow(length, 0, LayoutId::kMutableBytes,
                              ObjectFormat::kData);
  return MutableBytes::cast(result);
}

RawObject Heap::createTuple(word length) {
  word size = Tuple::allocationSize(length);
  RawObject raw = allocate(size, HeapObject::headerSize(length));
  CHECK(!raw.isError(), "out of memory");
  auto result = raw.rawCast<RawTuple>();
  result.setHeaderAndOverflow(length, 0, LayoutId::kTuple,
                              ObjectFormat::kObjects);
  result.initialize(length * kPointerSize, NoneType::object());
  return Tuple::cast(result);
}

}  // namespace python
