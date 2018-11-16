#include "thread.h"

#include <cstdio>
#include <cstring>

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "utils.h"
#include "visitor.h"

namespace python {

thread_local Thread* current_thread_ = nullptr;

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

Thread* Thread::currentThread() {
  return current_thread_;
}

void Thread::setCurrentThread(Thread* thread) {
  current_thread_ = thread;
}

byte* Thread::stackPtr() {
  return reinterpret_cast<byte*>(currentFrame_->valueStackTop());
}

Frame* Thread::openAndLinkFrame(word numArgs, word numVars, word stackDepth) {
  CHECK(numArgs >= 0, "must have 0 or more arguments");
  CHECK(numVars >= 0, "must have 0 or more locals");
  CHECK(stackDepth >= 0, "stack depth cannot be negative");
  // HACK: Reserve one extra stack slot for the case where we need to unwrap a
  // bound method
  stackDepth += 1;

  // Check that there is sufficient space on the stack
  // TODO: Grow stack
  byte* sp = reinterpret_cast<byte*>(currentFrame_->valueStackTop());
  word maxSize = Frame::kSize + (numVars + stackDepth) * kPointerSize;
  CHECK(sp - maxSize >= start_, "stack overflow");

  // Initialize the frame.
  word size = Frame::kSize + numVars * kPointerSize;
  sp -= size;
  std::memset(sp, 0, size);
  auto frame = reinterpret_cast<Frame*>(sp);
  frame->setPreviousFrame(currentFrame_);
  frame->setValueStackTop(reinterpret_cast<Object**>(frame));
  frame->setNumLocals(numArgs + numVars);

  currentFrame_ = frame;

  // return a pointer to the base of the frame
  return frame;
}

Frame* Thread::pushFrame(Object* object) {
  HandleScope scope(this);
  Handle<Code> code(&scope, object);
  word numVars = code->nlocals() + code->numCellvars() + code->numFreevars();
  auto frame = openAndLinkFrame(code->argcount(), numVars, code->stacksize());
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
  return result;
}

void Thread::pushInitialFrame() {
  CHECK(initialFrame_ == nullptr, "inital frame already pushed");

  byte* sp = end_ - Frame::kSize;
  CHECK(sp > start_, "no space for initial frame");
  initialFrame_ = reinterpret_cast<Frame*>(sp);
  initialFrame_->makeSentinel();
  initialFrame_->setValueStackTop(reinterpret_cast<Object**>(sp));

  currentFrame_ = initialFrame_;
}

void Thread::popFrame() {
  Frame* frame = currentFrame_;
  CHECK(!frame->isSentinelFrame(), "cannot pop initial frame");
  currentFrame_ = frame->previousFrame();
}

Object* Thread::run(Object* object) {
  CHECK(currentFrame_ == initialFrame_, "thread must be inactive");
  Frame* frame = pushFrame(object);
  return Interpreter::execute(this, frame);
}

Object* Thread::runModuleFunction(Module* module, Object* object) {
  CHECK(currentFrame_ == initialFrame_, "thread must be inactive");
  Frame* frame = pushModuleFunctionFrame(module, object);
  return Interpreter::execute(this, frame);
}

Object* Thread::runClassFunction(Object* function, Object* dictionary) {
  Frame* frame = pushClassFunctionFrame(function, dictionary);
  return Interpreter::execute(this, frame);
}

// Convenience method for throwing a RuntimeError exception with an error
// message.
void Thread::throwRuntimeError(String* message) {
  // TODO: instantiate RuntimeError object.
  pending_exception_ = message;
}

Object* Thread::throwRuntimeErrorFromCString(const char* message) {
  throwRuntimeError(String::cast(runtime()->newStringFromCString(message)));
  return Error::object();
}

// Convenience method for throwing a TypeError exception with an error message.
void Thread::throwTypeError(String* message) {
  // TODO: instantiate TypeError object.
  pending_exception_ = message;
}

Object* Thread::throwTypeErrorFromCString(const char* message) {
  throwRuntimeError(String::cast(runtime()->newStringFromCString(message)));
  return Error::object();
}

// Convenience method for throwing a ValueError exception with an error message.
Object* Thread::throwValueError(String* message) {
  // TODO: instantiate ValueError object.
  pending_exception_ = message;
  return Error::object();
}

Object* Thread::throwValueErrorFromCString(const char* message) {
  // TODO: instantiate ValueError object.
  pending_exception_ = runtime()->newStringFromCString(message);
  return Error::object();
}

Object* Thread::throwAttributeError(String* message) {
  // TODO: instantiate an AttributeError object.
  pending_exception_ = message;
  return Error::object();
}

Object* Thread::throwAttributeErrorFromCString(const char* message) {
  // TODO: instantiate an AttributeError object.
  pending_exception_ = runtime()->newStringFromCString(message);
  return Error::object();
}

void Thread::abortOnPendingException() {
  HandleScope scope(this);
  Handle<Object> pending_exception(&scope, pendingException());
  if (pending_exception->isNone()) {
    return;
  }

  fprintf(stderr, "aborting due to pending exception");
  if (pending_exception->isString()) {
    String* message = String::cast(*pending_exception);
    word len = message->length();
    byte* buffer = new byte[len + 1];
    message->copyTo(buffer, len);
    buffer[len] = 0;
    fprintf(stderr, ": %.*s", static_cast<int>(len), buffer);
  }
  fprintf(stderr, "\n");
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

Object* Thread::pendingException() {
  return pending_exception_;
}

} // namespace python
