#include "iterator-builtins.h"

namespace py {
const BuiltinAttribute SeqIteratorBuiltins::kAttributes[] = {
    {ID(_iterator__iterable), RawSeqIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_iterator__index), RawSeqIterator::kIndexOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};
}  // namespace py
