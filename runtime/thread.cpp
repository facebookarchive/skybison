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
      pending_exception_(None::object()) {
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
  visitor->visitPointer(&pending_exception_);
}

void Thread::visitStackRoots(PointerVisitor* visitor) {
  auto address = reinterpret_cast<uword>(stackPtr());
  auto end = reinterpret_cast<uword>(end_);
  for (; address < end; address += kPointerSize) {
    visitor->visitPointer(reinterpret_cast<Object**>(address));
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

  // Check that there is sufficient space on the stack
  // TODO: Grow stack
  byte* sp = reinterpret_cast<byte*>(currentFrame_->valueStackTop());
  word max_size = Frame::kSize + (num_vars + stack_depth) * kPointerSize;
  CHECK(sp - max_size >= start_, "stack overflow");

  // Initialize the frame.
  word size = Frame::kSize + num_vars * kPointerSize;
  sp -= size;
  std::memset(sp, 0, size);
  auto frame = reinterpret_cast<Frame*>(sp);
  frame->setPreviousFrame(currentFrame_);
  frame->setValueStackTop(reinterpret_cast<Object**>(frame));
  frame->setNumLocals(num_args + num_vars);

  currentFrame_ = frame;

  // return a pointer to the base of the frame
  return frame;
}

Frame* Thread::pushNativeFrame(void* fn, word nargs) {
  // TODO: native frames push arguments onto the stack when calling back into
  // the interpreter, but we can't statically know how much stack space they
  // will need. We may want to extend the api for such native calls to include
  // a declaration of how much space is needed. However, that's of limited use
  // right now since we can't detect an "overflow" of a frame anyway.
  Frame* frame = openAndLinkFrame(nargs, 0, 0);
  frame->makeNativeFrame(runtime()->newIntegerFromCPointer(fn));
  return frame;
}

Frame* Thread::pushFrame(Object* object) {
  HandleScope scope(this);
  Handle<Code> code(&scope, object);
  word num_vars = code->nlocals() + code->numCellvars() + code->numFreevars();
  auto frame = openAndLinkFrame(code->totalArgs(), num_vars, code->stacksize());
  frame->setCode(*code);
  return frame;
}

Frame* Thread::pushModuleFunctionFrame(Module* module, Object* object) {
  HandleScope scope;
  Frame* result = pushFrame(object);
  Handle<Code> code(&scope, object);
  Handle<Dictionary> globals(&scope, module->dictionary());
  Handle<Object> name(&scope, runtime()->symbols()->Builtins());
  Handle<Dictionary> builtins(
      &scope, Module::cast(runtime()->findModule(name))->dictionary());
  result->setGlobals(*globals);
  result->setBuiltins(*builtins);
  result->setFastGlobals(
      runtime()->computeFastGlobals(code, globals, builtins));
  result->setImplicitGlobals(*globals);
  return result;
}

Frame* Thread::pushClassFunctionFrame(Object* function, Object* dictionary) {
  HandleScope scope;
  Handle<Code> code(&scope, Function::cast(function)->code());
  Frame* result = pushFrame(*code);
  Handle<Dictionary> globals(&scope, Function::cast(function)->globals());
  Handle<Object> name(&scope, runtime()->symbols()->Builtins());
  Handle<Dictionary> builtins(
      &scope, Module::cast(runtime()->findModule(name))->dictionary());
  result->setGlobals(*globals);
  result->setBuiltins(*builtins);
  result->setFastGlobals(
      runtime()->computeFastGlobals(code, globals, builtins));
  result->setImplicitGlobals(dictionary);

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
              ObjectArray::cast(Function::cast(function)->closure())->length(),
      "Number of freevars is different than the closure.");
  for (word i = 0; i < code->numFreevars(); i++) {
    result->setLocal(
        num_locals + num_cellvars + i,
        ObjectArray::cast(Function::cast(function)->closure())->at(i));
  }
  return result;
}

void Thread::pushInitialFrame() {
  DCHECK(initialFrame_ == nullptr, "initial frame already pushed");

  byte* sp = end_ - Frame::kSize;
  CHECK(sp > start_, "no space for initial frame");
  initialFrame_ = reinterpret_cast<Frame*>(sp);
  initialFrame_->makeSentinel();
  initialFrame_->setValueStackTop(reinterpret_cast<Object**>(sp));

  currentFrame_ = initialFrame_;
}

void Thread::popFrame() {
  Frame* frame = currentFrame_;
  DCHECK(!frame->isSentinelFrame(), "cannot pop initial frame");
  currentFrame_ = frame->previousFrame();
}

Object* Thread::run(Object* object) {
  DCHECK(currentFrame_ == initialFrame_, "thread must be inactive");
  Frame* frame = pushFrame(object);
  return Interpreter::execute(this, frame);
}

Object* Thread::runModuleFunction(Module* module, Object* object) {
  DCHECK(currentFrame_ == initialFrame_, "thread must be inactive");
  Frame* frame = pushModuleFunctionFrame(module, object);
  return Interpreter::execute(this, frame);
}

Object* Thread::runClassFunction(Object* function, Object* dictionary) {
  Frame* frame = pushClassFunctionFrame(function, dictionary);
  return Interpreter::execute(this, frame);
}

// Convenience method for throwing a RuntimeError exception with an error
// message.
Object* Thread::throwRuntimeError(Object* value) {
  // TODO: instantiate RuntimeError object.
  pending_exception_ = value;
  return Error::object();
}

Object* Thread::throwRuntimeErrorFromCString(const char* message) {
  return throwRuntimeError(runtime()->newStringFromCString(message));
}

// Convenience method for throwing a TypeError exception with an error message.
Object* Thread::throwTypeError(Object* value) {
  // TODO: instantiate TypeError object.
  pending_exception_ = value;
  return Error::object();
}

Object* Thread::throwTypeErrorFromCString(const char* message) {
  return throwTypeError(runtime()->newStringFromCString(message));
}

// Convenience method for throwing a ValueError exception with an error message.
Object* Thread::throwValueError(Object* value) {
  // TODO: instantiate ValueError object.
  pending_exception_ = value;
  return Error::object();
}

Object* Thread::throwValueErrorFromCString(const char* message) {
  // TODO: instantiate ValueError object.
  return throwValueError(runtime()->newStringFromCString(message));
}

Object* Thread::throwAttributeError(Object* value) {
  // TODO: instantiate an AttributeError object.
  pending_exception_ = value;
  return Error::object();
}

Object* Thread::throwAttributeErrorFromCString(const char* message) {
  // TODO: instantiate an AttributeError object.
  return throwAttributeError(runtime()->newStringFromCString(message));
}

Object* Thread::throwKeyError(Object* value) {
  // TODO(jeethu): instantiate an KeyError object.
  pending_exception_ = value;
  return Error::object();
}

Object* Thread::throwKeyErrorFromCString(const char* message) {
  // TODO(jeethu): instantiate a KeyError object.
  return throwKeyError(runtime()->newStringFromCString(message));
}

Object* Thread::throwIndexError(Object* value) {
  // TODO(jeethu): instantiate an IndexError object.
  pending_exception_ = value;
  return Error::object();
}

Object* Thread::throwIndexErrorFromCString(const char* message) {
  return throwIndexError(runtime()->newStringFromCString(message));
}

void Thread::ignorePendingException() {
  HandleScope scope(this);
  Handle<Object> pending_exception(&scope, pendingException());
  if (pending_exception->isNone()) {
    return;
  }

  *builtinStderr << "ignore pending exception";
  if (pending_exception->isString()) {
    String* message = String::cast(*pending_exception);
    word len = message->length();
    byte* buffer = new byte[len + 1];
    message->copyTo(buffer, len);
    buffer[len] = 0;
    *builtinStderr << ": " << buffer;
    delete[] buffer;
  }
  *builtinStderr << "\n";
  Utils::printTraceback();
}

void Thread::abortOnPendingException() {
  HandleScope scope(this);
  Handle<Object> pending_exception(&scope, pendingException());
  if (pending_exception->isNone()) {
    return;
  }

  std::cerr << "aborting due to pending exception";
  if (pending_exception->isString()) {
    String* message = String::cast(*pending_exception);
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

Object* Thread::pendingException() { return pending_exception_; }

}  // namespace python
