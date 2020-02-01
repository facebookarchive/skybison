#include "traceback-builtins.h"

namespace py {

const BuiltinAttribute TracebackBuiltins::kAttributes[] = {
    {ID(_traceback__next), RawTraceback::kNextOffset, AttributeFlags::kHidden},
    {ID(_traceback__frame), RawTraceback::kFrameOffset,
     AttributeFlags::kHidden},
    {ID(_traceback__lasti), RawTraceback::kLastiOffset,
     AttributeFlags::kHidden},
    {ID(_traceback__lineno), RawTraceback::kLinenoOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
