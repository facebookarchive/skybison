#include "trampolines.h"

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// Pushes a frame for the callee and initializes it
static Frame* pushCallee(Thread* thread, const Function& function,
                         Frame* caller, const Code& code) {
  // Set up the new frame
  Frame* callee_frame = thread->pushFrame(code);

  // Initialize it
  callee_frame->setGlobals(function->globals());
  if (callee_frame->globals() == caller->globals()) {
    callee_frame->setBuiltins(caller->builtins());
  } else {
    // TODO(T36407403): Set builtins appropriately
    // See PyFrame_New in cpython frameobject.c.
    // If caller->globals() contains the __builtins__ key and the
    // associated value is a module, use its dict. Otherwise, create a new
    // dict that contains the single assoc
    // ("None", None::object())
    callee_frame->setBuiltins(thread->runtime()->newDict());
  }
  callee_frame->setVirtualPC(0);
  callee_frame->setFastGlobals(function->fastGlobals());

  return callee_frame;
}

// Populate the free variable and cell variable arguments.
static void processFreevarsAndCellvars(Thread* thread, const Function& function,
                                       Frame* callee_frame, const Code& code) {
  CHECK(code->hasFreevarsOrCellvars(), "no free variables or cell variables");

  // initialize cell variables
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word num_locals = code->nlocals();
  word num_cellvars = code->numCellvars();
  for (word i = 0; i < code->numCellvars(); i++) {
    ValueCell value_cell(&scope, runtime->newValueCell());

    // Allocate a cell for a local variable if cell2arg is not preset
    if (code->cell2arg()->isNoneType()) {
      callee_frame->setLocal(num_locals + i, *value_cell);
      continue;
    }

    // Allocate a cell for a local variable if cell2arg is present but
    // the cell does not match any argument
    Object arg_index(&scope, Tuple::cast(code->cell2arg())->at(i));
    if (arg_index->isNoneType()) {
      callee_frame->setLocal(num_locals + i, *value_cell);
      continue;
    }

    // Allocate a cell for an argument
    word local_idx = Int::cast(arg_index)->asWord();
    value_cell->setValue(callee_frame->getLocal(local_idx));
    callee_frame->setLocal(local_idx, NoneType::object());
    callee_frame->setLocal(num_locals + i, *value_cell);
  }

  // initialize free variables
  DCHECK(
      code->numFreevars() == 0 ||
          code->numFreevars() == RawTuple::cast(function->closure())->length(),
      "Number of freevars is different than the closure.");
  for (word i = 0; i < code->numFreevars(); i++) {
    callee_frame->setLocal(num_locals + num_cellvars + i,
                           RawTuple::cast(function->closure())->at(i));
  }
}

static RawObject processDefaultArguments(Thread* thread,
                                         const Function& function,
                                         const Code& code, Frame* caller,
                                         const word argc) {
  HandleScope scope(thread);
  Object tmp_varargs(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  word new_argc = argc;
  if (new_argc < code->argcount() && function->hasDefaults()) {
    // Add default positional args
    Tuple default_args(&scope, function->defaults());
    if (default_args->length() < (code->argcount() - new_argc)) {
      // TODO(T39316354): Fix this up to remove the toCStr grossness.
      Str fn_name_str(&scope, function->name());
      unique_c_ptr<char> fn_name(fn_name_str->toCStr());
      return thread->raiseTypeError(runtime->newStrFromFormat(
          "TypeError: '%s' takes min %ld positional arguments but %ld given",
          fn_name.get(), code->argcount() - default_args->length(), argc));
    }
    const word positional_only = code->argcount() - default_args->length();
    for (; new_argc < code->argcount(); new_argc++) {
      caller->pushValue(default_args->at(new_argc - positional_only));
    }
  }
  if ((new_argc > code->argcount()) || code->hasVarargs()) {
    // VARARGS - spill extra positional args into the varargs tuple.
    if (code->hasVarargs()) {
      word len =
          Utils::maximum(static_cast<word>(0), new_argc - code->argcount());
      Tuple varargs(&scope, thread->runtime()->newTuple(len));
      for (word i = (len - 1); i >= 0; i--) {
        varargs->atPut(i, caller->topValue());
        caller->popValue();
        new_argc--;
      }
      tmp_varargs = *varargs;
    } else {
      // TODO(T39316354): Fix this up to remove the toCStr grossness.
      Str fn_name_str(&scope, function->name());
      unique_c_ptr<char> fn_name(fn_name_str->toCStr());
      return thread->raiseTypeError(runtime->newStrFromFormat(
          "TypeError: '%s' takes max %ld positional arguments but %ld given",
          fn_name.get(), code->argcount(), argc));
    }
  }

  // If there are any keyword-only args, there must be defaults for them
  // because we arrived here via CALL_FUNCTION (and thus, no keywords were
  // supplied at the call site).
  if (code->kwonlyargcount() != 0 && !function->kwDefaults()->isNoneType()) {
    Dict kw_defaults(&scope, function->kwDefaults());
    if (!kw_defaults->isNoneType()) {
      Tuple formal_names(&scope, code->varnames());
      word first_kw = code->argcount();
      for (word i = 0; i < code->kwonlyargcount(); i++) {
        Object name(&scope, formal_names->at(first_kw + i));
        RawObject val = thread->runtime()->dictAt(kw_defaults, name);
        if (!val->isError()) {
          caller->pushValue(val);
          new_argc++;
        } else {
          return thread->raiseTypeErrorWithCStr(
              "TypeError: missing keyword-only argument");
        }
      }
    } else {
      return thread->raiseTypeErrorWithCStr(
          "TypeError: missing keyword-only argument");
    }
  }

  if (code->hasVarargs()) {
    caller->pushValue(*tmp_varargs);
    new_argc++;
  }

  if (code->hasVarkeyargs()) {
    // VARKEYARGS - because we arrived via CALL_FUNCTION, no keyword arguments
    // provided.  Just add an empty dict.
    Object kwdict(&scope, thread->runtime()->newDict());
    caller->pushValue(*kwdict);
    new_argc++;
  }

  // At this point, we should have the correct number of arguments.  Throw if
  // not.
  if (new_argc != code->totalArgs()) {
    // TODO(T39316354): Fix this up to remove the toCStr grossness.
    Str fn_name_str(&scope, function->name());
    unique_c_ptr<char> fn_name(fn_name_str->toCStr());
    return thread->raiseTypeError(runtime->newStrFromFormat(
        "TypeError: '%s' takes %ld arguments but %ld given", fn_name.get(),
        code->totalArgs(), new_argc));
  }
  return NoneType::object();  // value not significant, it's just not an error
}

static word findName(RawObject name, RawTuple name_list) {
  word len = name_list->length();
  for (word i = 0; i < len; i++) {
    if (name == name_list->at(i)) {
      return i;
    }
  }
  return len;
}

// Verify correct number and order of arguments.  If order is wrong, try to
// fix it.  If argument is missing (denoted by Error::object()), try to supply
// it with a default.  This routine expects the number of args on the stack
// and number of names in the actual_names tuple to match.  Caller must pad
// prior to calling to ensure this.
// Return None::object() if successful, error object if not.
static RawObject checkArgs(const Function& function, RawObject* kw_arg_base,
                           const Tuple& actual_names, const Tuple& formal_names,
                           word start) {
  word num_actuals = actual_names->length();
  // Helper function to swap actual arguments and names
  auto swap = [&kw_arg_base, &actual_names](word arg_pos1,
                                            word arg_pos2) -> void {
    RawObject tmp = *(kw_arg_base - arg_pos1);
    *(kw_arg_base - arg_pos1) = *(kw_arg_base - arg_pos2);
    *(kw_arg_base - arg_pos2) = tmp;
    tmp = actual_names->at(arg_pos1);
    actual_names->atPut(arg_pos1, actual_names->at(arg_pos2));
    actual_names->atPut(arg_pos2, tmp);
  };
  // Helper function to retrieve argument
  auto arg_at = [&kw_arg_base](word idx) -> RawObject& {
    return *(kw_arg_base - idx);
  };
  for (word arg_pos = 0; arg_pos < num_actuals; arg_pos++) {
    if (actual_names->at(arg_pos) == formal_names->at(arg_pos + start)) {
      // We're good here: actual & formal arg names match.  Check the next one.
      continue;
    }
    // Mismatch.  Try to fix it.  Note: args grow down.
    bool swapped = false;
    // Look for expected Formal name in Actuals tuple.
    for (word i = arg_pos + 1; i < num_actuals; i++) {
      if (actual_names->at(i) == formal_names->at(arg_pos + start)) {
        // Found it.  Swap both the stack and the actual_names tuple.
        swap(arg_pos, i);
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
    if (!arg_at(arg_pos)->isError()) {
      for (word i = arg_pos + 1; i < num_actuals; i++) {
        if (arg_at(i)->isError()) {
          // Found an uninitialized slot.  Use it to save current actual.
          swap(arg_pos, i);
          break;
        }
      }
      // If we were unable to find a slot to swap into, TypeError
      if (!arg_at(arg_pos)->isError()) {
        return Thread::currentThread()->raiseTypeErrorWithCStr(
            "TypeError: invalid arguments");
      }
    }
    // Now, can we fill that slot with a default argument?
    HandleScope scope;
    Code code(&scope, function->code());
    word absolute_pos = arg_pos + start;
    if (absolute_pos < code->argcount()) {
      word defaults_size = function->hasDefaults()
                               ? RawTuple::cast(function->defaults())->length()
                               : 0;
      word defaults_start = code->argcount() - defaults_size;
      if (absolute_pos >= (defaults_start)) {
        // Set the default value
        Tuple default_args(&scope, function->defaults());
        *(kw_arg_base - arg_pos) =
            default_args->at(absolute_pos - defaults_start);
        continue;  // Got it, move on to the next
      }
    } else if (!function->kwDefaults()->isNoneType()) {
      // How about a kwonly default?
      Dict kw_defaults(&scope, function->kwDefaults());
      Thread* thread = Thread::currentThread();
      Object name(&scope, formal_names->at(arg_pos + start));
      RawObject val = thread->runtime()->dictAt(kw_defaults, name);
      if (!val->isError()) {
        *(kw_arg_base - arg_pos) = val;
        continue;  // Got it, move on to the next
      }
    }
    return Thread::currentThread()->raiseTypeErrorWithCStr(
        "TypeError: missing argument");
  }
  return NoneType::object();
}

// Converts keyword arguments into positional arguments.
static RawObject processKeywordArguments(Thread* thread, Frame* caller,
                                         word argc) {
  HandleScope scope(thread);
  // Destructively pop the tuple of kwarg names
  Tuple keywords(&scope, caller->topValue());
  caller->popValue();
  DCHECK(keywords->length() >= 0, "Invalid keyword name tuple");
  Function function(&scope, caller->peek(argc));
  Code code(&scope, function->code());
  word expected_args = code->argcount() + code->kwonlyargcount();
  word num_keyword_args = keywords->length();
  word num_positional_args = argc - num_keyword_args;
  Tuple formal_parm_names(&scope, code->varnames());
  Object tmp_varargs(&scope, NoneType::object());
  Object tmp_dict(&scope, NoneType::object());

  // We expect use of keyword argument calls to be uncommon, but when used
  // we anticipate mostly use of simple forms.  General scheme here is to
  // normalize the odd forms into standard form and then handle them all
  // in the same place.
  if (code->hasVarargsOrVarkeyargs()) {
    if (code->hasVarargs()) {
      // If we have more positional than expected, add the remainder to a tuple,
      // remove from the stack and close up the hole.
      word excess =
          Utils::maximum<word>(0, num_positional_args - code->argcount());
      Tuple varargs(&scope, thread->runtime()->newTuple(excess));
      if (excess > 0) {
        // Point to the leftmost excess argument
        RawObject* p =
            (caller->valueStackTop() + num_keyword_args + excess) - 1;
        // Copy the excess to the * tuple
        for (word i = 0; i < excess; i++) {
          varargs->atPut(i, *(p - i));
        }
        // Fill in the hole
        for (word i = 0; i < num_keyword_args; i++) {
          *p = *(p - excess);
          p--;
        }
        // Adjust the counts
        caller->dropValues(excess);
        argc -= excess;
        num_positional_args -= excess;
      }
      tmp_varargs = *varargs;
    }
    if (code->hasVarkeyargs()) {
      // Too many positional args passed?
      if (num_positional_args > code->argcount()) {
        return thread->raiseTypeErrorWithCStr(
            "TypeError: Too many positional arguments");
      }
      // If we have keyword arguments that don't appear in the formal parameter
      // list, add them to a keyword dict.
      Dict dict(&scope, thread->runtime()->newDict());
      List saved_keyword_list(&scope, thread->runtime()->newList());
      List saved_values(&scope, thread->runtime()->newList());
      word formal_parm_size = formal_parm_names->length();
      Runtime* runtime = thread->runtime();
      RawObject* p = caller->valueStackTop() + (num_keyword_args - 1);
      for (word i = 0; i < num_keyword_args; i++) {
        Object key(&scope, keywords->at(i));
        Object value(&scope, *(p - i));
        if (findName(*key, *formal_parm_names) < formal_parm_size) {
          // Got a match, stash pair for future restoration on the stack
          runtime->listAdd(saved_keyword_list, key);
          runtime->listAdd(saved_values, value);
        } else {
          // New, add it and associated value to the varkeyargs dict
          runtime->dictAtPut(dict, key, value);
          argc--;
        }
      }
      // Now, restore the stashed values to the stack and build a new
      // keywords name list.
      caller->dropValues(
          num_keyword_args);  // Pop all of the old keyword values
      num_keyword_args = saved_keyword_list->numItems();
      // Replace the old keywords list with a new one.
      keywords = runtime->newTuple(num_keyword_args);
      for (word i = 0; i < num_keyword_args; i++) {
        caller->pushValue(saved_values->at(i));
        keywords->atPut(i, saved_keyword_list->at(i));
      }
      tmp_dict = *dict;
    }
  }
  // At this point, all vararg forms have been normalized
  RawObject* kw_arg_base = (caller->valueStackTop() + num_keyword_args) -
                           1;  // pointer to first non-positional arg
  if (UNLIKELY(argc > expected_args)) {
    return thread->raiseTypeErrorWithCStr("TypeError: Too many arguments");
  }
  if (UNLIKELY(argc < expected_args)) {
    // Too few args passed.  Can we supply default args to make it work?
    // First, normalize & pad keywords and stack arguments
    word name_tuple_size = expected_args - num_positional_args;
    Tuple padded_keywords(&scope, thread->runtime()->newTuple(name_tuple_size));
    for (word i = 0; i < num_keyword_args; i++) {
      padded_keywords->atPut(i, keywords->at(i));
    }
    // Fill in missing spots w/ Error code
    for (word i = num_keyword_args; i < name_tuple_size; i++) {
      caller->pushValue(Error::object());
      padded_keywords->atPut(i, Error::object());
    }
    keywords = *padded_keywords;
  }
  // Now we've got the right number.  Do they match up?
  RawObject res = checkArgs(function, kw_arg_base, keywords, formal_parm_names,
                            num_positional_args);
  if (res->isError()) {
    return res;  // TypeError created by checkArgs.
  }
  CHECK(res->isNoneType(), "checkArgs should return an Error or None");
  // If we're a vararg form, need to push the tuple/dict.
  if (code->hasVarargs()) {
    caller->pushValue(*tmp_varargs);
  }
  if (code->hasVarkeyargs()) {
    caller->pushValue(*tmp_dict);
  }
  return res;
}

// Converts explode arguments into positional arguments.
//
// Returns the new number of positional arguments as a SmallInt, or Error if an
// exception was raised (most likely due to a non-string keyword name).
static RawObject processExplodeArguments(Thread* thread, Frame* caller,
                                         word arg) {
  HandleScope scope(thread);
  Object kw_dict(&scope, NoneType::object());
  if (arg & CallFunctionExFlag::VAR_KEYWORDS) {
    kw_dict = caller->topValue();
    caller->popValue();
  }
  Tuple positional_args(&scope, caller->popValue());
  for (word i = 0; i < positional_args->length(); i++) {
    caller->pushValue(positional_args->at(i));
  }
  word argc = positional_args->length();
  if (arg & CallFunctionExFlag::VAR_KEYWORDS) {
    Runtime* runtime = thread->runtime();
    Dict dict(&scope, *kw_dict);
    Tuple keys(&scope, runtime->dictKeys(dict));
    for (word i = 0; i < keys->length(); i++) {
      Object key(&scope, keys->at(i));
      if (!thread->runtime()->isInstanceOfStr(*key)) {
        return thread->raiseTypeErrorWithCStr("keywords must be strings");
      }
      caller->pushValue(runtime->dictAt(dict, key));
    }
    argc += keys->length();
    caller->pushValue(*keys);
  }
  return SmallInt::fromWord(argc);
}

// Takes the outgoing arguments of a positional argument call and rearranges
// them into the form expected by the callee and opens a new frame for the
// callee to execute in.
RawObject preparePositionalCall(Thread* thread, const Function& function,
                                const Code& code, Frame* caller, word argc) {
  // Are we one of the less common cases?
  if (argc != code->argcount() || !(code->flags() & Code::SIMPLE_CALL)) {
    RawObject result =
        processDefaultArguments(thread, function, code, caller, argc);
    if (result->isError()) {
      return result;
    }
  }
  return NoneType::object();
}

// Takes the outgoing arguments of an explode argument call and rearranges them
// into the form expected by the callee and opens a new frame for the callee to
// execute in.
RawObject prepareExplodeCall(Thread* thread, const Function& function,
                             const Code& code, Frame* caller, word arg) {
  RawObject arg_obj = processExplodeArguments(thread, caller, arg);
  if (arg_obj.isError()) return arg_obj;
  word new_argc = RawSmallInt::cast(arg_obj).value();

  if (arg & CallFunctionExFlag::VAR_KEYWORDS) {
    RawObject result = processKeywordArguments(thread, caller, new_argc);
    if (result->isError()) {
      return result;
    }
  } else {
    // Are we one of the less common cases?
    if (new_argc != code->argcount() || !(code->flags() & Code::SIMPLE_CALL)) {
      RawObject result =
          processDefaultArguments(thread, function, code, caller, new_argc);
      if (result->isError()) {
        return result;
      }
    }
  }
  return NoneType::object();
}

static RawObject createGenerator(const Code& code, Thread* thread) {
  CHECK(code->hasCoroutineOrGenerator(), "must be a coroutine or generator");
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  GeneratorBase gen_base(&scope, code->hasGenerator()
                                     ? runtime->newGenerator()
                                     : runtime->newCoroutine());
  gen_base->setHeapFrame(runtime->newHeapFrame(code));
  gen_base->setExceptionState(runtime->newExceptionState());
  runtime->genSave(thread, gen_base);
  return *gen_base;
}

RawObject generatorTrampoline(Thread* thread, Frame* caller, word arg) {
  HandleScope scope(thread);
  Function function(&scope, caller->peek(arg));
  Code code(&scope, function->code());
  RawObject error = preparePositionalCall(thread, function, code, caller, arg);
  if (error->isError()) {
    return error;
  }
  pushCallee(thread, function, caller, code);
  return createGenerator(code, thread);
}

RawObject generatorTrampolineKw(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip over the keyword dictionary to read the function object.
  Function function(&scope, caller->peek(argc + 1));
  RawObject error = processKeywordArguments(thread, caller, argc);
  if (error->isError()) {
    return error;
  }
  Code code(&scope, function->code());
  pushCallee(thread, function, caller, code);
  return createGenerator(code, thread);
}

RawObject generatorTrampolineEx(Thread* thread, Frame* caller, word arg) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, caller->peek((arg & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  Code code(&scope, function->code());
  RawObject error = prepareExplodeCall(thread, function, code, caller, arg);
  if (error->isError()) {
    return error;
  }
  pushCallee(thread, function, caller, code);
  return createGenerator(code, thread);
}

RawObject generatorClosureTrampoline(Thread* thread, Frame* caller, word arg) {
  HandleScope scope(thread);
  Function function(&scope, caller->peek(arg));
  Code code(&scope, function->code());
  RawObject error = preparePositionalCall(thread, function, code, caller, arg);
  if (error->isError()) {
    return error;
  }
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  processFreevarsAndCellvars(thread, function, callee_frame, code);
  return createGenerator(code, thread);
}

RawObject generatorClosureTrampolineKw(Thread* thread, Frame* caller,
                                       word argc) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, caller->peek(argc + 1));
  RawObject error = processKeywordArguments(thread, caller, argc);
  if (error->isError()) {
    return error;
  }
  Code code(&scope, function->code());
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  processFreevarsAndCellvars(thread, function, callee_frame, code);
  return createGenerator(code, thread);
}

RawObject generatorClosureTrampolineEx(Thread* thread, Frame* caller,
                                       word arg) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, caller->peek((arg & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  Code code(&scope, function->code());
  RawObject error = prepareExplodeCall(thread, function, code, caller, arg);
  if (error->isError()) {
    return error;
  }
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  processFreevarsAndCellvars(thread, function, callee_frame, code);
  return createGenerator(code, thread);
}

RawObject interpreterTrampoline(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  Function function(&scope, caller->peek(argc));
  Code code(&scope, function->code());
  RawObject error = preparePositionalCall(thread, function, code, caller, argc);
  if (error->isError()) {
    return error;
  }
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  return Interpreter::execute(thread, callee_frame);
}

RawObject interpreterTrampolineKw(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, caller->peek(argc + 1));
  RawObject error = processKeywordArguments(thread, caller, argc);
  if (error->isError()) {
    return error;
  }
  Code code(&scope, function->code());
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  return Interpreter::execute(thread, callee_frame);
}

RawObject interpreterTrampolineEx(Thread* thread, Frame* caller, word arg) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, caller->peek((arg & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  Code code(&scope, function->code());
  RawObject error = prepareExplodeCall(thread, function, code, caller, arg);
  if (error->isError()) {
    return error;
  }
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  return Interpreter::execute(thread, callee_frame);
}

RawObject interpreterClosureTrampoline(Thread* thread, Frame* caller,
                                       word argc) {
  HandleScope scope(thread);
  Function function(&scope, caller->peek(argc));
  Code code(&scope, function->code());
  RawObject error = preparePositionalCall(thread, function, code, caller, argc);
  if (error->isError()) {
    return error;
  }
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  processFreevarsAndCellvars(thread, function, callee_frame, code);
  return Interpreter::execute(thread, callee_frame);
}

RawObject interpreterClosureTrampolineKw(Thread* thread, Frame* caller,
                                         word argc) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, caller->peek(argc + 1));
  RawObject error = processKeywordArguments(thread, caller, argc);
  if (error->isError()) {
    return error;
  }
  Code code(&scope, function->code());
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  processFreevarsAndCellvars(thread, function, callee_frame, code);
  return Interpreter::execute(thread, callee_frame);
}

RawObject interpreterClosureTrampolineEx(Thread* thread, Frame* caller,
                                         word arg) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, caller->peek((arg & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  Code code(&scope, function->code());
  RawObject error = prepareExplodeCall(thread, function, code, caller, arg);
  if (error->isError()) {
    return error;
  }
  Frame* callee_frame = pushCallee(thread, function, caller, code);
  processFreevarsAndCellvars(thread, function, callee_frame, code);
  return Interpreter::execute(thread, callee_frame);
}

static RawObject callMethNoArgs(Thread* thread, const Function& function) {
  HandleScope scope(thread);
  Int address(&scope, function->code());
  binaryfunc method = bit_cast<binaryfunc>(address->asCPtr());
  DCHECK(function->module()->isModule(), "Function must have a module");
  PyObject* module = ApiHandle::borrowedReference(thread, function->module());
  PyObject* result = (*method)(module, nullptr);
  if (result != nullptr) return ApiHandle::fromPyObject(result)->asObject();
  if (thread->hasPendingException()) return Error::object();
  return thread->raiseSystemErrorWithCStr("NULL return without exception set");
}

RawObject moduleTrampolineNoArgs(Thread* thread, Frame* caller, word argc) {
  if (argc != 0) {
    return thread->raiseTypeErrorWithCStr("function takes no arguments");
  }
  HandleScope scope(thread);
  Function function(&scope, caller->peek(0));
  return callMethNoArgs(thread, function);
}

RawObject moduleTrampolineNoArgsKw(Thread* thread, Frame* caller, word argc) {
  if (argc != 0) {
    return thread->raiseTypeErrorWithCStr(
        "function takes no keyword arguments");
  }
  HandleScope scope(thread);
  Function function(&scope, caller->peek(1));
  return callMethNoArgs(thread, function);
}

RawObject moduleTrampolineNoArgsEx(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  bool has_varkeywords = argc & CallFunctionExFlag::VAR_KEYWORDS;
  Object varargs(&scope, caller->peek(has_varkeywords));
  if (!varargs->isTuple()) UNIMPLEMENTED("sequence varargs");
  if (Tuple::cast(varargs)->length() != 0) {
    return thread->raiseTypeErrorWithCStr("function takes no arguments");
  }
  if (has_varkeywords) {
    Object kw_args(&scope, caller->topValue());
    if (!kw_args->isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(kw_args)->numItems() != 0) {
      return thread->raiseTypeErrorWithCStr(
          "function takes no keyword arguments");
    }
  }
  Function function(&scope, caller->peek(has_varkeywords + 1));
  return callMethNoArgs(thread, function);
}

static RawObject callMethOneArg(Thread* thread, const Function& function,
                                const Object& object) {
  HandleScope scope(thread);
  Int address(&scope, function->code());
  binaryfunc method = bit_cast<binaryfunc>(address->asCPtr());
  DCHECK(function->module()->isModule(), "Function must have a module");
  PyObject* module = ApiHandle::borrowedReference(thread, function->module());
  PyObject* arg = ApiHandle::borrowedReference(thread, *object);
  PyObject* result = (*method)(module, arg);
  if (result != nullptr) return ApiHandle::fromPyObject(result)->asObject();
  if (thread->hasPendingException()) return Error::object();
  return thread->raiseSystemErrorWithCStr("NULL return without exception set");
}

RawObject moduleTrampolineOneArg(Thread* thread, Frame* caller, word argc) {
  if (argc != 1) {
    return thread->raiseTypeErrorWithCStr(
        "function takes exactly one argument");
  }
  HandleScope scope(thread);
  Object arg(&scope, caller->peek(0));
  Function function(&scope, caller->peek(1));
  return callMethOneArg(thread, function, arg);
}

RawObject moduleTrampolineOneArgKw(Thread* thread, Frame* caller, word argc) {
  if (argc != 1) {
    return thread->raiseTypeErrorWithCStr(
        "function takes exactly one argument");
  }
  HandleScope scope(thread);
  Tuple kwargs(&scope, caller->peek(0));
  if (kwargs->length() != 0) {
    return thread->raiseTypeErrorWithCStr(
        "function takes no keyword arguments");
  }
  Object arg(&scope, caller->peek(1));
  Function function(&scope, caller->peek(2));
  return callMethOneArg(thread, function, arg);
}

RawObject moduleTrampolineOneArgEx(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  bool has_varkeywords = argc & CallFunctionExFlag::VAR_KEYWORDS;
  if (has_varkeywords) {
    Object kw_args(&scope, caller->topValue());
    if (!kw_args->isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(kw_args)->numItems() != 0) {
      return thread->raiseTypeErrorWithCStr(
          "function takes no keyword arguments");
    }
  }
  Object varargs(&scope, caller->peek(has_varkeywords));
  if (!varargs->isTuple()) UNIMPLEMENTED("sequence varargs");
  Tuple args(&scope, *varargs);
  if (args->length() != 1) {
    return thread->raiseTypeErrorWithCStr(
        "function takes exactly one argument");
  }
  Object arg(&scope, args->at(0));
  Function function(&scope, caller->peek(has_varkeywords + 1));
  return callMethOneArg(thread, function, arg);
}

static RawObject callMethVarArgs(Thread* thread, const Function& function,
                                 const Tuple& varargs) {
  HandleScope scope(thread);
  Int address(&scope, function->code());
  binaryfunc method = bit_cast<binaryfunc>(address->asCPtr());
  DCHECK(function->module()->isModule(), "Function must have a module");
  PyObject* module = ApiHandle::borrowedReference(thread, function->module());
  PyObject* args = ApiHandle::borrowedReference(thread, *varargs);
  PyObject* result = (*method)(module, args);
  if (result != nullptr) return ApiHandle::fromPyObject(result)->asObject();
  if (thread->hasPendingException()) return Error::object();
  return thread->raiseSystemErrorWithCStr("NULL return without exception set");
}

RawObject moduleTrampolineVarArgs(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  Function function(&scope, caller->peek(argc));
  Tuple varargs(&scope, thread->runtime()->newTuple(argc));
  for (word i = 0; i < argc; i++) {
    varargs->atPut(argc - i - 1, caller->peek(i));
  }
  return callMethVarArgs(thread, function, varargs);
}

RawObject moduleTrampolineVarArgsKw(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  Tuple kwargs(&scope, caller->peek(0));
  if (kwargs->length() != 0) {
    return thread->raiseTypeErrorWithCStr(
        "function takes no keyword arguments");
  }
  Function function(&scope, caller->peek(argc + 1));
  Tuple varargs(&scope, thread->runtime()->newTuple(argc));
  for (word i = 0; i < argc; i++) {
    varargs->atPut(argc - i - 1, caller->peek(i + 1));
  }
  return callMethVarArgs(thread, function, varargs);
}

RawObject moduleTrampolineVarArgsEx(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);
  bool has_varkeywords = argc & CallFunctionExFlag::VAR_KEYWORDS;
  if (has_varkeywords) {
    Object kw_args(&scope, caller->topValue());
    if (!kw_args->isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(kw_args)->numItems() != 0) {
      return thread->raiseTypeErrorWithCStr(
          "function takes no keyword arguments");
    }
  }
  Object varargs(&scope, caller->peek(has_varkeywords));
  if (!varargs->isTuple()) UNIMPLEMENTED("sequence varargs");
  Tuple args(&scope, *varargs);
  Function function(&scope, caller->peek(has_varkeywords + 1));
  return callMethVarArgs(thread, function, args);
}

static RawObject callMethKeywordArgs(Thread* thread, const Function& function,
                                     const Tuple& varargs,
                                     const Dict& keywords) {
  HandleScope scope(thread);
  Int address(&scope, function->code());
  ternaryfunc method = bit_cast<ternaryfunc>(address->asCPtr());
  DCHECK(function->module()->isModule(), "Function must have a module");
  PyObject* module = ApiHandle::borrowedReference(thread, function->module());
  PyObject* args = ApiHandle::borrowedReference(thread, *varargs);
  PyObject* kwargs = ApiHandle::borrowedReference(thread, *keywords);
  PyObject* result = (*method)(module, args, kwargs);
  if (result != nullptr) return ApiHandle::fromPyObject(result)->asObject();
  if (thread->hasPendingException()) return Error::object();
  return thread->raiseSystemErrorWithCStr("NULL return without exception set");
}

RawObject moduleTrampolineKeywordArgs(Thread* thread, Frame* caller,
                                      word argc) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, caller->peek(argc));
  Tuple varargs(&scope, runtime->newTuple(argc));
  for (word i = 0; i < argc; i++) {
    varargs->atPut(argc - i - 1, caller->peek(i));
  }
  Dict keywords(&scope, runtime->newDict());
  return callMethKeywordArgs(thread, function, varargs, keywords);
}

RawObject moduleTrampolineKeywordArgsKw(Thread* thread, Frame* caller,
                                        word argc) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple kw_names(&scope, caller->peek(0));
  word num_keywords = kw_names->length();
  Dict keywords(&scope, runtime->newDict());
  for (word i = 0; i < num_keywords; i++) {
    Object key(&scope, kw_names->at(i));
    Object value(&scope, caller->peek(num_keywords - i));
    runtime->dictAtPut(keywords, key, value);
  }
  word num_varargs = argc - num_keywords;
  Tuple varargs(&scope, runtime->newTuple(num_varargs));
  for (word i = 0; i < num_varargs; i++) {
    varargs->atPut(i, caller->peek(argc - i));
  }
  Function function(&scope, caller->peek(argc + 1));
  return callMethKeywordArgs(thread, function, varargs, keywords);
}

RawObject moduleTrampolineKeywordArgsEx(Thread* thread, Frame* caller,
                                        word argc) {
  HandleScope scope(thread);
  bool has_varkeywords = argc & CallFunctionExFlag::VAR_KEYWORDS;
  Object args(&scope, caller->peek(has_varkeywords));
  if (!args->isTuple()) UNIMPLEMENTED("sequence varargs");
  Tuple varargs(&scope, *args);
  Object kwargs(&scope, has_varkeywords ? caller->topValue()
                                        : thread->runtime()->newDict());
  if (!kwargs->isDict()) UNIMPLEMENTED("mapping kwargs");
  Dict keywords(&scope, *kwargs);
  Function function(&scope, caller->peek(has_varkeywords + 1));
  return callMethKeywordArgs(thread, function, varargs, keywords);
}

typedef PyObject* (*PyCFunction)(PyObject*, PyObject*, PyObject*);

RawObject extensionTrampoline(Thread* thread, Frame* caller, word argc) {
  HandleScope scope(thread);

  // Set the address pointer to the function pointer
  Function function(&scope, caller->peek(argc));
  Int address(&scope, function->code());

  Object object(&scope, caller->topValue());
  Runtime* runtime = thread->runtime();
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  PyObject* none = ApiHandle::borrowedReference(thread, NoneType::object());

  if (object->isType()) {
    Type type_class(&scope, *object);
    PyCFunction new_function = bit_cast<PyCFunction>(address->asCPtr());
    PyObject* new_pyobject = (*new_function)(
        ApiHandle::borrowedReference(thread, *type_class), none, none);
    return ApiHandle::fromPyObject(new_pyobject)->asObject();
  }

  HeapObject instance(&scope, *object);
  Int object_ptr(&scope, runtime->instanceAt(thread, instance, attr_name));
  PyObject* self = static_cast<PyObject*>(object_ptr->asCPtr());

  PyCFunction init_function = bit_cast<PyCFunction>(address->asCPtr());
  (*init_function)(self, none, none);

  return *instance;
}

RawObject extensionTrampolineKw(Thread*, Frame*, word) {
  UNIMPLEMENTED("ExtensionTrampolineKw");
}

RawObject extensionTrampolineEx(Thread*, Frame*, word) {
  UNIMPLEMENTED("ExtensionTrampolineEx");
}

RawObject unimplementedTrampoline(Thread*, Frame*, word) {
  UNIMPLEMENTED("Trampoline");
}

template <typename InitArgs>
static RawObject builtinTrampolineImpl(Thread* thread, Frame* caller, word argc,
                                       word function_idx, Function::Entry entry,
                                       InitArgs init_args) {
  HandleScope scope(thread);
  Function function(&scope, caller->peek(function_idx));
  DCHECK(!function->code().isNoneType(),
         "builtin functions should have annotated code objects");
  Code code(&scope, function->code());
  DCHECK(code->code().isNoneType(),
         "builtin functions should not have bytecode");
  // The native function has been annotated in managed code, so do some more
  // advanced argument checking.
  Object result(&scope, init_args(function, code));
  if (result->isError()) return *result;
  argc = code->argcount();
  Frame* frame = thread->pushNativeFrame(bit_cast<void*>(entry), argc);
  result = entry(thread, frame, argc);
  DCHECK(result->isError() == thread->hasPendingException(),
         "error/exception mismatch");
  thread->popFrame();
  return *result;
}

RawObject builtinTrampoline(Thread* thread, Frame* caller, word argc,
                            Function::Entry fn) {
  return builtinTrampolineImpl(thread, caller, argc, argc, fn,
                               [&](const Function& function, const Code& code) {
                                 return preparePositionalCall(
                                     thread, function, code, caller, argc);
                               });
}

RawObject builtinTrampolineKw(Thread* thread, Frame* caller, word argc,
                              Function::Entry fn) {
  return builtinTrampolineImpl(
      thread, caller, argc, argc + 1 /* skip over implicit tuple of kwargs */,
      fn, [&](const Function&, const Code&) {
        return processKeywordArguments(thread, caller, argc);
      });
}

}  // namespace python
