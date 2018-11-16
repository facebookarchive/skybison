#pragma once

#include <cassert>

#include "Globals.h"

class Utils {
 public:
  template <typename T>
  static inline bool IsPowerOfTwo(T x) {
    return (x & (x - 1)) == 0;
  }

  template <typename T>
  static inline T RoundDown(T x, int n) {
    assert(IsPowerOfTwo(n));
    return (x & -n);
  }

  template <typename T>
  static inline T RoundUp(T x, int n) {
    return RoundDown(x + n - 1, n);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Utils);
};
