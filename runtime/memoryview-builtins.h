#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

RawObject memoryviewGetitem(Thread* thread, const MemoryView& view, word index);
RawObject memoryviewGetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step);
word memoryviewItemsize(Thread* thread, const MemoryView& view);
RawObject memoryviewSetitem(Thread* thread, const MemoryView& view, word index,
                            const Object& value);
RawObject memoryviewSetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step, word slice_len,
                             const Object& value);

class MemoryViewBuiltins : public Builtins<MemoryViewBuiltins, ID(memoryview),
                                           LayoutId::kMemoryView> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MemoryViewBuiltins);
};

}  // namespace py
