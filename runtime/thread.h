#pragma once

#include "globals.h"

namespace python {

class Class;
class Dictionary;
class Frame;
class FrameVisitor;
class Function;
class Handles;
class Module;
class Object;
class ObjectArray;
class PointerVisitor;
class Runtime;
class String;

class Thread {
 public:
  static const int kDefaultStackSize = 1 * MiB;

  explicit Thread(word size);
  ~Thread();

  static Thread* currentThread();
  static void setCurrentThread(Thread* thread);

  Frame* openAndLinkFrame(word numArgs, word numVars, word stackDepth);
  Frame* pushFrame(Object* code);
  Frame* pushNativeFrame(void* fn, word nargs);
  Frame* pushModuleFunctionFrame(Module* module, Object* code);
  Frame* pushClassFunctionFrame(Object* function, Object* dictionary);

  void popFrame();

  Object* run(Object* object);
  Object* runModuleFunction(Module* module, Object* object);
  Object* runClassFunction(Object* function, Object* dictionary);

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

  Frame* currentFrame() {
    return currentFrame_;
  }

  // The stack pointer is computed by taking the value stack top of the current
  // frame.
  byte* stackPtr();

  void visitRoots(PointerVisitor* visitor);

  void visitStackRoots(PointerVisitor* visitor);

  void setRuntime(Runtime* runtime) {
    runtime_ = runtime;
  }

  // Exception API
  //
  // Native code that wishes to throw an exception into python must do two
  // things:
  //
  // 1. Call `throwException` or one of its convenience wrappers.
  // 2. Return an Error object from the native entry point.
  //
  // It is an error to do one of these but not the other. When the native entry
  // point returns, the pending exception will be raised in the python
  // interpreter.
  //
  // Note that it is perfectly ok to use the Error return value internally
  // without throwing exceptions. The restriction on returning an Error only
  // applies to native entry points.

  // Instantiates an exception with the given arguments and posts it to be
  // thrown upon returning to managed code.
  // TODO: decide on the signature for this function.
  // void throwException(...);

  // Convenience methods for throwing a RuntimeError exception.
  Object* throwRuntimeError(Object* value);
  Object* throwRuntimeErrorFromCString(const char* message);

  // Convenience methods for throwing a TypeError exception.
  Object* throwTypeError(Object* value);
  Object* throwTypeErrorFromCString(const char* message);

  // Convenience methods for throwing a ValueError exception.
  Object* throwValueError(Object* value);
  Object* throwValueErrorFromCString(const char* message);

  // Convenience methods for throwing an AttributeError exception.
  Object* throwAttributeError(Object* value);
  Object* throwAttributeErrorFromCString(const char* message);

  // Convenience methods for throwing a KeyError exception.
  Object* throwKeyError(Object* value);
  Object* throwKeyErrorFromCString(const char* message);

  // Gets the pending exception object - if it is None, no exception has been
  // posted.
  Object* pendingException();

  // If there's a pending exception, prints it and ignores
  void ignorePendingException();

  // If there is a pending exception, prints it and aborts
  void abortOnPendingException();

  // Walk all the frames on the stack starting with the top-most frame
  void visitFrames(FrameVisitor* visitor);

 private:
  void pushInitialFrame();

  Handles* handles_;

  word size_;
  byte* start_;
  byte* end_;

  // initialFrame_ is a sentinel frame (all zeros) that is pushed onto the
  // stack when the thread is created.
  Frame* initialFrame_;

  // currentFrame_ always points to the top-most frame on the stack. When there
  // are no activations (e.g. immediately after the thread is created) this
  // points at initialFrame_.
  Frame* currentFrame_;
  Thread* next_;
  Runtime* runtime_;

  // A pending exception object which should be thrown upon returning to
  // managed code. Is set to None if there is no pending exception.
  Object* pending_exception_;

  static thread_local Thread* current_thread_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

} // namespace python
