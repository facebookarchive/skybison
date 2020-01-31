#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

// Returns the same value as Slice::length for SmallInt, but allows LargeInt.
RawObject rangeLen(Thread* thread, const Object& start_obj,
                   const Object& stop_obj, const Object& step_obj);

class LongRangeIteratorBuiltins
    : public Builtins<LongRangeIteratorBuiltins, ID(longrange_iterator),
                      LayoutId::kLongRangeIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LongRangeIteratorBuiltins);
};

class RangeBuiltins
    : public Builtins<RangeBuiltins, ID(range), LayoutId::kRange> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeBuiltins);
};

class RangeIteratorBuiltins
    : public Builtins<RangeIteratorBuiltins, ID(range_iterator),
                      LayoutId::kRangeIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeIteratorBuiltins);
};

}  // namespace py
