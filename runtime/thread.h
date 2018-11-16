#pragma once

#include "globals.h"
#include "utils.h"

namespace python {

class Frame;
class Handles;
class Object;

class Thread {
 public:
  static const int kDefaultStackSize = 1 * MiB;

  explicit Thread(int size);
  ~Thread();

  static Thread* currentThread();
  static void setCurrentThread(Thread* thread);

  Frame* pushFrame(Object* code);
  void popFrame(Frame* frame);

  // Push an object onto the thread stack
  inline void pushObject(Object* object);

  // Pop an object off of the thread stack
  inline Object* popObject();

  Object* run(Object* object);

  Thread* next() {
    return next_;
  }

  Handles* handles() {
    return handles_;
  }

 private:
  Handles* handles_;

  int size_;
  byte* start_;
  byte* end_;
  byte* ptr_;

  Thread* next_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

void Thread::pushObject(Object* object) {
  assert(ptr_ - kPointerSize >= start_);
  ptr_ -= kPointerSize;
  *reinterpret_cast<Object**>(ptr_) = object;
}

Object* Thread::popObject() {
  assert(ptr_ + kPointerSize <= end_);
  auto ret = *reinterpret_cast<Object**>(ptr_);
  ptr_ += kPointerSize;
  return ret;
}

} // namespace python
