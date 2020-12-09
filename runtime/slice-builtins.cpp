#include "slice-builtins.h"

#include "builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kSliceAttributes[] = {
    {ID(start), RawSlice::kStartOffset, AttributeFlags::kReadOnly},
    {ID(stop), RawSlice::kStopOffset, AttributeFlags::kReadOnly},
    {ID(step), RawSlice::kStepOffset, AttributeFlags::kReadOnly},
};

void initializeSliceType(Thread* thread) {
  addBuiltinType(thread, ID(slice), LayoutId::kSlice,
                 /*superclass_id=*/LayoutId::kObject, kSliceAttributes,
                 Slice::kSize, /*basetype=*/false);
}

bool tryUnpackSlice(const RawObject& key, word* start, word* stop) {
  if (!key.isSlice()) {
    return false;
  }

  RawSlice slice = Slice::cast(key);
  if (!slice.step().isNoneType()) {
    return false;
  }

  RawObject start_obj = slice.start();
  if (start_obj.isNoneType()) {
    *start = 0;
  } else if (start_obj.isSmallInt()) {
    *start = SmallInt::cast(start_obj).value();
  } else {
    return false;
  }

  RawObject stop_obj = slice.stop();
  if (stop_obj.isNoneType()) {
    *stop = kMaxWord;
  } else if (stop_obj.isSmallInt()) {
    *stop = SmallInt::cast(stop_obj).value();
  } else {
    return false;
  }

  return true;
}

RawObject METH(slice, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "'__new__' requires a type object");
  }
  Type type(&scope, *type_obj);
  Layout layout(&scope, type.instanceLayout());
  if (layout.id() != LayoutId::kSlice) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "slice.__new__ requires the slice type");
  }
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  if (args.get(2).isUnbound()) {
    stop = args.get(1);
    return runtime->newSlice(start, stop, step);
  }
  start = args.get(1);
  stop = args.get(2);
  step = args.get(3);  // defaults to None
  return runtime->newSlice(start, stop, step);
}

}  // namespace py
