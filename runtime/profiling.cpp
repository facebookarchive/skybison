#include "profiling.h"

#include "bytecode.h"
#include "frame.h"
#include "runtime.h"
#include "thread.h"

namespace py {

void profiling_call(Thread* thread) {
  thread->disableProfiling();

  HandleScope scope(thread);

  Object saved_type(&scope, thread->pendingExceptionType());
  Object saved_value(&scope, thread->pendingExceptionValue());
  Object saved_traceback(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  word opcodes = thread->opcodeCount();

  thread->stackPush(thread->runtime()->profilingCall());
  thread->stackPush(thread->profilingData());
  Frame* frame = thread->currentFrame();
  Frame* previous = frame->previousFrame();
  thread->stackPush(previous->isSentinel()
                        ? static_cast<RawObject>(NoneType::object())
                        : previous->function());
  thread->stackPush(frame->function());
  thread->stackPush(SmallInt::fromWord(opcodes));
  if (!frame->isNative()) {
    frame->addReturnMode(Frame::kProfilerReturn);
  }
  RawObject result = Interpreter::call(thread, 4);
  if (result.isErrorException()) {
    thread->ignorePendingException();
  }

  thread->setPendingExceptionType(*saved_type);
  thread->setPendingExceptionValue(*saved_value);
  thread->setPendingExceptionTraceback(*saved_traceback);

  word slack = thread->opcodeCount() - opcodes;
  thread->countOpcodes(-slack);
  thread->enableProfiling();
}

void profiling_return(Thread* thread) {
  if (!thread->profilingEnabled()) return;
  thread->disableProfiling();

  HandleScope scope(thread);
  Object saved_type(&scope, thread->pendingExceptionType());
  Object saved_value(&scope, thread->pendingExceptionValue());
  Object saved_traceback(&scope, thread->pendingExceptionTraceback());
  thread->clearPendingException();

  word opcodes = thread->opcodeCount();

  Frame* frame = thread->currentFrame();
  Object from(&scope, frame->function());
  Object to(&scope, NoneType::object());
  Frame* previous_frame = frame->previousFrame();
  if (!previous_frame->isSentinel()) {
    to = previous_frame->function();
  }

  thread->stackPush(thread->runtime()->profilingReturn());
  thread->stackPush(thread->profilingData());
  thread->stackPush(*from);
  thread->stackPush(*to);
  thread->stackPush(SmallInt::fromWord(opcodes));
  RawObject result = Interpreter::call(thread, 4);
  if (result.isErrorException()) {
    thread->ignorePendingException();
  }

  thread->setPendingExceptionType(*saved_type);
  thread->setPendingExceptionValue(*saved_value);
  thread->setPendingExceptionTraceback(*saved_traceback);

  word slack = thread->opcodeCount() - opcodes;
  thread->countOpcodes(-slack);
  thread->enableProfiling();
}

}  // namespace py
