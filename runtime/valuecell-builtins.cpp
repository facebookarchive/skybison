#include "valuecell-builtins.h"

namespace py {

const BuiltinAttribute CellBuiltins::kAttributes[] = {
    {ID(cell_contents), RawCell::kValueOffset, AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

const BuiltinAttribute ValueCellBuiltins::kAttributes[] = {
    {ID(_valuecell__value), RawValueCell::kValueOffset,
     AttributeFlags::kHidden},
    {ID(_valuecell__dependency_link), RawValueCell::kDependencyLinkOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
