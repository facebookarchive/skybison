#pragma once

#include "globals.h"
#include "objects.h"
#include "space.h"

namespace python {

class Heap {
 public:
  explicit Heap(intptr_t size);
  ~Heap();

  Object* allocate(intptr_t size);

  bool contains(void* address);
  bool verify();
  void flip();
  void scavenge();
  void scavengePointer(Object** pointer);
  Object* transport(Object* oldObject);

  Object* createByteArray(Object* byte_array_class, intptr_t length);

  Object* createClass(
      Object* class_class,
      Layout layout,
      int size,
      bool isArray,
      bool isRoot);

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

  Object* createDictionary(Object* dictionary_class, Object* items);

  Object* createFunction(Object* function_class);

  Object* createList(Object* list_class, Object* elements);

  Object*
  createObjectArray(Object* object_array_class, intptr_t length, Object* value);

  Object* createString(Object* string_class, intptr_t length);

 private:
  Space* from_;
  Space* to_;
};

} // namespace python
