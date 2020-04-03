#include "frame-proxy-builtins.h"

#include "builtins.h"

namespace py {

const BuiltinAttribute FrameProxyBuiltins::kAttributes[] = {
    {ID(_function), FrameProxy::kFunctionOffset, AttributeFlags::kReadOnly},
    {ID(f_back), FrameProxy::kBackOffset, AttributeFlags::kReadOnly},
    {ID(f_lasti), FrameProxy::kLastiOffset, AttributeFlags::kReadOnly},
    {ID(f_locals), FrameProxy::kLocalsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

}  // namespace py
