#pragma once

#include "runtime.h"

namespace py {

class SeqIteratorBuiltins : public Builtins<SeqIteratorBuiltins, ID(iterator),
                                            LayoutId::kSeqIterator> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SeqIteratorBuiltins);
};

}  // namespace py
