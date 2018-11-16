#include "handle.h"

#include "globals.h"
#include "objects.h"

namespace python {

intptr_t Handles::size_ = 0;
Object** Handles::stack_ = nullptr;
intptr_t Handles::top_ = 0;

void Handles::Initialize() {
  Handles::size_ = 100;
  Handles::stack_ = new Object*[Handles::size_];
}

void Handles::Grow() {
  intptr_t new_size = Handles::size_ * 2;
  Object** new_stack = new Object*[new_size];
  memcpy(new_stack, Handles::stack_, Handles::size_ * sizeof(Handles::stack_[0]));
  delete[] Handles::stack_;
  Handles::size_ = new_size;
  Handles::stack_ = new_stack;
}

intptr_t Handles::Push(Object* object) {
  if (Handles::top_ == Handles::size_) {
    Grow();
  }
  intptr_t result = Handles::top_;
  Handles::stack_[Handles::top_++] = object;
  return result;
}

void Handles::Visit(void callback(Object** object)) {
  for (intptr_t i = 0; i < Handles::size_; i++) {
    (*callback)(&Handles::stack_[i]);
  }
}

}  // namespace python
