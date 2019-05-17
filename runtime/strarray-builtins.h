#pragma once

#include "runtime.h"

namespace python {

class StrArrayBuiltins
    : public Builtins<StrArrayBuiltins, SymbolId::kUnderStrArray,
                      LayoutId::kStrArray> {
 public:
  static RawObject dunderIadd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderStr(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StrArrayBuiltins);
};

}  // namespace python
