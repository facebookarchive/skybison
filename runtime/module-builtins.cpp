#include "module-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinAttribute ModuleBuiltins::kAttributes[] = {
    {SymbolId::kDunderName, RawModule::kNameOffset},
    {SymbolId::kDunderDict, RawModule::kDictOffset},
};

const BuiltinMethod ModuleBuiltins::kMethods[] = {
    {SymbolId::kDunderNew, builtinTrampolineWrapper<dunderNew>}};

void ModuleBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinType(SymbolId::kModule, LayoutId::kModule,
                          LayoutId::kObject, kAttributes, kMethods);
}

RawObject ModuleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  if (RawType::cast(args.get(0)).builtinBase() != LayoutId::kModule) {
    return thread->raiseTypeErrorWithCStr("not a subtype of module");
  }
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(args.get(1))) {
    return thread->raiseTypeErrorWithCStr("argument must be a str instance");
  }
  HandleScope scope(thread);
  Str name(&scope, args.get(1));
  return runtime->newModule(name);
}

}  // namespace python
