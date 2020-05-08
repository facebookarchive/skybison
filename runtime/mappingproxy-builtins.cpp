#include "mappingproxy-builtins.h"

#include "builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kMappingProxyAttributes[] = {
    {ID(_mappingproxy__mapping), RawMappingProxy::kMappingOffset,
     AttributeFlags::kHidden},
};

void initializeMappingProxyType(Thread* thread) {
  addBuiltinType(thread, ID(mappingproxy), LayoutId::kMappingProxy,
                 /*superclass_id=*/LayoutId::kObject, kMappingProxyAttributes);
}

}  // namespace py
