#include "slice-builtins.h"

#include "runtime.h"

namespace python {

// TODO(T39495507): make these attributes readonly
const BuiltinAttribute SliceBuiltins::kAttributes[] = {
    {SymbolId::kStart, RawSlice::kStartOffset},
    {SymbolId::kStop, RawSlice::kStopOffset},
    {SymbolId::kStep, RawSlice::kStepOffset},
};

void SliceBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinTypeWithAttrs(SymbolId::kSlice, LayoutId::kSlice,
                                             LayoutId::kObject, kAttributes));
}

}  // namespace python
