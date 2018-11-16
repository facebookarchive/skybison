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

  template <typename T>
  Object* create();

  Object* createBytes(word length);

  Object* createClass(LayoutId metaclass_id);

  Object* createComplex(double real, double imag);

  Object* createFloat(double value);

  Object* createEllipsis();

  Object* createInstance(LayoutId layout_id, word num_attributes);

  Object* createLargeInt(word num_digits);

  Object* createLargeStr(word length);

  Object* createLayout(LayoutId layout_id);

  Object* createObjectArray(word length, Object* value);

  Object* createRange();

 private:
  Space* space_;
};

template <typename T>
Object* Heap::create() {
  return createInstance(ObjectLayoutId<T>::value, T::kSize / kPointerSize);
}

}  // namespace python
