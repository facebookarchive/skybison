#pragma once

#include "globals.h"
#include "utils.h"

namespace python {

class Frame;
class Handles;
class Object;
class Runtime;

class Thread {
 public:
  static const int kDefaultStackSize = 1 * MiB;

  explicit Thread(int size);
  ~Thread();

  static Thread* currentThread();
  static void setCurrentThread(Thread* thread);

  Frame* pushFrame(Object* code);
  void popFrame(Frame* frame);

  // Grab the n-th item from the top of the stack (e.g. 0 grabs top of stack).
  inline Object* peekObject(int offset);

  // Push an object onto the thread stack
  inline void pushObject(Object* object);

  // Pop an object off of the thread stack
  inline Object* popObject();

  // Pop count objects off of the stack
  inline Object* popObjects(int count);

  Object* run(Object* object);

  Thread* next() {
    return next_;
  }

  Handles* handles() {
    return handles_;
  }

  Runtime* runtime() {
    return runtime_;
  }

  void setRuntime(Runtime* runtime) {
    runtime_ = runtime;
  }

 private:
  Handles* handles_;

  int size_;
  byte* start_;
  byte* end_;
  byte* ptr_;

  Thread* next_;
  Runtime* runtime_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

Object* Thread::peekObject(int offset) {
  assert(ptr_ + (offset + 1) * kPointerSize <= end_);
  return *reinterpret_cast<Object**>(ptr_ + offset * kPointerSize);
}

void Thread::pushObject(Object* object) {
  assert(ptr_ - kPointerSize >= start_);
  ptr_ -= kPointerSize;
  *reinterpret_cast<Object**>(ptr_) = object;
}

Object* Thread::popObject() {
  return popObjects(1);
}

Object* Thread::popObjects(int count) {
  assert(count > 0);
  assert(ptr_ + kPointerSize * count <= end_);
  ptr_ += kPointerSize * count;
  return *reinterpret_cast<Object**>(ptr_ - kPointerSize);
}

} // namespace python
