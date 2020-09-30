#include "code-builtins.h"

#include "builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kCodeAttributes[] = {
    {ID(co_argcount), RawCode::kArgcountOffset, AttributeFlags::kReadOnly},
    {ID(co_cellvars), RawCode::kCellvarsOffset, AttributeFlags::kReadOnly},
    {ID(co_code), RawCode::kCodeOffset, AttributeFlags::kReadOnly},
    {ID(co_consts), RawCode::kConstsOffset, AttributeFlags::kReadOnly},
    {ID(co_filename), RawCode::kFilenameOffset, AttributeFlags::kReadOnly},
    {ID(co_firstlineno), RawCode::kFirstlinenoOffset,
     AttributeFlags::kReadOnly},
    {ID(co_flags), RawCode::kFlagsOffset, AttributeFlags::kReadOnly},
    {ID(co_freevars), RawCode::kFreevarsOffset, AttributeFlags::kReadOnly},
    {ID(co_kwonlyargcount), RawCode::kKwonlyargcountOffset,
     AttributeFlags::kReadOnly},
    {ID(co_lnotab), RawCode::kLnotabOffset, AttributeFlags::kReadOnly},
    {ID(co_name), RawCode::kNameOffset, AttributeFlags::kReadOnly},
    {ID(co_names), RawCode::kNamesOffset, AttributeFlags::kReadOnly},
    {ID(co_nlocals), RawCode::kNlocalsOffset, AttributeFlags::kReadOnly},
    {ID(co_posonlyargcount), RawCode::kPosonlyargcountOffset,
     AttributeFlags::kReadOnly},
    {ID(co_stacksize), RawCode::kStacksizeOffset, AttributeFlags::kReadOnly},
    {ID(co_varnames), RawCode::kVarnamesOffset, AttributeFlags::kReadOnly},
};

void initializeCodeType(Thread* thread) {
  addBuiltinType(thread, ID(code), LayoutId::kCode,
                 /*superclass_id=*/LayoutId::kObject, kCodeAttributes,
                 /*basetype=*/false);
}

}  // namespace py
