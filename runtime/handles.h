#pragma once

#include "globals.h"
#include "thread.h"
#include "utils.h"
#include "vector.h"

namespace python {

class HandleScope;
template <typename T, bool is_checked = true>
class Handle;
class PointerVisitor;

class Handles {
 public:
  static const int kInitialSize = 10;

  Handles();

  void visitPointers(PointerVisitor* visitor);

 private:
  void push(HandleScope* scope) { scopes_.push_back(scope); }

  void pop() {
    DCHECK(!scopes_.empty(), "pop on empty");
    scopes_.pop_back();
  }

  HandleScope* top() {
    DCHECK(!scopes_.empty(), "top on empty");
    return scopes_.back();
  }

  Vector<HandleScope*> scopes_;

  template <typename, bool>
  friend class Handle;
  friend class HandleScope;

  DISALLOW_COPY_AND_ASSIGN(Handles);
};

class HandleScope {
 public:
  explicit HandleScope()
      : list_(nullptr), handles_(Thread::currentThread()->handles()) {
    handles_->push(this);
  }

  explicit HandleScope(Thread* thread)
      : list_(nullptr), handles_(thread->handles()) {
    handles_->push(this);
  }

  ~HandleScope() {
    DCHECK(this == handles_->top(), "unexpected this");
    handles_->pop();
  }

  template <typename T, bool is_checked>
  Handle<RawObject>* push(Handle<T, is_checked>* handle) {
    DCHECK(this == handles_->top(), "unexpected this");
    Handle<RawObject>* result = list_;
    list_ = reinterpret_cast<Handle<RawObject>*>(handle);
    return result;
  }

 private:
  explicit HandleScope(Handles* handles) : list_(nullptr), handles_(handles) {
    handles_->push(this);
  }

  Handle<RawObject>* list() { return list_; }

  Handle<RawObject>* list_;
  Handles* handles_;

  friend class Handles;
  template <typename, bool>
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

template <typename T, bool is_checked>
class Handle : public T {
 public:
  Handle(HandleScope* scope, RawObject pointer)
      : T(is_checked ? T::cast(pointer) : bit_cast<T>(pointer)),
        next_(scope->push(this)),
        scope_(scope) {}

  // Don't allow constructing a Handle directly from another Handle; require
  // dereferencing the source.
  template <typename S, bool checked>
  Handle(HandleScope*, Handle<S, checked>) = delete;

  ~Handle() {
    DCHECK(scope_->list_ == reinterpret_cast<Handle<RawObject>*>(this),
           "unexpected this");
    scope_->list_ = next_;
  }

  RawObject* pointer() { return this; }

  Handle<RawObject>* next() const { return next_; }

  T* operator->() const { return const_cast<Handle*>(this); }

  T operator*() const { return *operator->(); }

  // Note that Handle<T>::operator= takes a raw pointer, not a handle, so
  // to assign handles, one writes: `lhandle = *rhandle;`. (This is to avoid
  // confusion about which HandleScope tracks lhandle after the assignment.)
  template <typename S>
  Handle& operator=(S other) {
    static_assert(std::is_base_of<S, T>::value || std::is_base_of<T, S>::value,
                  "Only up- and down-casts are permitted.");
    *static_cast<T*>(this) = is_checked ? T::cast(other) : bit_cast<T>(other);
    return *this;
  }

  // Let Handle<T> pretend to be a subtype of Handle<S> when T is a subtype of
  // S.
  template <typename S>
  operator const Handle<S, is_checked>&() const {
    static_assert(std::is_base_of<S, T>::value, "Only up-casts are permitted");
    return *reinterpret_cast<const Handle<S, is_checked>*>(this);
  }

  static_assert(std::is_base_of<RawObject, T>::value,
                "You can only get a handle to a python::RawObject.");

  DISALLOW_IMPLICIT_CONSTRUCTORS(Handle);
  DISALLOW_HEAP_ALLOCATION();

 private:
  template <typename, bool>
  friend class Handle;

  Handle<RawObject>* next_;
  HandleScope* scope_;
};

// This is a temporary workaround until there is infrastructure to do a
// checked up-cast based on a class flag instead of a class ID.  This is only
// needed when a handle must up-cast a user-defined subclass to a built-in class
// type.  The use of this handle should be limited to handles types that are
// subclasses of BaseException.
template <typename T>
using UncheckedHandle = Handle<T, false>;

// TODO(T34683229): These typedefs are temporary as part of an in-progress
// migration.
#define HANDLE_ALIAS(ty)                                                       \
  class Raw##ty;                                                               \
  using ty = Handle<Raw##ty>
HANDLE_ALIAS(Object);
HANDLE_ALIAS(Int);
HANDLE_ALIAS(SmallInt);
HANDLE_ALIAS(Header);
HANDLE_ALIAS(Bool);
HANDLE_ALIAS(NoneType);
HANDLE_ALIAS(Error);
HANDLE_ALIAS(Str);
HANDLE_ALIAS(SmallStr);
HANDLE_ALIAS(HeapObject);
HANDLE_ALIAS(BaseException);
HANDLE_ALIAS(Exception);
HANDLE_ALIAS(StopIteration);
HANDLE_ALIAS(SystemExit);
HANDLE_ALIAS(RuntimeError);
HANDLE_ALIAS(NotImplementedError);
HANDLE_ALIAS(ImportError);
HANDLE_ALIAS(ModuleNotFoundError);
HANDLE_ALIAS(LookupError);
HANDLE_ALIAS(IndexError);
HANDLE_ALIAS(KeyError);
HANDLE_ALIAS(Type);
HANDLE_ALIAS(Array);
HANDLE_ALIAS(Bytes);
HANDLE_ALIAS(Tuple);
HANDLE_ALIAS(LargeStr);
HANDLE_ALIAS(LargeInt);
HANDLE_ALIAS(Float);
HANDLE_ALIAS(Complex);
HANDLE_ALIAS(Property);
HANDLE_ALIAS(Range);
HANDLE_ALIAS(RangeIterator);
HANDLE_ALIAS(Slice);
HANDLE_ALIAS(StaticMethod);
HANDLE_ALIAS(ListIterator);
HANDLE_ALIAS(SetIterator);
HANDLE_ALIAS(StrIterator);
HANDLE_ALIAS(TupleIterator);
HANDLE_ALIAS(Code);
HANDLE_ALIAS(Function);
HANDLE_ALIAS(Instance);
HANDLE_ALIAS(Module);
HANDLE_ALIAS(NotImplemented);
HANDLE_ALIAS(Dict);
HANDLE_ALIAS(DictItemIterator);
HANDLE_ALIAS(DictItems);
HANDLE_ALIAS(DictKeyIterator);
HANDLE_ALIAS(DictKeys);
HANDLE_ALIAS(DictValueIterator);
HANDLE_ALIAS(DictValues);
HANDLE_ALIAS(Set);
HANDLE_ALIAS(List);
HANDLE_ALIAS(ValueCell);
HANDLE_ALIAS(Ellipsis);
HANDLE_ALIAS(WeakRef);
HANDLE_ALIAS(BoundMethod);
HANDLE_ALIAS(ClassMethod);
HANDLE_ALIAS(Layout);
HANDLE_ALIAS(Super);
HANDLE_ALIAS(GeneratorBase);
HANDLE_ALIAS(Generator);
HANDLE_ALIAS(Coroutine);
HANDLE_ALIAS(HeapFrame);
#undef HANDLE_ALIAS

}  // namespace python
