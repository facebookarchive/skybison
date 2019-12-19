#pragma once

#include "objects.h"

namespace py {

template <typename>
class Handle;

// TODO(T34683229): This macro and its uses are temporary as part of an
// in-progress migration.
#define HANDLE_TYPES(V)                                                        \
  V(Object)                                                                    \
  V(Bool)                                                                      \
  V(BoundMethod)                                                               \
  V(BufferedRandom)                                                            \
  V(BufferedReader)                                                            \
  V(BufferedWriter)                                                            \
  V(ByteArrayIterator)                                                         \
  V(Bytes)                                                                     \
  V(BytesIO)                                                                   \
  V(BytesIterator)                                                             \
  V(ClassMethod)                                                               \
  V(Code)                                                                      \
  V(Complex)                                                                   \
  V(Coroutine)                                                                 \
  V(DictItemIterator)                                                          \
  V(DictItems)                                                                 \
  V(DictKeyIterator)                                                           \
  V(DictKeys)                                                                  \
  V(DictValueIterator)                                                         \
  V(DictValues)                                                                \
  V(Ellipsis)                                                                  \
  V(Error)                                                                     \
  V(Exception)                                                                 \
  V(ExceptionState)                                                            \
  V(FileIO)                                                                    \
  V(Float)                                                                     \
  V(Function)                                                                  \
  V(Generator)                                                                 \
  V(GeneratorBase)                                                             \
  V(Header)                                                                    \
  V(HeapFrame)                                                                 \
  V(HeapObject)                                                                \
  V(IncrementalNewlineDecoder)                                                 \
  V(IndexError)                                                                \
  V(Instance)                                                                  \
  V(Int)                                                                       \
  V(KeyError)                                                                  \
  V(LargeBytes)                                                                \
  V(LargeInt)                                                                  \
  V(LargeStr)                                                                  \
  V(Layout)                                                                    \
  V(ListIterator)                                                              \
  V(LongRangeIterator)                                                         \
  V(LookupError)                                                               \
  V(MappingProxy)                                                              \
  V(MemoryView)                                                                \
  V(ModuleProxy)                                                               \
  V(ModuleNotFoundError)                                                       \
  V(MutableBytes)                                                              \
  V(MutableTuple)                                                              \
  V(NoneType)                                                                  \
  V(NotImplementedError)                                                       \
  V(NotImplementedType)                                                        \
  V(Property)                                                                  \
  V(Range)                                                                     \
  V(RangeIterator)                                                             \
  V(RuntimeError)                                                              \
  V(SeqIterator)                                                               \
  V(SetIterator)                                                               \
  V(Slice)                                                                     \
  V(SmallBytes)                                                                \
  V(SmallInt)                                                                  \
  V(SmallStr)                                                                  \
  V(StaticMethod)                                                              \
  V(Str)                                                                       \
  V(StrArray)                                                                  \
  V(StrIterator)                                                               \
  V(Super)                                                                     \
  V(TextIOWrapper)                                                             \
  V(Tuple)                                                                     \
  V(TupleIterator)                                                             \
  V(TypeProxy)                                                                 \
  V(Unbound)                                                                   \
  V(UnderBufferedIOBase)                                                       \
  V(UnderBufferedIOMixin)                                                      \
  V(UnderIOBase)                                                               \
  V(UnderRawIOBase)                                                            \
  V(ValueCell)                                                                 \
  V(WeakLink)                                                                  \
  V(WeakRef)

// The handles for certain types allow user-defined subtypes.
#define SUBTYPE_HANDLE_TYPES(V)                                                \
  V(BaseException)                                                             \
  V(ByteArray)                                                                 \
  V(Dict)                                                                      \
  V(FrozenSet)                                                                 \
  V(ImportError)                                                               \
  V(List)                                                                      \
  V(Module)                                                                    \
  V(Set)                                                                       \
  V(SetBase)                                                                   \
  V(StopIteration)                                                             \
  V(StringIO)                                                                  \
  V(SystemExit)                                                                \
  V(Type)                                                                      \
  V(UnicodeDecodeError)                                                        \
  V(UnicodeEncodeError)                                                        \
  V(UnicodeError)                                                              \
  V(UnicodeErrorBase)                                                          \
  V(UnicodeTranslateError)                                                     \
  V(UserBytesBase)                                                             \
  V(UserFloatBase)                                                             \
  V(UserIntBase)                                                               \
  V(UserStrBase)                                                               \
  V(UserTupleBase)

#define HANDLE_ALIAS(ty) using ty = Handle<class Raw##ty>;
HANDLE_TYPES(HANDLE_ALIAS)
SUBTYPE_HANDLE_TYPES(HANDLE_ALIAS)
#undef HANDLE_ALIAS

}  // namespace py
