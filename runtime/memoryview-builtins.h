#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

RawObject memoryviewItemsize(Thread* thread, const MemoryView& view);

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

}  // namespace python
