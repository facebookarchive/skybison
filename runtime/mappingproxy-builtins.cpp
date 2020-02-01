#include "mappingproxy-builtins.h"

namespace py {

const BuiltinAttribute MappingProxyBuiltins::kAttributes[] = {
    {ID(_mappingproxy__mapping), MappingProxy::kMappingOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
