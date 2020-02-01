#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

// Returns the same value as Slice::length for SmallInt, but allows LargeInt.
RawObject rangeLen(Thread* thread, const Object& start_obj,
                   const Object& stop_obj, const Object& step_obj);

RawObject METH(longrange_iterator, __iter__)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject METH(longrange_iterator, __length_hint__)(Thread* thread,
                                                    Frame* frame, word nargs);
RawObject METH(longrange_iterator, __next__)(Thread* thread, Frame* frame,
                                             word nargs);

class LongRangeIteratorBuiltins
    : public Builtins<LongRangeIteratorBuiltins, ID(longrange_iterator),
                      LayoutId::kLongRangeIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LongRangeIteratorBuiltins);
};

RawObject METH(range, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(range, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(range, __new__)(Thread* thread, Frame* frame, word nargs);

class RangeBuiltins
    : public Builtins<RangeBuiltins, ID(range), LayoutId::kRange> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeBuiltins);
};

RawObject METH(range_iterator, __iter__)(Thread* thread, Frame* frame,
                                         word nargs);
RawObject METH(range_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                                word nargs);
RawObject METH(range_iterator, __next__)(Thread* thread, Frame* frame,
                                         word nargs);

class RangeIteratorBuiltins
    : public Builtins<RangeIteratorBuiltins, ID(range_iterator),
                      LayoutId::kRangeIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeIteratorBuiltins);
};

}  // namespace py
