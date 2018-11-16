#pragma once

#include "globals.h"

namespace python {

class Type;
class Dict;
class Frame;
class FrameVisitor;
class Function;
class Handles;
class Module;
class Object;
class ObjectArray;
class PointerVisitor;
class Runtime;
class Str;

class Thread {
 public:
  static const int kDefaultStackSize = 1 * kMiB;

  explicit Thread(word size);
  ~Thread();

  static Thread* currentThread();
  static void setCurrentThread(Thread* thread);

  Frame* openAndLinkFrame(word num_args, word num_vars, word stack_depth);
  Frame* linkFrame(Frame* frame);
  Frame* pushFrame(Object* code);
  Frame* pushNativeFrame(void* fn, word nargs);
  Frame* pushModuleFunctionFrame(Module* module, Object* code);
  Frame* pushClassFunctionFrame(Object* function, Object* dict);
  void checkStackOverflow(word max_size);

  void popFrame();

  Object* run(Object* object);
  Object* runModuleFunction(Module* module, Object* object);
  Object* runClassFunction(Object* function, Object* dict);

  Thread* next() { return next_; }

  Handles* handles() { return handles_; }

  Runtime* runtime() { return runtime_; }

  Frame* initialFrame() { return initialFrame_; }

  Frame* currentFrame() { return currentFrame_; }

  // The stack pointer is computed by taking the value stack top of the current
  // frame.
  byte* stackPtr();

  void visitRoots(PointerVisitor* visitor);

  void visitStackRoots(PointerVisitor* visitor);

  void setRuntime(Runtime* runtime) { runtime_ = runtime; }

  // Raises an AttributeError exception and returns an Error that must be
  // returned up the stack by the caller.
  Object* raiseAttributeError(Object* value);
  Object* raiseAttributeErrorWithCStr(const char* message);

  // Raises an Index exception and returns an Error object that must be returned
  // up the stack by the caller.
  Object* raiseIndexError(Object* value);
  Object* raiseIndexErrorWithCStr(const char* message);

  // Raises a KeyError exception and returns an Error that must be returned up
  // the stack by the caller.
  Object* raiseKeyError(Object* value);
  Object* raiseKeyErrorWithCStr(const char* message);

  // Raises an OverflowError exception and returns an Error object that must be
  // returned up the stack by the caller.
  Object* raiseOverflowError(Object* value);
  Object* raiseOverflowErrorWithCStr(const char* message);

  // Raises a RuntimeError exception and returns an Error object that must be
  // returned up the stack by the caller.
  Object* raiseRuntimeError(Object* value);
  Object* raiseRuntimeErrorWithCStr(const char* message);

  // Raises a SystemError exception and returns an Error object that must be
  // returned up the stack by the caller.
  Object* raiseSystemError(Object* value);
  Object* raiseSystemErrorWithCStr(const char* message);

  // Raises a TypeError exception and returns an Error object that must be
  // returned up the stack by the caller.
  Object* raiseTypeError(Object* value);
  Object* raiseTypeErrorWithCStr(const char* message);

  // Raises a ValueError exception and returns an Error object that must be
  // returned up the stack by the caller.
  Object* raiseValueError(Object* value);
  Object* raiseValueErrorWithCStr(const char* message);

  // Raises a StopIteration exception and returns an Error object that must be
  // returned up the stack by the caller.
  Object* raiseStopIteration(Object* value);

  // Returns true if there is an exception pending.
  bool hasPendingException();

  // Returns true if there is a StopIteration exception pending. Special-cased
  // to support generators until we have proper exception support.
  bool hasPendingStopIteration();

  // If there's a pending exception, clear it.
  void clearPendingException();

  // If there's a pending exception, prints it and ignores it.
  void ignorePendingException();

  // If there is a pending exception, prints it and aborts the runtime.
  void abortOnPendingException();

  // Returns the type of the current exception or None if no exception is
  // pending.
  Object* exceptionType() { return exception_type_; }

  // Returns the value of the current exception.
  Object* exceptionValue() { return exception_value_; }

  // Walk all the frames on the stack starting with the top-most frame
  void visitFrames(FrameVisitor* visitor);

 private:
  void pushInitialFrame();

  void setExceptionType(Object* type) { exception_type_ = type; }

  void setExceptionValue(Object* value) { exception_value_ = value; }

  void setExceptionTraceback(Object* traceback) {
    exception_traceback_ = traceback;
  }

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

  // The type object corresponding to the pending exception.  If there is no
  // pending exception this will be set to None.
  Object* exception_type_;

  // The value of the pending exception.
  Object* exception_value_;

  // The traceback of the pending exception.
  Object* exception_traceback_;

  static thread_local Thread* current_thread_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace python
