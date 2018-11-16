#pragma once

#include "globals.h"

namespace python {

class Frame;
class Handles;
class Object;
class Runtime;

class Thread {
 public:
  static const int kDefaultStackSize = 1 * MiB;

  explicit Thread(word size);
  ~Thread();

  static Thread* currentThread();
  static void setCurrentThread(Thread* thread);

  byte* ptr() {
    return ptr_;
  }

  Frame* pushFrame(Object* code, Frame* previousFrame);
  void popFrame(Frame* frame);

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

  Frame* initialFrame() {
    return initialFrame_;
  }

  void setRuntime(Runtime* runtime) {
    runtime_ = runtime;
  }

 private:
  void pushInitialFrame();

  Handles* handles_;

  word size_;
  byte* start_;
  byte* end_;
  byte* ptr_;

  Frame* initialFrame_;
  Thread* next_;
  Runtime* runtime_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

} // namespace python
