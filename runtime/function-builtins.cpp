#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod FunctionBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGet, dunderGet},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinAttribute FunctionBuiltins::kAttributes[] = {
    {SymbolId::kDunderCode, RawFunction::kCodeOffset},
    {SymbolId::kDunderDoc, RawFunction::kDocOffset},
    {SymbolId::kDunderModule, RawFunction::kModuleOffset},
    {SymbolId::kDunderName, RawFunction::kNameOffset},
    {SymbolId::kDunderQualname, RawFunction::kQualnameOffset},
    {SymbolId::kDunderDict, RawFunction::kDictOffset},
    {SymbolId::kSentinelId, -1},
};

void FunctionBuiltins::postInitialize(Runtime*, const Type& new_type) {
  HandleScope scope;
  Layout layout(&scope, new_type.instanceLayout());
  layout.setOverflowAttributes(SmallInt::fromWord(RawFunction::kDictOffset));
}

RawObject FunctionBuiltins::dunderGet(Thread* thread, Frame* frame,
                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!self.isFunction()) {
    return thread->raiseTypeErrorWithCStr(
        "__get__ must be called with function instance as first argument");
  }
  Object instance(&scope, args.get(1));
  if (!instance.isNoneType()) {
    return thread->runtime()->newBoundMethod(self, instance);
  }
  Object type_obj(&scope, args.get(2));
  if (thread->runtime()->isInstanceOfType(*type_obj)) {
    Type type(&scope, *type_obj);
    if (RawLayout::cast(type.instanceLayout()).id() == LayoutId::kNoneType) {
      return thread->runtime()->newBoundMethod(self, instance);
    }
  }
  return *self;
}

}  // namespace python
