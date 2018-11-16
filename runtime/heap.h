#pragma once

#include "globals.h"
#include "objects.h"
#include "space.h"

namespace python {

class Heap {
 public:
  explicit Heap(word size);
  ~Heap();

  Object* allocate(word size, word offset);

  bool contains(void* address);
  bool verify();
  void flip();
  void scavenge();
  void scavengePointer(Object** pointer);
  Object* transport(Object* oldObject);

  Object* createByteArray(word length);

  Object* createClass(ClassId class_id);

  Object* createCode(Object* empty_object_array);

  Object* createDictionary(Object* items);

  Object* createFunction();

  Object* createList(Object* elements);

  Object* createModule(Object* name, Object* dictionary);

  Object* createObjectArray(word length, Object* value);

  Object* createLargeString(word length);

  Object* createValueCell();

  Object* createEllipsis();

 private:
  Space* from_;
  Space* to_;
};

} // namespace python
