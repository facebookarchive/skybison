#include "slice-builtins.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

// Converts obj to Int using __index__ if it is not already an instance of Int.
static RawObject sliceIndex(Thread* thread, const Object& obj) {
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfInt(*obj)) {
    return *obj;
  }
  HandleScope scope(thread);
  Object index(&scope, thread->invokeMethod1(obj, SymbolId::kDunderIndex));
  if (index.isError()) {
    if (!thread->hasPendingException()) {
      // Attribute lookup failed
      return thread->raiseTypeErrorWithCStr(
          "slice indices must be integers or None or have an __index__ method");
    }
    return *index;
  }
  if (!runtime->isInstanceOfInt(*index)) {
    return thread->raiseTypeErrorWithCStr("__index__() returned non-int");
  }
  return *index;
}

RawObject sliceUnpack(Thread* thread, const Slice& slice, word* start,
                      word* stop, word* step) {
  HandleScope scope(thread);
  Object step_obj(&scope, slice.step());
  Int max_index(&scope, SmallInt::fromWord(SmallInt::kMaxValue));
  Int min_index(&scope, SmallInt::fromWord(SmallInt::kMinValue));
  if (step_obj.isNoneType()) {
    *step = 1;
  } else {
    step_obj = sliceIndex(thread, step_obj);
    if (step_obj.isError()) return *step_obj;
    Int index(&scope, *step_obj);
    if (index.isZero()) {
      return thread->raiseValueErrorWithCStr("slice step cannot be zero");
    }
    if (index.compare(*max_index) > 0) {
      *step = SmallInt::kMaxValue;
    } else if (index.compare(*min_index) <= 0) {
      // Here *step might be -SmallInt::kMaxValue - 1.
      // In that case, we replace it with -SmallInt::kMaxValue.
      // This doesn't affect the semantics,
      // and it guards against later undefined behaviour resulting from
      // code that does "step = -step" as part of a slice reversal.
      *step = -SmallInt::kMaxValue;
    } else {
      *step = index.asWord();
    }
  }

  Object start_obj(&scope, slice.start());
  if (start_obj.isNoneType()) {
    *start = (*step < 0) ? SmallInt::kMaxValue : 0;
  } else {
    start_obj = sliceIndex(thread, start_obj);
    if (start_obj.isError()) return *start_obj;
    Int index(&scope, *start_obj);
    if (index.compare(*max_index) > 0) {
      *start = SmallInt::kMaxValue;
    } else if (index.compare(*min_index) < 0) {
      *start = SmallInt::kMinValue;
    } else {
      *start = index.asWord();
    }
  }

  Object stop_obj(&scope, slice.stop());
  if (stop_obj.isNoneType()) {
    *stop = (*step < 0) ? SmallInt::kMinValue : SmallInt::kMaxValue;
  } else {
    stop_obj = sliceIndex(thread, stop_obj);
    if (stop_obj.isError()) return *stop_obj;
    Int index(&scope, *stop_obj);
    if (index.compare(*max_index) > 0) {
      *stop = SmallInt::kMaxValue;
    } else if (index.compare(*min_index) < 0) {
      *stop = SmallInt::kMinValue;
    } else {
      *stop = index.asWord();
    }
  }

  return NoneType::object();
}

// TODO(T39495507): make these attributes readonly
const BuiltinAttribute SliceBuiltins::kAttributes[] = {
    {SymbolId::kStart, RawSlice::kStartOffset},
    {SymbolId::kStop, RawSlice::kStopOffset},
    {SymbolId::kStep, RawSlice::kStepOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod SliceBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject SliceBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object type_obj(&scope, args.get(0));
  if (!type_obj.isType()) {
    return thread->raiseTypeErrorWithCStr("'__new__' requires a type object");
  }
  Layout layout(&scope, RawType::cast(*type_obj).instanceLayout());
  if (layout.id() != LayoutId::kSlice) {
    return thread->raiseTypeErrorWithCStr(
        "slice.__new__ requires the slice type");
  }
  Slice slice(&scope, thread->runtime()->newSlice());
  Object arg2(&scope, args.get(2));
  if (arg2.isUnboundValue()) {
    slice.setStop(args.get(1));
    return *slice;
  }
  slice.setStart(args.get(1));
  slice.setStop(*arg2);
  slice.setStep(args.get(3));  // defaults to None
  return *slice;
}

}  // namespace python
