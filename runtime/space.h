#pragma once

#include "globals.h"
#include "utils.h"

namespace py {

class Space {
 public:
  explicit Space(word size);
  ~Space();

  static word roundAllocationSize(word size) {
    return Utils::roundUp(size, kPointerSize * 2);
  }

  uword allocate(word size) {
    word rounded = roundAllocationSize(size);
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

  bool contains(uword address) { return address >= start() && address < end(); }

  bool isAllocated(uword address) {
    return address >= start() && address < fill();
  }

  uword start() { return start_; }

  uword end() { return end_; }

  uword fill() { return fill_; }

  void reset();

  word size() { return end_ - start_; }

  static int endOffset() { return offsetof(Space, end_); }

  static int fillOffset() { return offsetof(Space, fill_); }

 private:
  uword start_;
  uword end_;
  uword fill_;

  byte* raw_;

  DISALLOW_COPY_AND_ASSIGN(Space);
};

}  // namespace py
