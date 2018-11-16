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

  Space* space() { return space_; }

  void setSpace(Space* new_space) { space_ = new_space; }

  Object* createBoundMethod();

  Object* createByteArray(word length);

  Object* createClass(LayoutId metaclass_id);

  Object* createClassMethod();

  Object* createCode();

  Object* createComplex(double real, double imag);

  Object* createDictionary();

  Object* createFloat(double value);

  Object* createEllipsis();

  Object* createFunction();

  Object* createInstance(LayoutId layout_id, word num_attributes);

  Object* createLargeInteger(word num_digits);

  Object* createLargeString(word length);

  Object* createLayout(LayoutId layout_id);

  Object* createList();

  Object* createListIterator();

  Object* createModule();

  Object* createNotImplemented();

  Object* createObjectArray(word length, Object* value);

  Object* createProperty();

  Object* createRange();

  Object* createRangeIterator();

  Object* createSet();

  Object* createSlice();

  Object* createStaticMethod();

  Object* createSuper();

  Object* createValueCell();

  Object* createWeakRef();

 private:
  Space* space_;
};

}  // namespace python
