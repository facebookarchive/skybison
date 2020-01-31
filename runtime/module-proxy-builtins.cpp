#include "module-proxy-builtins.h"

namespace py {

const BuiltinAttribute ModuleProxyBuiltins::kAttributes[] = {
    {ID(__module_object__), ModuleProxy::kModuleOffset},
    {SymbolId::kSentinelId, 0},
};

}
