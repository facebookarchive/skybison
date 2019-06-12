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
  Handles() = default;

  Handle<RawObject>* head() const { return head_; }

  Handle<RawObject>* push(Handle<RawObject>* new_head) {
    Handle<RawObject>* old_head = head_;
    head_ = new_head;
    return old_head;
  }

  void pop(Handle<RawObject>* new_head) { head_ = new_head; }

  void visitPointers(PointerVisitor* visitor);

  DISALLOW_COPY_AND_ASSIGN(Handles);

 private:
  Handle<RawObject>* head_ = nullptr;
};

class Thread {
 public:
  static const int kDefaultStackSize = 1 * kMiB;

  explicit Thread(word size);
  ~Thread();

  static Thread* current();
  static void setCurrentThread(Thread* thread);

  void linkFrame(Frame* frame);
  // Private method. Call pushCallFrame() or pushNativeFrame() isntead.
  Frame* pushCallFrame(RawFunction function);
  Frame* pushNativeFrame(word nargs);
  Frame* pushClassFunctionFrame(const Handle<RawFunction>& function);
  void checkStackOverflow(word max_size);

  void popFrame();

  // Runs a code object on the current thread.
  RawObject exec(const Handle<RawCode>& code, const Handle<RawDict>& globals,
                 const Handle<RawObject>& locals);

  // Runs a class body function on the current thread.
  NODISCARD RawObject runClassFunction(const Handle<RawFunction>& function,
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

  // Calls out to the interpreter to lookup and call a method on the receiver
  // with the given argument(s). Returns Error<NotFound> if the method can't be
  // found, or the result of the call otheriwse (which may be Error<Exception>).
  RawObject invokeMethod1(const Handle<RawObject>& receiver, SymbolId selector);
  RawObject invokeMethod2(const Handle<RawObject>& receiver, SymbolId selector,
                          const Handle<RawObject>& arg1);
  RawObject invokeMethod3(const Handle<RawObject>& receiver, SymbolId selector,
                          const Handle<RawObject>& arg1,
                          const Handle<RawObject>& arg2);

  // Looks up a method on a type and invokes it with the given receiver and
  // argument(s). Returns Error<NotFound> if the method can't be found, or the
  // result of the call otheriwse (which may be Error<Exception>).
  // ex: str.foo(receiver, arg1, ...)
  RawObject invokeMethodStatic2(LayoutId type, SymbolId method_name,
                                const Handle<RawObject>& receiver,
                                const Handle<RawObject>& arg1);
  RawObject invokeMethodStatic3(LayoutId type, SymbolId method_name,
                                const Handle<RawObject>& receiver,
                                const Handle<RawObject>& arg1,
                                const Handle<RawObject>& arg2);
  RawObject invokeMethodStatic4(LayoutId type, SymbolId method_name,
                                const Handle<RawObject>& receiver,
                                const Handle<RawObject>& arg1,
                                const Handle<RawObject>& arg2,
                                const Handle<RawObject>& arg3);

  // Calls out to the interpreter to lookup and call a function with the given
  // argument(s). Returns Error<NotFound> if the function can't be found, or the
  // result of the call otherwise (which may be Error<Exception>).
  RawObject invokeFunction1(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1);
  RawObject invokeFunction2(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1,
                            const Handle<RawObject>& arg2);
  RawObject invokeFunction3(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1,
                            const Handle<RawObject>& arg2,
                            const Handle<RawObject>& arg3);
  RawObject invokeFunction4(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1,
                            const Handle<RawObject>& arg2,
                            const Handle<RawObject>& arg3,
                            const Handle<RawObject>& arg4);
  RawObject invokeFunction5(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1,
                            const Handle<RawObject>& arg2,
                            const Handle<RawObject>& arg3,
                            const Handle<RawObject>& arg4,
                            const Handle<RawObject>& arg5);
  RawObject invokeFunction6(SymbolId module, SymbolId name,
                            const Handle<RawObject>& arg1,
                            const Handle<RawObject>& arg2,
                            const Handle<RawObject>& arg3,
                            const Handle<RawObject>& arg4,
                            const Handle<RawObject>& arg5,
                            const Handle<RawObject>& arg6);

  // Raises an exception with the given type and returns an Error that must be
  // returned up the stack by the caller.
  RawObject raise(LayoutId type, RawObject value);
  RawObject raiseWithType(RawObject type, RawObject value);
  RawObject raiseWithFmt(LayoutId type, const char* fmt, ...);
  RawObject raiseWithId(LayoutId type, SymbolId msg);

  // Raises a TypeError exception for PyErr_BadArgument.
  void raiseBadArgument();

  // Raises a SystemError exception for PyErr_BadInternalCall.
  void raiseBadInternalCall();

  // Raises a MemoryError exception and returns an Error that must be returned
  // up the stack by the caller.
  RawObject raiseMemoryError();

  // Raises a TypeError exception of the form '<method> requires a <type(obj)>
  // object but got <expected_type>' and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseRequiresType(const Handle<RawObject>& obj,
                              SymbolId expected_type);

  // Raises a TypeError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseUnsupportedBinaryOperation(const Handle<RawObject>& left,
                                            const Handle<RawObject>& right,
                                            SymbolId op_name);

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

  // Assuming there is a StopIteration pending, returns its value, accounting
  // for various potential states of normalization.
  RawObject pendingStopIterationValue();

  // If there's a pending exception, clears it.
  void clearPendingException();

  // If there's a pending exception, prints it and ignores it.
  void ignorePendingException();

  // Gets the type, value, or traceback of the pending exception. No pending
  // exception is indicated with a type of None.
  RawObject pendingExceptionType() { return pending_exc_type_; }
  RawObject pendingExceptionValue() { return pending_exc_value_; }
  RawObject pendingExceptionTraceback() { return pending_exc_traceback_; }

  // Returns whether or not the pending exception type (which must be set) is a
  // subtype of the given type.
  bool pendingExceptionMatches(LayoutId type);

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

  // If there is a current caught exception, attach it to the given exception's
  // __context__ attribute.
  //
  // Returns the updated value, which may be the result of createException(type,
  // value) if value is not an instance of BaseException.
  RawObject chainExceptionContext(const Handle<RawType>& type,
                                  const Handle<RawObject>& value);

  // Returns true if and only if obj is not an Error and there is no pending
  // exception, or obj is an Error<Exception> and there is a pending exception.
  // Mostly used in assertions around call boundaries.
  bool isErrorValueOk(RawObject obj);

  // Walk all the frames on the stack starting with the top-most frame
  void visitFrames(FrameVisitor* visitor);

  RawObject reprEnter(const Handle<RawObject>& obj);
  void reprLeave(const Handle<RawObject>& obj);

  int recursionLimit() { return recursion_limit_; }
  int recursionDepth() { return recursion_depth_; }
  int increaseRecursionDepth() {
    DCHECK(recursion_depth_ <= recursion_limit_,
           "recursion depth can't be more than the recursion limit");
    return recursion_depth_++;
  }
  void decreaseRecursionDepth() {
    DCHECK(recursion_depth_ > 0, "recursion depth can't be less than 0");
    recursion_depth_--;
  }
  void setRecursionLimit(int limit) { recursion_limit_ = limit; }

 private:
  void pushInitialFrame();
  Frame* openAndLinkFrame(word num_args, word num_vars, word stack_depth);

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

  RawObject api_repr_list_;

  // C-API current recursion depth used via _PyThreadState_GetRecursionDepth
  int recursion_depth_ = 0;

  // C-API recursion limit as set via Py_SetRecursionLimit.
  int recursion_limit_ = 1000;  // CPython's default: Py_DEFAULT_RECURSION_LIMIT

  static thread_local Thread* current_thread_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace python
