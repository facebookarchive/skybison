#include "heap.h"

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

Object* Heap::allocate(word size) {
  uword address = to_->allocate(size);
  if (address == 0) {
    return nullptr;
  }
  return HeapObject::fromAddress(address);
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
    // Scan the class pointer embedded in the header word.
    HeapObject* object = HeapObject::fromAddress(scan);
    // Scan pointers that follow the header word, if any.
    word size = object->size();
    if (object->isRoot()) {
      auto pointer = reinterpret_cast<Object**>(scan + HeapObject::kSize);
      for (word i = HeapObject::kSize; i < size; i += kPointerSize) {
        scavengePointer(pointer++);
      }
    }
    scan += size;
  }
  from_->reset();
  from_->protect();
}

Object* Heap::transport(Object* object) {
  HeapObject* from_object = HeapObject::cast(object);
  word size = from_object->size();
  uword address = to_->allocate(size);
  auto dst = reinterpret_cast<void*>(address);
  auto src = reinterpret_cast<void*>(from_object->address());
  memcpy(dst, src, size);
  HeapObject* to_object = HeapObject::fromAddress(address);
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
  uword address = to_->start();
  while (address < to_->fill()) {
    HeapObject* object = HeapObject::fromAddress(address);
    // Check that the object is entirely within the heap.
    word size = object->size();
    if (address + size > to_->fill()) {
      return false;
    }
    // If the object has pointers, check that they are valid.
    if (object->isRoot()) {
      auto pointer = reinterpret_cast<Object**>(address + HeapObject::kSize);
      for (word i = HeapObject::kSize; i < size; i += kPointerSize) {
        if ((*pointer)->isHeapObject()) {
          if (!to_->isAllocated(HeapObject::cast(*pointer)->address()))
            return false;
        }
      }
    }
    address += size;
  }
  return true;
}

Object* Heap::createClass(ClassId class_id, Object* super_class) {
  Object* raw = allocate(Class::allocationSize());
  assert(raw != nullptr);
  auto result = reinterpret_cast<Class*>(raw);
  result->setHeader(Header::from(
      Class::bodySize() / kPointerSize,
      static_cast<uword>(class_id),
      ClassId::kClass,
      ObjectFormat::kObjectInstance));
  result->initialize(super_class);
  return Class::cast(result);
}

Object* Heap::createClassClass() {
  Object* raw = allocate(Class::allocationSize());
  assert(raw != nullptr);
  auto result = reinterpret_cast<Class*>(raw);
  result->setHeader(Header::from(
      Class::bodySize() / kPointerSize,
      static_cast<uword>(ClassId::kClass),
      ClassId::kClass,
      ObjectFormat::kObjectInstance));
  result->initialize(raw);
  return Class::cast(result);
}

Object* Heap::createCode(Object* empty_object_array) {
  Object* raw = allocate(Code::allocationSize());
  assert(raw != nullptr);
  auto result = reinterpret_cast<Code*>(raw);
  result->setHeader(Header::from(
      Code::bodySize() / kPointerSize,
      0,
      ClassId::kCode,
      ObjectFormat::kObjectInstance));
  result->initialize(empty_object_array);
  return Code::cast(result);
}

Object* Heap::createByteArray(word length) {
  word size = ByteArray::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  auto result = reinterpret_cast<ByteArray*>(raw);
  result->setHeader(Header::from(
      length, 0, ClassId::kByteArray, ObjectFormat::kObjectInstance));
  result->initialize(length);
  return ByteArray::cast(result);
}

Object* Heap::createDictionary(Object* items) {
  word size = Dictionary::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Dictionary*>(raw);
  result->setHeader(Header::from(
      Dictionary::bodySize() / kPointerSize,
      0,
      ClassId::kDictionary,
      ObjectFormat::kObjectInstance));
  result->initialize(items);
  return Dictionary::cast(result);
}

Object* Heap::createFunction() {
  word size = Function::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Function*>(raw);
  result->setHeader(Header::from(
      Function::bodySize() / kPointerSize,
      0,
      ClassId::kFunction,
      ObjectFormat::kObjectInstance));
  result->initialize();
  return Function::cast(result);
}

Object* Heap::createList(Object* elements) {
  word size = List::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  auto result = reinterpret_cast<List*>(raw);
  result->setHeader(Header::from(
      List::bodySize() / kPointerSize,
      0,
      ClassId::kList,
      ObjectFormat::kObjectInstance));
  result->initialize(elements);
  return List::cast(result);
}

Object* Heap::createModule(Object* name, Object* dictionary) {
  int size = Module::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  auto result = reinterpret_cast<Module*>(raw);
  result->setHeader(Header::from(
      Module::bodySize() / kPointerSize,
      0,
      ClassId::kModule,
      ObjectFormat::kObjectInstance));
  result->initialize(name, dictionary);
  return Module::cast(result);
}

Object* Heap::createObjectArray(word length, Object* value) {
  int size = ObjectArray::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  auto result = reinterpret_cast<ObjectArray*>(raw);
  result->setHeader(Header::from(
      length, 0, ClassId::kObjectArray, ObjectFormat::kObjectArray));
  result->initialize(size, value);
  return ObjectArray::cast(result);
}

Object* Heap::createString(word length) {
  int size = String::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  auto result = reinterpret_cast<String*>(raw);
  result->setHeader(
      Header::from(length, 0, ClassId::kString, ObjectFormat::kDataArray8));
  result->initialize(length);
  return String::cast(result);
}

} // namespace python
