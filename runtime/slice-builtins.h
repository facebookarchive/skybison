#pragma once

#include "runtime.h"

namespace py {

RawObject METH(slice, __new__)(Thread* thread, Frame* frame, word nargs);

class SliceBuiltins
    : public Builtins<SliceBuiltins, ID(slice), LayoutId::kSlice> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SliceBuiltins);
};

}  // namespace py
