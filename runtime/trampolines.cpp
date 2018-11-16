#include "trampolines.h"

#include <cstdlib>

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* interpreterTrampoline(Thread* thread, Frame* previousFrame, word argc) {
  auto function = previousFrame->function(argc);
  auto code = Code::cast(function->code());

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
    frame->setBuiltins(thread->runtime()->newDictionary());
  }
  frame->setLastInstruction(SmallInteger::fromWord(0));
  frame->setFastGlobals(function->fastGlobals());

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
