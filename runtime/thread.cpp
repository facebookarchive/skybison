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
      next_(nullptr),
      runtime_(nullptr),
      pending_exception_(None::object()) {
  start_ = new byte[size];
  // Stack growns down in order to match machine convention
  end_ = ptr_ = start_ + size;
  pushInitialFrame();
}

Thread::~Thread() {
  delete handles_;
  delete[] start_;
}

void Thread::visitRoots(PointerVisitor* visitor) {
  handles()->visitPointers(visitor);
  visitor->visitPointer(&pending_exception_);
}

Thread* Thread::currentThread() {
  return current_thread_;
}

void Thread::setCurrentThread(Thread* thread) {
  current_thread_ = thread;
}

Frame* Thread::openAndLinkFrame(word numArgs, word numVars, word stackDepth) {
  CHECK(numArgs >= 0, "must have 0 or more arguments");
  CHECK(numVars >= 0, "must have 0 or more locals");
  CHECK(stackDepth >= 0, "stack depth cannot be negative");
  // HACK: Reserve one extra stack slot for the case where we need to unwrap a
  // bound method
  stackDepth += 1;

  word size = Frame::kSize + (numVars + stackDepth) * kPointerSize;

  // allocate that much space on the stack
  // TODO: Grow stack
  byte* prevSp = ptr_;
  assert(ptr_ - size >= start_);
  ptr_ -= size;

  // Take care to align the frame such that the arguments that were pushed on
  // the stack by the caller are adjacent to the locals of the callee.
  byte* sp = ptr_;
  if (!currentFrame_->isSentinelFrame()) {
    sp = reinterpret_cast<byte*>(currentFrame_->valueStackTop()) - size;
    CHECK(sp >= ptr_, "value stack overflow by %lu bytes\n", ptr_ - sp);
  }
  auto frame = reinterpret_cast<Frame*>(sp + stackDepth * kPointerSize);

  // Initialize the frame.
  std::memset(sp, 0, size);
  frame->setPreviousFrame(currentFrame_);
  frame->setValueStackTop(reinterpret_cast<Object**>(frame));
  frame->setPreviousSp(prevSp);
  frame->setNumLocals(numArgs + numVars);

  currentFrame_ = frame;

  // return a pointer to the base of the frame
  return frame;
}

Frame* Thread::pushFrame(Object* object) {
  HandleScope scope(handles());
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
  Handle<Object> name(&scope, runtime()->newStringFromCString("builtins"));
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
  Handle<Object> name(&scope, runtime()->newStringFromCString("builtins"));
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
  CHECK(ptr_ == end_, "stack must be empty when pushing initial frame");
  CHECK(ptr_ - Frame::kSize > start_, "no space for initial frame");

  ptr_ -= Frame::kSize;
  initialFrame_ = reinterpret_cast<Frame*>(ptr_);
  initialFrame_->makeSentinel();
  currentFrame_ = initialFrame_;
}

void Thread::popFrame() {
  Frame* frame = currentFrame_;
  CHECK(!frame->isSentinelFrame(), "cannot pop initial frame");
  currentFrame_ = frame->previousFrame();
  ptr_ = frame->previousSp();
}

Object* Thread::run(Object* object) {
  CHECK(
      ptr_ == reinterpret_cast<byte*>(initialFrame_),
      "thread must be inactive");
  Frame* frame = pushFrame(object);
  return Interpreter::execute(this, frame);
}

Object* Thread::runModuleFunction(Module* module, Object* object) {
  CHECK(
      ptr_ == reinterpret_cast<byte*>(initialFrame_),
      "thread must be inactive");
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
  HandleScope scope(handles());
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
