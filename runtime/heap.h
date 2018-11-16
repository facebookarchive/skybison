#pragma once

#include "globals.h"
#include "objects.h"

namespace python {

class Heap {
 public:
  static void initialize(intptr_t size);

  static Object* allocate(intptr_t size);

  static Object* createCode(
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
  static Object* createByteArray(int length);
  static Object* createObjectArray(int length);
  static Object* createClass(Layout layout);
  static Object* createString(int length);
  static Object* createFunction();
  static Object* createDictionary();

 private:
  static char* start_;
  static char* end_;
  static char* ptr_;
  static intptr_t size_;
};

} // namespace python
