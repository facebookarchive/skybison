#include "valuecell-builtins.h"

namespace py {

const BuiltinAttribute CellBuiltins::kAttributes[] = {
    {ID(cell_contents), Cell::kValueOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

const BuiltinAttribute ValueCellBuiltins::kAttributes[] = {
    {ID(_valuecell__value), ValueCell::kValueOffset, AttributeFlags::kHidden},
    {ID(_valuecell__dependency_link), ValueCell::kDependencyLinkOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
