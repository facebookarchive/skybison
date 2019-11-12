#include "module-proxy-builtins.h"

namespace py {

const BuiltinAttribute ModuleProxyBuiltins::kAttributes[] = {
    {SymbolId::kDunderModuleObject, ModuleProxy::kModuleOffset},
    {SymbolId::kSentinelId, 0},
};

}
