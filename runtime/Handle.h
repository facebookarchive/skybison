#pragma once

#include "Globals.h"
#include "Object.h"

namespace python {

class Handles {
 public:
  static void Initialize();
  static void Visit(void callback(Object**));

 private:
  static void Grow();
  static intptr_t Push(Object* object);

  static Object** stack_;
  static intptr_t top_;
  static intptr_t size_;

  friend class Handle;
  friend class HandleScope;

  DISALLOW_IMPLICIT_CONSTRUCTORS(Handles);
};

class HandleScope {
};

class Handle {
public:
  Handle(Object* object) {
    index_ = Handles::Push(object);
  }

  template<typename T>
  T* operator->() const {
    return reinterpret_cast<T*>(Handles::stack_[index_]);
  }

private:
  intptr_t index_;

  DISALLOW_COPY_AND_ASSIGN(Handle);
};

}  // namespace python
