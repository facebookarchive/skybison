#pragma once

#include "globals.h"
#include "utils.h"

namespace python {

class Space {
 public:
  explicit Space(intptr_t size);
  ~Space();

  uword allocate(intptr_t size) {
    intptr_t rounded = Utils::roundUp(size, kPointerSize);
    intptr_t free = end() - fill();
    if (rounded > free) {
      return 0;
    }
    uword result = fill_;
    fill_ += rounded;
    return result;
  }

  void protect();

  void unprotect();

  bool contains(uword address) {
    return address >= start() && address < end();
  }

  bool isAllocated(uword address) {
    return address >= start() && address < fill();
  }

  uword start() {
    return start_;
  }

  uword end() {
    return end_;
  }

  uword fill() {
    return fill_;
  }

  void reset() {
    memset(reinterpret_cast<void*>(start()), 0xFF, size());
    fill_ = start();
  }

  intptr_t size() {
    return end_ - start_;
  }

 private:
  uword start_;
  uword end_;
  uword fill_;

  byte* raw_;

  DISALLOW_COPY_AND_ASSIGN(Space);
};

} // namespace python
