#include "heap-frame-builtins.h"

#include "builtins.h"

namespace py {

const BuiltinAttribute HeapFrameBuiltins::kAttributes[] = {
    {ID(_function), HeapFrame::kFunctionOffset, AttributeFlags::kReadOnly},
    {ID(f_back), HeapFrame::kBackOffset, AttributeFlags::kReadOnly},
    {ID(f_lasti), HeapFrame::kLastiOffset, AttributeFlags::kReadOnly},
    {ID(f_locals), HeapFrame::kLocalsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
