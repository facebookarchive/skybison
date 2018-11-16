#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod FunctionBuiltins::kMethods[] = {
    {SymbolId::kDunderGet, nativeTrampoline<dunderGet>},
};

const BuiltinAttribute FunctionBuiltins::kAttributes[] = {
    {SymbolId::kDunderModule, RawFunction::kModuleOffset},
    {SymbolId::kDunderName, RawFunction::kNameOffset},
    {SymbolId::kDunderQualname, RawFunction::kQualnameOffset},
};

void FunctionBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type function(&scope, runtime->addBuiltinClass(
                            SymbolId::kFunction, LayoutId::kFunction,
                            LayoutId::kObject, kAttributes, kMethods));
}

RawObject FunctionBuiltins::dunderGet(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object instance(&scope, args.get(1));
  if (instance->isNoneType()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

}  // namespace python
