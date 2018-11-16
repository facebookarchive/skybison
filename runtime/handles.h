#pragma once

#include "globals.h"
#include "thread.h"
#include "utils.h"
#include "vector.h"

namespace python {

class HandleScope;
class Object;
class ObjectHandle;
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

  template <typename T>
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

  ObjectHandle* push(ObjectHandle* handle) {
    DCHECK(this == handles_->top(), "unexpected this");
    ObjectHandle* result = list_;
    list_ = handle;
    return result;
  }

 private:
  explicit HandleScope(Handles* handles) : list_(nullptr), handles_(handles) {
    handles_->push(this);
  }

  ObjectHandle* list() { return list_; }

  ObjectHandle* list_;
  Handles* handles_;

  friend class Handles;
  friend class ObjectHandle;

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

class ObjectHandle {
 public:
  ObjectHandle(HandleScope* scope, Object* pointer)
      : pointer_(pointer), next_(scope->push(this)), scope_(scope) {}

  ~ObjectHandle() {
    DCHECK(scope_->list() == this, "unexpected this");
    scope_->list_ = next_;
  }

  Object** pointer() { return &pointer_; }

  ObjectHandle* next() const { return next_; }

 protected:
  Object* pointer_;

  // The casting constructor below needs this, but it appears you can't
  // access your parent's protected fields when they are on a sibling class.
  static HandleScope* scope(const ObjectHandle& o) { return o.scope_; }

 private:
  ObjectHandle* next_;
  HandleScope* scope_;
  DISALLOW_COPY_AND_ASSIGN(ObjectHandle);
};

template <typename T>
class Handle : public ObjectHandle {
 public:
  T* operator->() const { return reinterpret_cast<T*>(pointer_); }

  T* operator*() const { return reinterpret_cast<T*>(pointer_); }

  // Note that Handle<T>::operator= takes a raw pointer, not a handle, so
  // to assign handles, one writes: `lhandle = *rhandle;`. (This is to avoid
  // confusion about which HandleScope tracks lhandle after the assignment.)
  template <typename S>
  Handle<T>& operator=(S* other) {
    static_assert(std::is_base_of<S, T>::value || std::is_base_of<T, S>::value,
                  "Only up- and down-casts are permitted.");
    pointer_ = T::cast(other);
    return *this;
  }

  template <typename S>
  explicit Handle(const Handle<S>& other)
      : ObjectHandle(scope(other), T::cast(*other)) {
    static_assert(std::is_base_of<S, T>::value || std::is_base_of<T, S>::value,
                  "Only up- and down-casts are permitted.");
  }

  Handle(HandleScope* scope, Object* pointer)
      : ObjectHandle(scope, T::cast(pointer)) {
    static_assert(std::is_base_of<Object, T>::value,
                  "You can only get a handle to a python::Object.");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(Handle);
};

// This class is a temporary workaround until there is infrastructure to do a
// checked up-cast based on a class flag instead of a class ID.  This is only
// needed when a handle must up-cast a user-defined subclass to a built-in class
// type.  The use of this handle should be limited to handles types that are
// subclasses of BaseException.
template <typename T>
class UncheckedHandle : public ObjectHandle {
 public:
  T* operator->() const { return reinterpret_cast<T*>(pointer_); }

  T* operator*() const { return reinterpret_cast<T*>(pointer_); }

  UncheckedHandle(HandleScope* scope, Object* pointer)
      : ObjectHandle(scope, pointer) {
    static_assert(std::is_base_of<Object, T>::value,
                  "You can only get a handle to a python::Object.");
  }

 private:
  DISALLOW_HEAP_ALLOCATION();
  DISALLOW_COPY_AND_ASSIGN(UncheckedHandle);
};

}  // namespace python
