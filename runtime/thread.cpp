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
      next_(nullptr) {
  start_ = new byte[size];
  // Stack growns down in order to match machine convention
  end_ = ptr_ = start_ + size;
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

Frame* Thread::pushFrame(Object* object) {
  auto code = Code::cast(object);
  int size = Frame::allocationSize(code);

  // allocate that much space on the stack
  // TODO: Grow stack
  assert(ptr_ - size >= start_);

  ptr_ -= size;
  Frame* frame = reinterpret_cast<Frame*>(ptr_);

  // Initialize the frame.
  memset(ptr_, 0, size);
  frame->setCode(code);

  // return a pointer to the base of the frame

  return frame;
}

void Thread::popFrame(Frame* frame) {
  ptr_ += Frame::allocationSize(frame->code());
}

Object* Thread::run(Object* object) {
  Frame* frame = pushFrame(object);
  return Interpreter::execute(this, frame);
}

} // namespace python
