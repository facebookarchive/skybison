#pragma once

#include "globals.h"
#include "handles-decl.h"
#include "objects.h"
#include "symbols.h"
#include "vector.h"

namespace py {

class Frame;
class FrameVisitor;
class HandleScope;
class PointerVisitor;
class Runtime;

class Handles {
 public:
  Handles() = default;

  Object* head() const { return head_; }

  Object* push(Object* new_head) {
    Object* old_head = head_;
    head_ = new_head;
    return old_head;
  }

  void pop(Object* new_head) { head_ = new_head; }

  void visitPointers(PointerVisitor* visitor);

  DISALLOW_COPY_AND_ASSIGN(Handles);

 private:
  Object* head_ = nullptr;
};

RawObject uninitializedInterpreterFunc(Thread*);

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
  Frame* pushGeneratorFrame(const GeneratorFrame& generator_frame);
  Frame* pushClassFunctionFrame(const Function& function);

  Frame* popFrame();
  Frame* popFrameToGeneratorFrame(const GeneratorFrame& generator_frame);

  // Runs a code object on the current thread.
  RawObject exec(const Code& code, const Module& module,
                 const Object& implicit_globals);

  // Runs a class body function on the current thread.
  NODISCARD RawObject runClassFunction(const Function& function,
                                       const Dict& dict);

  Thread* next() { return next_; }

  Handles* handles() { return &handles_; }

  Runtime* runtime() { return runtime_; }

  Frame* currentFrame() { return current_frame_; }

  RawObject heapFrameAtDepth(word depth);

  using InterpreterFunc = RawObject (*)(Thread*);

  InterpreterFunc interpreterFunc() { return interpreter_func_; }
  void setInterpreterFunc(InterpreterFunc func) { interpreter_func_ = func; }

  // The stack pointer is computed by taking the value stack top of the current
  // frame.
  byte* stackPtr();

  void interrupt() {
    limit_ = stackPtr();
    is_interrupted_ = true;
  }

  bool isInterrupted() { return is_interrupted_; }

  bool isMainThread();

  void visitRoots(PointerVisitor* visitor);

  void visitStackRoots(PointerVisitor* visitor);

  void setRuntime(Runtime* runtime) { runtime_ = runtime; }

  // Calls out to the interpreter to lookup and call a method on the receiver
  // with the given argument(s). Returns Error<NotFound> if the method can't be
  // found, or the result of the call otheriwse (which may be Error<Exception>).
  RawObject invokeMethod1(const Object& receiver, SymbolId selector);
  RawObject invokeMethod2(const Object& receiver, SymbolId selector,
                          const Object& arg1);
  RawObject invokeMethod3(const Object& receiver, SymbolId selector,
                          const Object& arg1, const Object& arg2);

  // Looks up a method on a type and invokes it with the given receiver and
  // argument(s). Returns Error<NotFound> if the method can't be found, or the
  // result of the call otheriwse (which may be Error<Exception>).
  // ex: str.foo(receiver, arg1, ...)
  RawObject invokeMethodStatic1(LayoutId type, SymbolId method_name,
                                const Object& receiver);
  RawObject invokeMethodStatic2(LayoutId type, SymbolId method_name,
                                const Object& receiver, const Object& arg1);
  RawObject invokeMethodStatic3(LayoutId type, SymbolId method_name,
                                const Object& receiver, const Object& arg1,
                                const Object& arg2);
  RawObject invokeMethodStatic4(LayoutId type, SymbolId method_name,
                                const Object& receiver, const Object& arg1,
                                const Object& arg2, const Object& arg3);

  // Calls out to the interpreter to lookup and call a function with the given
  // argument(s). Returns Error<NotFound> if the function can't be found, or the
  // result of the call otherwise (which may be Error<Exception>).
  RawObject invokeFunction0(SymbolId module, SymbolId name);
  RawObject invokeFunction1(SymbolId module, SymbolId name, const Object& arg1);
  RawObject invokeFunction2(SymbolId module, SymbolId name, const Object& arg1,
                            const Object& arg2);
  RawObject invokeFunction3(SymbolId module, SymbolId name, const Object& arg1,
                            const Object& arg2, const Object& arg3);
  RawObject invokeFunction4(SymbolId module, SymbolId name, const Object& arg1,
                            const Object& arg2, const Object& arg3,
                            const Object& arg4);
  RawObject invokeFunction5(SymbolId module, SymbolId name, const Object& arg1,
                            const Object& arg2, const Object& arg3,
                            const Object& arg4, const Object& arg5);
  RawObject invokeFunction6(SymbolId module, SymbolId name, const Object& arg1,
                            const Object& arg2, const Object& arg3,
                            const Object& arg4, const Object& arg5,
                            const Object& arg6);

  // Raises an exception with the given type and returns an Error that must be
  // returned up the stack by the caller.
  RawObject raise(LayoutId type, RawObject value);
  RawObject raiseWithType(RawObject type, RawObject value);
  RawObject raiseWithFmt(LayoutId type, const char* fmt, ...);

  // Raises a TypeError exception for PyErr_BadArgument.
  void raiseBadArgument();

  // Raises a SystemError exception for PyErr_BadInternalCall.
  void raiseBadInternalCall();

  // Raises a MemoryError exception and returns an Error that must be returned
  // up the stack by the caller.
  RawObject raiseMemoryError();

  // Raises an OSError exception generated from the value of errno.
  RawObject raiseOSErrorFromErrno(int errno_value);

  // Raises a TypeError exception of the form '<method> requires a <type(obj)>
  // object but received a <expected_type>' and returns an Error object that
  // must be returned up the stack by the caller.
  RawObject raiseRequiresType(const Object& obj, SymbolId expected_type);

  // Raise a StopAsyncIteration exception, indicating an asynchronous generator
  // has returned. Note, unlike generators and coroutines, asynchoronous
  // generators cannot have non-None return values.
  RawObject raiseStopAsyncIteration();

  // Raise a StopIteration exception with the value attribute set. This is
  // typically used to transport a return value from a generator or coroutine.
  RawObject raiseStopIterationWithValue(const Object& value);

  // Raises a TypeError exception and returns an Error object that must be
  // returned up the stack by the caller.
  RawObject raiseUnsupportedBinaryOperation(const Object& left,
                                            const Object& right,
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
  RawObject chainExceptionContext(const Type& type, const Object& value);

  // Returns true if and only if obj is not an Error and there is no pending
  // exception, or obj is an Error<Exception> and there is a pending exception.
  // Mostly used in assertions around call boundaries.
  bool isErrorValueOk(RawObject obj);

  // Walk all the frames on the stack starting with the top-most frame
  void visitFrames(FrameVisitor* visitor);

  int recursionDepth() { return recursion_depth_; }

  int recursionEnter() {
    DCHECK(recursion_depth_ <= recursion_limit_,
           "recursion depth can't be more than the recursion limit");
    return recursion_depth_++;
  }

  void recursionLeave() {
    DCHECK(recursion_depth_ > 0, "recursion depth can't be less than 0");
    recursion_depth_--;
  }

  int recursionLimit() { return recursion_limit_; }
  // TODO(T62600497): Enforce the recursion limit
  void setRecursionLimit(int limit) { recursion_limit_ = limit; }

  RawObject reprEnter(const Object& obj);
  void reprLeave(const Object& obj);

  // Get/set the current thread-global _contextvars.Context
  RawObject contextvarsContext() { return contextvars_context_; }
  void setContextvarsContext(RawObject context) {
    contextvars_context_ = context;
  }

  // Returns thread ID.
  word id() {
    // Currently we only ever have a single thread.
    return 0;
  }

  static int currentFrameOffset() { return offsetof(Thread, current_frame_); }

  static int runtimeOffset() { return offsetof(Thread, runtime_); }

  static int startOffset() { return offsetof(Thread, start_); }

 private:
  Frame* pushInitialFrame();
  Frame* openAndLinkFrame(word size, word total_locals);

  Handles handles_;

  word size_;
  byte* start_;  // base address of the stack
  byte* end_;    // exclusive limit of the stack
  byte* limit_;  // current limit of the stack

  // Has the runtime requested a thread interruption? (e.g. signals, GC)
  bool is_interrupted_;

  // current_frame_ always points to the top-most frame on the stack.
  Frame* current_frame_;
  Thread* next_;
  Runtime* runtime_;
  InterpreterFunc interpreter_func_ = uninitializedInterpreterFunc;

  // State of the pending exception.
  RawObject pending_exc_type_;
  RawObject pending_exc_value_;
  RawObject pending_exc_traceback_;

  // Stack of ExceptionStates for the current caught exception. Generators push
  // their private state onto this stack before resuming, and pop it after
  // suspending.
  RawObject caught_exc_stack_;

  RawObject api_repr_list_;

  RawObject contextvars_context_;

  // C-API current recursion depth used via _PyThreadState_GetRecursionDepth
  int recursion_depth_ = 0;

  // C-API recursion limit as set via Py_SetRecursionLimit.
  int recursion_limit_ = 1000;  // CPython's default: Py_DEFAULT_RECURSION_LIMIT

  static thread_local Thread* current_thread_;

  bool wouldStackOverflow(word max_size);

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace py
