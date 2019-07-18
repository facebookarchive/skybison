#include "range-builtins.h"

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
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
    return thread->raiseRequiresType(self, SymbolId::kRange);
  }
  Range range(&scope, *self);
  return thread->runtime()->newRangeIterator(range);
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
    return thread->raiseRequiresType(self, SymbolId::kRangeIterator);
  }
  return *self;
}

RawObject RangeIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kRangeIterator);
  }
  RangeIterator iter(&scope, *self);
  Range range(&scope, iter.iterable());
  word stop = range.stop();
  word step = range.step();
  word index = iter.index();
  if ((step < 0 && index <= stop) || (step > 0 && index >= stop)) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  iter.setIndex(index + step);
  return SmallInt::fromWord(index);
}

RawObject RangeIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kRangeIterator);
  }
  RangeIterator iter(&scope, *self);
  Range range(&scope, iter.iterable());
  word stop = range.stop();
  word step = range.step();
  word index = iter.index();
  return SmallInt::fromWord(std::abs((stop - index) / step));
}

}  // namespace python
