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
  start_ = ptr_ = new byte[size];
  end_ = start_ + size;
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
  // compute the frame size

  Code* code = Code::cast(object);
  int ncells = ObjectArray::cast(code->cellvars())->length();
  int nfrees = ObjectArray::cast(code->freevars())->length();
  int extras0 = code->nlocals() + ncells + nfrees;
  int extras = code->stacksize() + extras0;
  int size = OFFSET_OF(Frame, f_localsplus) + extras * kPointerSize;

  // allocate that much space on the stack

  Frame* frame = reinterpret_cast<Frame*>(ptr_);
  ptr_ += size;

  // Initialize the frame.
  memset(frame, 0, size);

  frame->f_code = code;
  frame->f_valuestack = frame->f_localsplus + extras0;
  frame->f_stacktop = frame->f_valuestack;

  // return a pointer to the base of the frame

  return frame;
}

void Thread::popFrame(Frame* frame) {}

Object* Thread::run(Object* object) {
  Frame* frame = pushFrame(object);
  return Interpreter::execute(this, frame);
}

} // namespace python
