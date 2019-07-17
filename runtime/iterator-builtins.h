#pragma once

#include "runtime.h"

namespace python {

class SeqIteratorBuiltins
    : public Builtins<SeqIteratorBuiltins, SymbolId::kSeqIterator,
                      LayoutId::kSeqIterator> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SeqIteratorBuiltins);
};

}  // namespace python
