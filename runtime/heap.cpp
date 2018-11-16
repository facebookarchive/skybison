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
  return T::kSize + Header::kSize;
}

Object* Heap::allocate(word size, word offset) {
  DCHECK(size >= HeapObject::kMinimumSize, "allocation %ld too small", size);
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
      Thread::currentThread()->runtime()->collectGarbage();
    }
  }
  return Error::object();
}

bool Heap::contains(void* address) {
  return space_->contains(reinterpret_cast<uword>(address));
}

bool Heap::verify() {
  uword scan = space_->start();
  while (scan < space_->fill()) {
    if (!(*reinterpret_cast<Object**>(scan))->isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      HeapObject* object = HeapObject::fromAddress(scan + Header::kSize);
      // Objects start before the start of the space they are allocated in.
      if (object->baseAddress() < space_->start()) {
        return false;
      }
      // Objects must have their instance data after their header.
      if (object->address() < object->baseAddress()) {
        return false;
      }
      // Objects cannot start after the end of the space they are allocated in.
      if (object->address() > space_->fill()) {
        return false;
      }
      // Objects cannot end after the end of the space they are allocated in.
      uword end = object->baseAddress() + object->size();
      if (end > space_->fill()) {
        return false;
      }
      // Scan pointers that follow the header word, if any.
      if (!object->isRoot()) {
        scan = end;
      } else {
        for (scan += Header::kSize; scan < end; scan += kPointerSize) {
          auto pointer = reinterpret_cast<Object**>(scan);
          if ((*pointer)->isHeapObject()) {
            if (!space_->isAllocated(HeapObject::cast(*pointer)->address())) {
              return false;
            }
          }
        }
      }
    }
  }
  return true;
}

Object* Heap::createBytes(word length) {
  word size = Bytes::allocationSize(length);
  Object* raw = allocate(size, Bytes::headerSize(length));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Bytes*>(raw);
  result->setHeaderAndOverflow(length, 0, LayoutId::kBytes,
                               ObjectFormat::kDataArray8);
  return Bytes::cast(result);
}

Object* Heap::createClass(LayoutId metaclass_id) {
  Object* raw = allocate(allocationSize<Type>(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Type*>(raw);
  result->setHeader(Header::from(Type::kSize / kPointerSize, 0, metaclass_id,
                                 ObjectFormat::kObjectInstance));
  result->initialize(Type::kSize, NoneType::object());
  return Type::cast(result);
}

Object* Heap::createComplex(double real, double imag) {
  Object* raw = allocate(allocationSize<Complex>(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Complex*>(raw);
  result->setHeader(Header::from(Complex::kSize / kPointerSize, 0,
                                 LayoutId::kComplex,
                                 ObjectFormat::kDataInstance));
  result->initialize(real, imag);
  return Complex::cast(result);
}

Object* Heap::createFloat(double value) {
  Object* raw = allocate(allocationSize<Float>(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Float*>(raw);
  result->setHeader(Header::from(Float::kSize / kPointerSize, 0,
                                 LayoutId::kFloat,
                                 ObjectFormat::kDataInstance));
  result->initialize(value);
  return Float::cast(result);
}

Object* Heap::createEllipsis() {
  Object* raw = allocate(allocationSize<Ellipsis>(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Ellipsis*>(raw);
  result->setHeader(Header::from(Ellipsis::kSize / kPointerSize, 0,
                                 LayoutId::kEllipsis,
                                 ObjectFormat::kDataInstance));
  return Ellipsis::cast(result);
}

Object* Heap::createInstance(LayoutId layout_id, word num_attributes) {
  word size = Instance::allocationSize(num_attributes);
  Object* raw = allocate(size, HeapObject::headerSize(num_attributes));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Instance*>(raw);
  result->setHeader(Header::from(num_attributes, 0, layout_id,
                                 ObjectFormat::kObjectInstance));
  result->initialize(num_attributes * kPointerSize, NoneType::object());
  return result;
}

Object* Heap::createLargeInt(word num_digits) {
  DCHECK(num_digits > 0, "num_digits must be positive");
  word size = LargeInt::allocationSize(num_digits);
  Object* raw = allocate(size, LargeInt::headerSize(num_digits));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<LargeInt*>(raw);
  result->setHeader(Header::from(num_digits, 0, LayoutId::kLargeInt,
                                 ObjectFormat::kDataArray64));
  return LargeInt::cast(result);
}

Object* Heap::createLargeStr(word length) {
  DCHECK(length > SmallStr::kMaxLength,
         "string len %ld is too small to be a large string", length);
  word size = LargeStr::allocationSize(length);
  Object* raw = allocate(size, LargeStr::headerSize(length));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<LargeStr*>(raw);
  result->setHeaderAndOverflow(length, 0, LayoutId::kLargeStr,
                               ObjectFormat::kDataArray8);
  return LargeStr::cast(result);
}

Object* Heap::createLayout(LayoutId layout_id) {
  Object* raw = allocate(allocationSize<Layout>(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Layout*>(raw);
  result->setHeader(
      Header::from(Layout::kSize / kPointerSize, static_cast<word>(layout_id),
                   LayoutId::kLayout, ObjectFormat::kObjectInstance));
  result->initialize(Layout::kSize, NoneType::object());
  return Layout::cast(result);
}

Object* Heap::createObjectArray(word length, Object* value) {
  DCHECK(!value->isHeapObject(), "value must be an immediate object");
  word size = ObjectArray::allocationSize(length);
  Object* raw = allocate(size, HeapObject::headerSize(length));
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<ObjectArray*>(raw);
  result->setHeaderAndOverflow(length, 0, LayoutId::kObjectArray,
                               ObjectFormat::kObjectArray);
  result->initialize(size, value);
  return ObjectArray::cast(result);
}

Object* Heap::createRange() {
  Object* raw = allocate(allocationSize<Range>(), Header::kSize);
  CHECK(raw != Error::object(), "out of memory");
  auto result = reinterpret_cast<Range*>(raw);
  result->setHeader(Header::from(Range::kSize / kPointerSize, 0,
                                 LayoutId::kRange,
                                 ObjectFormat::kDataInstance));
  return Range::cast(result);
}

}  // namespace python
