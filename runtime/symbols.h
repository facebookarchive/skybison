#pragma once

#include "heap.h"
#include "visitor.h"

namespace python {

class Runtime;

// List of predefined symbols, one per line
// clang-format off
#define FOREACH_SYMBOL(V)                \
  V(DunderInit, "__init__")              \
  V(DunderMain, "__main__")              \
  V(DunderName, "__name__")              \
  V(DunderGet, "__get__")                \
  V(DunderSet, "__set__")                \
  V(ObjectClassname, "object")           \
  V(ListClassname, "list")               \
  V(ClassmethodClassname, "classmethod")
// clang-format on

// Provides convenient, fast access to commonly used names. Stolen from Dart.
class Symbols {
 public:
  Symbols(Runtime* runtime);
  ~Symbols();

  enum SymbolId {
  // clang-format off
#define DEFINE_SYMBOL_INDEX(symbol, value) k##symbol##Id,
    FOREACH_SYMBOL(DEFINE_SYMBOL_INDEX)
#undef DEFINE_SYMBOL_INDEX
    // clang-format on
    kMaxSymbolId,
  };

  // clang-format off
#define DEFINE_SYMBOL_ACCESSOR(symbol, value) \
  inline Object* symbol() {                   \
    return symbols_[k##symbol##Id];           \
  }
  FOREACH_SYMBOL(DEFINE_SYMBOL_ACCESSOR)
#undef DEFINE_SYMBOL_ACCESSOR
  // clang-format on

  void visit(PointerVisitor* visitor);

  Object* at(SymbolId id) {
    assert(id >= 0);
    assert(id < kMaxSymbolId);
    return symbols_[id];
  }

  const char* literalAt(SymbolId id);

 private:
  // TODO(T25010996) - Benchmark whether this is faster than an ObjectArray
  Object** symbols_;
};

} // namespace python
