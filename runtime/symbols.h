#pragma once

#include "heap.h"
#include "visitor.h"

namespace python {

class Runtime;

// List of predefined symbols, one per line
#define FOREACH_SYMBOL(V)                                                      \
  V(DunderAbs, "__abs__")                                                      \
  V(DunderAdd, "__add__")                                                      \
  V(DunderAenter, "__aenter__")                                                \
  V(DunderAexit, "__aexit__")                                                  \
  V(DunderAnd, "__and__")                                                      \
  V(DunderAnnotations, "__annotations__")                                      \
  V(DunderAiter, "__aiter__")                                                  \
  V(DunderAnext, "__anext__")                                                  \
  V(DunderAwait, "__await__")                                                  \
  V(DunderBool, "__bool__")                                                    \
  V(DunderBuildClass, "__build_class__")                                       \
  V(DunderBuiltins, "__builtins__")                                            \
  V(DunderBytes, "__bytes__")                                                  \
  V(DunderCall, "__call__")                                                    \
  V(DunderClass, "__class__")                                                  \
  V(DunderClassCell, "__classcell__")                                          \
  V(DunderCode, "__code__")                                                    \
  V(DunderComplex, "__complex__")                                              \
  V(DunderContains, "__contains__")                                            \
  V(DunderContext, "__context__")                                              \
  V(DunderDelItem, "__delitem__")                                              \
  V(DunderDelattr, "__delattr__")                                              \
  V(DunderDelete, "__delete__")                                                \
  V(DunderDict, "__dict__")                                                    \
  V(DunderDivmod, "__divmod__")                                                \
  V(DunderDoc, "__doc__")                                                      \
  V(DunderEnter, "__enter__")                                                  \
  V(DunderEq, "__eq__")                                                        \
  V(DunderExit, "__exit__")                                                    \
  V(DunderFile, "__file__")                                                    \
  V(DunderFlags, "__flags__")                                                  \
  V(DunderFloat, "__float__")                                                  \
  V(DunderFloordiv, "__floordiv__")                                            \
  V(DunderFormat, "__format__")                                                \
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
  V(DunderKeys, "__keys__")                                                    \
  V(DunderLe, "__le__")                                                        \
  V(DunderLen, "__len__")                                                      \
  V(DunderLengthHint, "__length_hint__")                                       \
  V(DunderLoader, "__loader__")                                                \
  V(DunderLshift, "__lshift__")                                                \
  V(DunderLt, "__lt__")                                                        \
  V(DunderMain, "__main__")                                                    \
  V(DunderMatmul, "__matmul__")                                                \
  V(DunderMod, "__mod__")                                                      \
  V(DunderModule, "__module__")                                                \
  V(DunderMro, "__mro__")                                                      \
  V(DunderMul, "__mul__")                                                      \
  V(DunderName, "__name__")                                                    \
  V(DunderNe, "__ne__")                                                        \
  V(DunderNeg, "__neg__")                                                      \
  V(DunderNew, "__new__")                                                      \
  V(DunderNext, "__next__")                                                    \
  V(DunderOr, "__or__")                                                        \
  V(DunderPackage, "__package__")                                              \
  V(DunderPos, "__pos__")                                                      \
  V(DunderPow, "__pow__")                                                      \
  V(DunderQualname, "__qualname__")                                            \
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
  V(DunderSpec, "__spec__")                                                    \
  V(DunderStr, "__str__")                                                      \
  V(DunderSub, "__sub__")                                                      \
  V(DunderTruediv, "__truediv__")                                              \
  V(DunderValues, "__values__")                                                \
  V(DunderXor, "__xor__")                                                      \
  V(AcquireLock, "acquire_lock")                                               \
  V(Add, "add")                                                                \
  V(Allocated, "allocated")                                                    \
  V(Append, "append")                                                          \
  V(Args, "args")                                                              \
  V(Argv, "argv")                                                              \
  V(ArithmeticError, "ArithmeticError")                                        \
  V(Ascii, "ascii")                                                            \
  V(AssertionError, "AssertionError")                                          \
  V(AttributeError, "AttributeError")                                          \
  V(BaseException, "BaseException")                                            \
  V(Big, "big")                                                                \
  V(BitLength, "bit_length")                                                   \
  V(BlockingIOError, "BlockingIOError")                                        \
  V(Bool, "bool")                                                              \
  V(Bootstrap, "bootstrap")                                                    \
  V(BrokenPipeError, "BrokenPipeError")                                        \
  V(BufferError, "BufferError")                                                \
  V(BuiltinModuleNames, "builtin_module_names")                                \
  V(Builtins, "builtins")                                                      \
  V(ByteArray, "bytearray")                                                    \
  V(Byteorder, "byteorder")                                                    \
  V(Bytes, "bytes")                                                            \
  V(BytesWarning, "BytesWarning")                                              \
  V(Callable, "callable")                                                      \
  V(Cause, "cause")                                                            \
  V(ChildProcessError, "ChildProcessError")                                    \
  V(Chr, "chr")                                                                \
  V(Classmethod, "classmethod")                                                \
  V(Code, "code")                                                              \
  V(Compile, "compile")                                                        \
  V(Complex, "complex")                                                        \
  V(ConnectionAbortedError, "ConnectionAbortedError")                          \
  V(ConnectionError, "ConnectionError")                                        \
  V(ConnectionRefusedError, "ConnectionRefusedError")                          \
  V(ConnectionResetError, "ConnectionResetError")                              \
  V(Contains, "contains")                                                      \
  V(CountOf, "countOf")                                                        \
  V(Coroutine, "coroutine")                                                    \
  V(CreateBuiltin, "create_builtin")                                           \
  V(Deleter, "deleter")                                                        \
  V(DeprecationWarning, "DeprecationWarning")                                  \
  V(Dict, "dict")                                                              \
  V(DictItems, "dict_items")                                                   \
  V(DictItemIterator, "dict_itemiterator")                                     \
  V(DictKeys, "dict_keys")                                                     \
  V(DictKeyIterator, "dict_keyiterator")                                       \
  V(DictValues, "dict_values")                                                 \
  V(DictValueIterator, "dict_valueiterator")                                   \
  V(Displayhook, "displayhook")                                                \
  V(DotSo, ".so")                                                              \
  V(Dummy, "dummy")                                                            \
  V(EOFError, "EOFError")                                                      \
  V(Ellipsis, "ellipsis")                                                      \
  V(End, "end")                                                                \
  V(Exception, "Exception")                                                    \
  V(ExceptionState, "ExceptionState")                                          \
  V(Exec, "exec")                                                              \
  V(ExecBuiltin, "exec_builtin")                                               \
  V(ExecDynamic, "exec_dynamic")                                               \
  V(Exit, "exit")                                                              \
  V(Extend, "extend")                                                          \
  V(ExtensionPtr, "___extension___")                                           \
  V(ExtensionSuffixes, "extension_suffixes")                                   \
  V(File, "file")                                                              \
  V(FileExistsError, "FileExistsError")                                        \
  V(FileNotFoundError, "FileNotFoundError")                                    \
  V(FixCoFilename, "_fix_co_filename")                                         \
  V(Format, "format")                                                          \
  V(Float, "float")                                                            \
  V(FloatingPointError, "FloatingPointError")                                  \
  V(Frame, "frame")                                                            \
  V(FromBytes, "from_bytes")                                                   \
  V(FrozenSet, "frozenset")                                                    \
  V(Function, "function")                                                      \
  V(FutureWarning, "FutureWarning")                                            \
  V(Generator, "generator")                                                    \
  V(GeneratorExit, "GeneratorExit")                                            \
  V(Get, "get")                                                                \
  V(GetFrozenObject, "get_frozen_object")                                      \
  V(Getattr, "getattr")                                                        \
  V(Getter, "getter")                                                          \
  V(Hasattr, "hasattr")                                                        \
  V(ImportError, "ImportError")                                                \
  V(ImportWarning, "ImportWarning")                                            \
  V(IndentationError, "IndentationError")                                      \
  V(IndexError, "IndexError")                                                  \
  V(IndexOf, "indexOf")                                                        \
  V(Insert, "insert")                                                          \
  V(Int, "int")                                                                \
  V(InterruptedError, "InterruptedError")                                      \
  V(Intersection, "intersection")                                              \
  V(IsADirectoryError, "IsADirectoryError")                                    \
  V(IsBuiltin, "is_builtin")                                                   \
  V(IsDisjoint, "isdisjoint")                                                  \
  V(IsFrozen, "is_frozen")                                                     \
  V(IsFrozenPackage, "is_frozen_package")                                      \
  V(IsInstance, "isinstance")                                                  \
  V(IsSubclass, "issubclass")                                                  \
  V(Items, "items")                                                            \
  V(Join, "join")                                                              \
  V(KeyError, "KeyError")                                                      \
  V(KeyboardInterrupt, "KeyboardInterrupt")                                    \
  V(Keys, "keys")                                                              \
  V(Length, "length")                                                          \
  V(LStrip, "lstrip")                                                          \
  V(LargeInt, "largeint")                                                      \
  V(LargeStr, "largestr")                                                      \
  V(Layout, "layout")                                                          \
  V(List, "list")                                                              \
  V(ListIterator, "list_iterator")                                             \
  V(Little, "little")                                                          \
  V(Loads, "loads")                                                            \
  V(LookupError, "LookupError")                                                \
  V(Lower, "lower")                                                            \
  V(Marshal, "marshal")                                                        \
  V(MemoryError, "MemoryError")                                                \
  V(MetaPath, "meta_path")                                                     \
  V(Metaclass, "metaclass")                                                    \
  V(Method, "method")                                                          \
  V(Module, "module")                                                          \
  V(ModuleNotFoundError, "ModuleNotFoundError")                                \
  V(Modules, "modules")                                                        \
  V(Msg, "msg")                                                                \
  V(NFields, "n_fields")                                                       \
  V(NSequenceFields, "n_sequence_fields")                                      \
  V(NUnnamedFields, "n_unnamed_fields")                                        \
  V(Name, "name")                                                              \
  V(NameError, "NameError")                                                    \
  V(None, "None")                                                              \
  V(NoneType, "NoneType")                                                      \
  V(NotADirectoryError, "NotADirectoryError")                                  \
  V(NotImplemented, "NotImplemented")                                          \
  V(NotImplementedError, "NotImplementedError")                                \
  V(NotImplementedType, "NotImplementedType")                                  \
  V(Null, "<NULL>")                                                            \
  V(OSError, "OSError")                                                        \
  V(ObjectTypename, "object")                                                  \
  V(Operator, "operator")                                                      \
  V(Ord, "ord")                                                                \
  V(OverflowError, "OverflowError")                                            \
  V(Path, "path")                                                              \
  V(PendingDeprecationWarning, "PendingDeprecationWarning")                    \
  V(PermissionError, "PermissionError")                                        \
  V(Platform, "platform")                                                      \
  V(Pop, "pop")                                                                \
  V(ProcessLookupError, "ProcessLookupError")                                  \
  V(Property, "property")                                                      \
  V(RStrip, "rstrip")                                                          \
  V(Range, "range")                                                            \
  V(RangeIterator, "range_iterator")                                           \
  V(RecursionError, "RecursionError")                                          \
  V(Ref, "ref")                                                                \
  V(ReferenceError, "ReferenceError")                                          \
  V(ReleaseLock, "release_lock")                                               \
  V(Remove, "remove")                                                          \
  V(Repr, "repr")                                                              \
  V(ResourceWarning, "ResourceWarning")                                        \
  V(RuntimeError, "RuntimeError")                                              \
  V(RuntimeWarning, "RuntimeWarning")                                          \
  V(Send, "send")                                                              \
  V(Set, "set")                                                                \
  V(SetIterator, "set_iterator")                                               \
  V(SeqIterator, "iterator")                                                   \
  V(Setattr, "setattr")                                                        \
  V(Setter, "setter")                                                          \
  V(Signed, "signed")                                                          \
  V(Size, "size")                                                              \
  V(Slice, "slice")                                                            \
  V(SmallInt, "smallint")                                                      \
  V(SmallStr, "smallstr")                                                      \
  V(Start, "start")                                                            \
  V(StaticMethod, "staticmethod")                                              \
  V(Stderr, "stderr")                                                          \
  V(Stdout, "stdout")                                                          \
  V(Step, "step")                                                              \
  V(Stop, "stop")                                                              \
  V(StopAsyncIteration, "StopAsyncIteration")                                  \
  V(StopIteration, "StopIteration")                                            \
  V(Str, "str")                                                                \
  V(StrIterator, "str_iterator")                                               \
  V(Strip, "strip")                                                            \
  V(Super, "super")                                                            \
  V(SyntaxError, "SyntaxError")                                                \
  V(SyntaxWarning, "SyntaxWarning")                                            \
  V(Sys, "sys")                                                                \
  V(SystemError, "SystemError")                                                \
  V(SystemExit, "SystemExit")                                                  \
  V(TabError, "TabError")                                                      \
  V(Time, "time")                                                              \
  V(TimeoutError, "TimeoutError")                                              \
  V(ToBytes, "to_bytes")                                                       \
  V(Traceback, "traceback")                                                    \
  V(Tuple, "tuple")                                                            \
  V(TupleIterator, "tuple_iterator")                                           \
  V(Type, "type")                                                              \
  V(TypeError, "TypeError")                                                    \
  V(UnboundLocalError, "UnboundLocalError")                                    \
  V(UnderAddress, "_address")                                                  \
  V(UnderComplexImag, "_complex_imag")                                         \
  V(UnderComplexReal, "_complex_real")                                         \
  V(UnderImp, "_imp")                                                          \
  V(UnderIo, "_io")                                                            \
  V(UnderPatch, "_patch")                                                      \
  V(UnderPrintStr, "_print_str")                                               \
  V(UnderReadBytes, "_readbytes")                                              \
  V(UnderReadFile, "_readfile")                                                \
  V(UnderStdout, "_stdout")                                                    \
  V(UnderStrEscapeNonAscii, "_str_escape_non_ascii")                           \
  V(UnderThread, "_thread")                                                    \
  V(UnderUnboundValue, "_UnboundValue")                                        \
  V(UnderWarnings, "_warnings")                                                \
  V(UnderWeakRef, "_weakref")                                                  \
  V(Anonymous, "<anonymous>")                                                  \
  V(UnicodeDecodeError, "UnicodeDecodeError")                                  \
  V(UnicodeEncodeError, "UnicodeEncodeError")                                  \
  V(UnicodeError, "UnicodeError")                                              \
  V(UnicodeTranslateError, "UnicodeTranslateError")                            \
  V(UnicodeWarning, "UnicodeWarning")                                          \
  V(Update, "update")                                                          \
  V(UserWarning, "UserWarning")                                                \
  V(Value, "value")                                                            \
  V(Values, "values")                                                          \
  V(ValueCell, "valuecell")                                                    \
  V(ValueError, "ValueError")                                                  \
  V(Version, "version")                                                        \
  V(Warn, "warn")                                                              \
  V(Warning, "Warning")                                                        \
  V(ZeroDivisionError, "ZeroDivisionError")

// clang-format off
enum class SymbolId {
  kInvalid = -1,
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
  RawObject symbol() { return at(SymbolId::k##symbol); }
  FOREACH_SYMBOL(DEFINE_SYMBOL_ACCESSOR)
#undef DEFINE_SYMBOL_ACCESSOR

  void visit(PointerVisitor* visitor);

  RawObject at(SymbolId id) {
    int index = static_cast<int>(id);
    DCHECK_INDEX(index, static_cast<int>(SymbolId::kMaxId));
    return symbols_[index];
  }

  const char* literalAt(SymbolId id);

 private:
  // TODO(T25010996) - Benchmark whether this is faster than an Tuple
  RawObject* symbols_;
};

}  // namespace python
