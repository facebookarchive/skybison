#pragma once

#include "globals.h"
#include "objects.h"
#include "space.h"

namespace python {

class Heap {
 public:
  explicit Heap(word size);
  ~Heap();

  RawObject allocate(word size, word offset);

  bool contains(void* address);
  bool verify();

  Space* space() { return space_; }

  void setSpace(Space* new_space) { space_ = new_space; }

  template <typename T>
  RawObject create();

  RawObject createBytes(word length);

  RawObject createClass(LayoutId metaclass_id);

  RawObject createComplex(double real, double imag);

  RawObject createFloat(double value);

  RawObject createEllipsis();

  RawObject createInstance(LayoutId layout_id, word num_attributes);

  RawObject createLargeInt(word num_digits);

  RawObject createLargeStr(word length);

  RawObject createLayout(LayoutId layout_id);

  RawObject createObjectArray(word length, RawObject value);

  RawObject createRange();

 private:
  Space* space_;
};

template <typename T>
RawObject Heap::create() {
  return createInstance(ObjectLayoutId<T>::value, T::kSize / kPointerSize);
}

}  // namespace python
