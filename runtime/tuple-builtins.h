#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Return the next item from the iterator, or Error if there are no items left.
RawObject tupleIteratorNext(Thread* thread, const TupleIterator& iter);

// Return a slice of the given tuple.
RawObject tupleSlice(Thread* thread, const Tuple& tuple, word start, word stop,
                     word step);

RawObject tupleHash(Thread* thread, const Tuple& tuple);

class TupleBuiltins
    : public Builtins<TupleBuiltins, SymbolId::kTuple, LayoutId::kTuple> {
 public:
  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderContains(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderHash(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleBuiltins);
};

class TupleIteratorBuiltins
    : public Builtins<TupleIteratorBuiltins, SymbolId::kTupleIterator,
                      LayoutId::kTupleIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleIteratorBuiltins);
};

}  // namespace py
