#include "heap.h"

#include "objects.h"
#include "runtime.h"
#include "visitor.h"

namespace python {

Heap::Heap(intptr_t size) {
  from_ = new Space(size);
  to_ = new Space(size);
}

Heap::~Heap() {
  delete from_;
  delete to_;
}

Object* Heap::allocate(intptr_t size) {
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
    if (from_->contains(object->getClass()->address())) {
      Class* the_class = object->getClass();
      scavengePointer(reinterpret_cast<Object**>(&the_class));
      object->setClass(the_class);
    }

    // Scan pointers that follow the header word, if any.
    intptr_t size = object->size();
    if (object->getClass()->isRoot()) {
      auto pointer = reinterpret_cast<Object**>(scan + HeapObject::kSize);
      for (int i = HeapObject::kSize; i < size; i += kPointerSize) {
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
  intptr_t size = from_object->size();
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
    // Check that the object's class pointer is in to-space.
    if (!to_->isAllocated(object->getClass()->address())) {
      return false;
    }
    // Check that the object is entirely within the heap.
    intptr_t size = object->size();
    if (address + size > to_->fill()) {
      return false;
    }
    // If the object has pointers, check that they are valid.
    if (object->getClass()->isRoot()) {
      auto pointer = reinterpret_cast<Object**>(address + HeapObject::kSize);
      for (int i = HeapObject::kSize; i < size; i += kPointerSize) {
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

Object* Heap::createClass(
    Object* class_class,
    Layout layout,
    int size,
    bool isArray,
    bool isRoot) {
  Object* raw = allocate(Class::allocationSize());
  assert(raw != nullptr);
  auto result = reinterpret_cast<Class*>(raw);
  result->setClass(Class::cast(class_class));
  result->initialize(layout, size, isArray, isRoot);
  return Class::cast(result);
}

Object* Heap::createClassClass() {
  Object* raw = allocate(Class::allocationSize());
  assert(raw != nullptr);
  auto result = reinterpret_cast<Class*>(raw);
  result->setClass(result);
  result->initialize(Layout::CLASS, Class::kSize, false, true);
  return Class::cast(result);
}

Object* Heap::createCode(
    Object* code_class,
    int argcount,
    int kwonlyargcount,
    int nlocals,
    int stacksize,
    int flags,
    Object* code,
    Object* consts,
    Object* names,
    Object* varnames,
    Object* freevars,
    Object* cellvars,
    Object* filename,
    Object* name,
    int firstlineno,
    Object* lnotab) {
  Object* raw = allocate(Code::allocationSize());
  assert(raw != nullptr); // TODO
  Code* result = reinterpret_cast<Code*>(raw);
  result->setClass(Class::cast(code_class));
  result->initialize(
      argcount,
      kwonlyargcount,
      nlocals,
      stacksize,
      flags,
      code,
      consts,
      names,
      varnames,
      freevars,
      cellvars,
      filename,
      name,
      firstlineno,
      lnotab);
  return Code::cast(result);
}

Object* Heap::createByteArray(Object* byte_array_class, intptr_t length) {
  int size = ByteArray::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  ByteArray* result = reinterpret_cast<ByteArray*>(raw);
  result->setClass(Class::cast(byte_array_class));
  result->initialize(length);
  return ByteArray::cast(result);
}

Object* Heap::createDictionary(Object* dictionary_class, Object* items) {
  int size = Dictionary::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  Dictionary* result = reinterpret_cast<Dictionary*>(raw);
  result->setClass(Class::cast(dictionary_class));
  result->initialize(items);
  return Dictionary::cast(result);
}

Object* Heap::createFunction(Object* function_class) {
  int size = Function::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  Function* result = reinterpret_cast<Function*>(raw);
  result->setClass(Class::cast(function_class));
  result->initialize();
  return Function::cast(result);
}

Object* Heap::createList(Object* list_class, Object* elements) {
  int size = List::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  List* result = reinterpret_cast<List*>(raw);
  result->setClass(Class::cast(list_class));
  result->initialize(elements);
  return List::cast(result);
}

Object* Heap::createObjectArray(
    Object* object_array_class,
    intptr_t length,
    Object* value) {
  int size = ObjectArray::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr); // TODO
  ObjectArray* result = reinterpret_cast<ObjectArray*>(raw);
  result->setClass(Class::cast(object_array_class));
  result->initialize(length, size, value);
  return ObjectArray::cast(result);
}

Object* Heap::createString(Object* string_class, intptr_t length) {
  int size = String::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  String* result = reinterpret_cast<String*>(raw);
  result->setClass(Class::cast(string_class));
  result->initialize(length);
  return String::cast(result);
}

} // namespace python
