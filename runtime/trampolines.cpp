#include "trampolines.h"

#include <cstdlib>

#include "frame.h"
#include "interpreter.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* interpreterTrampoline(Thread* thread, Frame* previousFrame, word argc) {
  // TODO: We may get passed a BoundMethod.  We'll need to detect that, unwrap
  // it, handle shifting the arguments down (or potentially replacing the
  // BoundMethod with the wrapped this object).
  auto function = previousFrame->function(argc);
  auto code = Code::cast(function->code());

  // TODO: Raise an exception here instead of asserting
  assert(argc == code->argcount());

  // Set up the frame
  auto frame = thread->pushFrame(code, previousFrame);
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

Object* interpreterTrampolineKw(Thread*, Frame*, word) {
  std::abort();
}

Object* unimplementedTrampoline(Thread*, Frame*, word) {
  std::abort();
}

} // namespace python
