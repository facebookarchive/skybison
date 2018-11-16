#include "heap.h"

#include <cstring>

#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace python {

Heap::Heap(word size) {
  from_ = new Space(size);
  to_ = new Space(size);
}

Heap::~Heap() {
  delete from_;
  delete to_;
}

Object* Heap::allocate(word size, word offset) {
  assert(size >= HeapObject::kMinimumSize);
  assert(Utils::isAligned(size, kPointerSize));
  uword address = to_->allocate(size);
  if (address == 0) {
    return nullptr;
  }
  return HeapObject::fromAddress(address + offset);
}

void Heap::scavengePointer(Object** pointer) {
  if (!(*pointer)->isHeapObject()) {
    return;
  }
  HeapObject* object = HeapObject::cast(*pointer);
  if (from_->contains(object->address())) {
    if (object->isForwarding()) {
      *pointer = object->forward();
    } else {
      *pointer = transport(object);
    }
  }
}

void Heap::scavenge() {
  uword scan = to_->start();
  while (scan < to_->fill()) {
    if (!(*reinterpret_cast<Object**>(scan))->isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      HeapObject* object = HeapObject::fromAddress(scan + Header::kSize);
      uword end = object->baseAddress() + object->size();
      // Scan pointers that follow the header word, if any.
      if (!object->isRoot()) {
        scan = end;
      } else {
        for (scan += Header::kSize; scan < end; scan += kPointerSize) {
          scavengePointer(reinterpret_cast<Object**>(scan));
        }
      }
    }
  }
  from_->reset();
  from_->protect();
}

Object* Heap::transport(Object* object) {
  HeapObject* from_object = HeapObject::cast(object);
  word size = from_object->size();
  uword address = to_->allocate(size);
  auto dst = reinterpret_cast<void*>(address);
  auto src = reinterpret_cast<void*>(from_object->baseAddress());
  std::memcpy(dst, src, size);
  word offset = from_object->address() - from_object->baseAddress();
  HeapObject* to_object = HeapObject::fromAddress(address + offset);
  from_object->forwardTo(to_object);
  return to_object;
}

void Heap::flip() {
  from_->unprotect();
  Space* tmp = from_;
  from_ = to_;
  to_ = tmp;
}

bool Heap::contains(void* address) {
  return to_->contains(reinterpret_cast<uword>(address));
}

bool Heap::verify() {
  uword scan = to_->start();
  while (scan < to_->fill()) {
    if (!(*reinterpret_cast<Object**>(scan))->isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      HeapObject* object = HeapObject::fromAddress(scan + Header::kSize);
      // Objects start before the start of the space they are allocated in.
      if (object->baseAddress() < to_->start()) {
        return false;
      }
      // Objects must have their instance data after their header.
      if (object->address() < object->baseAddress()) {
        return false;
      }
      // Objects cannot start after the end of the space they are allocated in.
      if (object->address() > to_->fill()) {
        return false;
      }
      // Objects cannot end after the end of the space they are allocated in.
      uword end = object->baseAddress() + object->size();
      if (end > to_->fill()) {
        return false;
      }
      // Scan pointers that follow the header word, if any.
      if (!object->isRoot()) {
        scan = end;
      } else {
        for (scan += Header::kSize; scan < end; scan += kPointerSize) {
          auto pointer = reinterpret_cast<Object**>(scan);
          if ((*pointer)->isHeapObject()) {
            if (!to_->isAllocated(HeapObject::cast(*pointer)->address())) {
              return false;
            }
          }
        }
      }
    }
  }
  return true;
}

Object* Heap::createClass(ClassId class_id) {
  Object* raw = allocate(Class::allocationSize(), Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Class*>(raw);
  result->setHeader(Header::from(
      Class::kSize / kPointerSize,
      static_cast<uword>(class_id),
      ClassId::kType,
      ObjectFormat::kObjectInstance));
  return Class::cast(result);
}

Object* Heap::createCode(Object* empty_object_array) {
  Object* raw = allocate(Code::allocationSize(), Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Code*>(raw);
  result->setHeader(Header::from(
      Code::kSize / kPointerSize,
      0,
      ClassId::kCode,
      ObjectFormat::kObjectInstance));
  result->initialize(empty_object_array);
  return Code::cast(result);
}

Object* Heap::createByteArray(word length) {
  word size = ByteArray::allocationSize(length);
  Object* raw = allocate(size, ByteArray::headerSize(length));
  assert(raw != nullptr);
  auto result = reinterpret_cast<ByteArray*>(raw);
  result->setHeaderAndOverflow(
      length, 0, ClassId::kByteArray, ObjectFormat::kDataArray8);
  return ByteArray::cast(result);
}

Object* Heap::createDictionary(Object* data) {
  word size = Dictionary::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Dictionary*>(raw);
  result->setHeader(Header::from(
      Dictionary::kSize / kPointerSize,
      0,
      ClassId::kDictionary,
      ObjectFormat::kObjectInstance));
  result->initialize(data);
  return Dictionary::cast(result);
}

Object* Heap::createFunction() {
  word size = Function::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Function*>(raw);
  result->setHeader(Header::from(
      Function::kSize / kPointerSize,
      0,
      ClassId::kFunction,
      ObjectFormat::kObjectInstance));
  result->initialize();
  return Function::cast(result);
}

Object* Heap::createList(Object* elements) {
  word size = List::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<List*>(raw);
  result->setHeader(Header::from(
      List::kSize / kPointerSize,
      0,
      ClassId::kList,
      ObjectFormat::kObjectInstance));
  result->initialize(elements);
  return List::cast(result);
}

Object* Heap::createModule(Object* name, Object* dictionary) {
  word size = Module::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Module*>(raw);
  result->setHeader(Header::from(
      Module::kSize / kPointerSize,
      0,
      ClassId::kModule,
      ObjectFormat::kObjectInstance));
  result->initialize(name, dictionary);
  return Module::cast(result);
}

Object* Heap::createObjectArray(word length, Object* value) {
  word size = ObjectArray::allocationSize(length);
  Object* raw = allocate(size, HeapObject::headerSize(length));
  assert(raw != nullptr);
  auto result = reinterpret_cast<ObjectArray*>(raw);
  result->setHeaderAndOverflow(
      length, 0, ClassId::kObjectArray, ObjectFormat::kObjectArray);
  result->initialize(size, value);
  return ObjectArray::cast(result);
}

Object* Heap::createLargeString(word length) {
  assert(length > SmallString::kMaxLength);
  word size = LargeString::allocationSize(length);
  Object* raw = allocate(size, LargeString::headerSize(length));
  assert(raw != nullptr);
  auto result = reinterpret_cast<LargeString*>(raw);
  result->setHeaderAndOverflow(
      length, 0, ClassId::kLargeString, ObjectFormat::kDataArray8);
  return LargeString::cast(result);
}

Object* Heap::createValueCell() {
  word size = ValueCell::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<ValueCell*>(raw);
  result->setHeader(Header::from(
      ValueCell::kSize / kPointerSize,
      0,
      ClassId::kValueCell,
      ObjectFormat::kObjectInstance));
  return ValueCell::cast(result);
}

Object* Heap::createEllipsis() {
  word size = Ellipsis::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Ellipsis*>(raw);
  result->setHeader(Header::from(
      Ellipsis::kSize / kPointerSize,
      0,
      ClassId::kEllipsis,
      ObjectFormat::kDataInstance));
  return Ellipsis::cast(result);
}

Object* Heap::createRange() {
  word size = Range::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Range*>(raw);
  result->setHeader(Header::from(
      Range::kSize / kPointerSize,
      0,
      ClassId::kRange,
      ObjectFormat::kDataInstance));
  return Range::cast(result);
}

Object* Heap::createRangeIterator() {
  word size = RangeIterator::allocationSize();
  Object* raw = allocate(size, Header::kSize);
  assert(raw != nullptr);
  auto result = reinterpret_cast<RangeIterator*>(raw);
  result->setHeader(Header::from(
      RangeIterator::kSize / kPointerSize,
      0,
      ClassId::kRangeIterator,
      ObjectFormat::kObjectInstance));
  return RangeIterator::cast(result);
}

} // namespace python
