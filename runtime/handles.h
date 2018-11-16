#pragma once

#include <cassert>
#include <type_traits>

#include "globals.h"
#include "thread.h"

namespace python {

class HandleScope;
class Object;
class ObjectHandle;
class PointerVisitor;

class Handles {
 public:
  static const int kInitialSize = 10;

  Handles();

  ~Handles();

  void visitPointers(PointerVisitor* visitor);

 private:
  void grow();

  void push(HandleScope* scope) {
    if (size_ == top_) {
      grow();
    }
    scopes_[top_++] = scope;
  }

  void pop() {
    assert(top_ != 0);
    top_--;
  }

  HandleScope* top() {
    return scopes_[top_ - 1];
  }

  HandleScope** scopes_;
  intptr_t top_;
  intptr_t size_;

  template <typename T>
  friend class Handle;
  friend class HandleScope;

  DISALLOW_COPY_AND_ASSIGN(Handles);
};

class HandleScope {
 public:
  explicit HandleScope()
      : handles_(Thread::currentThread()->handles()), list_(nullptr) {
    handles_->push(this);
  }

  // TODO: only for tests.
  explicit HandleScope(Handles* handles) : handles_(handles), list_(nullptr) {
    handles_->push(this);
  }

  ~HandleScope() {
    assert(this == handles_->top());
    handles_->pop();
  }

  ObjectHandle* push(ObjectHandle* handle) {
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

  ObjectHandle* next() {
    return next_;
  }

 protected:
  Object* pointer_;

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
