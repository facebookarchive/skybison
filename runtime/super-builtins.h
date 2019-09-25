#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject superGetAttribute(Thread* thread, const Super& super,
                            const Object& name_str, const Object& name_hash);

class SuperBuiltins
    : public Builtins<SuperBuiltins, SymbolId::kSuper, LayoutId::kSuper> {
 public:
  static RawObject dunderGetattribute(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];
};

}  // namespace python
