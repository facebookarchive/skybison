#pragma once

#include "objects.h"
#include "thread.h"

namespace python {

template <typename>
class Handle;

class WARN_UNUSED HandleScope {
 public:
  explicit HandleScope() : HandleScope(Thread::current()) {}

  explicit HandleScope(Thread* thread) : list_(nullptr), thread_(thread) {
    handles()->push(this);
  }

  ~HandleScope() {
    DCHECK(this == handles()->top(), "unexpected this");
    handles()->pop();
  }

  template <typename T>
  Handle<RawObject>* push(Handle<T>* handle) {
    DCHECK(this == handles()->top(), "unexpected this");
    Handle<RawObject>* result = list_;
    list_ = reinterpret_cast<Handle<RawObject>*>(handle);
    return result;
  }

 private:
  Handle<RawObject>* list() { return list_; }
  Handles* handles() { return thread()->handles(); }
  Thread* thread() { return thread_; }

  Handle<RawObject>* list_;
  Thread* thread_;

  friend class Handles;
  template <typename>
  friend class Handle;

  // TODO(jeethu): use FRIEND_TEST
  friend class HandlesTest_UpCastTest_Test;
  friend class HandlesTest_DownCastTest_Test;
  friend class HandlesTest_IllegalCastRunTimeTest_Test;
  friend class HandlesTest_VisitEmptyScope_Test;
  friend class HandlesTest_VisitOneHandle_Test;
  friend class HandlesTest_VisitTwoHandles_Test;
  friend class HandlesTest_VisitObjectInNestedScope_Test;
  friend class HandlesTest_NestedScopes_Test;

  DISALLOW_COPY_AND_ASSIGN(HandleScope);
};

template <typename T>
class WARN_UNUSED Handle : public T {
 public:
  Handle(HandleScope* scope, RawObject obj)
      : T(obj.rawCast<T>()), next_(scope->push(this)), scope_(scope) {
    DCHECK(isValidType(), "Invalid Handle construction");
  }

  // Don't allow constructing a Handle directly from another Handle; require
  // dereferencing the source.
  template <typename S>
  Handle(HandleScope*, const Handle<S>&) = delete;

  ~Handle() {
    DCHECK(scope_->list_ == reinterpret_cast<Handle<RawObject>*>(this),
           "unexpected this");
    scope_->list_ = next_;
  }

  RawObject* pointer() { return this; }

  Handle<RawObject>* nextHandle() const { return next_; }

  T operator*() const { return *static_cast<const T*>(this); }

  // Note that Handle<T>::operator= takes a raw pointer, not a handle, so
  // to assign handles, one writes: `lhandle = *rhandle;`. (This is to avoid
  // confusion about which HandleScope tracks lhandle after the assignment.)
  template <typename S>
  Handle& operator=(S other) {
    static_assert(std::is_base_of<S, T>::value || std::is_base_of<T, S>::value,
                  "Only up- and down-casts are permitted.");
    *static_cast<T*>(this) = other.template rawCast<T>();
    DCHECK(isValidType(), "Invalid Handle assignment");
    return *this;
  }

  // Let Handle<T> pretend to be a subtype of Handle<S> when T is a subtype of
  // S.
  template <typename S>
  operator const Handle<S>&() const {
    static_assert(std::is_base_of<S, T>::value, "Only up-casts are permitted");
    return *reinterpret_cast<const Handle<S>*>(this);
  }

  static_assert(std::is_base_of<RawObject, T>::value,
                "You can only get a handle to a python::RawObject.");

  DISALLOW_IMPLICIT_CONSTRUCTORS(Handle);
  DISALLOW_HEAP_ALLOCATION();

 private:
  template <typename>
  friend class Handle;

  bool isValidType() const;

  Handle<RawObject>* next_;
  HandleScope* scope_;
};

// TODO(T34683229): This macro and its uses are temporary as part of an
// in-progress migration.
#define HANDLE_TYPES(V)                                                        \
  V(Object)                                                                    \
  V(Bool)                                                                      \
  V(BoundMethod)                                                               \
  V(ByteArray)                                                                 \
  V(ByteArrayIterator)                                                         \
  V(Bytes)                                                                     \
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
  V(Float)                                                                     \
  V(Function)                                                                  \
  V(Generator)                                                                 \
  V(GeneratorBase)                                                             \
  V(Header)                                                                    \
  V(HeapFrame)                                                                 \
  V(HeapObject)                                                                \
  V(IndexError)                                                                \
  V(Instance)                                                                  \
  V(Int)                                                                       \
  V(KeyError)                                                                  \
  V(LargeBytes)                                                                \
  V(LargeInt)                                                                  \
  V(LargeStr)                                                                  \
  V(Layout)                                                                    \
  V(ListIterator)                                                              \
  V(LookupError)                                                               \
  V(MemoryView)                                                                \
  V(Module)                                                                    \
  V(ModuleNotFoundError)                                                       \
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
  V(StrIterator)                                                               \
  V(Super)                                                                     \
  V(Tuple)                                                                     \
  V(TupleIterator)                                                             \
  V(Unbound)                                                                   \
  V(ValueCell)                                                                 \
  V(WeakRef)

// The handles for certain types allow user-defined subtypes.
#define SUBTYPE_HANDLE_TYPES(V)                                                \
  V(BaseException)                                                             \
  V(Dict)                                                                      \
  V(FrozenSet)                                                                 \
  V(ImportError)                                                               \
  V(List)                                                                      \
  V(Set)                                                                       \
  V(SetBase)                                                                   \
  V(StopIteration)                                                             \
  V(SystemExit)                                                                \
  V(Type)                                                                      \
  V(UnicodeDecodeError)                                                        \
  V(UnicodeEncodeError)                                                        \
  V(UnicodeError)                                                              \
  V(UnicodeTranslateError)                                                     \
  V(UserFloatBase)                                                             \
  V(UserIntBase)                                                               \
  V(UserStrBase)                                                               \
  V(UserTupleBase)

#define HANDLE_ALIAS(ty) using ty = Handle<class Raw##ty>;
HANDLE_TYPES(HANDLE_ALIAS)
SUBTYPE_HANDLE_TYPES(HANDLE_ALIAS)
#undef HANDLE_ALIAS

}  // namespace python
