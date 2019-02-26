#include "range-builtins.h"

#include "frame.h"
#include "globals.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

const NativeMethod RangeBuiltins::kNativeMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RangeBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
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

const NativeMethod RangeIteratorBuiltins::kNativeMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kSentinelId, nullptr},
};

RawObject RangeIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                            word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
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
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a range iterator instance as the first "
        "argument");
  }
  Object value(&scope, RawRangeIterator::cast(*self)->next());
  if (value.isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject RangeIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                  word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
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
