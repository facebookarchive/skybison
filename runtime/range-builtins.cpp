#include "range-builtins.h"

#include "frame.h"
#include "globals.h"
#include "runtime.h"

namespace python {

const BuiltinAttribute RangeBuiltins::kAttributes[] = {
    {SymbolId::kStart, Range::kStartOffset, AttributeFlags::kReadOnly},
    {SymbolId::kStep, Range::kStepOffset, AttributeFlags::kReadOnly},
    {SymbolId::kStop, Range::kStopOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod RangeBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RangeBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kRange) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of range");
  }
  for (word i = 1; i < nargs; i++) {
    if (!args.get(i).isSmallInt() && !args.get(i).isUnbound()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "Arguments to range() must be integers");
    }
  }

  word start = 0;
  word stop = 0;
  word step = 1;
  if (args.get(2).isUnbound() && args.get(3).isUnbound()) {
    stop = SmallInt::cast(args.get(1)).value();
  } else if (args.get(3).isUnbound()) {
    start = SmallInt::cast(args.get(1)).value();
    stop = SmallInt::cast(args.get(2)).value();
  } else {
    start = SmallInt::cast(args.get(1)).value();
    stop = SmallInt::cast(args.get(2)).value();
    step = SmallInt::cast(args.get(3)).value();
  }

  if (step == 0) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "range() step argument must not be zero");
  }

  return thread->runtime()->newRange(start, stop, step);
}

RawObject RangeBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRange()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
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
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
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
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__next__() must be called with a range iterator instance as the first "
        "argument");
  }
  Object value(&scope, RangeIterator::cast(*self).next());
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject RangeIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__length_hint__() must be called with a range iterator instance as "
        "the first argument");
  }
  RangeIterator range_iterator(&scope, *self);
  return SmallInt::fromWord(range_iterator.pendingLength());
}

}  // namespace python
