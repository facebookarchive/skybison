#pragma once

#include "globals.h"
#include "handles-decl.h"
#include "objects.h"
#include "os.h"
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

  enum InterruptKind {
    kSignal = 1 << 0,
    kReinitInterpreter = 1 << 1,
    kProfile = 1 << 2,
  };

  explicit Thread(word size);
  ~Thread();

  void begin();

  static Thread* current();
  static void setCurrentThread(Thread* thread);

  void linkFrame(Frame* frame);
  // Private method. Call pushCallFrame() or pushNativeFrame() isntead.
  Frame* pushCallFrame(RawFunction function);
  Frame* pushNativeFrame(word nargs);
  Frame* pushGeneratorFrame(const GeneratorFrame& generator_frame);

  Frame* popFrame();
  Frame* popFrameToGeneratorFrame(const GeneratorFrame& generator_frame);

  RawObject* stackPointer() { return stack_pointer_; }
  void setStackPointer(RawObject* stack_pointer) {
    stack_pointer_ = stack_pointer;
  }

  // Begin of the value stack of the current frame.
  RawObject* valueStackBase();

  // Returns the number of items on the value stack of the current frame.
  word valueStackSize();

  // Pop n items off the stack.
  void stackDrop(word count);

  // Insert value at offset on the stack.
  void stackInsertAt(word offset, RawObject value);

  // Return the object at offset from the top of the value stack (e.g. peek(0)
  // returns the top of the stack)
  RawObject stackPeek(word offset);

  // Pop the top value off the stack and return it.
  RawObject stackPop();

  // Push value on the stack.
  void stackPush(RawObject value);

  // Remove value at offset on the stack.
  void stackRemoveAt(word offset);

  // Set value at offset on the stack.
  void stackSetAt(word offset, RawObject value);

  // Set the top value of the stack.
  void stackSetTop(RawObject value);

  // Return the top value of the stack.
  RawObject stackTop();

  // Runs a code object on the current thread.
  NODISCARD RawObject exec(const Code& code, const Module& module,
                           const Object& implicit_globals);

  NODISCARD RawObject callFunctionWithImplicitGlobals(
      const Function& function, const Object& implicit_globals);

  Thread* next() { return next_; }
  void setNext(Thread* next) { next_ = next; }

  Thread* prev() { return prev_; }
  void setPrev(Thread* prev) { prev_ = prev; }

  Handles* handles() { return &handles_; }

  Runtime* runtime() { return runtime_; }

  Frame* currentFrame() { return current_frame_; }

  RawObject heapFrameAtDepth(word depth);

  using InterpreterFunc = RawObject (*)(Thread*);

  InterpreterFunc interpreterFunc() { return interpreter_func_; }
  void setInterpreterFunc(InterpreterFunc func) { interpreter_func_ = func; }

  void* interpreterData() { return interpreter_data_; }
  void setInterpreterData(void* data) { interpreter_data_ = data; }

  void clearInterrupt(InterruptKind kind);
  void interrupt(InterruptKind kind);

  bool isMainThread();

  void visitRoots(PointerVisitor* visitor);

  void visitStackRoots(PointerVisitor* visitor);

  void setRuntime(Runtime* runtime) { runtime_ = runtime; }

  // Calls out to the interpreter to lookup and call a method on the receiver
  // with the given argument(s). Returns Error<NotFound> if the method can't be
  // found, or the result of the call otheriwse (which may be Error<Exception>).
  NODISCARD RawObject invokeMethod1(const Object& receiver, SymbolId selector);
  NODISCARD RawObject invokeMethod2(const Object& receiver, SymbolId selector,
                                    const Object& arg1);
  NODISCARD RawObject invokeMethod3(const Object& receiver, SymbolId selector,
                                    const Object& arg1, const Object& arg2);

  // Looks up a method on a type and invokes it with the given receiver and
  // argument(s). Returns Error<NotFound> if the method can't be found, or the
  // result of the call otheriwse (which may be Error<Exception>).
  // ex: str.foo(receiver, arg1, ...)
  NODISCARD RawObject invokeMethodStatic1(LayoutId type, SymbolId method_name,
                                          const Object& receiver);
  NODISCARD RawObject invokeMethodStatic2(LayoutId type, SymbolId method_name,
                                          const Object& receiver,
                                          const Object& arg1);
  NODISCARD RawObject invokeMethodStatic3(LayoutId type, SymbolId method_name,
                                          const Object& receiver,
                                          const Object& arg1,
                                          const Object& arg2);
  NODISCARD RawObject invokeMethodStatic4(LayoutId type, SymbolId method_name,
                                          const Object& receiver,
                                          const Object& arg1,
                                          const Object& arg2,
                                          const Object& arg3);

  // Calls out to the interpreter to lookup and call a function with the given
  // argument(s). Returns Error<NotFound> if the function can't be found, or the
  // result of the call otherwise (which may be Error<Exception>).
  NODISCARD RawObject invokeFunction0(SymbolId module, SymbolId name);
  NODISCARD RawObject invokeFunction1(SymbolId module, SymbolId name,
                                      const Object& arg1);
  NODISCARD RawObject invokeFunction2(SymbolId module, SymbolId name,
                                      const Object& arg1, const Object& arg2);
  NODISCARD RawObject invokeFunction3(SymbolId module, SymbolId name,
                                      const Object& arg1, const Object& arg2,
                                      const Object& arg3);
  NODISCARD RawObject invokeFunction4(SymbolId module, SymbolId name,
                                      const Object& arg1, const Object& arg2,
                                      const Object& arg3, const Object& arg4);
  NODISCARD RawObject invokeFunction5(SymbolId module, SymbolId name,
                                      const Object& arg1, const Object& arg2,
                                      const Object& arg3, const Object& arg4,
                                      const Object& arg5);
  NODISCARD RawObject invokeFunction6(SymbolId module, SymbolId name,
                                      const Object& arg1, const Object& arg2,
                                      const Object& arg3, const Object& arg4,
                                      const Object& arg5, const Object& arg6);

  // Raises an exception with the given type and returns an Error that must be
  // returned up the stack by the caller.
  RawObject raise(LayoutId type, RawObject value);
  RawObject raiseWithType(RawObject type, RawObject value);
  RawObject raiseWithFmt(LayoutId type, const char* fmt, ...);

  // Raises a new exception, attaching the pending exception (if any) to the
  // new exception's __cause__ and __context__ attributes.
  RawObject raiseWithFmtChainingPendingAsCause(LayoutId type, const char* fmt,
                                               ...);

  // Raises a TypeError exception for PyErr_BadArgument.
  void raiseBadArgument();

  // Raises a SystemError exception for PyErr_BadInternalCall.
  void raiseBadInternalCall();

  // Raises a MemoryError exception and returns an Error that must be returned
  // up the stack by the caller.
  RawObject raiseMemoryError();

  // Raises an OSError exception generated from the value of errno.
  RawObject raiseOSErrorFromErrno(int errno_value);

  RawObject raiseFromErrnoWithFilenames(const Object& type, int errno_value,
                                        const Object& filename0,
                                        const Object& filename1);

  // Raises a TypeError exception of the form '<method> requires a <type(obj)>
  // object but received a <expected_type>' and returns an Error object that
  // must be returned up the stack by the caller.
  RawObject raiseRequiresType(const Object& obj, SymbolId expected_type);

  // Raise a StopAsyncIteration exception, indicating an asynchronous generator
  // has returned. Note, unlike generators and coroutines, asynchoronous
  // generators cannot have non-None return values.
  RawObject raiseStopAsyncIteration();

  // Raise a StopIteration exception with no value attribute set. This can
  // indicate a a return value from a generator with value None, but can also
  // mean the generator did not run, e.g. it is exhausted.
  RawObject raiseStopIteration();

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

  // Sets the type, value, or traceback of the caught exception.
  void setCaughtExceptionType(RawObject type);
  void setCaughtExceptionValue(RawObject value);
  void setCaughtExceptionTraceback(RawObject traceback);

  // Gets or sets the current caught ExceptionState. See also
  // topmostCaughtExceptionState().
  RawObject caughtExceptionState();
  void setCaughtExceptionState(RawObject state);

  // Searches up the stack for the nearest caught exception. Returns None if
  // there are no caught exceptions being handled, or an ExceptionState.
  RawObject topmostCaughtExceptionState();

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

  // Get/set the thread-global callbacks for asynchronous generators.
  RawObject asyncgenHooksFirstIter() { return asyncgen_hooks_first_iter_; }
  RawObject asyncgenHooksFinalizer() { return asyncgen_hooks_finalizer_; }

  void setAsyncgenHooksFirstIter(RawObject first_iter) {
    asyncgen_hooks_first_iter_ = first_iter;
  }
  void setAsyncgenHooksFinalizer(RawObject finalizer) {
    asyncgen_hooks_finalizer_ = finalizer;
  }

  // Get/set the current thread-global _contextvars.Context
  RawObject contextvarsContext() { return contextvars_context_; }
  void setContextvarsContext(RawObject context) {
    contextvars_context_ = context;
  }

  void countOpcodes(word count) { opcode_count_ += count; }
  // Returns number of opcodes executed in this thread. This is only guaranteed
  // to be accurate when `Runtime::supportProfiling()` is enabled.
  word opcodeCount() { return opcode_count_; }

  bool profilingEnabled();
  void enableProfiling();
  void disableProfiling();

  RawObject profilingData() { return profiling_data_; }
  void setProfilingData(RawObject data) { profiling_data_ = data; }

  word strOffset(const Str& str, word index);

  bool wouldStackOverflow(word size);
  bool handleInterrupt(word size);

  static int currentFrameOffset() { return offsetof(Thread, current_frame_); }

  static int interpreterDataOffset() {
    return offsetof(Thread, interpreter_data_);
  }

  static int opcodeCountOffset() { return offsetof(Thread, opcode_count_); }

  static int runtimeOffset() { return offsetof(Thread, runtime_); }

  static int limitOffset() { return offsetof(Thread, limit_); }

  static int stackPointerOffset() { return offsetof(Thread, stack_pointer_); }

 private:
  Frame* pushInitialFrame();
  Frame* openAndLinkFrame(word size, word locals_offset);

  void handleInterruptWithFrame();
  Frame* handleInterruptPushCallFrame(RawFunction function, word max_stack_size,
                                      word initial_stack_size,
                                      word locals_offset);
  Frame* handleInterruptPushGeneratorFrame(
      const GeneratorFrame& generator_frame, word size);
  Frame* handleInterruptPushNativeFrame(word locals_offset);
  Frame* pushCallFrameImpl(RawFunction function, word stack_size,
                           word locals_offset);
  Frame* pushGeneratorFrameImpl(const GeneratorFrame& generator_frame,
                                word size);
  Frame* pushNativeFrameImpl(word locals_offset);

  Handles handles_;

  byte* start_;  // base address of the stack
  byte* end_;    // exclusive limit of the stack
  byte* limit_;  // current limit of the stack

  RawObject* stack_pointer_;

  // Has the runtime requested a thread interruption? (e.g. signals, GC)
  uint8_t interrupt_flags_ = 0;

  // Number of opcodes executed in the thread while opcode counting was enabled.
  word opcode_count_ = 0;

  // current_frame_ always points to the top-most frame on the stack.
  Frame* current_frame_;
  Thread* next_ = nullptr;
  Thread* prev_ = nullptr;
  Runtime* runtime_ = nullptr;
  InterpreterFunc interpreter_func_ = uninitializedInterpreterFunc;
  void* interpreter_data_ = nullptr;

  // State of the pending exception.
  RawObject pending_exc_type_ = RawNoneType::object();
  RawObject pending_exc_value_ = RawNoneType::object();
  RawObject pending_exc_traceback_ = RawNoneType::object();

  // Stack of ExceptionStates for the current caught exception. Generators push
  // their private state onto this stack before resuming, and pop it after
  // suspending.
  RawObject caught_exc_stack_ = RawNoneType::object();

  RawObject api_repr_list_ = RawNoneType::object();

  RawObject asyncgen_hooks_first_iter_ = RawNoneType::object();
  RawObject asyncgen_hooks_finalizer_ = RawNoneType::object();
  RawObject contextvars_context_ = RawNoneType::object();

  RawObject profiling_data_ = RawNoneType::object();

  RawObject str_offset_str_ = RawNoneType::object();
  word str_offset_index_;
  word str_offset_offset_;

  // C-API current recursion depth used via _PyThreadState_GetRecursionDepth
  int recursion_depth_ = 0;

  // C-API recursion limit as set via Py_SetRecursionLimit.
  int recursion_limit_ = 1000;  // CPython's default: Py_DEFAULT_RECURSION_LIMIT

  static thread_local Thread* current_thread_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

inline RawObject* Thread::valueStackBase() {
  return reinterpret_cast<RawObject*>(current_frame_);
}

inline word Thread::valueStackSize() {
  return valueStackBase() - stack_pointer_;
}

inline void Thread::stackDrop(word count) {
  DCHECK(stack_pointer_ + count <= valueStackBase(), "stack underflow");
  stack_pointer_ += count;
}

inline void Thread::stackInsertAt(word offset, RawObject value) {
  RawObject* sp = stack_pointer_ - 1;
  DCHECK(sp + offset < valueStackBase(), "stack underflow");
  for (word i = 0; i < offset; i++) {
    sp[i] = sp[i + 1];
  }
  sp[offset] = value;
  stack_pointer_ = sp;
}

inline RawObject Thread::stackPeek(word offset) {
  DCHECK(stack_pointer_ + offset < valueStackBase(), "stack underflow");
  return *(stack_pointer_ + offset);
}

inline RawObject Thread::stackPop() {
  DCHECK(stack_pointer_ + 1 <= valueStackBase(), "stack underflow");
  return *(stack_pointer_++);
}

inline void Thread::stackPush(RawObject value) { *(--stack_pointer_) = value; }

inline void Thread::stackRemoveAt(word offset) {
  DCHECK(stack_pointer_ + offset < valueStackBase(), "stack underflow");
  RawObject* sp = stack_pointer_;
  for (word i = offset; i >= 1; i--) {
    sp[i] = sp[i - 1];
  }
  stack_pointer_ = sp + 1;
}

inline void Thread::stackSetAt(word offset, RawObject value) {
  DCHECK(stack_pointer_ + offset < valueStackBase(), "stack underflow");
  *(stack_pointer_ + offset) = value;
}

inline void Thread::stackSetTop(RawObject value) {
  DCHECK(stack_pointer_ < valueStackBase(), "stack underflow");
  *stack_pointer_ = value;
}

inline RawObject Thread::stackTop() {
  DCHECK(stack_pointer_ < valueStackBase(), "stack underflow");
  return *stack_pointer_;
}

inline bool Thread::profilingEnabled() { return interrupt_flags_ & kProfile; }

inline bool Thread::wouldStackOverflow(word size) {
  // Check that there is sufficient space on the stack
  return reinterpret_cast<byte*>(stack_pointer_) - size < limit_;
}

}  // namespace py
