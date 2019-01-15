#include "module-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const BuiltinAttribute ModuleBuiltins::kAttributes[] = {
    {SymbolId::kDunderName, RawModule::kNameOffset},
};

void ModuleBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinTypeWithAttrs(SymbolId::kModule, LayoutId::kModule,
                                   LayoutId::kObject, kAttributes);
}

}  // namespace python
