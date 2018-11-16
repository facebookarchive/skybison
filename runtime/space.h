#pragma once

#include "globals.h"
#include "utils.h"

namespace python {

class Space {
 public:
  explicit Space(word size);
  ~Space();

  uword allocate(word size) {
    word rounded = Utils::roundUp(size, kPointerSize * 2);
    word free = end() - fill();
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

  word size() {
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
