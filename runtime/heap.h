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

  Space* space() {
    return space_;
  }

  void setSpace(Space* new_space) {
    space_ = new_space;
  }

  Object* createBoundMethod();

  Object* createByteArray(word length);

  Object* createClass(ClassId class_id);

  Object* createClassMethod();

  Object* createCode(Object* empty_object_array);

  Object* createDictionary(Object* items);

  Object* createDouble(double value);

  Object* createSet(Object* items);

  Object* createFunction();

  Object* createInstance(ClassId class_id, word num_attributes);

  Object* createLargeInteger(word value);

  Object* createList(Object* elements);

  Object* createListIterator();

  Object* createModule(Object* name, Object* dictionary);

  Object* createObjectArray(word length, Object* value);

  Object* createLargeString(word length);

  Object* createValueCell();

  Object* createEllipsis();

  Object* createRange();

  Object* createRangeIterator();

  Object* createSlice();

  Object* createWeakRef();

 private:
  Space* space_;
};

} // namespace python
