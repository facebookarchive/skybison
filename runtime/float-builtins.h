#pragma once

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

word doubleHash(double value);

word floatHash(RawObject value);

class FloatBuiltins
    : public Builtins<FloatBuiltins, ID(float), LayoutId::kFloat> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FloatBuiltins);
};

void decodeDouble(double value, bool* is_neg, int* exp, uint64_t* mantissa);

inline word floatHash(RawObject value) {
  return doubleHash(Float::cast(value).value());
}

}  // namespace py
