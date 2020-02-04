#include "layout-builtins.h"

#include "builtins.h"

namespace py {

const BuiltinAttribute LayoutBuiltins::kAttributes[] = {
    {ID(_layout__described_type), Layout::kDescribedTypeOffset},
    {ID(_layout__in_object_attributes), Layout::kInObjectAttributesOffset},
    {ID(_layout__overflow_attributes), Layout::kOverflowAttributesOffset},
    {ID(_layout__additions), Layout::kAdditionsOffset},
    {ID(_layout__deletions), Layout::kDeletionsOffset},
    {ID(_layout__num_in_object_attributes),
     Layout::kNumInObjectAttributesOffset},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
