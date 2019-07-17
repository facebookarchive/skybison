#include "iterator-builtins.h"

namespace python {
const BuiltinAttribute SeqIteratorBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawSeqIterator::kIterableOffset},
    {SymbolId::kInvalid, RawSeqIterator::kIndexOffset},
    {SymbolId::kSentinelId, -1},
};
}  // namespace python
