#pragma once

#include "runtime.h"

namespace python {

// Unpacks the slice indices into the three words.
// Picks an appropriate default value for any None index.
// If the index is not None or an integer, calls __index__ to convert it.
// Silently fits each index into a SmallInt,
// and ensures that step >= -SmallInt::kMaxValue for safe slice reversal.
// Raises a ValueError when the step is 0.
RawObject sliceUnpack(Thread* thread, const Slice& slice, word* start,
                      word* stop, word* step);

class SliceBuiltins
    : public Builtins<SliceBuiltins, SymbolId::kSlice, LayoutId::kSlice> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SliceBuiltins);
};

}  // namespace python
