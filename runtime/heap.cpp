#include "heap.h"

#include "objects.h"
#include "runtime.h"

namespace python {

char* Heap::start_ = nullptr;
char* Heap::end_ = nullptr;
char* Heap::ptr_ = nullptr;
intptr_t Heap::size_ = 0;

void Heap::initialize(intptr_t size) {
  Heap::size_ = size;
  Heap::start_ = Heap::ptr_ = new char[size];
  Heap::end_ = &start_[size];
}

Object* Heap::allocate(intptr_t size) {
  char* result = Heap::ptr_;
  Heap::ptr_+= size;
  return HeapObject::fromAddress(reinterpret_cast<uword>(result));
}

Object* Heap::createClass(Layout layout) {
  Object* raw = allocate(Class::allocationSize());
  assert(raw != nullptr);
  Class* result = reinterpret_cast<Class*>(raw);
  result->setClass(Runtime::classClass());
  result->setLayout(layout);
  return Class::cast(result);
}

Object* Heap::createCode(
    int argcount,
    int kwonlyargcount,
    int nlocals,
    int stacksize,
    int flags,
    Object *code,
    Object *consts,
    Object *names,
    Object *varnames,
    Object *freevars,
    Object *cellvars,
    Object *filename,
    Object *name,
    int firstlineno,
    Object *lnotab) {
  Object* raw = allocate(Code::allocationSize());
  assert(raw != nullptr);  // TODO
  Code* result = reinterpret_cast<Code*>(raw);
  result->setClass(Runtime::codeClass());
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

Object* Heap::createByteArray(int length) {
  int size = String::allocationSize(length);
  Object *raw = allocate(size);
  assert(raw != nullptr);
  ByteArray *result = reinterpret_cast<ByteArray*>(raw);
  result->setClass(Runtime::byteArrayClass());
  result->initialize(length);
  return ByteArray::cast(result);
}

Object* Heap::createObjectArray(int length) {
  int size = ObjectArray::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);  // TODO
  ObjectArray* result = reinterpret_cast<ObjectArray*>(raw);
  result->setClass(Runtime::objectArrayClass());
  result->initialize(length, size, 0);
  return ObjectArray::cast(result);
}

Object* Heap::createString(int length) {
  int size = String::allocationSize(length);
  Object* raw = allocate(size);
  assert(raw != nullptr);
  String* result = reinterpret_cast<String*>(raw);
  result->setClass(Runtime::stringClass());
  result->setLength(length);
  return String::cast(result);
}

Object* Heap::createFunction() {
  int size = Function::allocationSize();
  Object* raw = allocate(size);
  assert(raw != nullptr);
  Function* result = reinterpret_cast<Function*>(raw);
  result->setClass(Runtime::functionClass());
  result->initialize();
  return Function::cast(result);
}

Object* Heap::createDictionary() {
    return nullptr;
}

}  // namespace python
