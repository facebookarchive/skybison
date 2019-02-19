#pragma once

#include "globals.h"
#include "objects.h"
#include "symbols.h"
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
  Frame* pushExecFrame(const Handle<RawCode>& code,
                       const Handle<RawDict>& globals,
                       const Handle<RawObject>& locals);
  Frame* pushModuleFunctionFrame(const Handle<RawModule>& module,
                                 const Handle<RawCode>& code);
  Frame* pushClassFunctionFrame(const Handle<RawFunction>& function,
                                const Handle<RawDict>& dict);
  void checkStackOverflow(word max_size);

  void popFrame();

  // Runs a code object on the current thread.  Assumes that the initial frame
  // is at the top of the stack.
  RawObject run(const Handle<RawCode>& code);

  // Runs a code object on the current thread.
  RawObject exec(const Handle<RawCode>& code, const Handle<RawDict>& globals,
                 const Handle<RawObject>& locals);

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

  // Calls out to the interpreter to lookup and call a method on the receiver.
  RawObject invokeMethod1(const Handle<RawObject>& receiver, SymbolId selector);

  // Calls out to the interpreter to lookup and call a method on the receiver
  // with one argument.
  RawObject invokeMethod2(const Handle<RawObject>& receiver, SymbolId selector,
                          const Handle<RawObject>& arg1);

  // Calls out to the interpreter to lookup and call a method on the receiver
  // with two arguments.
  RawObject invokeMethod3(const Handle<RawObject>& receiver, SymbolId selector,
                          const Handle<RawObject>& arg1,
                          const Handle<RawObject>& arg2);

  // Calls out to the interpreter to lookup and call a function with one
  // argument
  RawObject invokeFunction1(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1);

  // Calls out to the interpreter to lookup and call a function with two
  // arguments
  RawObject invokeFunction2(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1,
                            const Handle<RawObject>& arg2);

  // Calls out to the interpreter to lookup and call a function with three
  // arguments
  RawObject invokeFunction3(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1,
                            const Handle<RawObject>& arg2,
                            const Handle<RawObject>& arg3);

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

  // Raises a TypeError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseUnsupportedBinaryOperation(RawObject left, RawObject right,
                                            RawStr op_name);

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

  // Exception support
  //
  // We track two sets of exception state, a "pending" exception and a "caught"
  // exception. Each one has a type, value, and traceback.
  //
  // An exception is pending from the moment it is raised until it is caught by
  // a handler. It transitions from pending to caught right before execution of
  // the handler. If the handler re-raises, the exception transitions back to
  // pending to resume unwinding; otherwise, the caught exception is cleared
  // when the handler block is popped.
  //
  // The pending exception is stored directly in the Thread, since there is at
  // most one active at any given time. The caught exception is kept in a stack
  // of ExceptionState objects, and the Thread holds a pointer to the top of
  // the stack. When the runtime enters a generator or coroutine, it pushes the
  // ExceptionState owned by that object onto this stack, allowing that state
  // to be preserved if we yield in an except block. When there is no generator
  // or coroutine running, the default ExceptionState created with this Thread
  // holds the caught exception.

  // Returns true if there is a pending exception.
  bool hasPendingException();

  // Returns true if there is a StopIteration exception pending.
  bool hasPendingStopIteration();

  // If there is a StopIteration exception pending, clear it and return
  // true. Otherwise, return false.
  bool clearPendingStopIteration();

  // If there's a pending exception, clears it.
  void clearPendingException();

  // If there's a pending exception, prints it and ignores it.
  void ignorePendingException();

  // Assuming there is a pending exception, print its type and __str__() to
  // stderr.
  void printPendingException();

  // Gets the type, value, or traceback of the pending exception. No pending
  // exception is indicated with a type of None.
  RawObject pendingExceptionType() { return pending_exc_type_; }
  RawObject pendingExceptionValue() { return pending_exc_value_; }
  RawObject pendingExceptionTraceback() { return pending_exc_traceback_; }

  // Sets the type, value, or traceback of the pending exception.
  void setPendingExceptionType(RawObject type) { pending_exc_type_ = type; }
  void setPendingExceptionValue(RawObject value) { pending_exc_value_ = value; }
  void setPendingExceptionTraceback(RawObject traceback) {
    pending_exc_traceback_ = traceback;
  }

  // Returns true if there is a caught exception.
  bool hasCaughtException();

  // Gets the type, value or traceback of the caught exception. No caught
  // exception is indicated with a type of None.
  RawObject caughtExceptionType();
  RawObject caughtExceptionValue();
  RawObject caughtExceptionTraceback();

  // Sets the type, value, or traceback of the caught exception.
  void setCaughtExceptionType(RawObject type);
  void setCaughtExceptionValue(RawObject value);
  void setCaughtExceptionTraceback(RawObject traceback);

  // Gets or sets the current caught ExceptionState.
  RawObject caughtExceptionState();
  void setCaughtExceptionState(RawObject state);

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

  // State of the pending exception.
  RawObject pending_exc_type_;
  RawObject pending_exc_value_;
  RawObject pending_exc_traceback_;

  // Stack of ExceptionStates for the current caught exception. Generators push
  // their private state onto this stack before resuming, and pop it after
  // suspending.
  RawObject caught_exc_stack_;

  static thread_local Thread* current_thread_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace python
