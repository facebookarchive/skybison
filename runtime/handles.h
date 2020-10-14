#pragma once

#include "handles-decl.h"
#include "objects.h"
#include "thread.h"

namespace py {

class WARN_UNUSED HandleScope {
 public:
  explicit HandleScope(Thread* thread) : handles_(thread->handles()) {}

  Handles* handles() const { return handles_; }

 private:
  Handles* const handles_;

  DISALLOW_COPY_AND_ASSIGN(HandleScope);
};

template <typename T>
class WARN_UNUSED Handle : public T {
 public:
  Handle(HandleScope* scope, RawObject obj)
      : T(obj.rawCast<T>()),
        handles_(scope->handles()),
        next_(handles_->push(pointer())) {
    DCHECK(isValidType(), "Invalid Handle construction");
  }

  // Don't allow constructing a Handle directly from another Handle; require
  // dereferencing the source.
  template <typename S>
  Handle(HandleScope*, const Handle<S>&) = delete;

  ~Handle() {
    DCHECK(handles_->head() == pointer(), "unexpected this");
    handles_->pop(next_);
  }

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
                "You can only get a handle to a py::RawObject.");

  DISALLOW_IMPLICIT_CONSTRUCTORS(Handle);
  DISALLOW_HEAP_ALLOCATION();

 private:
  bool isValidType() const;

  Handle<RawObject>* pointer() {
    return reinterpret_cast<Handle<RawObject>*>(this);
  }

  Handles* const handles_;
  Handle<RawObject>* const next_;
};

}  // namespace py
