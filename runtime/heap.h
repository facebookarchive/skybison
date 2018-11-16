#pragma once

#include "globals.h"
#include "objects.h"

namespace python {

class Heap {
public:
  static void Initialize(intptr_t size);

  static Object* Allocate(intptr_t size);

  static Object* CreateCode(
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
  static Object* CreateByteArray(int length);
  static Object* CreateObjectArray(int length);
  static Object* CreateClass(Layout layout);
  static Object* CreateString(int length);
  static Object* CreateFunction();
  static Object* CreateDictionary();

private:
  static char* start_;
  static char* end_;
  static char* ptr_;
  static intptr_t size_;
};

}  // namespace python
