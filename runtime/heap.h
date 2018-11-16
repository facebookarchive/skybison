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

  Object* createClass();

  Object* createClassMethod();

  Object* createCode();

  Object* createComplex(double real, double imag);

  Object* createDictionary();

  Object* createDouble(double value);

  Object* createEllipsis();

  Object* createFunction();

  Object* createInstance(word layout_id, word num_attributes);

  Object* createLargeInteger(word value);

  Object* createLargeString(word length);

  Object* createLayout(word id);

  Object* createList();

  Object* createListIterator();

  Object* createModule();

  Object* createNotImplemented();

  Object* createObjectArray(word length, Object* value);

  Object* createRange();

  Object* createRangeIterator();

  Object* createSet();

  Object* createSlice();

  Object* createSuper();

  Object* createValueCell();

  Object* createWeakRef();

 private:
  Space* space_;
};

} // namespace python
