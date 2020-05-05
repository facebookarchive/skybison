#include "layout-builtins.h"

#include "builtins.h"

namespace py {

const BuiltinAttribute LayoutBuiltins::kAttributes[] = {
    {ID(_layout__described_type), RawLayout::kDescribedTypeOffset},
    {ID(_layout__in_object_attributes), RawLayout::kInObjectAttributesOffset},
    {ID(_layout__overflow_attributes), RawLayout::kOverflowAttributesOffset},
    {ID(_layout__additions), RawLayout::kAdditionsOffset},
    {ID(_layout__deletions), RawLayout::kDeletionsOffset},
    {ID(_layout__num_in_object_attributes),
     RawLayout::kNumInObjectAttributesOffset},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
