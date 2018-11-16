#include "trampolines.h"

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

static inline void push(Object**& sp, Object* val) {
  *--sp = val;
}

static inline Object* pop(Object**& sp) {
  return *sp++;
}

static inline void initFrame(
    Thread* thread,
    Function* function,
    Frame* newFrame,
    Frame* callerFrame) {
  newFrame->setGlobals(function->globals());
  if (newFrame->globals() == callerFrame->globals()) {
    newFrame->setBuiltins(callerFrame->builtins());
  } else {
    // TODO: Set builtins appropriately
    // See PyFrame_New in cpython frameobject.c.
    // If callerFrame->globals() contains the __builtins__ key and the
    // associated value is a module, use its dictionary. Otherwise, create a new
    // dictionary that contains the single assoc
    // ("None", None::object())
    newFrame->setBuiltins(thread->runtime()->newDictionary());
  }
  newFrame->setVirtualPC(0);
  newFrame->setFastGlobals(function->fastGlobals());
}

// Final stage of a call when arguments are all in place
static inline Object* callNoChecks(
    Thread* thread,
    Function* function,
    Frame* callerFrame,
    Code* code) {
  // Set up the new frame
  auto calleeFrame = thread->pushFrame(code);

  // Initialize it
  initFrame(thread, function, calleeFrame, callerFrame);

  // Off we go!
  return Interpreter::execute(thread, calleeFrame);
}

// Final stage of a call with possible Freevars/Cellvars
static inline Object* callCheckFreeCell(
    Thread* thread,
    Function* function,
    Frame* callerFrame,
    Code* code) {
  // Set up the new frame
  auto calleeFrame = thread->pushFrame(code);

  // Initialize it
  initFrame(thread, function, calleeFrame, callerFrame);

  // initialize cell var
  auto nLocals = code->nlocals();
  auto nCellvars = code->numCellvars();
  for (word i = 0; i < code->numCellvars(); i++) {
    // TODO: implement cell2arg
    calleeFrame->setLocal(nLocals + i, thread->runtime()->newValueCell());
  }

  // initialize free var
  DCHECK(
      code->numFreevars() == 0 ||
          code->numFreevars() ==
              ObjectArray::cast(function->closure())->length(),
      "Number of freevars is different than the closure.");
  for (word i = 0; i < code->numFreevars(); i++) {
    calleeFrame->setLocal(
        nLocals + nCellvars + i, ObjectArray::cast(function->closure())->at(i));
  }

  // Off we go!
  return Interpreter::execute(thread, calleeFrame);
}

// The basic interpreterTrampoline biases for the common case for a
// CALL_FUNCTION - correct number of arguments passed by position with no
// cell or free vars. If not, we bail out to a more general routine.
Object* interpreterTrampoline(Thread* thread, Frame* callerFrame, word argc) {
  HandleScope scope(thread->handles());
  Handle<Function> function(&scope, callerFrame->function(argc));
  Handle<Code> code(&scope, function->code());
  // Are we one of the less common cases?
  if ((argc != code->argcount() || !(code->flags() & Code::SIMPLE_CALL))) {
    return interpreterTrampolineSlowPath(
        thread, *function, *code, callerFrame, argc);
  }
  DCHECK(
      (code->kwonlyargcount() == 0) && (code->flags() & Code::NOFREE) &&
          !(code->flags() & (Code::VARARGS | Code::VARKEYARGS)),
      "Code::SIMPLE_CALL out of sync with kwonlyargcount()/NOFREE/VARARGS/VARKEYARGS");

  return callNoChecks(thread, *function, callerFrame, *code);
}

Object* interpreterTrampolineSlowPath(
    Thread* thread,
    Function* function,
    Code* code,
    Frame* callerFrame,
    word argc) {
  uword flags = code->flags();
  HandleScope scope(thread->handles());
  if (argc < code->argcount() && function->hasDefaults()) {
    // Add default positional args
    Handle<ObjectArray> default_args(&scope, function->defaults());
    // TODO: Throw exception here instead of CHECK
    CHECK(
        default_args->length() >= (code->argcount() - argc),
        "not enough positional arguments");
    Object** sp = callerFrame->valueStackTop();
    const word positional_only = code->argcount() - default_args->length();
    for (; argc < code->argcount(); argc++) {
      push(sp, default_args->at(argc - positional_only));
    }
    callerFrame->setValueStackTop(sp);
  }

  if ((argc > code->argcount()) || (flags & Code::VARARGS)) {
    // VARARGS - spill extra positional args into the varargs tuple.
    if (flags & Code::VARARGS) {
      word len = std::max<word>(0, argc - code->argcount());
      Handle<ObjectArray> varargs(
          &scope, thread->runtime()->newObjectArray(len));
      Object** sp = callerFrame->valueStackTop();
      for (word i = (len - 1); i >= 0; i--) {
        varargs->atPut(i, pop(sp));
        argc--;
      }
      push(sp, *varargs);
      argc++;
      callerFrame->setValueStackTop(sp);
    } else {
      // TODO: throw exception here instead of CHECK
      CHECK(false, "TypeError");
    }

    if (flags & Code::VARKEYARGS) {
      // VARKEYARGS - because we arrived via CALL_FUNCTION, no keyword arguments
      // provided.  Just add an empty dictionary.
      Handle<Object> kwdict(&scope, thread->runtime()->newDictionary());
      Object** sp = callerFrame->valueStackTop();
      push(sp, *kwdict);
      callerFrame->setValueStackTop(sp);
      argc++;
    }
  }

  // At this point, we should have the correct number of arguments.  Throw if
  // not.
  // TODO: Throw exception here instead of CHECK
  CHECK(
      argc == code->totalArgs(),
      "got %ld args, expected %ld",
      argc,
      code->totalArgs());

  return callCheckFreeCell(thread, function, callerFrame, code);
}

uword findName(Object* name, ObjectArray* nameList) {
  uword len = nameList->length();
  for (uword i = 0; i < len; i++) {
    if (name == nameList->at(i)) {
      return i;
    }
  }
  return len;
}

// Verify correct number and order of arguments.  If order is wrong, try to
// fix it.  If argument is missing (denoted by Error::object()), try to supply
// it with a default.  This routine expects the number of args on the stack
// and number of names in the actualNames tuple to match.  Caller must pad
// prior to calling to ensure this.
// Return None::object() if successfull, error object if not.
Object* checkArgs(
    Function* function,
    Object** kwArgBase,
    ObjectArray* actualNames,
    ObjectArray* formalNames,
    word start) {
  word numActuals = actualNames->length();
  DCHECK(
      (numActuals + start == formalNames->length()),
      "invalid use of checkArgs");
  // Helper function to swap actual arguments and names
  auto swap = [kwArgBase, actualNames](word argPos1, word argPos2) -> void {
    Object* tmp = *(kwArgBase - argPos1);
    *(kwArgBase - argPos1) = *(kwArgBase - argPos2);
    *(kwArgBase - argPos2) = tmp;
    tmp = actualNames->at(argPos1);
    actualNames->atPut(argPos1, actualNames->at(argPos2));
    actualNames->atPut(argPos2, tmp);
  };
  // Helper function to retrieve argument
  auto argAt = [kwArgBase](word idx) -> Object*& { return *(kwArgBase - idx); };
  for (word argPos = 0; argPos < numActuals; argPos++) {
    if (actualNames->at(argPos) == formalNames->at(argPos + start)) {
      // We're good here: actual & formal arg names match.  Check the next one.
      continue;
    }
    // Mismatch.  Try to fix it.  Note: args grow down.
    bool swapped = false;
    // Look for expected Formal name in Actuals tuple.
    for (word i = argPos + 1; i < numActuals; i++) {
      if (actualNames->at(i) == formalNames->at(argPos + start)) {
        // Found it.  Swap both the stack and the actualNames tuple.
        swap(argPos, i);
        swapped = true;
        break;
      }
    }
    if (swapped) {
      // We managed to fix it.  Check the next one.
      continue;
    }
    // Can't find an Actual for this Formal.
    // If we have a real actual in current slot, move it somewhere safe.
    if (!argAt(argPos)->isError()) {
      for (word i = argPos + 1; i < numActuals; i++) {
        if (argAt(i)->isError()) {
          // Found an uninitialized slot.  Use it to save current actual.
          swap(argPos, i);
          break;
        }
      }
      // If we were unable to find a slot to swap into, TypeError
      if (!argAt(argPos)->isError()) {
        // TODO: TypeError
        return Error::object();
      }
    }
    // Now, can we fill that slot with a default argument?
    HandleScope scope;
    Handle<Code> code(&scope, function->code());
    word absolutePos = argPos + start;
    if (absolutePos < code->argcount()) {
      word defaultsSize = function->hasDefaults()
          ? ObjectArray::cast(function->defaults())->length()
          : 0;
      word defaultsStart = code->argcount() - defaultsSize;
      if (absolutePos >= (defaultsStart)) {
        // Set the default value
        Handle<ObjectArray> default_args(&scope, function->defaults());
        *(kwArgBase - argPos) = default_args->at(absolutePos - defaultsStart);
        continue; // Got it, move on to the next
      }
    } else if (!function->kwDefaults()->isNone()) {
      // How about a kwonly default?
      Handle<Dictionary> kwDefaults(&scope, function->kwDefaults());
      Thread* thread = Thread::currentThread();
      Handle<Object> name(&scope, formalNames->at(argPos + start));
      Object* val = thread->runtime()->dictionaryAt(kwDefaults, name);
      if (!val->isError()) {
        *(kwArgBase - argPos) = val;
        continue; // Got it, move on to the next
      }
    }
    // TODO: No default for this position available - TypeError
    return Error::object();
  }
  return None::object();
}

// Trampoline for calls in which the caller provided keyword arguments.  On
// TOS will be a tuple of the provided keyword names.  Above them the associated
// values in left-to-right order.
Object* interpreterTrampolineKw(Thread* thread, Frame* callerFrame, word argc) {
  HandleScope scope(thread->handles());
  Object** sp = callerFrame->valueStackTop();
  // Destructively pop the tuple of kwarg names
  Handle<ObjectArray> keywords(&scope, pop(sp));
  callerFrame->setValueStackTop(sp);
  DCHECK(keywords->length() > 0, "Invalid keyword name tuple");
  Handle<Function> function(&scope, *(sp + argc));
  Handle<Code> code(&scope, function->code());
  word expectedArgs = code->argcount() + code->kwonlyargcount();
  word numKeywordArgs = keywords->length();
  Object** kwArgBase =
      (sp + numKeywordArgs) - 1; // pointer to first non-positional arg
  word numPositionalArgs = argc - numKeywordArgs;
  Handle<ObjectArray> formalParmNames(&scope, code->varnames());
  word flags = code->flags();
  Object* res;

  // We expect use of keyword argument calls to be uncommon, but when used
  // we anticipate mostly use of simple forms.  Identify those that can
  // be easily converted to fast calls here.
  if (!(flags & (Code::VARARGS | Code::VARKEYARGS))) {
    if (UNLIKELY(argc > expectedArgs)) {
      // Error: too many args for a non-varargs call.
      // TODO: return TypeError
      CHECK(false, "TypeError");
    } else if (UNLIKELY(argc < expectedArgs)) {
      // Too few args passed.  Can we supply default args to make it work?
      // First, normalize & pad keywords and stack arguments
      word nameTupleSize = expectedArgs - numPositionalArgs;
      Handle<ObjectArray> paddedKeywords(
          &scope, thread->runtime()->newObjectArray(nameTupleSize));
      for (word i = 0; i < numKeywordArgs; i++) {
        paddedKeywords->atPut(i, keywords->at(i));
      }
      // Fill in missing spots w/ Error code
      for (word i = numKeywordArgs; i < nameTupleSize; i++) {
        push(sp, Error::object());
        paddedKeywords->atPut(i, Error::object());
      }
      callerFrame->setValueStackTop(sp);
      keywords = *paddedKeywords;
    }
    // Now we've got the right number.  Do they match up?
    res = checkArgs(
        *function, kwArgBase, *keywords, *formalParmNames, numPositionalArgs);
    if (res->isNone()) {
      return callNoChecks(thread, *function, callerFrame, *code);
    }
    // TODO: return res (which will be TypeError)
    CHECK(false, "TypeError");
  }
  UNIMPLEMENTED("SlowPath varargs keyword trampoline");
}

Object* interpreterTrampolineEx(Thread*, Frame*, word) {
  UNIMPLEMENTED("TrampolineEx");
}

Object* unimplementedTrampoline(Thread*, Frame*, word) {
  UNIMPLEMENTED("Trampoline");
}

} // namespace python
