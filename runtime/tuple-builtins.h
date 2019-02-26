#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// Helper function for struct sequences to look for hidden fields in the
// instance's attributes. This should only be used through the
// struct sequence field descriptors and through the C-API
RawObject underStructseqGetAttr(Thread* thread, Frame* frame, word nargs);

// Helper function for struct sequences to bypass its descriptor immutability.
// This should only be used when creating struct sequences and
// through the C-API
RawObject underStructseqSetAttr(Thread* thread, Frame* frame, word nargs);

// If seq is a tuple (not a tuple subtype), return it. Otherwise, attempt to
// treat it as an iterable object and return a tuple with its elements. May
// return Error if an exception is raised at any point.
RawObject sequenceAsTuple(Thread* thread, const Object& seq);

// Return the next item from the iterator, or Error if there are no items left.
RawObject tupleIteratorNext(Thread* thread, const TupleIterator& iter);

class TupleBuiltins
    : public Builtins<TupleBuiltins, SymbolId::kTuple, LayoutId::kTuple> {
 public:
  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static RawObject slice(Thread* thread, const Tuple& tuple,
                         const Slice& slice);
  static RawObject sliceWithWords(Thread* thread, const Tuple& tuple,
                                  word start, word stop, word step);

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

}  // namespace python
