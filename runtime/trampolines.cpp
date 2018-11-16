#include "frame.h"
#include "interpreter.h"
#include "objects.h"
#include "thread.h"
#include "trampolines.h"

#include <cstdlib>

namespace python {

FunctionTrampoline trampolineFromObject(Object* object) {
  return reinterpret_cast<FunctionTrampoline>(
      SmallInteger::cast(object)->value());
}

Object* trampolineToObject(FunctionTrampoline trampoline) {
  return SmallInteger::fromWord(reinterpret_cast<uword>(trampoline));
}

Object* interpreterTrampoline(Thread* thread, Frame* previousFrame, int argc) {
  // TODO: We may get passed a BoundMethod.  We'll need to detect that, unwrap
  // it, handle shifting the arguments down (or potentially replacing the
  // BoundMethod with the wrapped this object).
  auto function = thread->peekObject(argc)->cast<Function>();
  auto code = function->code()->cast<Code>();

  // TODO: Raise an exception here instead of asserting
  assert(argc == code->argcount());

  // Set up the frame
  auto frame = thread->pushFrame(code);
  frame->setGlobals(function->globals());

  if (frame->globals() == previousFrame->globals()) {
    frame->setBuiltins(previousFrame->builtins());
  } else {
    // TODO: Set builtins appropriately
    // If frame->globals() contains the __builtins__ key and the associated
    // value is a module, use its dictionary.
    // Otherwise, create a new dictionary that contains the single assoc
    // ("None", None::object())
    frame->setBuiltins(None::object());
  }
  frame->setLastInstruction(SmallInteger::fromWord(0));

  // Off we go!
  return Interpreter::execute(thread, frame);
}

Object*
interpreterTrampolineKw(Thread* thread, Frame* previousFrame, int argc) {
  assert(false);
}

Object*
unimplementedTrampoline(Thread* thread, Frame* previousFrame, int argc) {
  std::abort();
}

} // namespace python
