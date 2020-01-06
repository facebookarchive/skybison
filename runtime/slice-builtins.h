#pragma once

#include "runtime.h"

namespace py {

class SliceBuiltins
    : public Builtins<SliceBuiltins, SymbolId::kSlice, LayoutId::kSlice> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SliceBuiltins);
};

}  // namespace py
