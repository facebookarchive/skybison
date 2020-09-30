#include "layout-builtins.h"

#include "builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kLayoutAttributes[] = {
    {ID(_layout__described_type), RawLayout::kDescribedTypeOffset},
    {ID(_layout__in_object_attributes), RawLayout::kInObjectAttributesOffset},
    {ID(_layout__overflow_attributes), RawLayout::kOverflowAttributesOffset},
    {ID(_layout__additions), RawLayout::kAdditionsOffset},
    {ID(_layout__deletions), RawLayout::kDeletionsOffset},
    {ID(_layout__num_in_object_attributes),
     RawLayout::kNumInObjectAttributesOffset},
};

void initializeLayoutType(Thread* thread) {
  addBuiltinType(thread, ID(layout), LayoutId::kLayout,
                 /*superclass_id=*/LayoutId::kObject, kLayoutAttributes,
                 /*basetype=*/false);
}

}  // namespace py
