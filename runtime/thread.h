#pragma once

#include "globals.h"
#include "objects.h"
#include "vector.h"

namespace python {

template <typename>
class Handle;
class Frame;
class FrameVisitor;
class HandleScope;
class PointerVisitor;
class Runtime;

class Handles {
 public:
  static const int kInitialSize = 10;

  Handles();

  void visitPointers(PointerVisitor* visitor);

 private:
  void push(HandleScope* scope) { scopes_.push_back(scope); }

  void pop() {
    DCHECK(!scopes_.empty(), "pop on empty");
    scopes_.pop_back();
  }

  HandleScope* top() {
    DCHECK(!scopes_.empty(), "top on empty");
    return scopes_.back();
  }

  Vector<HandleScope*> scopes_;

  friend class HandleScope;

  DISALLOW_COPY_AND_ASSIGN(Handles);
};

class Thread {
 public:
  static const int kDefaultStackSize = 1 * kMiB;

  explicit Thread(word size);
  ~Thread();

  static Thread* currentThread();
  static void setCurrentThread(Thread* thread);

  Frame* openAndLinkFrame(word num_args, word num_vars, word stack_depth);
  Frame* linkFrame(Frame* frame);
  Frame* pushFrame(const Handle<RawCode>& code);
  Frame* pushNativeFrame(void* fn, word nargs);
  Frame* pushModuleFunctionFrame(const Handle<RawModule>& module,
                                 const Handle<RawCode>& code);
  Frame* pushClassFunctionFrame(const Handle<RawFunction>& function,
                                const Handle<RawDict>& dict);
  void checkStackOverflow(word max_size);

  void popFrame();

  // Runs a code object on the current thread.  Assumes that the initial frame
  // is at the top of the stack.
  RawObject run(const Handle<RawCode>& code);

  // Runs a module body function on the current thread.  Assumes that the
  // initial frame is at the top of the stack.
  RawObject runModuleFunction(const Handle<RawModule>& module,
                              const Handle<RawCode>& code);

  // Runs a class body function on the current thread.
  RawObject runClassFunction(const Handle<RawFunction>& function,
                             const Handle<RawDict>& dict);

  Thread* next() { return next_; }

  Handles* handles() { return &handles_; }

  Runtime* runtime() { return runtime_; }

  Frame* initialFrame() { return initialFrame_; }

  Frame* currentFrame() { return currentFrame_; }

  // The stack pointer is computed by taking the value stack top of the current
  // frame.
  byte* stackPtr();

  void visitRoots(PointerVisitor* visitor);

  void visitStackRoots(PointerVisitor* visitor);

  void setRuntime(Runtime* runtime) { runtime_ = runtime; }

  // Raises an exception with the given type and returns an Error that must be
  // returned up the stack by the caller.
  RawObject raise(LayoutId type, RawObject value);
  RawObject raiseWithCStr(LayoutId type, const char* message);

  // Raises an AttributeError exception and returns an Error that must be
  // returned up the stack by the caller.
  RawObject raiseAttributeError(RawObject value);
  RawObject raiseAttributeErrorWithCStr(const char* message);

  // Raises a TypeError exception for PyErr_BadArgument.
  void raiseBadArgument();

  // Raises a SystemError exception for PyErr_BadInternalCall.
  void raiseBadInternalCall();

  // Raises an Index exception and returns an Error object that must be returned
  // up the stack by the caller.
  RawObject raiseIndexError(RawObject value);
  RawObject raiseIndexErrorWithCStr(const char* message);

  // Raises a KeyError exception and returns an Error that must be returned up
  // the stack by the caller.
  RawObject raiseKeyError(RawObject value);
  RawObject raiseKeyErrorWithCStr(const char* message);

  // Raises a MemoryError exception and returns an Error that must be returned
  // up the stack by the caller.
  RawObject raiseMemoryError();

  // Raises an OverflowError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseOverflowError(RawObject value);
  RawObject raiseOverflowErrorWithCStr(const char* message);

  // Raises a RuntimeError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseRuntimeError(RawObject value);
  RawObject raiseRuntimeErrorWithCStr(const char* message);

  // Raises a SystemError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseSystemError(RawObject value);
  RawObject raiseSystemErrorWithCStr(const char* message);

  // Raises a TypeError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseTypeError(RawObject value);
  RawObject raiseTypeErrorWithCStr(const char* message);

  // Raises a ValueError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseValueError(RawObject value);
  RawObject raiseValueErrorWithCStr(const char* message);

  // Raises a StopIteration exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseStopIteration(RawObject value);

  // Raises a ZeroDivision exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseZeroDivisionError(RawObject value);
  RawObject raiseZeroDivisionErrorWithCStr(const char* message);

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
  RawObject exceptionType() { return exception_type_; }

  void setExceptionType(RawObject new_type) { exception_type_ = new_type; }

  // Returns the value of the current exception.
  RawObject exceptionValue() { return exception_value_; }

  void setExceptionValue(RawObject new_value) { exception_value_ = new_value; }

  // Returns the traceback of the current exception.
  RawObject exceptionTraceback() { return exception_traceback_; }

  void setExceptionTraceback(RawObject new_traceback) {
    exception_traceback_ = new_traceback;
  }

  // Walk all the frames on the stack starting with the top-most frame
  void visitFrames(FrameVisitor* visitor);

 private:
  void pushInitialFrame();

  Handles handles_;

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
  RawObject exception_type_;

  // The value of the pending exception.
  RawObject exception_value_;

  // The traceback of the pending exception.
  RawObject exception_traceback_;

  static thread_local Thread* current_thread_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace python
