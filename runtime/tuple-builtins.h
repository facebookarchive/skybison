#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// If seq is a tuple (not a tuple subtype), return it. Otherwise, attempt to
// treat it as an iterable object and return a tuple with its elements. May
// return Error if an exception is raised at any point.
RawObject sequenceAsTuple(Thread* thread, const Object& seq);

// Return the next item from the iterator, or Error if there are no items left.
RawObject tupleIteratorNext(Thread* thread, const TupleIterator& iter);

class TupleBuiltins {
 public:
  static void initialize(Runtime* runtime);

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

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleBuiltins);
};

class TupleIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleIteratorBuiltins);
};

}  // namespace python
