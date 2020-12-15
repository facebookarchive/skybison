#pragma once

#include "objects.h"

namespace py {

template <typename>
class Handle;

// TODO(T34683229): This macro and its uses are temporary as part of an
// in-progress migration.
#define HANDLE_TYPES(V)                                                        \
  V(AsyncGenerator)                                                            \
  V(AsyncGeneratorAclose)                                                      \
  V(AsyncGeneratorAsend)                                                       \
  V(AsyncGeneratorAthrow)                                                      \
  V(AsyncGeneratorOpIterBase)                                                  \
  V(AsyncGeneratorWrappedValue)                                                \
  V(AttributeDict)                                                             \
  V(Bool)                                                                      \
  V(BoundMethod)                                                               \
  V(BufferedRandom)                                                            \
  V(BufferedReader)                                                            \
  V(BufferedWriter)                                                            \
  V(BytearrayIterator)                                                         \
  V(Bytes)                                                                     \
  V(BytesIterator)                                                             \
  V(Cell)                                                                      \
  V(Code)                                                                      \
  V(Complex)                                                                   \
  V(Context)                                                                   \
  V(ContextVar)                                                                \
  V(Coroutine)                                                                 \
  V(CoroutineWrapper)                                                          \
  V(DataArray)                                                                 \
  V(DequeIterator)                                                             \
  V(DequeReverseIterator)                                                      \
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
  V(Float)                                                                     \
  V(FrameProxy)                                                                \
  V(Function)                                                                  \
  V(Generator)                                                                 \
  V(GeneratorBase)                                                             \
  V(GeneratorFrame)                                                            \
  V(Header)                                                                    \
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
  V(ModuleNotFoundError)                                                       \
  V(ModuleProxy)                                                               \
  V(MutableBytes)                                                              \
  V(MutableTuple)                                                              \
  V(NoneType)                                                                  \
  V(NotImplementedError)                                                       \
  V(NotImplementedType)                                                        \
  V(Object)                                                                    \
  V(Pointer)                                                                   \
  V(Range)                                                                     \
  V(RangeIterator)                                                             \
  V(RuntimeError)                                                              \
  V(SeqIterator)                                                               \
  V(SetIterator)                                                               \
  V(Slice)                                                                     \
  V(SlotDescriptor)                                                            \
  V(SmallBytes)                                                                \
  V(SmallInt)                                                                  \
  V(SmallStr)                                                                  \
  V(Str)                                                                       \
  V(StrArray)                                                                  \
  V(StrIterator)                                                               \
  V(Super)                                                                     \
  V(Token)                                                                     \
  V(Traceback)                                                                 \
  V(Tuple)                                                                     \
  V(TupleIterator)                                                             \
  V(TypeProxy)                                                                 \
  V(Unbound)                                                                   \
  V(UnderBufferedIOBase)                                                       \
  V(UnderBufferedIOMixin)                                                      \
  V(UnderIOBase)                                                               \
  V(UnderRawIOBase)                                                            \
  V(ValueCell)                                                                 \
  V(WeakCallableProxy)                                                         \
  V(WeakProxy)                                                                 \
  V(WeakLink)                                                                  \
  V(WeakRef)

// The handles for certain types allow user-defined subtypes.
#define SUBTYPE_HANDLE_TYPES(V)                                                \
  V(Array)                                                                     \
  V(BaseException)                                                             \
  V(Bytearray)                                                                 \
  V(BytesIO)                                                                   \
  V(ClassMethod)                                                               \
  V(Deque)                                                                     \
  V(Dict)                                                                      \
  V(FileIO)                                                                    \
  V(FrozenSet)                                                                 \
  V(ImportError)                                                               \
  V(List)                                                                      \
  V(Mmap)                                                                      \
  V(Module)                                                                    \
  V(NativeProxy)                                                               \
  V(Property)                                                                  \
  V(Set)                                                                       \
  V(SetBase)                                                                   \
  V(StaticMethod)                                                              \
  V(StopIteration)                                                             \
  V(StringIO)                                                                  \
  V(SystemExit)                                                                \
  V(TextIOWrapper)                                                             \
  V(Type)                                                                      \
  V(UnicodeDecodeError)                                                        \
  V(UnicodeEncodeError)                                                        \
  V(UnicodeError)                                                              \
  V(UnicodeErrorBase)                                                          \
  V(UnicodeTranslateError)                                                     \
  V(UserBytesBase)                                                             \
  V(UserComplexBase)                                                           \
  V(UserFloatBase)                                                             \
  V(UserIntBase)                                                               \
  V(UserStrBase)                                                               \
  V(UserTupleBase)                                                             \
  V(UserWeakRefBase)

#define HANDLE_ALIAS(ty) using ty = Handle<class Raw##ty>;
HANDLE_TYPES(HANDLE_ALIAS)
SUBTYPE_HANDLE_TYPES(HANDLE_ALIAS)
#undef HANDLE_ALIAS

}  // namespace py
