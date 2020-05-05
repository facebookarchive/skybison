#include "frame-proxy-builtins.h"

#include "builtins.h"

namespace py {

const BuiltinAttribute FrameProxyBuiltins::kAttributes[] = {
    {ID(_function), RawFrameProxy::kFunctionOffset, AttributeFlags::kReadOnly},
    {ID(f_back), RawFrameProxy::kBackOffset, AttributeFlags::kReadOnly},
    {ID(f_lasti), RawFrameProxy::kLastiOffset, AttributeFlags::kReadOnly},
    {ID(f_locals), RawFrameProxy::kLocalsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
