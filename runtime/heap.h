#pragma once

#include "globals.h"
#include "objects.h"

namespace python {

class Heap {
 public:
  explicit Heap(intptr_t size);
  ~Heap();

  Object* allocate(intptr_t size);

  bool contains(void* address);

  Object* createByteArray(Object* byte_array_class, intptr_t length);

  Object* createClass(Object* class_class, Layout layout);

  Object* createClassClass();

  Object* createCode(
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
      Object* lnotab);

  Object* createDictionary(Object* dictionary_class);

  Object* createFunction(Object* function_class);

  Object* createObjectArray(Object* object_array_class, intptr_t length);

  Object* createString(Object* string_class, intptr_t length);

 private:
  byte* start_;
  byte* end_;
  byte* ptr_;
  intptr_t size_;
};

} // namespace python
