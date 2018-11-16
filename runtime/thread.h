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

} // namespace python
