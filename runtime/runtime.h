#pragma once

#include "heap.h"
#include "objects.h"

namespace python {

class Runtime {
 public:
  Runtime();
  ~Runtime();

  Object* createByteArray(intptr_t length);

  Object* createCode(
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
      Object* lnotab);

  Object* createObjectArray(intptr_t length);

  Object* createString(intptr_t length);

  Object* createList();

  Heap* heap() {
    return &heap_;
  };

  Object* modules() {
    return modules_;
  };

 private:
  void allocateClasses();

  Heap heap_;

  // the equivalent of sys.modules in python
  Object* modules_;

  Object* byteArrayClass_;
  Object* classClass_;
  Object* codeClass_;
  Object* dictionaryClass_;
  Object* functionClass_;
  Object* listClass_;
  Object* moduleClass_;
  Object* objectArrayClass_;
  Object* stringClass_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

} // namespace python
