#pragma once

#include <cassert>

#include "globals.h"
#include "thread.h"
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
  void push(HandleScope* scope) {
    scopes_.push_back(scope);
  }

  void pop() {
    assert(!scopes_.empty());
    scopes_.pop_back();
  }

  HandleScope* top() {
    assert(!scopes_.empty());
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

  // TODO: only for tests.
  explicit HandleScope(Handles* handles) : list_(nullptr), handles_(handles) {
    handles_->push(this);
  }

  ~HandleScope() {
    assert(this == handles_->top());
    handles_->pop();
  }

  ObjectHandle* push(ObjectHandle* handle) {
    assert(this == handles_->top());
    ObjectHandle* result = list_;
    list_ = handle;
    return result;
  }

 private:
  ObjectHandle* list() {
    return list_;
  }

  ObjectHandle* list_;
  Handles* handles_;

  friend class Handles;
  friend class ObjectHandle;

  DISALLOW_COPY_AND_ASSIGN(HandleScope);
};

class ObjectHandle {
 public:
  ObjectHandle(HandleScope* scope, Object* pointer)
      : pointer_(pointer), next_(scope->push(this)), scope_(scope) {}

  ~ObjectHandle() {
    assert(scope_->list() == this);
    scope_->list_ = next_;
  }

  Object** pointer() {
    return &pointer_;
  }

  ObjectHandle* next() const {
    return next_;
  }

 protected:
  Object* pointer_;

  // The casting constructor below needs this, but it appears you can't
  // access your parent's protected fields when they are on a sibling class.
  static HandleScope* scope(const ObjectHandle& o) {
    return o.scope_;
  }

 private:
  ObjectHandle* next_;
  HandleScope* scope_;
  DISALLOW_COPY_AND_ASSIGN(ObjectHandle);
};

template <typename T>
class Handle : public ObjectHandle {
 public:
  T* operator->() const {
    return reinterpret_cast<T*>(pointer_);
  }

  T* operator*() const {
    return reinterpret_cast<T*>(pointer_);
  }

  // Note that Handle<T>::operator= takes a raw pointer, not a handle, so
  // to assign handles, one writes: `lhandle = *rhandle;`. (This is to avoid
  // confusion about which HandleScope tracks lhandle after the assignment.)
  template <typename S>
  Handle<T>& operator=(S* other) {
    static_assert(
        std::is_base_of<S, T>::value || std::is_base_of<T, S>::value,
        "Only up- and down-casts are permitted.");
    pointer_ = T::cast(other);
    return *this;
  }

  template <typename S>
  explicit Handle(const Handle<S>& other)
      : ObjectHandle(scope(other), T::cast(*other)) {
    static_assert(
        std::is_base_of<S, T>::value || std::is_base_of<T, S>::value,
        "Only up- and down-casts are permitted.");
  }

  Handle(HandleScope* scope, Object* pointer)
      : ObjectHandle(scope, T::cast(pointer)) {
    static_assert(
        std::is_base_of<Object, T>::value,
        "You can only get a handle to a python::Object.");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(Handle);
};

} // namespace python
