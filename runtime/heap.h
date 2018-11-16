#pragma once

#include "globals.h"
#include "objects.h"
#include "space.h"

namespace python {

class Heap {
 public:
  explicit Heap(word size);
  ~Heap();

  Object* allocate(word size);

  bool contains(void* address);
  bool verify();
  void flip();
  void scavenge();
  void scavengePointer(Object** pointer);
  Object* transport(Object* oldObject);

  Object* createByteArray(word length);

  Object* createClass(ClassId class_id, Object* super_class);

  Object* createClassClass();

  Object* createCode(
      Object* code_class,
      int32 argcount,
      int32 kwonlyargcount,
      int32 nlocals,
      int32 stacksize,
      int32 flags,
      Object* code,
      Object* consts,
      Object* names,
      Object* varnames,
      Object* freevars,
      Object* cellvars,
      Object* filename,
      Object* name,
      int32 firstlineno,
      Object* lnotab);

  Object* createDictionary(Object* items);

  Object* createFunction();

  Object* createList(Object* elements);

  Object* createModule(Object* name, Object* dictionary);

  Object* createObjectArray(word length, Object* value);

  Object* createString(word length);

 private:
  Space* from_;
  Space* to_;
};

} // namespace python
