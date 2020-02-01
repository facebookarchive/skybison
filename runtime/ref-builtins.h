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

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RefBuiltins);
};

class WeakLinkBuiltins
    : public Builtins<WeakLinkBuiltins, ID(_weaklink), LayoutId::kWeakLink> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(WeakLinkBuiltins);
};

}  // namespace py
