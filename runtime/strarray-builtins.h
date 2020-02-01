#pragma once

#include "runtime.h"

namespace py {

RawObject METH(_strarray, __init__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(_strarray, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(_strarray, __str__)(Thread* thread, Frame* frame, word nargs);

class StrArrayBuiltins
    : public Builtins<StrArrayBuiltins, ID(_strarray), LayoutId::kStrArray> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrArrayBuiltins);
};

}  // namespace py
