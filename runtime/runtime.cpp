#include "runtime.h"

#include "globals.h"
#include "handle.h"
#include "heap.h"

namespace python {

Runtime::Runtime() : heap_(64 * MiB) {
  Handles::initialize();
  allocateClasses();
}

Runtime::~Runtime() {}

Object* Runtime::createByteArray(intptr_t length) {
  return heap()->createByteArray(Runtime::byteArrayClass_, length);
}

Object* Runtime::createCode(
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
  return heap()->createCode(
      codeClass_,
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
}

Object* Runtime::createObjectArray(intptr_t length) {
  return heap()->createObjectArray(objectArrayClass_, length);
}

Object* Runtime::createString(intptr_t length) {
  return heap()->createString(stringClass_, length);
}

void Runtime::allocateClasses() {
  classClass_ = heap()->createClassClass();
  byteArrayClass_ = heap()->createClass(classClass_, Layout::BYTE_ARRAY);
  objectArrayClass_ = heap()->createClass(classClass_, Layout::OBJECT_ARRAY);
  codeClass_ = heap()->createClass(classClass_, Layout::CODE);
  stringClass_ = heap()->createClass(classClass_, Layout::STRING);
  functionClass_ = heap()->createClass(classClass_, Layout::FUNCTION);
}

} // namespace python
