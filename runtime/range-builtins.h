#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

// Returns the same value as Slice::length for SmallInt, but allows LargeInt.
RawObject rangeLen(Thread* thread, const Object& start_obj,
                   const Object& stop_obj, const Object& step_obj);

class LongRangeIteratorBuiltins
    : public Builtins<LongRangeIteratorBuiltins, ID(longrange_iterator),
                      LayoutId::kLongRangeIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LongRangeIteratorBuiltins);
};

class RangeBuiltins
    : public Builtins<RangeBuiltins, ID(range), LayoutId::kRange> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeBuiltins);
};

class RangeIteratorBuiltins
    : public Builtins<RangeIteratorBuiltins, ID(range_iterator),
                      LayoutId::kRangeIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeIteratorBuiltins);
};

}  // namespace py
