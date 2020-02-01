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

RawObject METH(tuple, __add__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(tuple, __contains__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(tuple, __hash__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(tuple, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(tuple, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(tuple, __mul__)(Thread* thread, Frame* frame, word nargs);

class TupleBuiltins
    : public Builtins<TupleBuiltins, ID(tuple), LayoutId::kTuple> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleBuiltins);
};

RawObject METH(tuple_iterator, __iter__)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject METH(tuple_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject METH(tuple_iterator, __next__)(Thread* thread, Frame* frame,
                                         word nargs);

class TupleIteratorBuiltins
    : public Builtins<TupleIteratorBuiltins, ID(tuple_iterator),
                      LayoutId::kTupleIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleIteratorBuiltins);
};

}  // namespace py
