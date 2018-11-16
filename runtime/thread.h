#pragma once

#include "globals.h"

namespace python {

class Frame;
class Function;
class Handles;
class Module;
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

  Frame*
  openAndLinkFrame(word localsSize, word stackSize, Frame* previousFrame);
  Frame* pushFrame(Object* code, Frame* previousFrame);
  Frame*
  pushModuleFunctionFrame(Module* module, Object* code, Frame* previousFrame);
  Frame*
  pushClassFunctionFrame(Object* function, Object* dictionary, Frame* caller);

  void popFrame(Frame* frame);

  Object* run(Object* object);
  Object* runModuleFunction(Module* module, Object* object);
  Object* runClassFunction(Object* function, Object* dictionary, Frame* caller);

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
