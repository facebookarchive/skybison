#include "symbols.h"

#include <cstring>

#include "visitor.h"

namespace python {

static const char* kPredefinedSymbolLiterals[] = {
// clang-format off
#define DEFINE_SYMBOL_LITERAL(symbol, value) value,
  FOREACH_SYMBOL(DEFINE_SYMBOL_LITERAL)
#undef DEFINE_SYMBOL_VALUE
    // clang-format on
};

Symbols::Symbols(Heap* heap) {
  symbols_ = new Object*[kMaxSymbolId];
  // clang-format off
#define ADD_SYMBOL(symbol, value) addSymbol(heap, k##symbol##Id, value);
  FOREACH_SYMBOL(ADD_SYMBOL)
#undef ADD_SYMBOL
  // clang-format on
}

Symbols::~Symbols() {
  delete[] symbols_;
}

void Symbols::addSymbol(Heap* heap, SymbolId id, const char* value) {
  word len = std::strlen(value);
  Object* str = heap->createLargeString(len);
  assert(str != nullptr);
  char* dst = reinterpret_cast<char*>(LargeString::cast(str)->address());
  memcpy(dst, value, len);
  symbols_[id] = str;
}

void Symbols::visit(PointerVisitor* visitor) {
  for (word i = 0; i < kMaxSymbolId; i++) {
    visitor->visitPointer(&symbols_[i]);
  }
}

const char* Symbols::literalAt(SymbolId id) {
  assert(id >= 0);
  assert(id < kMaxSymbolId);
  return kPredefinedSymbolLiterals[id];
}

} // namespace python
