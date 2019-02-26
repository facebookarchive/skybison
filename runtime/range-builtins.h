#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

// TODO(dulinr): write this in Python
class RangeBuiltins
    : public Builtins<RangeBuiltins, SymbolId::kRange, LayoutId::kRange> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeBuiltins);
};

class RangeIteratorBuiltins
    : public Builtins<RangeIteratorBuiltins, SymbolId::kRangeIterator,
                      LayoutId::kRangeIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeIteratorBuiltins);
};

}  // namespace python
