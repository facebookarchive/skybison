#pragma once

#include "globals.h"
#include "objects.h"

namespace python {

class Handles {
 public:
  static void initialize();
  static void visit(void callback(Object**));

 private:
  static void grow();
  static intptr_t push(Object* object);

  static Object** stack_;
  static intptr_t top_;
  static intptr_t size_;

  friend class Handle;
  friend class HandleScope;

  DISALLOW_IMPLICIT_CONSTRUCTORS(Handles);
};

class HandleScope {};

class Handle {
 public:
  Handle(Object* object) {
    index_ = Handles::push(object);
  }

  template <typename T>
  T* operator->() const {
    return reinterpret_cast<T*>(Handles::stack_[index_]);
  }

 private:
  intptr_t index_;

  DISALLOW_COPY_AND_ASSIGN(Handle);
};

} // namespace python
