#include "range-builtins.h"

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "runtime.h"
#include "type-builtins.h"

namespace py {

RawObject rangeLen(Thread* thread, const Object& start_obj,
                   const Object& stop_obj, const Object& step_obj) {
  HandleScope scope(thread);
  Int start(&scope, intUnderlying(*start_obj));
  Int stop(&scope, intUnderlying(*stop_obj));
  Int step(&scope, intUnderlying(*step_obj));
  if (!(start.isLargeInt() || stop.isLargeInt() || step.isLargeInt())) {
    return thread->runtime()->newInt(
        Slice::length(start.asWord(), stop.asWord(), step.asWord()));
  }
  word diff = start.compare(*stop);
  if (step.isNegative()) {
    if (diff > 0) {
      Runtime* runtime = thread->runtime();
      Int tmp1(&scope, runtime->intSubtract(thread, start, stop));
      Int one(&scope, SmallInt::fromWord(1));
      tmp1 = runtime->intSubtract(thread, tmp1, one);
      Int tmp2(&scope, runtime->intNegate(thread, step));
      Object quotient(&scope, NoneType::object());
      bool division_succeeded =
          runtime->intDivideModulo(thread, tmp1, tmp2, &quotient, nullptr);
      DCHECK(division_succeeded, "step must be nonzero");
      tmp1 = *quotient;
      return runtime->intAdd(thread, tmp1, one);
    }
  } else if (diff < 0) {
    Runtime* runtime = thread->runtime();
    Int tmp(&scope, runtime->intSubtract(thread, stop, start));
    Int one(&scope, SmallInt::fromWord(1));
    tmp = runtime->intSubtract(thread, tmp, one);
    Object quotient(&scope, NoneType::object());
    bool division_succeeded =
        runtime->intDivideModulo(thread, tmp, step, &quotient, nullptr);
    DCHECK(division_succeeded, "step must be nonzero");
    tmp = *quotient;
    return runtime->intAdd(thread, tmp, one);
  }
  return SmallInt::fromWord(0);
}

RawObject rangeIteratorNext(const RangeIterator& iter) {
  word length = iter.length();
  if (length == 0) {
    return Error::noMoreItems();
  }
  iter.setLength(length - 1);
  word next = iter.next();
  if (length > 1) {
    word step = iter.step();
    iter.setNext(next + step);
  }
  return SmallInt::fromWord(next);
}

RawObject METH(longrange_iterator, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isLongRangeIterator()) {
    return thread->raiseRequiresType(self, ID(longrange_iterator));
  }
  return *self;
}

RawObject METH(longrange_iterator, __length_hint__)(Thread* thread,
                                                    Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isLongRangeIterator()) {
    return thread->raiseRequiresType(self, ID(longrange_iterator));
  }
  LongRangeIterator iter(&scope, *self);
  Object next(&scope, iter.next());
  Object stop(&scope, iter.stop());
  Object step(&scope, iter.step());
  return rangeLen(thread, next, stop, step);
}

RawObject METH(longrange_iterator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isLongRangeIterator()) {
    return thread->raiseRequiresType(self, ID(longrange_iterator));
  }
  LongRangeIterator iter(&scope, *self);
  Int next(&scope, iter.next());
  Int stop(&scope, iter.stop());
  Int step(&scope, iter.step());
  word diff = next.compare(*stop);
  if ((step.isNegative() && diff <= 0) || (step.isPositive() && diff >= 0)) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  iter.setNext(thread->runtime()->intAdd(thread, next, step));
  return *next;
}

static const BuiltinAttribute kLongRangeIteratorAttributes[] = {
    {ID(_longrange_iterator__next), RawLongRangeIterator::kNextOffset,
     AttributeFlags::kHidden},
    {ID(_longrange_iterator__stop), RawLongRangeIterator::kStopOffset,
     AttributeFlags::kHidden},
    {ID(_longrange_iterator__step), RawLongRangeIterator::kStepOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kRangeAttributes[] = {
    {ID(start), RawRange::kStartOffset, AttributeFlags::kReadOnly},
    {ID(stop), RawRange::kStopOffset, AttributeFlags::kReadOnly},
    {ID(step), RawRange::kStepOffset, AttributeFlags::kReadOnly},
};

static const BuiltinAttribute kRangeIteratorAttributes[] = {
    {ID(_range_iterator__next), RawRangeIterator::kNextOffset,
     AttributeFlags::kHidden},
    {ID(_range_iterator__step), RawRangeIterator::kStepOffset,
     AttributeFlags::kHidden},
    {ID(_range_iterator__length), RawRangeIterator::kLengthOffset,
     AttributeFlags::kHidden},
};

void initializeRangeTypes(Thread* thread) {
  addBuiltinType(thread, ID(range), LayoutId::kRange,
                 /*superclass_id=*/LayoutId::kObject, kRangeAttributes,
                 Range::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(range_iterator), LayoutId::kRangeIterator,
                 /*superclass_id=*/LayoutId::kObject, kRangeIteratorAttributes,
                 RangeIterator::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(longrange_iterator), LayoutId::kLongRangeIterator,
                 /*superclass_id=*/LayoutId::kObject,
                 kLongRangeIteratorAttributes, LongRangeIterator::kSize,
                 /*basetype=*/false);
}

RawObject METH(range, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRange()) {
    return thread->raiseRequiresType(self, ID(range));
  }
  Range range(&scope, *self);
  Object start_obj(&scope, range.start());
  Object stop_obj(&scope, range.stop());
  Object step_obj(&scope, range.step());
  Int start_int(&scope, intUnderlying(*start_obj));
  Int stop_int(&scope, intUnderlying(*stop_obj));
  Int step_int(&scope, intUnderlying(*step_obj));
  Runtime* runtime = thread->runtime();
  if (start_int.isLargeInt() || stop_int.isLargeInt() ||
      step_int.isLargeInt()) {
    return runtime->newLongRangeIterator(start_int, stop_int, step_int);
  }
  word start = start_int.asWord();
  word stop = stop_int.asWord();
  word step = step_int.asWord();
  word length = Slice::length(start, stop, step);
  if (SmallInt::isValid(length)) {
    return runtime->newRangeIterator(start, step, length);
  }
  return runtime->newLongRangeIterator(start_int, stop_int, step_int);
}

RawObject METH(range, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isRange()) {
    return thread->raiseRequiresType(self_obj, ID(range));
  }
  Range self(&scope, *self_obj);
  Object start(&scope, self.start());
  Object stop(&scope, self.stop());
  Object step(&scope, self.step());
  Int len(&scope, rangeLen(thread, start, stop, step));
  // TODO(T55084422): Streamline int bounds checking in Pyro
  if (len.isLargeInt() && LargeInt::cast(*len).numDigits() > 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C ssize_t");
  }
  return *len;
}

RawObject METH(range, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object cls(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*cls)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "range.__new__(X): X is not a type object (%T)",
                                &cls);
  }
  Type type(&scope, *cls);
  if (type.builtinBase() != LayoutId::kRange) {
    Str name(&scope, type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "range.__new__(%S): %S is not a subtype of range",
        &name, &name);
  }

  Object start_or_stop(&scope, args.get(1));
  Object maybe_stop(&scope, args.get(2));
  Object maybe_step(&scope, args.get(3));
  if (maybe_stop.isUnbound()) {
    DCHECK(maybe_step.isUnbound(),
           "cannot provide step without providing both start and stop");
    Object start(&scope, SmallInt::fromWord(0));
    Object stop(&scope, intFromIndex(thread, start_or_stop));
    if (stop.isError()) return *stop;
    Object step(&scope, SmallInt::fromWord(1));
    return runtime->newRange(start, stop, step);
  }

  if (maybe_step.isUnbound()) {
    Object start(&scope, intFromIndex(thread, start_or_stop));
    if (start.isError()) return *start;
    Object stop(&scope, intFromIndex(thread, maybe_stop));
    if (stop.isError()) return *stop;
    Object step(&scope, SmallInt::fromWord(1));
    return runtime->newRange(start, stop, step);
  }

  Object start(&scope, intFromIndex(thread, start_or_stop));
  if (start.isError()) return *start;
  Object stop(&scope, intFromIndex(thread, maybe_stop));
  if (stop.isError()) return *stop;
  Object step(&scope, intFromIndex(thread, maybe_step));
  if (step.isError()) return *step;
  Int step_int(&scope, intUnderlying(*step));
  if (step_int.isZero()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "range() arg 3 must not be zero");
  }
  return runtime->newRange(start, stop, step);
}

RawObject METH(range_iterator, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseRequiresType(self, ID(range_iterator));
  }
  return *self;
}

RawObject METH(range_iterator, __length_hint__)(Thread* thread,
                                                Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseRequiresType(self, ID(range_iterator));
  }
  RangeIterator iter(&scope, *self);
  return SmallInt::fromWord(iter.length());
}

RawObject METH(range_iterator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isRangeIterator()) {
    return thread->raiseRequiresType(self, ID(range_iterator));
  }
  RangeIterator iter(&scope, *self);
  RawObject result = rangeIteratorNext(iter);
  if (result.isErrorNoMoreItems()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return result;
}

}  // namespace py
