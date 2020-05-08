#include "valuecell-builtins.h"

#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kCellAttributes[] = {
    {ID(cell_contents), RawCell::kValueOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kValueCellAttributes[] = {
    {ID(_valuecell__value), RawValueCell::kValueOffset,
     AttributeFlags::kHidden},
    {ID(_valuecell__dependency_link), RawValueCell::kDependencyLinkOffset,
     AttributeFlags::kHidden},
};

void initializeValueCellTypes(Thread* thread) {
  addBuiltinType(thread, ID(valuecell), LayoutId::kCell,
                 /*superclass_id=*/LayoutId::kObject, kCellAttributes);

  addBuiltinType(thread, ID(valuecell), LayoutId::kValueCell,
                 /*superclass_id=*/LayoutId::kObject, kValueCellAttributes);
}

}  // namespace py
