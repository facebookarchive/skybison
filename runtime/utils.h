#pragma once

#include <cassert>

#include "globals.h"

class Utils {
 public:
  template <typename T>
  static inline bool isPowerOfTwo(T x) {
    return (x & (x - 1)) == 0;
  }

  template <typename T>
  static inline T roundDown(T x, int n) {
    assert(isPowerOfTwo(n));
    return (x & -n);
  }

  template <typename T>
  static inline T roundUp(T x, int n) {
    return roundDown(x + n - 1, n);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Utils);
};
