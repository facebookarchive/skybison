#include "thread.h"

#include <cstdio>
#include <cstring>

#include "builtins-module.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "utils.h"
#include "visitor.h"

namespace python {

thread_local Thread* Thread::current_thread_ = nullptr;

Thread::Thread(word size)
    : handles_(new Handles()),
      size_(Utils::roundUp(size, kPointerSize)),
      initialFrame_(nullptr),
      next_(nullptr),
      runtime_(nullptr),
      exception_type_(NoneType::object()),
      exception_value_(NoneType::object()),
      exception_traceback_(NoneType::object()) {
  start_ = new byte[size];
  // Stack growns down in order to match machine convention
  end_ = start_ + size;
  pushInitialFrame();
}

Thread::~Thread() {
  delete handles_;
  delete[] start_;
}

void Thread::visitRoots(PointerVisitor* visitor) {
  visitStackRoots(visitor);
  handles()->visitPointers(visitor);
  visitor->visitPointer(&exception_type_);
  visitor->visitPointer(&exception_value_);
  visitor->visitPointer(&exception_traceback_);
}

void Thread::visitStackRoots(PointerVisitor* visitor) {
  auto address = reinterpret_cast<uword>(stackPtr());
  auto end = reinterpret_cast<uword>(end_);
  for (; address < end; address += kPointerSize) {
    visitor->visitPointer(reinterpret_cast<RawObject*>(address));
  }
}

Thread* Thread::currentThread() { return Thread::current_thread_; }

void Thread::setCurrentThread(Thread* thread) {
  Thread::current_thread_ = thread;
}

byte* Thread::stackPtr() {
  return reinterpret_cast<byte*>(currentFrame_->valueStackTop());
}

Frame* Thread::openAndLinkFrame(word num_args, word num_vars,
                                word stack_depth) {
  DCHECK(num_args >= 0, "must have 0 or more arguments");
  DCHECK(num_vars >= 0, "must have 0 or more locals");
  DCHECK(stack_depth >= 0, "stack depth cannot be negative");
  // HACK: Reserve one extra stack slot for the case where we need to unwrap a
  // bound method
  stack_depth += 1;

  checkStackOverflow(Frame::kSize + (num_vars + stack_depth) * kPointerSize);

  // Initialize the frame.
  byte* sp = stackPtr();
  word size = Frame::kSize + num_vars * kPointerSize;
  sp -= size;
  std::memset(sp, 0, size);
  auto frame = reinterpret_cast<Frame*>(sp);
  frame->setValueStackTop(reinterpret_cast<RawObject*>(frame));
  frame->setNumLocals(num_args + num_vars);

  // return a pointer to the base of the frame
  return linkFrame(frame);
}

Frame* Thread::linkFrame(Frame* frame) {
  frame->setPreviousFrame(currentFrame_);
  return currentFrame_ = frame;
}

void Thread::checkStackOverflow(word max_size) {
  // Check that there is sufficient space on the stack
  // TODO(T36407214): Grow stack
  byte* sp = stackPtr();
  CHECK(sp - max_size >= start_, "stack overflow");
}

Frame* Thread::pushNativeFrame(void* fn, word nargs) {
  // TODO(T36407290): native frames push arguments onto the stack when calling
  // back into the interpreter, but we can't statically know how much stack
  // space they will need. We may want to extend the api for such native calls
  // to include a declaration of how much space is needed. However, that's of
  // limited use right now since we can't detect an "overflow" of a frame
  // anyway.
  Frame* frame = openAndLinkFrame(nargs, 0, 0);
  frame->makeNativeFrame(runtime()->newIntFromCPtr(fn));
  return frame;
}

Frame* Thread::pushFrame(RawObject object) {
  HandleScope scope(this);
  Code code(&scope, object);
  auto frame =
      openAndLinkFrame(code->totalArgs(), code->totalVars(), code->stacksize());
  frame->setCode(*code);
  return frame;
}

Frame* Thread::pushModuleFunctionFrame(RawModule module, RawObject object) {
  HandleScope scope;
  Frame* result = pushFrame(object);
  Code code(&scope, object);
  Dict globals(&scope, module->dict());
  Object name(&scope, runtime()->symbols()->Builtins());
  Dict builtins(&scope, RawModule::cast(runtime()->findModule(name))->dict());
  result->setGlobals(*globals);
  result->setBuiltins(*builtins);
  result->setFastGlobals(
      runtime()->computeFastGlobals(code, globals, builtins));
  result->setImplicitGlobals(*globals);
  return result;
}

Frame* Thread::pushClassFunctionFrame(RawObject function, RawObject dict) {
  HandleScope scope;
  Code code(&scope, RawFunction::cast(function)->code());
  Frame* result = pushFrame(*code);
  Dict globals(&scope, RawFunction::cast(function)->globals());
  Object name(&scope, runtime()->symbols()->Builtins());
  Dict builtins(&scope, RawModule::cast(runtime()->findModule(name))->dict());
  result->setGlobals(*globals);
  result->setBuiltins(*builtins);
  result->setFastGlobals(
      runtime()->computeFastGlobals(code, globals, builtins));
  result->setImplicitGlobals(dict);

  word num_locals = code->nlocals();
  word num_cellvars = code->numCellvars();
  DCHECK(code->cell2arg() == 0, "class body cannot have cell2arg.");
  for (word i = 0; i < code->numCellvars(); i++) {
    result->setLocal(num_locals + i, runtime()->newValueCell());
  }

  // initialize free vars
  DCHECK(
      code->numFreevars() == 0 ||
          code->numFreevars() ==
              RawTuple::cast(RawFunction::cast(function)->closure())->length(),
      "Number of freevars is different than the closure.");
  for (word i = 0; i < code->numFreevars(); i++) {
    result->setLocal(
        num_locals + num_cellvars + i,
        RawTuple::cast(RawFunction::cast(function)->closure())->at(i));
  }
  return result;
}

void Thread::pushInitialFrame() {
  DCHECK(initialFrame_ == nullptr, "initial frame already pushed");

  byte* sp = end_ - Frame::kSize;
  CHECK(sp > start_, "no space for initial frame");
  initialFrame_ = reinterpret_cast<Frame*>(sp);
  initialFrame_->makeSentinel();
  initialFrame_->setValueStackTop(reinterpret_cast<RawObject*>(sp));

  currentFrame_ = initialFrame_;
}

void Thread::popFrame() {
  Frame* frame = currentFrame_;
  DCHECK(!frame->isSentinelFrame(), "cannot pop initial frame");
  currentFrame_ = frame->previousFrame();
}

RawObject Thread::run(RawObject code) {
  DCHECK(currentFrame_ == initialFrame_, "thread must be inactive");
  Frame* frame = pushFrame(code);
  return Interpreter::execute(this, frame);
}

RawObject Thread::runModuleFunction(RawModule module, RawObject object) {
  DCHECK(currentFrame_ == initialFrame_, "thread must be inactive");
  Frame* frame = pushModuleFunctionFrame(module, object);
  return Interpreter::execute(this, frame);
}

RawObject Thread::runClassFunction(RawObject function, RawObject dict) {
  Frame* frame = pushClassFunctionFrame(function, dict);
  return Interpreter::execute(this, frame);
}

// Convenience method for throwing a RuntimeError exception with an error
// message.
RawObject Thread::raiseRuntimeError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kRuntimeError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseRuntimeErrorWithCStr(const char* message) {
  return raiseRuntimeError(runtime()->newStrFromCStr(message));
}

// Convenience method for throwing a SystemError exception with an error
// message.
RawObject Thread::raiseSystemError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kSystemError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseSystemErrorWithCStr(const char* message) {
  return raiseSystemError(runtime()->newStrFromCStr(message));
}

// Convenience method for throwing a TypeError exception with an error message.
RawObject Thread::raiseTypeError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kTypeError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseTypeErrorWithCStr(const char* message) {
  return raiseTypeError(runtime()->newStrFromCStr(message));
}

// Convenience method for throwing a ValueError exception with an error message.
RawObject Thread::raiseValueError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kValueError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseValueErrorWithCStr(const char* message) {
  return raiseValueError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseAttributeError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kAttributeError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseAttributeErrorWithCStr(const char* message) {
  return raiseAttributeError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseKeyError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kKeyError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseKeyErrorWithCStr(const char* message) {
  return raiseKeyError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseOverflowError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kOverflowError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseOverflowErrorWithCStr(const char* message) {
  return raiseOverflowError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseIndexError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kIndexError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseIndexErrorWithCStr(const char* message) {
  return raiseIndexError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseStopIteration(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kStopIteration));
  setExceptionValue(value);
  return Error::object();
}

bool Thread::hasPendingException() { return !exception_type_->isNoneType(); }

bool Thread::hasPendingStopIteration() {
  return exception_type_->isType() &&
         RawType::cast(exception_type_).builtinBase() ==
             LayoutId::kStopIteration;
}

RawObject Thread::raiseZeroDivisionError(RawObject value) {
  setExceptionType(runtime()->typeAt(LayoutId::kZeroDivisionError));
  setExceptionValue(value);
  return Error::object();
}

RawObject Thread::raiseZeroDivisionErrorWithCStr(const char* message) {
  return raiseZeroDivisionError(runtime()->newStrFromCStr(message));
}

void Thread::ignorePendingException() {
  if (!hasPendingException()) {
    return;
  }
  *builtinStderr << "ignore pending exception";
  if (exceptionValue()->isStr()) {
    RawStr message = RawStr::cast(exceptionValue());
    word len = message->length();
    byte* buffer = new byte[len + 1];
    message->copyTo(buffer, len);
    buffer[len] = 0;
    *builtinStderr << ": " << buffer;
    delete[] buffer;
  }
  *builtinStderr << "\n";
  clearPendingException();
  Utils::printTraceback(builtinStderr);
}

void Thread::clearPendingException() {
  setExceptionType(NoneType::object());
  setExceptionValue(NoneType::object());
  setExceptionTraceback(NoneType::object());
}

void Thread::abortOnPendingException() {
  if (!hasPendingException()) {
    return;
  }
  std::cerr << "aborting due to pending exception";
  if (exceptionValue()->isStr()) {
    RawStr message = RawStr::cast(exceptionValue());
    word len = message->length();
    byte* buffer = new byte[len + 1];
    message->copyTo(buffer, len);
    buffer[len] = 0;
    std::cerr << ": " << buffer;
    delete[] buffer;
  }
  std::cerr << "\n";
  Utils::printTraceback();

  std::abort();
}

void Thread::visitFrames(FrameVisitor* visitor) {
  Frame* frame = currentFrame();
  while (!frame->isSentinelFrame()) {
    visitor->visit(frame);
    frame = frame->previousFrame();
  }
}

}  // namespace python
