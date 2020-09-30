#include "valuecell-builtins.h"

#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kCellAttributes[] = {
    {ID(cell_contents), RawCell::kValueOffset},
};

static const BuiltinAttribute kValueCellAttributes[] = {
    {ID(_valuecell__value), RawValueCell::kValueOffset,
     AttributeFlags::kHidden},
    {ID(_valuecell__dependency_link), RawValueCell::kDependencyLinkOffset,
     AttributeFlags::kHidden},
};

void initializeValueCellTypes(Thread* thread) {
  addBuiltinType(thread, ID(cell), LayoutId::kCell,
                 /*superclass_id=*/LayoutId::kObject, kCellAttributes,
                 Cell::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(valuecell), LayoutId::kValueCell,
                 /*superclass_id=*/LayoutId::kObject, kValueCellAttributes,
                 ValueCell::kSize, /*basetype=*/false);
}

}  // namespace py
