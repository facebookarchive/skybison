#include "heap.h"

#include "objects.h"
#include "os.h"
#include "runtime.h"

namespace python {

Heap::Heap(intptr_t size) {
  size_ = size;
  start_ = Heap::ptr_ = Os::allocateMemory(size);
  end_ = &start_[size];
}

Heap::~Heap() {
  Os::freeMemory(start_, size_);
}

Object* Heap::allocate(intptr_t size) {
  assert(size > 0);
  byte* result = Heap::ptr_;
  if (size > (end_ - ptr_)) {
    return nullptr;
  }
  ptr_ += size;
  return HeapObject::fromAddress(reinterpret_cast<uword>(result));
}

bool Heap::contains(void* address) {
  return address >= start_ && address < end_;
}

Object* Heap::createClass(Object* class_class, Layout layout) {
  Object* raw = allocate(Class::allocationSize());
  assert(raw != nullptr);
  Class* result = reinterpret_cast<Class*>(raw);
  result->setClass(Class::cast(class_class));
  result->setLayout(layout);
  return Class::cast(result);
}

Object* Heap::createClassClass() {
  Object* raw = allocate(Class::allocationSize());
  assert(raw != nullptr);
  Class* result = reinterpret_cast<Class*>(raw);
  result->setClass(result);
  result->setLayout(Layout::CLASS);
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
  int size = String::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  ByteArray* result = reinterpret_cast<ByteArray*>(raw);
  result->setClass(Class::cast(byte_array_class));
  result->initialize(length);
  return ByteArray::cast(result);
}

Object* Heap::createObjectArray(Object* object_array_class, intptr_t length) {
  int size = ObjectArray::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr); // TODO
  ObjectArray* result = reinterpret_cast<ObjectArray*>(raw);
  result->setClass(Class::cast(object_array_class));
  result->initialize(length, size, 0);
  return ObjectArray::cast(result);
}

Object* Heap::createString(Object* string_class, intptr_t length) {
  int size = String::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  String* result = reinterpret_cast<String*>(raw);
  result->setClass(Class::cast(string_class));
  result->setLength(length);
  return String::cast(result);
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

Object* Heap::createDictionary(Object* dictionary_class) {
  return nullptr;
}

Object* Heap::createList(Object* list_class) {
  int size = List::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  List* result = reinterpret_cast<List*>(raw);
  result->setClass(Class::cast(list_class));
  result->initialize();
  return List::cast(result);
}

} // namespace python
