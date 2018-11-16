#pragma once

#include "heap.h"
#include "visitor.h"

namespace python {

class Runtime;

// List of predefined symbols, one per line
#define FOREACH_SYMBOL(V)                                                      \
  V(DunderAdd, "__add__")                                                      \
  V(DunderAnd, "__and__")                                                      \
  V(DunderAnnotations, "__annotations__")                                      \
  V(DunderBool, "__bool__")                                                    \
  V(DunderBuildClass, "__build_class__")                                       \
  V(DunderCall, "__call__")                                                    \
  V(DunderClass, "__class__")                                                  \
  V(DunderClassCell, "__classcell__")                                          \
  V(DunderContains, "__contains__")                                            \
  V(DunderContext, "__context__")                                              \
  V(DunderDelattr, "__delattr__")                                              \
  V(DunderDelete, "__delete__")                                                \
  V(DunderDelItem, "__delitem__")                                              \
  V(DunderDivmod, "__divmod__")                                                \
  V(DunderEnter, "__enter__")                                                  \
  V(DunderEq, "__eq__")                                                        \
  V(DunderExit, "__exit__")                                                    \
  V(DunderFloat, "__float__")                                                  \
  V(DunderFloordiv, "__floordiv__")                                            \
  V(DunderGe, "__ge__")                                                        \
  V(DunderGet, "__get__")                                                      \
  V(DunderGetItem, "__getitem__")                                              \
  V(DunderGt, "__gt__")                                                        \
  V(DunderHash, "__hash__")                                                    \
  V(DunderIadd, "__iadd__")                                                    \
  V(DunderIand, "__iand__")                                                    \
  V(DunderIfloordiv, "__ifloordiv__")                                          \
  V(DunderIlshift, "__ilshift__")                                              \
  V(DunderImatmul, "__imatmul__")                                              \
  V(DunderImod, "__imod__")                                                    \
  V(DunderImul, "__imul__")                                                    \
  V(DunderIndex, "__index__")                                                  \
  V(DunderInit, "__init__")                                                    \
  V(DunderInt, "__int__")                                                      \
  V(DunderInvert, "__invert__")                                                \
  V(DunderIor, "__ior__")                                                      \
  V(DunderIpow, "__ipow__")                                                    \
  V(DunderIrshift, "__irshift__")                                              \
  V(DunderIsub, "__isub__")                                                    \
  V(DunderIter, "__iter__")                                                    \
  V(DunderItruediv, "__itruediv__")                                            \
  V(DunderIxor, "__ixor__")                                                    \
  V(DunderLe, "__le__")                                                        \
  V(DunderLen, "__len__")                                                      \
  V(DunderLengthHint, "__length_hint__")                                       \
  V(DunderLshift, "__lshift__")                                                \
  V(DunderLt, "__lt__")                                                        \
  V(DunderMain, "__main__")                                                    \
  V(DunderMatmul, "__matmul__")                                                \
  V(DunderMod, "__mod__")                                                      \
  V(DunderMul, "__mul__")                                                      \
  V(DunderName, "__name__")                                                    \
  V(DunderNe, "__ne__")                                                        \
  V(DunderNeg, "__neg__")                                                      \
  V(DunderNew, "__new__")                                                      \
  V(DunderNext, "__next__")                                                    \
  V(DunderOr, "__or__")                                                        \
  V(DunderPos, "__pos__")                                                      \
  V(DunderPow, "__pow__")                                                      \
  V(DunderRadd, "__radd__")                                                    \
  V(DunderRand, "__rand__")                                                    \
  V(DunderRdivmod, "__rdivmod__")                                              \
  V(DunderRepr, "__repr__")                                                    \
  V(DunderRfloordiv, "__rfloordiv__")                                          \
  V(DunderRlshift, "__rlshift__")                                              \
  V(DunderRmatmul, "__rmatmul__")                                              \
  V(DunderRmod, "__rmod__")                                                    \
  V(DunderRmul, "__rmul__")                                                    \
  V(DunderRor, "__ror__")                                                      \
  V(DunderRpow, "__rpow__")                                                    \
  V(DunderRrshift, "__rrshift__")                                              \
  V(DunderRshift, "__rshift__")                                                \
  V(DunderRsub, "__rsub__")                                                    \
  V(DunderRtruediv, "__rtruediv__")                                            \
  V(DunderRxor, "__rxor__")                                                    \
  V(DunderSet, "__set__")                                                      \
  V(DunderSetItem, "__setitem__")                                              \
  V(DunderStr, "__str__")                                                      \
  V(DunderSub, "__sub__")                                                      \
  V(DunderTruediv, "__truediv__")                                              \
  V(DunderXor, "__xor__")                                                      \
  V(Add, "add")                                                                \
  V(Allocated, "allocated")                                                    \
  V(Append, "append")                                                          \
  V(Args, "args")                                                              \
  V(Argv, "argv")                                                              \
  V(AttributeError, "AttributeError")                                          \
  V(BaseException, "BaseException")                                            \
  V(BitLength, "bit_length")                                                   \
  V(Bool, "bool")                                                              \
  V(Bootstrap, "bootstrap")                                                    \
  V(Builtins, "builtins")                                                      \
  V(ByteArray, "bytearray")                                                    \
  V(Callable, "callable")                                                      \
  V(Cause, "cause")                                                            \
  V(Chr, "chr")                                                                \
  V(Classmethod, "classmethod")                                                \
  V(Code, "code")                                                              \
  V(Complex, "complex")                                                        \
  V(Deleter, "deleter")                                                        \
  V(Dict, "dict")                                                              \
  V(Ellipsis, "ellipsis")                                                      \
  V(End, "end")                                                                \
  V(Exception, "Exception")                                                    \
  V(Exit, "exit")                                                              \
  V(Extend, "extend")                                                          \
  V(ExtensionPtr, "___extension___")                                           \
  V(File, "file")                                                              \
  V(Float, "float")                                                            \
  V(Function, "function")                                                      \
  V(Getattr, "getattr")                                                        \
  V(Getter, "getter")                                                          \
  V(Hasattr, "hasattr")                                                        \
  V(ImportError, "ImportError")                                                \
  V(Insert, "insert")                                                          \
  V(Int, "int")                                                                \
  V(IsInstance, "isinstance")                                                  \
  V(IsSubclass, "issubclass")                                                  \
  V(Items, "items")                                                            \
  V(IndexError, "IndexError")                                                  \
  V(KeyError, "KeyError")                                                      \
  V(LargeInt, "largeint")                                                      \
  V(LargeStr, "largestr")                                                      \
  V(Layout, "layout")                                                          \
  V(Len, "len")                                                                \
  V(List, "list")                                                              \
  V(ListIterator, "list_iterator")                                             \
  V(LookupError, "LookupError")                                                \
  V(Lower, "lower")                                                            \
  V(Metaclass, "metaclass")                                                    \
  V(MetaPath, "meta_path")                                                     \
  V(Method, "method")                                                          \
  V(Module, "module")                                                          \
  V(ModuleNotFoundError, "ModuleNotFoundError")                                \
  V(Modules, "modules")                                                        \
  V(Msg, "msg")                                                                \
  V(Name, "name")                                                              \
  V(NameError, "NameError")                                                    \
  V(NoneType, "NoneType")                                                      \
  V(NotImplemented, "NotImplemented")                                          \
  V(NotImplementedType, "NotImplementedType")                                  \
  V(ObjectClassname, "object")                                                 \
  V(Ord, "ord")                                                                \
  V(Path, "path")                                                              \
  V(Pop, "pop")                                                                \
  V(Print, "print")                                                            \
  V(Property, "property")                                                      \
  V(Range, "range")                                                            \
  V(RangeIterator, "range_iterator")                                           \
  V(Ref, "ref")                                                                \
  V(Remove, "remove")                                                          \
  V(Repr, "repr")                                                              \
  V(RuntimeError, "RuntimeError")                                              \
  V(Set, "set")                                                                \
  V(Setattr, "setattr")                                                        \
  V(Setter, "setter")                                                          \
  V(Size, "size")                                                              \
  V(Slice, "slice")                                                            \
  V(SmallInt, "smallint")                                                      \
  V(SmallStr, "smallstr")                                                      \
  V(StaticMethod, "staticmethod")                                              \
  V(Stderr, "stderr")                                                          \
  V(Stdout, "stdout")                                                          \
  V(Str, "str")                                                                \
  V(StopIteration, "StopIteration")                                            \
  V(Super, "super")                                                            \
  V(Sys, "sys")                                                                \
  V(SystemExit, "SystemExit")                                                  \
  V(Time, "time")                                                              \
  V(Traceback, "traceback")                                                    \
  V(Tuple, "tuple")                                                            \
  V(Type, "type")                                                              \
  V(TypeError, "TypeError")                                                    \
  V(UnderWeakRef, "_weakref")                                                  \
  V(Value, "value")                                                            \
  V(ValueError, "ValueError")                                                  \
  V(ValueCell, "valuecell")

// clang-format off
enum class SymbolId {
#define DEFINE_SYMBOL_INDEX(symbol, value) k##symbol,
  FOREACH_SYMBOL(DEFINE_SYMBOL_INDEX)
#undef DEFINE_SYMBOL_INDEX
  kMaxId,
};
// clang-format on

// Provides convenient, fast access to commonly used names. Stolen from Dart.
class Symbols {
 public:
  explicit Symbols(Runtime* runtime);
  ~Symbols();

#define DEFINE_SYMBOL_ACCESSOR(symbol, value)                                  \
  Object* symbol() { return at(SymbolId::k##symbol); }
  FOREACH_SYMBOL(DEFINE_SYMBOL_ACCESSOR)
#undef DEFINE_SYMBOL_ACCESSOR

  void visit(PointerVisitor* visitor);

  Object* at(SymbolId id) {
    int index = static_cast<int>(id);
    DCHECK_INDEX(index, static_cast<int>(SymbolId::kMaxId));
    return symbols_[index];
  }

  const char* literalAt(SymbolId id);

 private:
  // TODO(T25010996) - Benchmark whether this is faster than an ObjectArray
  Object** symbols_;
};

}  // namespace python
