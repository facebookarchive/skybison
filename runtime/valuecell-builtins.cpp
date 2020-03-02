#include "valuecell-builtins.h"

namespace py {

const BuiltinAttribute ValueCellBuiltins::kAttributes[] = {
    // TODO(T63326207): Rename this back to _valuecell__value after adding a
    // specialized Cell type for use with cell variables
    {ID(cell_contents), ValueCell::kValueOffset, AttributeFlags::kHidden},
    {ID(_valuecell__dependency_link), ValueCell::kDependencyLinkOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
