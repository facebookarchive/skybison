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
  HandleScope scope;
  Handle<Function> function(&scope, previousFrame->function(argc));
  Handle<Code> code(&scope, function->code());

  // TODO: Raise an exception here instead of asserting
  assert(argc == code->argcount());

  // Set up the frame
  auto frame = thread->pushFrame(*code);
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
  frame->setVirtualPC(0);
  frame->setFastGlobals(function->fastGlobals());

  // TODO: specialized cases without cell and free variables
  // initialized cell var
  auto nLocals = code->nlocals();
  auto nCellvars = code->numCellvars();
  for (int i = 0; i < code->numCellvars(); i++) {
    // TODO: implement cell2arg
    Handle<ValueCell> cell(&scope, thread->runtime()->newValueCell());
    frame->setLocal(nLocals + i, *cell);
  }

  // initialize free var
  CHECK(
      code->numFreevars() == 0 ||
          code->numFreevars() ==
              ObjectArray::cast(function->closure())->length(),
      "Number of freevars is different than the closure.");
  for (int i = 0; i < code->numFreevars(); i++) {
    frame->setLocal(
        nLocals + nCellvars + i, ObjectArray::cast(function->closure())->at(i));
  }

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
