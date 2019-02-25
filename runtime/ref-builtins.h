#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class RefBuiltins
    : public Builtins<RefBuiltins, SymbolId::kRef, LayoutId::kWeakRef> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
};

}  // namespace python
