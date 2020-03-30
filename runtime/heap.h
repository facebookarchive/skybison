#pragma once

#include "globals.h"
#include "objects.h"
#include "space.h"
#include "visitor.h"

namespace py {

class Heap {
 public:
  explicit Heap(word size);
  ~Heap();

  RawObject allocate(word size, word offset);

  bool contains(RawObject address);
  bool verify();

  Space* space() { return space_; }

  void setSpace(Space* new_space) { space_ = new_space; }

  template <typename T>
  RawObject create();

  RawObject createType(LayoutId metaclass_id);

  RawObject createComplex(double real, double imag);

  RawObject createFloat(double value);

  RawObject createEllipsis();

  RawObject createInstance(LayoutId layout_id, word num_attributes);

  RawObject createLargeBytes(word length);

  RawObject createLargeInt(word num_digits);

  RawObject createLargeStr(word length);

  RawObject createLayout(LayoutId layout_id);

  RawObject createMutableBytes(word length);

  RawObject createMutableTuple(word length);

  RawObject createPointer(void* c_ptr, word length);

  RawObject createRange();

  RawObject createTuple(word length);

  static int spaceOffset() { return offsetof(Heap, space_); };

  void visitAllObjects(HeapObjectVisitor* visitor);

 private:
  Space* space_;
};

template <typename T>
RawObject Heap::create() {
  return createInstance(ObjectLayoutId<T>::value, T::kSize / kPointerSize);
}

}  // namespace py
