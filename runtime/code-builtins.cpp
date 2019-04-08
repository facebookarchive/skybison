#include "code-builtins.h"

namespace python {

const BuiltinAttribute CodeBuiltins::kAttributes[] = {
    {SymbolId::kCoArgcount, RawCode::kArgcountOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kCoCellvars, RawCode::kCellvarsOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kCoCode, RawCode::kCodeOffset, AttributeFlags::kReadOnly},
    {SymbolId::kCoConsts, RawCode::kConstsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kCoFilename, RawCode::kFilenameOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kCoFirstlineno, RawCode::kFirstlinenoOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kCoFlags, RawCode::kFlagsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kCoFreevars, RawCode::kFreevarsOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kCoKwonlyargcount, RawCode::kKwonlyargcountOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kCoLnotab, RawCode::kLnotabOffset, AttributeFlags::kReadOnly},
    {SymbolId::kCoName, RawCode::kNameOffset, AttributeFlags::kReadOnly},
    {SymbolId::kCoNames, RawCode::kNamesOffset, AttributeFlags::kReadOnly},
    {SymbolId::kCoNlocals, RawCode::kNlocalsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kCoStacksize, RawCode::kStacksizeOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kCoVarnames, RawCode::kVarnamesOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

}  // namespace python
