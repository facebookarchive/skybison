#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

class MemoryViewBuiltins
    : public Builtins<MemoryViewBuiltins, SymbolId::kMemoryView,
                      LayoutId::kMemoryView> {
 public:
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MemoryViewBuiltins);
};

}  // namespace python
