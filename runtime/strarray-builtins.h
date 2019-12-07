#pragma once

#include "runtime.h"

namespace py {

class StrArrayBuiltins
    : public Builtins<StrArrayBuiltins, SymbolId::kUnderStrarray,
                      LayoutId::kStrArray> {
 public:
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderStr(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrArrayBuiltins);
};

}  // namespace py
