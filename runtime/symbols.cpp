#include "symbols.h"

#include "runtime.h"
#include "visitor.h"

namespace python {

// clang-format off
static const char* kPredefinedSymbolLiterals[] = {
#define DEFINE_SYMBOL_LITERAL(symbol, value) value,
  FOREACH_SYMBOL(DEFINE_SYMBOL_LITERAL)
#undef DEFINE_SYMBOL_VALUE
};
// clang-format on

Symbols::Symbols(Runtime* runtime) {
  symbols_ = new Object*[kMaxSymbolId];
#define ADD_SYMBOL(symbol, value)                                              \
  symbols_[k##symbol##Id] = runtime->newStringFromCString(value);
  FOREACH_SYMBOL(ADD_SYMBOL)
#undef ADD_SYMBOL
}

Symbols::~Symbols() {
  delete[] symbols_;
}

void Symbols::visit(PointerVisitor* visitor) {
  for (word i = 0; i < kMaxSymbolId; i++) {
    visitor->visitPointer(&symbols_[i]);
  }
}

const char* Symbols::literalAt(SymbolId id) {
  DCHECK_INDEX(id, kMaxSymbolId);
  return kPredefinedSymbolLiterals[id];
}

} // namespace python
