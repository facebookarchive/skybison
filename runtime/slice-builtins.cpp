#include "slice-builtins.h"

#include "frame.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"

namespace python {

static ALWAYS_INLINE RawObject sliceIndex(Thread* thread, const Object& obj) {
  if (obj.isInt()) return *obj;
  if (thread->runtime()->isInstanceOfInt(*obj)) {
    return intUnderlying(thread, obj);
  }
  return thread->invokeFunction1(SymbolId::kBuiltins,
                                 SymbolId::kUnderSliceIndex, obj);
}

RawObject sliceUnpack(Thread* thread, const Slice& slice, word* start,
                      word* stop, word* step) {
  HandleScope scope(thread);
  Object step_obj(&scope, slice.step());
  if (step_obj.isNoneType()) {
    *step = 1;
  } else {
    step_obj = sliceIndex(thread, step_obj);
    if (step_obj.isError()) return *step_obj;
    Int index(&scope, *step_obj);
    if (index.isZero()) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "slice step cannot be zero");
    }
    word step_word = index.asWordSaturated();
    if (step_word > SmallInt::kMaxValue) {
      *step = SmallInt::kMaxValue;
    } else if (step_word <= SmallInt::kMinValue) {
      // Here *step might be -SmallInt::kMaxValue - 1.
      // In that case, we replace it with -SmallInt::kMaxValue.
      // This doesn't affect the semantics,
      // and it guards against later undefined behaviour resulting from
      // code that does "step = -step" as part of a slice reversal.
      *step = -SmallInt::kMaxValue;
    } else {
      *step = step_word;
    }
  }

  Object start_obj(&scope, slice.start());
  if (start_obj.isNoneType()) {
    *start = (*step < 0) ? SmallInt::kMaxValue : 0;
  } else {
    start_obj = sliceIndex(thread, start_obj);
    if (start_obj.isError()) return *start_obj;
    Int index(&scope, *start_obj);
    word start_word = index.asWordSaturated();
    if (start_word > SmallInt::kMaxValue) {
      *start = SmallInt::kMaxValue;
    } else if (start_word < SmallInt::kMinValue) {
      *start = SmallInt::kMinValue;
    } else {
      *start = start_word;
    }
  }

  Object stop_obj(&scope, slice.stop());
  if (stop_obj.isNoneType()) {
    *stop = (*step < 0) ? SmallInt::kMinValue : SmallInt::kMaxValue;
  } else {
    stop_obj = sliceIndex(thread, stop_obj);
    if (stop_obj.isError()) return *stop_obj;
    Int index(&scope, *stop_obj);
    word stop_word = index.asWordSaturated();
    if (stop_word > SmallInt::kMaxValue) {
      *stop = SmallInt::kMaxValue;
    } else if (stop_word < SmallInt::kMinValue) {
      *stop = SmallInt::kMinValue;
    } else {
      *stop = stop_word;
    }
  }

  return NoneType::object();
}

// TODO(T39495507): make these attributes readonly
const BuiltinAttribute SliceBuiltins::kAttributes[] = {
    {SymbolId::kStart, RawSlice::kStartOffset, AttributeFlags::kReadOnly},
    {SymbolId::kStop, RawSlice::kStopOffset, AttributeFlags::kReadOnly},
    {SymbolId::kStep, RawSlice::kStepOffset, AttributeFlags::kReadOnly},
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
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__new__' requires a type object");
  }
  Layout layout(&scope, Type::cast(*type_obj).instanceLayout());
  if (layout.id() != LayoutId::kSlice) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "slice.__new__ requires the slice type");
  }
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  if (args.get(2).isUnbound()) {
    stop = args.get(1);
    return thread->runtime()->newSlice(start, stop, step);
  }
  start = args.get(1);
  stop = args.get(2);
  step = args.get(3);  // defaults to None
  return thread->runtime()->newSlice(start, stop, step);
}

}  // namespace python
