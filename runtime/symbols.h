#pragma once

#include "heap.h"
#include "visitor.h"

namespace python {

class Runtime;

// List of predefined symbols, one per line
// clang-format off
#define FOREACH_SYMBOL(V)                \
  V(DunderBool, "__bool__")              \
  V(DunderBuildClass, "__build_class__") \
  V(DunderCall, "__call__")              \
  V(DunderClass, "__class__")            \
  V(DunderGet, "__get__")                \
  V(DunderInit, "__init__")              \
  V(DunderInvert, "__invert__")          \
  V(DunderLen, "__len__")                \
  V(DunderMain, "__main__")              \
  V(DunderName, "__name__")              \
  V(DunderNeg, "__neg__")                \
  V(DunderNew, "__new__")                \
  V(DunderPos, "__pos__")                \
  V(DunderSet, "__set__")                \
  V(Append, "append")                    \
  V(Argv, "argv")                        \
  V(Builtins, "builtins")                \
  V(Chr, "chr")                          \
  V(Classmethod, "classmethod")          \
  V(Float, "float")                      \
  V(End, "end")                          \
  V(File, "file")                        \
  V(Insert, "insert")                    \
  V(Int, "int")                          \
  V(Dict, "dict")                        \
  V(Exit, "exit")                        \
  V(IsInstance, "isinstance")            \
  V(Len, "len")                          \
  V(List, "list")                        \
  V(NotImplemented, "NotImplemented")    \
  V(ObjectClassname, "object")           \
  V(Ord, "ord")                          \
  V(Pop, "pop")                          \
  V(Range, "range")                      \
  V(Super, "super")                      \
  V(Remove, "remove")                    \
  V(Sys, "sys")                          \
  V(Time, "time")                        \
  V(Stderr, "stderr")                    \
  V(Stdout, "stdout")
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
  Object* symbol() {                   \
    return symbols_[k##symbol##Id];           \
  }
  FOREACH_SYMBOL(DEFINE_SYMBOL_ACCESSOR)
#undef DEFINE_SYMBOL_ACCESSOR
  // clang-format on

  void visit(PointerVisitor* visitor);

  Object* at(SymbolId id) {
    DCHECK_INDEX(id, kMaxSymbolId);
    return symbols_[id];
  }

  const char* literalAt(SymbolId id);

 private:
  // TODO(T25010996) - Benchmark whether this is faster than an ObjectArray
  Object** symbols_;
};

} // namespace python
