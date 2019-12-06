#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

RawObject memoryviewItemsize(Thread* thread, const MemoryView& view);
RawObject memoryviewSetitem(Thread* thread, const MemoryView& view,
                            const Int& index, const Object& value);
RawObject memoryviewSetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step, word slice_len,
                             const Object& value);

class MemoryViewBuiltins
    : public Builtins<MemoryViewBuiltins, SymbolId::kMemoryView,
                      LayoutId::kMemoryView> {
 public:
  static RawObject cast(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MemoryViewBuiltins);
};

}  // namespace py
