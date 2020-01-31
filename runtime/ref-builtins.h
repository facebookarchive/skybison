#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class RefBuiltins : public Builtins<RefBuiltins, ID(ref), LayoutId::kWeakRef> {
 public:
  static RawObject dunderCall(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
};

}  // namespace py
