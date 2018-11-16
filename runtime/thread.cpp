#include "thread.h"

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"

namespace python {

thread_local Thread* current_thread_ = nullptr;

Thread::Thread(int size)
    : size_(Utils::roundUp(size, kPointerSize)),
      handles_(new Handles()),
      next_(nullptr),
      runtime_(nullptr) {
  start_ = new byte[size];
  // Stack growns down in order to match machine convention
  end_ = ptr_ = start_ + size;
  pushInitialFrame();
}

Thread::~Thread() {
  delete handles_;
  delete[] start_;
}

Thread* Thread::currentThread() {
  return current_thread_;
}

void Thread::setCurrentThread(Thread* thread) {
  current_thread_ = thread;
}

Frame* Thread::pushFrame(Object* object, Frame* previousFrame) {
  auto code = Code::cast(object);
  int size = Frame::allocationSize(code);

  // allocate that much space on the stack
  // TODO: Grow stack
  assert(ptr_ - size >= start_);
  ptr_ -= size;

  // Take care to align the frame such that the arguments that were pushed on
  // the stack by the caller are adjacent to the locals of the callee.
  byte* limit = ptr_;
  if (!previousFrame->isSentinelFrame()) {
    limit = reinterpret_cast<byte*>(previousFrame->top()) - size;
  }
  int stackSize = code->stacksize() * kPointerSize;
  auto frame = reinterpret_cast<Frame*>(limit + stackSize);

  // Initialize the frame.
  memset(ptr_, 0, size);
  frame->setCode(code);
  frame->setPreviousFrame(previousFrame);
  frame->setTop(reinterpret_cast<Object**>(frame));

  // return a pointer to the base of the frame

  return frame;
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
  ptr_ += Frame::allocationSize(frame->code());
}

Object* Thread::run(Object* object) {
  assert(ptr_ == reinterpret_cast<byte*>(initialFrame_));
  Frame* frame = pushFrame(object, initialFrame_);
  return Interpreter::execute(this, frame);
}

} // namespace python
