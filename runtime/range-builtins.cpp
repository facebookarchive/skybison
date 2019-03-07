#include "range-builtins.h"

#include "frame.h"
#include "globals.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod RangeBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RangeBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kRange) {
    return thread->raiseTypeErrorWithCStr("not a subtype of range");
  }
  for (word i = 1; i < nargs; i++) {
    if (!args.get(i).isSmallInt() && !args.get(i).isUnboundValue()) {
      return thread->raiseTypeErrorWithCStr(
          "Arguments to range() must be integers");
    }
  }

  word start = 0;
  word stop = 0;
  word step = 1;
  if (args.get(2).isUnboundValue() && args.get(3).isUnboundValue()) {
    stop = RawSmallInt::cast(args.get(1)).value();
  } else if (args.get(3).isUnboundValue()) {
    start = RawSmallInt::cast(args.get(1)).value();
    stop = RawSmallInt::cast(args.get(2)).value();
  } else {
    start = RawSmallInt::cast(args.get(1)).value();
    stop = RawSmallInt::cast(args.get(2)).value();
    step = RawSmallInt::cast(args.get(3)).value();
  }

  if (step == 0) {
    return thread->raiseValueErrorWithCStr(
        "range() step argument must not be zero");
  }

  return thread->runtime()->newRange(start, stop, step);
}

RawObject RangeBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRange()) {
    return thread->raiseTypeErrorWithCStr(
        "__getitem__() must be called with a range instance as the first "
        "argument");
  }
  return thread->runtime()->newRangeIterator(self);
}

const BuiltinMethod RangeIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RangeIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a range iterator instance as the first "
        "argument");
  }
  return *self;
}

RawObject RangeIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a range iterator instance as the first "
        "argument");
  }
  Object value(&scope, RawRangeIterator::cast(*self).next());
  if (value.isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject RangeIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a range iterator instance as "
        "the first argument");
  }
  RangeIterator range_iterator(&scope, *self);
  return SmallInt::fromWord(range_iterator.pendingLength());
}

}  // namespace python
