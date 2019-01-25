#include "slice-builtins.h"

#include "frame.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"

namespace python {

// TODO(T39495507): make these attributes readonly
const BuiltinAttribute SliceBuiltins::kAttributes[] = {
    {SymbolId::kStart, RawSlice::kStartOffset},
    {SymbolId::kStop, RawSlice::kStopOffset},
    {SymbolId::kStep, RawSlice::kStepOffset},
};

const BuiltinMethod SliceBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, builtinTrampolineWrapper<dunderNew>},
};

void SliceBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinType(SymbolId::kSlice, LayoutId::kSlice,
                                    LayoutId::kObject, kAttributes, kMethods));
}

RawObject SliceBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object type_obj(&scope, args.get(0));
  if (!type_obj->isType()) {
    return thread->raiseTypeErrorWithCStr("'__new__' requires a type object");
  }
  Layout layout(&scope, RawType::cast(*type_obj).instanceLayout());
  if (layout.id() != LayoutId::kSlice) {
    return thread->raiseTypeErrorWithCStr(
        "slice.__new__ requires the slice type");
  }
  Slice slice(&scope, thread->runtime()->newSlice());
  Object arg2(&scope, args.get(2));
  if (arg2->isUnboundValue()) {
    slice->setStop(args.get(1));
    return *slice;
  }
  slice->setStart(args.get(1));
  slice->setStop(*arg2);
  slice->setStep(args.get(3));  // defaults to None
  return *slice;
}

}  // namespace python
