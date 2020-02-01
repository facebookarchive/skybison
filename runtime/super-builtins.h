#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject superGetAttribute(Thread* thread, const Super& super,
                            const Object& name);

RawObject METH(super, __getattribute__)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject METH(super, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(super, __init__)(Thread* thread, Frame* frame, word nargs);

class SuperBuiltins
    : public Builtins<SuperBuiltins, ID(super), LayoutId::kSuper> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];
};

}  // namespace py
