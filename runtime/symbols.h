#pragma once

#include "heap.h"
#include "visitor.h"

namespace python {

class Runtime;

// List of predefined symbols, one per line
// clang-format off
#define FOREACH_SYMBOL(V)                \
  V(DunderAdd, "__add__")                \
  V(DunderAnd, "__and__")                \
  V(DunderBool, "__bool__")              \
  V(DunderBuildClass, "__build_class__") \
  V(DunderCall, "__call__")              \
  V(DunderClass, "__class__")            \
  V(DunderDivmod, "__divmod__")          \
  V(DunderEq, "__eq__")                  \
  V(DunderFloordiv, "__floordiv__")      \
  V(DunderGe, "__ge__")                  \
  V(DunderGet, "__get__")                \
  V(DunderGt, "__gt__")                  \
  V(DunderIadd, "__iadd__")              \
  V(DunderIand, "__iand__" )             \
  V(DunderIfloordiv, "__ifloordiv__" )   \
  V(DunderIlshift, "__ilshift__" )       \
  V(DunderImatmul, "__imatmul__" )       \
  V(DunderImod, "__imod__" )             \
  V(DunderImul, "__imul__" )             \
  V(DunderInit, "__init__")              \
  V(DunderInvert, "__invert__")          \
  V(DunderIor, "__ior__" )               \
  V(DunderIpow, "__ipow__" )             \
  V(DunderIrshift, "__irshift__" )       \
  V(DunderIsub, "__isub__" )             \
  V(DunderItruediv, "__itruediv__" )     \
  V(DunderIxor, "__ixor__" )             \
  V(DunderLe, "__le__")                  \
  V(DunderLen, "__len__")                \
  V(DunderLshift, "__lshift__")          \
  V(DunderLt, "__lt__")                  \
  V(DunderMain, "__main__")              \
  V(DunderMatmul, "__matmul__")          \
  V(DunderMod, "__mod__")                \
  V(DunderMul, "__mul__")                \
  V(DunderName, "__name__")              \
  V(DunderNe, "__ne__")                  \
  V(DunderNeg, "__neg__")                \
  V(DunderNew, "__new__")                \
  V(DunderOr, "__or__")                  \
  V(DunderPos, "__pos__")                \
  V(DunderPow, "__pow__")                \
  V(DunderRadd, "__radd__")              \
  V(DunderRand, "__rand__")              \
  V(DunderRdivmod, "__rdivmod__")        \
  V(DunderRfloordiv, "__rfloordiv__")    \
  V(DunderRlshift, "__rlshift__")        \
  V(DunderRmatmul, "__rmatmul__")        \
  V(DunderRmod, "__rmod__")              \
  V(DunderRmul, "__rmul__")              \
  V(DunderRor, "__ror__")                \
  V(DunderRpow, "__rpow__")              \
  V(DunderRrshift, "__rrshift__")        \
  V(DunderRshift, "__rshift__")          \
  V(DunderRsub, "__rsub__")              \
  V(DunderRtruediv, "__rtruediv__")      \
  V(DunderRxor, "__rxor__")              \
  V(DunderSet, "__set__")                \
  V(DunderSub, "__sub__")                \
  V(DunderTruediv, "__truediv__")        \
  V(DunderXor, "__xor__")                \
  V(Append, "append")                    \
  V(Argv, "argv")                        \
  V(Builtins, "builtins")                \
  V(Chr, "chr")                          \
  V(Classmethod, "classmethod")          \
  V(Dict, "dict")                        \
  V(End, "end")                          \
  V(Exit, "exit")                        \
  V(Extend, "extend")                    \
  V(ExtensionPtr, "___extension___")     \
  V(File, "file")                        \
  V(Float, "float")                      \
  V(Insert, "insert")                    \
  V(Int, "int")                          \
  V(IsInstance, "isinstance")            \
  V(Len, "len")                          \
  V(List, "list")                        \
  V(Metaclass, "metaclass")              \
  V(NotImplemented, "NotImplemented")    \
  V(ObjectClassname, "object")           \
  V(Ord, "ord")                          \
  V(Pop, "pop")                          \
  V(Range, "range")                      \
  V(Remove, "remove")                    \
  V(Ref, "ref")                          \
  V(StaticMethod, "staticmethod")        \
  V(Stderr, "stderr")                    \
  V(Stdout, "stdout")                    \
  V(Super, "super")                      \
  V(Sys, "sys")                          \
  V(Time, "time")                        \
  V(Type, "type")                        \
  V(UnderWeakRef, "_weakref")
// clang-format on

// Provides convenient, fast access to commonly used names. Stolen from Dart.
class Symbols {
 public:
  explicit Symbols(Runtime* runtime);
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
