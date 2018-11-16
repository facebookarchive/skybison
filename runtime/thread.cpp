#include "thread.h"

#include <cstdio>
#include <cstring>

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
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

Frame* Thread::openAndLinkFrame(
    word numArgs,
    word numVars,
    word stackDepth,
    Frame* previousFrame) {
  assert(numArgs >= 0);
  assert(numVars >= 0);
  assert(stackDepth >= 0);
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
  if (!previousFrame->isSentinelFrame()) {
    sp = reinterpret_cast<byte*>(previousFrame->valueStackTop()) - size;
  }
  auto frame = reinterpret_cast<Frame*>(sp + stackDepth * kPointerSize);

  // Initialize the frame.
  std::memset(ptr_, 0, size);
  frame->setPreviousFrame(previousFrame);
  frame->setValueStackTop(reinterpret_cast<Object**>(frame));
  frame->setPreviousSp(prevSp);
  frame->setNumLocals(numArgs + numVars);

  // return a pointer to the base of the frame
  return frame;
}

Frame* Thread::pushFrame(Object* object, Frame* previousFrame) {
  HandleScope scope(handles());
  Handle<Code> code(&scope, object);
  word numVars = code->nlocals() + code->numCellvars() + code->numFreevars();
  auto frame = openAndLinkFrame(
      code->argcount(), numVars, code->stacksize(), previousFrame);
  frame->setCode(*code);
  return frame;
}

Frame* Thread::pushModuleFunctionFrame(
    Module* module,
    Object* object,
    Frame* previousFrame) {
  Frame* result = pushFrame(object, previousFrame);
  result->setGlobals(module->dictionary());
  result->setImplicitGlobals(module->dictionary());
  return result;
}

Frame* Thread::pushClassFunctionFrame(
    Object* function,
    Object* dictionary,
    Frame* caller) {
  Object* code = Function::cast(function)->code();
  Frame* result = pushFrame(code, caller);
  result->setGlobals(Function::cast(function)->globals());
  result->setImplicitGlobals(dictionary);
  return result;
}

void Thread::pushInitialFrame() {
  assert(ptr_ == end_);
  assert(ptr_ - Frame::kSize > start_);

  ptr_ -= Frame::kSize;
  initialFrame_ = reinterpret_cast<Frame*>(ptr_);
  initialFrame_->makeSentinel();
}

void Thread::popFrame(Frame* frame) {
  assert(!frame->isSentinelFrame());
  ptr_ = frame->previousSp();
}

Object* Thread::run(Object* object) {
  assert(ptr_ == reinterpret_cast<byte*>(initialFrame_));
  Frame* frame = pushFrame(object, initialFrame_);
  return Interpreter::execute(this, frame);
}

Object* Thread::runModuleFunction(Module* module, Object* object) {
  assert(ptr_ == reinterpret_cast<byte*>(initialFrame_));
  Frame* frame = pushModuleFunctionFrame(module, object, initialFrame_);
  return Interpreter::execute(this, frame);
}

Object*
Thread::runClassFunction(Object* function, Object* dictionary, Frame* caller) {
  Frame* frame = pushClassFunctionFrame(function, dictionary, caller);
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
void Thread::throwValueError(String* message) {
  // TODO: instantiate ValueError object.
  pending_exception_ = message;
}

void Thread::throwValueErrorFromCString(const char* message) {
  // TODO: instantiate ValueError object.
  pending_exception_ = runtime()->newStringFromCString(message);
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

  std::abort();
}

Object* Thread::pendingException() {
  return pending_exception_;
}

} // namespace python
